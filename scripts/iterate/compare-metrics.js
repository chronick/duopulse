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

function loadMetrics(filePath) {
  const content = fs.readFileSync(filePath, 'utf-8');
  const data = JSON.parse(content);

  // Handle both baseline.json format and raw metrics format
  if (data.metrics && data.metrics.pentagonStats) {
    return data.metrics.pentagonStats;
  } else if (data.pentagonStats) {
    return data.pentagonStats;
  } else {
    throw new Error(`Unexpected metrics format in ${filePath}`);
  }
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
  let baseline, newMetrics;
  try {
    baseline = loadMetrics(baselinePath);
    newMetrics = loadMetrics(newPath);
  } catch (err) {
    console.error(`Error loading metrics: ${err.message}`);
    process.exit(2);
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
      deltas[zone][metric] = { before, after, delta };

      // Track target metric improvement
      if (targetMetric === metric && (targetZone === zone || targetZone === null || zone === 'total')) {
        if (targetImprovement === null || Math.abs(delta) > Math.abs(targetImprovement.delta)) {
          targetImprovement = { metric, zone, delta, before, after };
        }
      }

      // Track max regression (negative delta is regression for most metrics)
      // Note: Some metrics like "regularity" in wild zone might want to decrease
      if (delta < -maxRegression) {
        maxRegression = -delta;
        maxRegressionInfo = { metric, zone, delta, before, after };
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

  // Check max regression
  console.log('### Regression Check');
  if (maxRegressionInfo) {
    console.log(`- Worst regression: ${maxRegressionInfo.metric} in ${maxRegressionInfo.zone}`);
    console.log(`- Amount: ${formatPercent(maxRegressionInfo.delta)}`);

    if (maxRegression <= REGRESSION_THRESHOLD) {
      console.log(`- Status: PASS (<= ${formatPercent(REGRESSION_THRESHOLD)} threshold)`);
    } else {
      console.log(`- Status: FAIL (> ${formatPercent(REGRESSION_THRESHOLD)} threshold)`);
      success = false;
      issues.push(`Regression in ${maxRegressionInfo.metric}/${maxRegressionInfo.zone}: ${formatPercent(maxRegressionInfo.delta)}`);
    }
  } else {
    console.log('- No regressions detected');
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
