#!/usr/bin/env node
/**
 * tournament-select.js
 *
 * Tournament selection for ensemble search.
 * Compares candidates on target metric, filters regressions, selects top K.
 *
 * Usage:
 *   node tournament-select.js --results results.json --target syncopation --top-k 2
 *   node tournament-select.js --results results.json --target density --baseline baseline.json
 *
 * Input format (results.json):
 *   [
 *     {
 *       "id": "A",
 *       "weights": {...},
 *       "metrics": {
 *         "syncopation": 0.48,
 *         "density": 0.55,
 *         ...
 *       }
 *     },
 *     ...
 *   ]
 *
 * Output format:
 *   {
 *     "winners": ["B", "A"],
 *     "rankings": [
 *       { "id": "B", "score": 0.52, "rank": 1, "reason": "Best syncopation" },
 *       ...
 *     ]
 *   }
 */

const fs = require('fs');
const path = require('path');

const PROJECT_ROOT = path.join(__dirname, '../..');
const ENSEMBLE_CONFIG_PATH = path.join(PROJECT_ROOT, 'metrics/ensemble-config.json');
const BASELINE_PATH = path.join(PROJECT_ROOT, 'metrics/baseline.json');

// Metric target ranges by zone (from evaluate-expressiveness.js)
const METRIC_TARGETS = {
  syncopation: { stable: [0.0, 0.22], syncopated: [0.22, 0.48], wild: [0.42, 0.75] },
  density: { stable: [0.15, 0.32], syncopated: [0.25, 0.48], wild: [0.32, 0.65] },
  velocityRange: { stable: [0.12, 0.38], syncopated: [0.32, 0.58], wild: [0.25, 0.72] },
  voiceSeparation: { stable: [0.62, 0.88], syncopated: [0.52, 0.78], wild: [0.32, 0.68] },
  regularity: { stable: [0.72, 1.0], syncopated: [0.42, 0.68], wild: [0.12, 0.48] },
};

/**
 * Load JSON file
 */
function loadJSON(filePath) {
  if (!fs.existsSync(filePath)) {
    console.error(`Error: File not found: ${filePath}`);
    process.exit(1);
  }
  return JSON.parse(fs.readFileSync(filePath, 'utf-8'));
}

/**
 * Extract metric value from candidate metrics object
 * Handles nested structure from expressiveness evaluation
 */
function getMetricValue(metrics, metricName) {
  // Direct access
  if (metrics[metricName] !== undefined) {
    return metrics[metricName];
  }

  // Check pentagonStats.total
  if (metrics.pentagonStats && metrics.pentagonStats.total) {
    return metrics.pentagonStats.total[metricName];
  }

  // Check raw metrics
  if (metrics.raw && metrics.raw[metricName] !== undefined) {
    return metrics.raw[metricName];
  }

  return null;
}

/**
 * Get target range center for a metric (average across zones)
 */
function getTargetCenter(metricName) {
  const targets = METRIC_TARGETS[metricName];
  if (!targets) return 0.5;

  const centers = Object.values(targets).map(([min, max]) => (min + max) / 2);
  return centers.reduce((a, b) => a + b, 0) / centers.length;
}

/**
 * Check if a metric value is a regression from baseline
 * Returns { isRegression, amount, direction }
 */
function checkRegression(metricName, baseValue, newValue, regressionTolerance) {
  if (baseValue === null || newValue === null) {
    return { isRegression: false, amount: 0, direction: 'unknown' };
  }

  const targetCenter = getTargetCenter(metricName);
  const baseDist = Math.abs(baseValue - targetCenter);
  const newDist = Math.abs(newValue - targetCenter);

  // Regression = moved further from target center
  const distChange = newDist - baseDist;

  // Only a regression if distance increased beyond tolerance
  const isRegression = distChange > regressionTolerance;

  return {
    isRegression,
    amount: distChange,
    direction: newValue > baseValue ? 'increased' : 'decreased',
    baseDist,
    newDist,
  };
}

/**
 * Score a candidate's improvement on target metric
 * Higher = better (moved toward target center)
 */
function scoreImprovement(metricName, baseValue, newValue) {
  if (baseValue === null || newValue === null) {
    return 0;
  }

  const targetCenter = getTargetCenter(metricName);
  const baseDist = Math.abs(baseValue - targetCenter);
  const newDist = Math.abs(newValue - targetCenter);

  // Positive score = improvement (moved closer to center)
  return baseDist - newDist;
}

/**
 * Parse command line arguments
 */
function parseArgs() {
  const args = process.argv.slice(2);
  const opts = {
    results: null,
    target: 'syncopation',
    topK: 2,
    baseline: null,
    regressionTolerance: null,
    verbose: false,
  };

  for (let i = 0; i < args.length; i++) {
    const arg = args[i];

    if (arg === '--results' || arg === '-r') {
      opts.results = args[++i];
    } else if (arg === '--target' || arg === '-t') {
      opts.target = args[++i];
    } else if (arg === '--top-k' || arg === '-k') {
      opts.topK = parseInt(args[++i], 10);
    } else if (arg === '--baseline' || arg === '-b') {
      opts.baseline = args[++i];
    } else if (arg === '--regression-tolerance') {
      opts.regressionTolerance = parseFloat(args[++i]);
    } else if (arg === '--verbose' || arg === '-v') {
      opts.verbose = true;
    } else if (arg === '--help' || arg === '-h') {
      console.log(`
Usage: tournament-select.js [options]

Options:
  --results, -r <file>        Results JSON file with candidate metrics (required)
  --target, -t <metric>       Target metric to optimize (default: syncopation)
  --top-k, -k <N>             Number of winners to select (default: 2)
  --baseline, -b <file>       Baseline metrics file (default: metrics/baseline.json)
  --regression-tolerance <f>  Max allowed regression (default: from ensemble-config.json)
  --verbose, -v               Print detailed analysis
  --help, -h                  Show this help

Metrics:
  syncopation, density, velocityRange, voiceSeparation, regularity
`);
      process.exit(0);
    }
  }

  if (!opts.results) {
    console.error('Error: --results is required');
    process.exit(1);
  }

  return opts;
}

/**
 * Main
 */
function main() {
  const opts = parseArgs();

  // Load configurations
  const ensembleConfig = loadJSON(ENSEMBLE_CONFIG_PATH);
  const regressionTolerance =
    opts.regressionTolerance ?? ensembleConfig.search.regression_tolerance;

  // Load results
  const results = loadJSON(opts.results);

  // Load baseline
  const baselinePath = opts.baseline || BASELINE_PATH;
  const baseline = loadJSON(baselinePath);
  const baselineMetrics = baseline.metrics || baseline;

  // Get baseline value for target metric
  const baseTarget = getMetricValue(baselineMetrics, opts.target);

  if (opts.verbose) {
    console.error(`Target metric: ${opts.target}`);
    console.error(`Baseline ${opts.target}: ${baseTarget?.toFixed(4) || 'N/A'}`);
    console.error(`Regression tolerance: ${regressionTolerance}`);
    console.error('');
  }

  // Analyze each candidate
  const analysis = results.map((candidate) => {
    const metrics = candidate.metrics;
    const targetValue = getMetricValue(metrics, opts.target);
    const improvement = scoreImprovement(opts.target, baseTarget, targetValue);

    // Check regressions on all metrics
    const regressions = [];
    for (const metricName of Object.keys(METRIC_TARGETS)) {
      const baseVal = getMetricValue(baselineMetrics, metricName);
      const newVal = getMetricValue(metrics, metricName);
      const regCheck = checkRegression(metricName, baseVal, newVal, regressionTolerance);

      if (regCheck.isRegression) {
        regressions.push({
          metric: metricName,
          ...regCheck,
        });
      }
    }

    return {
      id: candidate.id,
      weights: candidate.weights,
      targetValue,
      improvement,
      regressions,
      hasRegressions: regressions.length > 0,
    };
  });

  if (opts.verbose) {
    console.error('Candidate Analysis:');
    for (const a of analysis) {
      console.error(`  ${a.id}: ${opts.target}=${a.targetValue?.toFixed(4) || 'N/A'} ` +
        `improvement=${a.improvement?.toFixed(4) || 'N/A'} ` +
        `regressions=${a.regressions.length}`);
    }
    console.error('');
  }

  // Sort by improvement (descending)
  const sorted = [...analysis].sort((a, b) => {
    // Prioritize non-regressing candidates
    if (a.hasRegressions !== b.hasRegressions) {
      return a.hasRegressions ? 1 : -1;
    }
    // Then by improvement
    return (b.improvement || 0) - (a.improvement || 0);
  });

  // Build rankings with reasons
  const rankings = sorted.map((a, idx) => {
    let reason;

    if (idx === 0 && !a.hasRegressions) {
      reason = `Best ${opts.target}`;
    } else if (!a.hasRegressions) {
      reason = `#${idx + 1}, no regressions`;
    } else {
      const regMetrics = a.regressions.map((r) => r.metric).join(', ');
      reason = `Regressed on ${regMetrics}`;
    }

    return {
      id: a.id,
      score: a.targetValue,
      improvement: a.improvement,
      rank: idx + 1,
      reason,
      hasRegressions: a.hasRegressions,
      regressions: a.regressions,
    };
  });

  // Select top K (only non-regressing candidates)
  const eligible = rankings.filter((r) => !r.hasRegressions);
  const winners = eligible.slice(0, opts.topK).map((r) => r.id);

  // If not enough non-regressing candidates, include best of the rest
  if (winners.length < opts.topK) {
    const remaining = rankings
      .filter((r) => !winners.includes(r.id))
      .slice(0, opts.topK - winners.length);
    winners.push(...remaining.map((r) => r.id));
  }

  // Output result
  const output = {
    target: opts.target,
    baseline: baseTarget,
    winners,
    rankings,
    summary: {
      total: results.length,
      nonRegressing: eligible.length,
      selected: winners.length,
      bestImprovement: rankings[0]?.improvement,
    },
  };

  console.log(JSON.stringify(output, null, 2));
}

main();
