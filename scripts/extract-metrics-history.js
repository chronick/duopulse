#!/usr/bin/env node
/**
 * Extract historical Pentagon metrics from baseline tags
 *
 * Walks through baseline-v* tags and extracts metrics/baseline.json
 * to build a timeline of metric improvements.
 */

const { execSync } = require('child_process');
const { writeFileSync } = require('fs');

const BASELINE_PATH = 'metrics/baseline.json';
const OUTPUT_PATH = 'tools/evals/public/data/metrics-history.json';

/**
 * Get list of baseline tags sorted by version
 * @returns {Array} Array of tag names
 */
function getBaselineTags() {
  try {
    const tags = execSync('git tag -l "baseline-v*"', {
      encoding: 'utf-8',
    }).trim().split('\n').filter(Boolean);

    // Sort tags by version number (v1.0.0, v1.0.1, etc.)
    return tags.sort((a, b) => {
      const aVer = a.replace('baseline-v', '').split('.').map(Number);
      const bVer = b.replace('baseline-v', '').split('.').map(Number);
      for (let i = 0; i < 3; i++) {
        if (aVer[i] !== bVer[i]) return aVer[i] - bVer[i];
      }
      return 0;
    });
  } catch (err) {
    console.error('Error getting baseline tags:', err.message);
    return [];
  }
}

/**
 * Get timestamp for a tag
 * @param {string} tag - Tag name
 * @returns {number} Unix timestamp
 */
function getTagTimestamp(tag) {
  try {
    const timestamp = execSync(`git log -1 --format=%ct ${tag}`, {
      encoding: 'utf-8',
    }).trim();
    return parseInt(timestamp, 10);
  } catch (err) {
    return null;
  }
}

/**
 * Get commit hash for a tag
 * @param {string} tag - Tag name
 * @returns {string} Commit hash
 */
function getTagCommit(tag) {
  try {
    return execSync(`git rev-parse ${tag}`, {
      encoding: 'utf-8',
    }).trim();
  } catch (err) {
    return null;
  }
}

/**
 * Extract baseline.json from a specific tag
 * @param {string} tag - Tag name
 * @returns {Object|null} Parsed JSON or null if not found
 */
function getBaselineAtTag(tag) {
  try {
    const content = execSync(`git show ${tag}:${BASELINE_PATH}`, {
      encoding: 'utf-8',
      maxBuffer: 10 * 1024 * 1024,
    });
    return JSON.parse(content);
  } catch (err) {
    return null;
  }
}

/**
 * Extract key metrics from baseline.json
 * @param {Object} baseline - Baseline metrics object
 * @returns {Object} Simplified metrics for charting
 */
function extractKeyMetrics(baseline) {
  if (!baseline || !baseline.metrics || !baseline.metrics.pentagonStats) {
    return null;
  }

  const stats = baseline.metrics.pentagonStats;
  const zones = ['stable', 'syncopated', 'wild'];

  // Calculate average across zones for each metric
  const avgMetrics = {};
  const metricKeys = ['syncopation', 'density', 'velocityRange', 'voiceSeparation', 'regularity'];

  for (const key of metricKeys) {
    const values = zones
      .map(z => stats[z]?.[key])
      .filter(v => typeof v === 'number');

    avgMetrics[key] = values.length > 0
      ? values.reduce((sum, v) => sum + v, 0) / values.length
      : 0;
  }

  return {
    overallAlignment: baseline.metrics.overallAlignment || 0,
    alignmentStatus: baseline.metrics.alignmentStatus || 'UNKNOWN',
    totalPatterns: baseline.metrics.totalPatterns || 0,
    metrics: avgMetrics,
    zoneCompliance: {
      stable: stats.stable?.composite || 0,
      syncopated: stats.syncopated?.composite || 0,
      wild: stats.wild?.composite || 0,
    },
  };
}

/**
 * Build metrics timeline from baseline tags
 */
function buildMetricsTimeline() {
  console.log('Building metrics timeline from baseline tags...');

  const tags = getBaselineTags();
  console.log(`Found ${tags.length} baseline tags`);

  if (tags.length === 0) {
    console.log('No baseline tags found');
    return null;
  }

  const timeline = tags
    .map((tag, i) => {
      process.stdout.write(`\rProcessing tag ${i + 1}/${tags.length}: ${tag}...`);

      const baseline = getBaselineAtTag(tag);
      if (!baseline) {
        console.log(`\nWarning: Could not read baseline at tag ${tag}`);
        return null;
      }

      const extracted = extractKeyMetrics(baseline);
      if (!extracted) {
        console.log(`\nWarning: Could not extract metrics from tag ${tag}`);
        return null;
      }

      const timestamp = getTagTimestamp(tag);
      const commit = getTagCommit(tag);

      return {
        tag,
        hash: commit ? commit.substring(0, 7) : null,
        fullHash: commit,
        timestamp,
        date: timestamp ? new Date(timestamp * 1000).toISOString() : null,
        version: baseline.version,
        ...extracted,
      };
    })
    .filter(Boolean);

  console.log('\n');
  console.log(`Successfully extracted ${timeline.length} data points`);

  return {
    generated: new Date().toISOString(),
    totalTags: tags.length,
    dataPoints: timeline.length,
    timeline,
  };
}

// Main
const history = buildMetricsTimeline();

if (history && history.timeline.length > 0) {
  writeFileSync(OUTPUT_PATH, JSON.stringify(history, null, 2));
  console.log(`Written metrics history to ${OUTPUT_PATH}`);
  console.log(`Date range: ${history.timeline[0].date} → ${history.timeline[history.timeline.length - 1].date}`);
  console.log(`Overall alignment: ${(history.timeline[0].overallAlignment * 100).toFixed(1)}% → ${(history.timeline[history.timeline.length - 1].overallAlignment * 100).toFixed(1)}%`);
} else {
  console.log('No timeline data to write');
}
