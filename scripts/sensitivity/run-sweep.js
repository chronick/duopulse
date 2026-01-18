#!/usr/bin/env node
/**
 * Parameter Sweep Runner
 *
 * Runs pattern_viz across a range of weight parameter values
 * to collect Pentagon metrics for sensitivity analysis.
 *
 * Usage:
 *   node scripts/sensitivity/run-sweep.js [options]
 *
 * Options:
 *   --parameter <name>   Only sweep this parameter (default: all)
 *   --output <path>      Output file path (default: stdout)
 *   --verbose           Show progress output
 */

import { readFileSync, writeFileSync, existsSync } from 'fs';
import { dirname, join } from 'path';
import { fileURLToPath } from 'url';
import { spawnSync } from 'child_process';

const __dirname = dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = join(__dirname, '../..');
const PATTERN_VIZ = process.env.PATTERN_VIZ || join(PROJECT_ROOT, 'build/pattern_viz');
const SWEEP_CONFIG = join(PROJECT_ROOT, 'metrics/sweep-config.json');

// Map sweep parameter names to pattern_viz CLI argument names
const PARAM_TO_CLI = {
  euclideanFadeStart: '--euclidean-fade-start',
  euclideanFadeEnd: '--euclidean-fade-end',
  syncopationCenter: '--syncopation-center',
  syncopationWidth: '--syncopation-width',
  randomFadeStart: '--random-fade-start',
  randomFadeEnd: '--random-fade-end',
};

// Parse CLI args
const args = process.argv.slice(2);
let targetParam = null;
let outputPath = null;
let verbose = false;

for (let i = 0; i < args.length; i++) {
  if (args[i] === '--parameter' && args[i + 1]) {
    targetParam = args[++i];
  } else if (args[i] === '--output' && args[i + 1]) {
    outputPath = args[++i];
  } else if (args[i] === '--verbose' || args[i] === '-v') {
    verbose = true;
  }
}

// LHL weights for syncopation calculation
const METRIC_WEIGHTS_32 = [
  1.00, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
  0.90, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
  0.95, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
  0.90, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
];

/**
 * Run pattern_viz with given parameters and weight overrides
 */
function runPatternViz(controlParams, weightOverrides = {}) {
  const args = [
    `--energy=${controlParams.energy.toFixed(2)}`,
    `--shape=${controlParams.shape.toFixed(2)}`,
    `--axis-x=${controlParams.axisX.toFixed(2)}`,
    `--axis-y=${controlParams.axisY.toFixed(2)}`,
    `--drift=${controlParams.drift.toFixed(2)}`,
    `--accent=${controlParams.accent.toFixed(2)}`,
    `--seed=${controlParams.seed}`,
    `--length=${controlParams.length}`,
    '--format=csv',
  ];

  // Add weight override arguments
  for (const [param, value] of Object.entries(weightOverrides)) {
    const cliArg = PARAM_TO_CLI[param];
    if (cliArg) {
      args.push(`${cliArg}=${value.toFixed(2)}`);
    }
  }

  const result = spawnSync(PATTERN_VIZ, args, { encoding: 'utf-8' });

  if (result.error || result.status !== 0) {
    throw new Error(`pattern_viz failed: ${result.error || result.stderr}`);
  }

  return parseCSV(result.stdout);
}

/**
 * Parse CSV output from pattern_viz
 */
function parseCSV(csv) {
  const lines = csv.trim().split('\n');
  const header = lines[0].split(',');

  return lines.slice(1).map(line => {
    const values = line.split(',');
    const row = {};
    header.forEach((col, i) => {
      const val = values[i];
      if (col === 'step' || col === 'v1' || col === 'v2' || col === 'aux') {
        row[col] = parseInt(val, 10);
      } else {
        row[col] = parseFloat(val);
      }
    });
    return {
      step: row.step,
      v1: row.v1 === 1,
      v2: row.v2 === 1,
      aux: row.aux === 1,
      v1Vel: row.v1_vel,
      v2Vel: row.v2_vel,
      auxVel: row.aux_vel,
    };
  });
}

/**
 * Compute Pentagon metrics from pattern steps
 */
function computePentagonMetrics(steps, length = 32) {
  const v1Hits = new Array(length).fill(false);
  const v2Hits = new Array(length).fill(false);
  const auxHits = new Array(length).fill(false);

  for (const s of steps) {
    v1Hits[s.step] = s.v1;
    v2Hits[s.step] = s.v2;
    auxHits[s.step] = s.aux;
  }

  return {
    syncopation: computeSyncopation(v1Hits, length),
    density: computeDensity(v1Hits, v2Hits, auxHits, length),
    velocityRange: computeVelocityRange(steps),
    voiceSeparation: computeVoiceSeparation(v1Hits, v2Hits, auxHits, length),
    regularity: computeRegularity(v1Hits, length),
  };
}

function computeSyncopation(v1Hits, length) {
  const weights = METRIC_WEIGHTS_32.slice(0, length);
  let tension = 0;
  let maxTension = 0;

  for (let i = 0; i < length; i++) {
    if (v1Hits[i]) {
      const nextPos = (i + 1) % length;
      const weightDiff = weights[nextPos] - weights[i];
      if (weightDiff > 0 && !v1Hits[nextPos]) {
        tension += weightDiff;
      }
      maxTension += Math.max(0, weights[(i + 1) % length] - weights[i]);
    }
  }

  return maxTension > 0 ? Math.min(1.0, tension / maxTension) : 0;
}

function computeDensity(v1Hits, v2Hits, auxHits, length) {
  let active = 0;
  for (let i = 0; i < length; i++) {
    if (v1Hits[i] || v2Hits[i] || auxHits[i]) active++;
  }
  return active / length;
}

function computeVelocityRange(steps) {
  const velocities = [];
  for (const s of steps) {
    if (s.v1 && s.v1Vel > 0) velocities.push(s.v1Vel);
    if (s.v2 && s.v2Vel > 0) velocities.push(s.v2Vel);
    if (s.aux && s.auxVel > 0) velocities.push(s.auxVel);
  }
  if (velocities.length < 2) return 0;
  return Math.max(...velocities) - Math.min(...velocities);
}

function computeVoiceSeparation(v1Hits, v2Hits, auxHits, length) {
  let overlap = 0;
  let totalActive = 0;

  for (let i = 0; i < length; i++) {
    const voicesActive = (v1Hits[i] ? 1 : 0) + (v2Hits[i] ? 1 : 0) + (auxHits[i] ? 1 : 0);
    if (voicesActive > 0) totalActive++;
    if (voicesActive >= 2) overlap++;
  }

  return totalActive > 0 ? 1.0 - (overlap / totalActive) : 0.5;
}

function computeRegularity(v1Hits, length) {
  const hitPositions = [];
  for (let i = 0; i < length; i++) {
    if (v1Hits[i]) hitPositions.push(i);
  }

  if (hitPositions.length < 2) return 0.5;

  const gaps = [];
  for (let i = 0; i < hitPositions.length; i++) {
    const nextIdx = (i + 1) % hitPositions.length;
    let gap;
    if (nextIdx === 0) {
      gap = (length - hitPositions[i]) + hitPositions[0];
    } else {
      gap = hitPositions[nextIdx] - hitPositions[i];
    }
    gaps.push(gap);
  }

  const mean = gaps.reduce((a, b) => a + b, 0) / gaps.length;
  if (mean === 0) return 1.0;

  const variance = gaps.reduce((a, g) => a + (g - mean) ** 2, 0) / gaps.length;
  const stdDev = Math.sqrt(variance);
  const cv = stdDev / mean;

  return Math.max(0, 1.0 - Math.min(1.0, cv));
}

// =============================================================================
// Main
// =============================================================================

if (verbose) {
  console.error('Parameter Sweep Runner');
  console.error('='.repeat(50));
}

// Check dependencies
if (!existsSync(PATTERN_VIZ)) {
  console.error(`Error: pattern_viz not found at ${PATTERN_VIZ}`);
  console.error('Run "make pattern-viz" first.');
  process.exit(1);
}

if (!existsSync(SWEEP_CONFIG)) {
  console.error(`Error: sweep config not found at ${SWEEP_CONFIG}`);
  process.exit(1);
}

// Load sweep config
const sweepConfig = JSON.parse(readFileSync(SWEEP_CONFIG, 'utf-8'));
const { parameters, controlSweep, baseline } = sweepConfig;

// Determine which parameters to sweep
const paramsToSweep = targetParam
  ? { [targetParam]: parameters[targetParam] }
  : parameters;

if (targetParam && !parameters[targetParam]) {
  console.error(`Error: Unknown parameter "${targetParam}"`);
  console.error(`Available: ${Object.keys(parameters).join(', ')}`);
  process.exit(1);
}

// Run sweeps
const results = {};

for (const [paramName, paramConfig] of Object.entries(paramsToSweep)) {
  if (verbose) {
    console.error(`\nSweeping ${paramName}...`);
  }

  const { min, max, steps } = paramConfig;
  const sweepPoints = [];

  for (let i = 0; i < steps; i++) {
    const value = min + (i / (steps - 1)) * (max - min);

    // Build weight overrides for this sweep point
    const weightOverrides = { [paramName]: value };

    // Collect metrics across control parameter combinations
    const controlMetrics = [];

    for (const energy of controlSweep.energy) {
      for (const shape of controlSweep.shape) {
        const controlParams = { ...baseline, energy, shape };

        try {
          const steps = runPatternViz(controlParams, weightOverrides);
          const metrics = computePentagonMetrics(steps, controlParams.length);
          controlMetrics.push({ energy, shape, metrics });
        } catch (err) {
          if (verbose) {
            console.error(`  Warning: Failed at ${paramName}=${value.toFixed(2)}, energy=${energy}, shape=${shape}: ${err.message}`);
          }
        }
      }
    }

    // Average metrics across all control combinations
    const avgMetrics = {
      syncopation: 0,
      density: 0,
      velocityRange: 0,
      voiceSeparation: 0,
      regularity: 0,
    };

    if (controlMetrics.length > 0) {
      for (const { metrics } of controlMetrics) {
        for (const key of Object.keys(avgMetrics)) {
          avgMetrics[key] += metrics[key];
        }
      }
      for (const key of Object.keys(avgMetrics)) {
        avgMetrics[key] /= controlMetrics.length;
      }
    }

    sweepPoints.push({
      value,
      metrics: avgMetrics,
      samples: controlMetrics.length,
    });

    if (verbose) {
      console.error(`  ${paramName}=${value.toFixed(2)}: ${controlMetrics.length} samples`);
    }
  }

  results[paramName] = {
    min,
    max,
    steps,
    sweepPoints,
  };
}

// Output
const output = {
  generated_at: new Date().toISOString(),
  sweep_config: SWEEP_CONFIG,
  parameters: results,
};

const outputJson = JSON.stringify(output, null, 2);

if (outputPath) {
  writeFileSync(outputPath, outputJson);
  if (verbose) {
    console.error(`\nWritten to ${outputPath}`);
  }
} else {
  console.log(outputJson);
}

if (verbose) {
  console.error('\nSweep complete!');
}
