#!/usr/bin/env node
/**
 * Sensitivity Matrix Computation
 *
 * Computes sensitivity coefficients from parameter sweep data.
 * Sensitivity = normalized gradient of metric with respect to parameter.
 *
 * Usage:
 *   node scripts/sensitivity/compute-matrix.js [options]
 *
 * Options:
 *   --sweep <path>    Path to sweep data JSON (default: runs run-sweep.js)
 *   --output <path>   Output path (default: metrics/sensitivity-matrix.json)
 *   --verbose         Show progress output
 */

import { readFileSync, writeFileSync, existsSync } from 'fs';
import { dirname, join } from 'path';
import { fileURLToPath } from 'url';
import { execSync } from 'child_process';

const __dirname = dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = join(__dirname, '../..');
const DEFAULT_OUTPUT = join(PROJECT_ROOT, 'metrics/sensitivity-matrix.json');
const RUN_SWEEP_SCRIPT = join(__dirname, 'run-sweep.js');

// Parse CLI args
const args = process.argv.slice(2);
let sweepPath = null;
let outputPath = DEFAULT_OUTPUT;
let verbose = false;

for (let i = 0; i < args.length; i++) {
  if (args[i] === '--sweep' && args[i + 1]) {
    sweepPath = args[++i];
  } else if (args[i] === '--output' && args[i + 1]) {
    outputPath = args[++i];
  } else if (args[i] === '--verbose' || args[i] === '-v') {
    verbose = true;
  }
}

const METRICS = ['syncopation', 'density', 'velocityRange', 'voiceSeparation', 'regularity'];

/**
 * Compute linear regression slope (sensitivity)
 * Uses least squares: slope = sum((x-mx)(y-my)) / sum((x-mx)^2)
 */
function computeSlope(xValues, yValues) {
  const n = xValues.length;
  if (n < 2) return 0;

  const meanX = xValues.reduce((a, b) => a + b, 0) / n;
  const meanY = yValues.reduce((a, b) => a + b, 0) / n;

  let numerator = 0;
  let denominator = 0;

  for (let i = 0; i < n; i++) {
    const dx = xValues[i] - meanX;
    const dy = yValues[i] - meanY;
    numerator += dx * dy;
    denominator += dx * dx;
  }

  return denominator !== 0 ? numerator / denominator : 0;
}

/**
 * Normalize sensitivity to [-1, 1] range
 * Uses the typical metric range (0 to 1) and parameter range to normalize
 */
function normalizeSensitivity(slope, paramRange) {
  // slope is in units of (metric change / param change)
  // Normalize by expected metric range (0-1) and param range
  const expectedMetricRange = 1.0;
  const normalizedSlope = slope * paramRange / expectedMetricRange;

  // Clamp to [-1, 1]
  return Math.max(-1, Math.min(1, normalizedSlope));
}

/**
 * Compute R-squared for regression quality
 */
function computeRSquared(xValues, yValues, slope) {
  const n = xValues.length;
  if (n < 2) return 0;

  const meanX = xValues.reduce((a, b) => a + b, 0) / n;
  const meanY = yValues.reduce((a, b) => a + b, 0) / n;

  // Predicted values using linear regression
  const predictions = xValues.map(x => meanY + slope * (x - meanX));

  // Total sum of squares
  const ssTotal = yValues.reduce((sum, y) => sum + (y - meanY) ** 2, 0);

  // Residual sum of squares
  const ssResidual = yValues.reduce((sum, y, i) => sum + (y - predictions[i]) ** 2, 0);

  return ssTotal > 0 ? 1 - ssResidual / ssTotal : 0;
}

// =============================================================================
// Main
// =============================================================================

if (verbose) {
  console.error('Sensitivity Matrix Computation');
  console.error('='.repeat(50));
}

// Get sweep data
let sweepData;

if (sweepPath && existsSync(sweepPath)) {
  if (verbose) console.error(`Loading sweep data from ${sweepPath}...`);
  sweepData = JSON.parse(readFileSync(sweepPath, 'utf-8'));
} else {
  if (verbose) console.error('Running parameter sweep...');
  try {
    const result = execSync(`node "${RUN_SWEEP_SCRIPT}"`, {
      encoding: 'utf-8',
      maxBuffer: 50 * 1024 * 1024, // 50MB buffer
      cwd: PROJECT_ROOT,
    });
    sweepData = JSON.parse(result);
  } catch (err) {
    console.error(`Error running sweep: ${err.message}`);
    process.exit(1);
  }
}

if (!sweepData || !sweepData.parameters) {
  console.error('Error: Invalid sweep data');
  process.exit(1);
}

// Compute sensitivity matrix
const matrix = {};
const details = {};

for (const [paramName, paramData] of Object.entries(sweepData.parameters)) {
  if (verbose) console.error(`Computing sensitivities for ${paramName}...`);

  const { min, max, sweepPoints } = paramData;
  const paramRange = max - min;

  matrix[paramName] = {};
  details[paramName] = {};

  for (const metric of METRICS) {
    // Extract x (param values) and y (metric values) from sweep points
    const xValues = sweepPoints.map(p => p.value);
    const yValues = sweepPoints.map(p => p.metrics[metric]);

    // Compute linear regression slope
    const slope = computeSlope(xValues, yValues);
    const rSquared = computeRSquared(xValues, yValues, slope);

    // Normalize to [-1, 1]
    const sensitivity = normalizeSensitivity(slope, paramRange);

    matrix[paramName][metric] = Math.round(sensitivity * 1000) / 1000; // 3 decimal places

    details[paramName][metric] = {
      rawSlope: slope,
      sensitivity,
      rSquared: Math.round(rSquared * 1000) / 1000,
      dataPoints: sweepPoints.length,
      metricRange: {
        min: Math.min(...yValues),
        max: Math.max(...yValues),
      },
    };
  }
}

// Compute levers for each metric
const levers = {};

for (const metric of METRICS) {
  const paramSensitivities = Object.entries(matrix)
    .map(([param, sensitivities]) => ({
      param,
      sensitivity: sensitivities[metric],
      absSensitivity: Math.abs(sensitivities[metric]),
    }))
    .sort((a, b) => b.absSensitivity - a.absSensitivity);

  // Primary levers: |sensitivity| >= 0.3
  const primary = paramSensitivities
    .filter(p => p.absSensitivity >= 0.3)
    .map(p => ({ param: p.param, sensitivity: p.sensitivity }));

  // Secondary levers: 0.1 <= |sensitivity| < 0.3
  const secondary = paramSensitivities
    .filter(p => p.absSensitivity >= 0.1 && p.absSensitivity < 0.3)
    .map(p => ({ param: p.param, sensitivity: p.sensitivity }));

  // Low impact: |sensitivity| < 0.1
  const lowImpact = paramSensitivities
    .filter(p => p.absSensitivity < 0.1)
    .map(p => p.param);

  levers[metric] = { primary, secondary, lowImpact };
}

// Build output
const output = {
  generated_at: new Date().toISOString(),
  baseline_commit: execSync('git rev-parse HEAD', { encoding: 'utf-8', cwd: PROJECT_ROOT }).trim(),
  sweep_source: sweepPath || 'computed',
  matrix,
  levers,
  details,
};

// Write output
writeFileSync(outputPath, JSON.stringify(output, null, 2));

if (verbose) {
  console.error(`\nSensitivity matrix written to ${outputPath}`);
  console.error('\nMatrix summary:');
  console.error('');

  // Print matrix as table
  const header = ['Parameter', ...METRICS.map(m => m.substring(0, 8))];
  console.error(header.map(h => h.padEnd(12)).join(' '));
  console.error('-'.repeat(12 * (METRICS.length + 1)));

  for (const [param, sensitivities] of Object.entries(matrix)) {
    const row = [param.substring(0, 12).padEnd(12)];
    for (const metric of METRICS) {
      const val = sensitivities[metric];
      const str = val >= 0 ? `+${val.toFixed(2)}` : val.toFixed(2);
      row.push(str.padStart(8).padEnd(12));
    }
    console.error(row.join(' '));
  }

  console.error('');
  console.error('Lever summary:');
  for (const metric of METRICS) {
    const { primary, secondary } = levers[metric];
    const primaryStr = primary.length > 0
      ? primary.map(p => `${p.param}(${p.sensitivity > 0 ? '+' : ''}${p.sensitivity.toFixed(2)})`).join(', ')
      : 'none';
    console.error(`  ${metric}: ${primaryStr}`);
  }
}

console.log(JSON.stringify(output, null, 2));
