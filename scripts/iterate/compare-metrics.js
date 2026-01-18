#!/usr/bin/env node
/**
 * compare-metrics.js
 *
 * Compares two Pentagon metric snapshots and outputs a formatted report.
 * Used by /iterate command to evaluate before/after metrics.
 *
 * Usage:
 *   node compare-metrics.js <baseline.json> <new-metrics.json>
 *   node compare-metrics.js metrics/baseline.json /tmp/new-metrics.json
 *
 * Output:
 *   - Markdown table with deltas
 *   - Success/fail assessment based on thresholds
 *   - Exit code: 0 = success criteria met, 1 = failed
 */

const fs = require('fs');
const path = require('path');

// Success thresholds
const IMPROVEMENT_THRESHOLD = 0.05; // 5% relative improvement required
const REGRESSION_THRESHOLD = 0.02; // 2% regression tolerance

const METRICS = ['syncopation', 'density', 'velocityRange', 'voiceSeparation', 'regularity', 'composite'];
const ZONES = ['stable', 'syncopated', 'wild', 'total'];

function loadMetricsFile(filePath) {
  const content = fs.readFileSync(filePath, 'utf-8');
  return JSON.parse(content);
}

function extractPentagonStats(data) {
  // Handle both baseline.json format and raw metrics format
  if (data.metrics && data.metrics.pentagonStats) {
    return data.metrics.pentagonStats;
  } else if (data.pentagonStats) {
    return data.pentagonStats;
  } else {
    throw new Error('Unexpected metrics format');
  }
}

function extractMetricDefinitions(data) {
  // Extract target ranges from baseline.json
  if (data.metrics && data.metrics.metricDefinitions) {
    return data.metrics.metricDefinitions;
  } else if (data.metricDefinitions) {
    return data.metricDefinitions;
  }
  return null;
}

function parseTargetRange(rangeStr) {
  // Parse "0.12-0.48" → {min: 0.12, max: 0.48}
  if (!rangeStr) return null;
  const parts = rangeStr.split('-').map(s => parseFloat(s.trim()));
  if (parts.length === 2 && !isNaN(parts[0]) && !isNaN(parts[1])) {
    return { min: parts[0], max: parts[1] };
  }
  return null;
}

function isRegression(before, after, targetRange) {
  // Direction-aware regression detection:
  // - A change is a regression if it moves AWAY from the target range
  // - A change toward the target range is an improvement, not regression

  if (!targetRange) {
    // No target range info - fall back to simple "decrease is bad"
    return after < before;
  }

  const delta = after - before;
  const { min, max } = targetRange;

  // If we're above the target max, decreasing is good
  if (before > max) {
    return after > before; // increasing when already too high is regression
  }

  // If we're below the target min, increasing is good
  if (before < min) {
    return after < before; // decreasing when already too low is regression
  }

  // We're in range - moving out of range is regression
  if (after < min || after > max) {
    return true; // moved out of target range
  }

  // Still in range - not a regression
  return false;
}

function calculateDelta(before, after) {
  if (before === 0) {
    return after === 0 ? 0 : Infinity;
  }
  return (after - before) / before;
}

function formatPercent(value) {
  const pct = value * 100;
  const sign = pct >= 0 ? '+' : '';
  return `${sign}${pct.toFixed(1)}%`;
}

function formatValue(value) {
  return value.toFixed(3);
}

function main() {
  const args = process.argv.slice(2);

  if (args.length < 2) {
    console.error('Usage: compare-metrics.js <baseline.json> <new-metrics.json> [--target=metric] [--zone=zone]');
    process.exit(2);
  }

  const baselinePath = args[0];
  const newPath = args[1];

  // Parse optional target metric and zone
  let targetMetric = null;
  let targetZone = null;

  for (const arg of args.slice(2)) {
    if (arg.startsWith('--target=')) {
      targetMetric = arg.split('=')[1];
    } else if (arg.startsWith('--zone=')) {
      targetZone = arg.split('=')[1];
    }
  }

  // Load metrics
  let baselineData, newData, baseline, newMetrics, metricDefinitions;
  try {
    baselineData = loadMetricsFile(baselinePath);
    newData = loadMetricsFile(newPath);
    baseline = extractPentagonStats(baselineData);
    newMetrics = extractPentagonStats(newData);
    metricDefinitions = extractMetricDefinitions(baselineData);
  } catch (err) {
    console.error(`Error loading metrics: ${err.message}`);
    process.exit(2);
  }

  // Helper to get target range for a metric/zone combination
  function getTargetRange(metric, zone) {
    if (!metricDefinitions || !metricDefinitions[metric]) return null;
    const targetByZone = metricDefinitions[metric].targetByZone;
    if (!targetByZone || !targetByZone[zone]) return null;
    return parseTargetRange(targetByZone[zone]);
  }

  // Calculate deltas
  const deltas = {};
  let maxRegression = 0;
  let maxRegressionInfo = null;
  let targetImprovement = null;

  for (const zone of ZONES) {
    deltas[zone] = {};
    const baseZone = baseline[zone] || baseline.total;
    const newZone = newMetrics[zone] || newMetrics.total;

    if (!baseZone || !newZone) continue;

    for (const metric of METRICS) {
      const before = baseZone[metric];
      const after = newZone[metric];

      if (before === undefined || after === undefined) continue;

      const delta = calculateDelta(before, after);
      const targetRange = getTargetRange(metric, zone);
      const regressed = isRegression(before, after, targetRange);

      deltas[zone][metric] = { before, after, delta, targetRange, regressed };

      // Track target metric improvement
      if (targetMetric === metric && (targetZone === zone || targetZone === null || zone === 'total')) {
        if (targetImprovement === null || Math.abs(delta) > Math.abs(targetImprovement.delta)) {
          targetImprovement = { metric, zone, delta, before, after, targetRange };
        }
      }

      // Track max regression using direction-aware detection
      // Only count as regression if it actually regressed toward wrong direction
      if (regressed) {
        const regressionAmount = Math.abs(delta);
        if (regressionAmount > maxRegression) {
          maxRegression = regressionAmount;
          maxRegressionInfo = { metric, zone, delta, before, after, targetRange };
        }
      }
    }
  }

  // Output markdown table
  console.log('## Metric Comparison\n');
  console.log('| Metric | Zone | Before | After | Delta | % Change |');
  console.log('|--------|------|--------|-------|-------|----------|');

  for (const zone of ZONES) {
    for (const metric of METRICS) {
      if (!deltas[zone] || !deltas[zone][metric]) continue;
      const d = deltas[zone][metric];
      const highlight = (targetMetric === metric && (targetZone === zone || zone === 'total')) ? '**' : '';
      console.log(`| ${highlight}${metric}${highlight} | ${zone} | ${formatValue(d.before)} | ${formatValue(d.after)} | ${formatValue(d.after - d.before)} | ${formatPercent(d.delta)} |`);
    }
  }

  console.log('\n---\n');

  // Evaluate success criteria
  let success = true;
  const issues = [];

  // Check target metric improvement
  if (targetMetric && targetImprovement) {
    console.log(`### Target Metric: ${targetMetric}`);
    console.log(`- Zone: ${targetImprovement.zone}`);
    console.log(`- Before: ${formatValue(targetImprovement.before)}`);
    console.log(`- After: ${formatValue(targetImprovement.after)}`);
    console.log(`- Delta: ${formatPercent(targetImprovement.delta)}`);

    if (targetImprovement.delta >= IMPROVEMENT_THRESHOLD) {
      console.log(`- Status: PASS (>= ${formatPercent(IMPROVEMENT_THRESHOLD)} threshold)`);
    } else {
      console.log(`- Status: FAIL (< ${formatPercent(IMPROVEMENT_THRESHOLD)} threshold)`);
      success = false;
      issues.push(`Target metric ${targetMetric} only improved ${formatPercent(targetImprovement.delta)}`);
    }
    console.log('');
  }

  // Check max regression (direction-aware)
  console.log('### Regression Check (Direction-Aware)');
  if (maxRegressionInfo) {
    console.log(`- Worst regression: ${maxRegressionInfo.metric} in ${maxRegressionInfo.zone}`);
    console.log(`- Amount: ${formatPercent(maxRegressionInfo.delta)}`);
    if (maxRegressionInfo.targetRange) {
      console.log(`- Target range: ${maxRegressionInfo.targetRange.min.toFixed(2)}-${maxRegressionInfo.targetRange.max.toFixed(2)}`);
    }

    if (maxRegression <= REGRESSION_THRESHOLD) {
      console.log(`- Status: PASS (<= ${formatPercent(REGRESSION_THRESHOLD)} threshold)`);
    } else {
      console.log(`- Status: FAIL (> ${formatPercent(REGRESSION_THRESHOLD)} threshold)`);
      success = false;
      issues.push(`Regression in ${maxRegressionInfo.metric}/${maxRegressionInfo.zone}: ${formatPercent(maxRegressionInfo.delta)}`);
    }
  } else {
    console.log('- No regressions detected (changes move toward target ranges)');
  }

  console.log('\n---\n');

  // Overall verdict
  console.log('## Verdict\n');
  if (success) {
    console.log('**SUCCESS** - All criteria met');
    console.log('- Target metric improved >= 5%');
    console.log('- No metric regressed > 2%');
  } else {
    console.log('**FAILED** - Criteria not met');
    for (const issue of issues) {
      console.log(`- ${issue}`);
    }
  }

  // Exit with appropriate code
  process.exit(success ? 0 : 1);
}

main();
