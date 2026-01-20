#!/usr/bin/env node
/**
 * Pattern Generation Script
 *
 * Runs the C++ pattern_viz binary and generates JSON output for the frontend.
 * Produces sweep data, presets, seed variation tests, and fill patterns.
 */

import { execSync, spawnSync } from 'child_process';
import { existsSync, mkdirSync, writeFileSync } from 'fs';
import { dirname, join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = join(__dirname, '../..');
const PATTERN_VIZ = process.env.PATTERN_VIZ || join(PROJECT_ROOT, 'build/pattern_viz');
const OUTPUT_DIR = join(__dirname, 'public/data');

// Ensure output directory exists
if (!existsSync(OUTPUT_DIR)) {
  mkdirSync(OUTPUT_DIR, { recursive: true });
}

/**
 * Run pattern_viz and parse CSV output
 */
function runPatternViz(params = {}) {
  const defaults = {
    energy: 0.5,
    shape: 0.3,
    axisX: 0.5,
    axisY: 0.5,
    drift: 0.0,
    accent: 0.5,
    seed: 0xDEADBEEF,
    length: 64,
  };

  const p = { ...defaults, ...params };

  const args = [
    `--energy=${p.energy.toFixed(2)}`,
    `--shape=${p.shape.toFixed(2)}`,
    `--axis-x=${p.axisX.toFixed(2)}`,
    `--axis-y=${p.axisY.toFixed(2)}`,
    `--drift=${p.drift.toFixed(2)}`,
    `--accent=${p.accent.toFixed(2)}`,
    `--seed=${p.seed}`,
    `--length=${p.length}`,
    '--format=csv',
  ];

  const result = spawnSync(PATTERN_VIZ, args, { encoding: 'utf-8' });

  if (result.error) {
    throw new Error(`Failed to run pattern_viz: ${result.error.message}`);
  }

  if (result.status !== 0) {
    throw new Error(`pattern_viz exited with code ${result.status}: ${result.stderr}`);
  }

  return parseCSV(result.stdout, p);
}

/**
 * Run pattern_viz with --fill flag and parse JSON output
 */
function runPatternVizFill(params = {}) {
  const defaults = {
    energy: 0.5,
    shape: 0.3,
    axisX: 0.5,
    axisY: 0.5,
    drift: 0.0,
    accent: 0.5,
    seed: 0xDEADBEEF,
    length: 64,
  };

  const p = { ...defaults, ...params };

  const args = [
    `--energy=${p.energy.toFixed(2)}`,
    `--shape=${p.shape.toFixed(2)}`,
    `--axis-x=${p.axisX.toFixed(2)}`,
    `--axis-y=${p.axisY.toFixed(2)}`,
    `--drift=${p.drift.toFixed(2)}`,
    `--accent=${p.accent.toFixed(2)}`,
    `--seed=${p.seed}`,
    `--length=${p.length}`,
    '--fill',
    '--firmware',
  ];

  const result = spawnSync(PATTERN_VIZ, args, { encoding: 'utf-8' });

  if (result.error) {
    throw new Error(`Failed to run pattern_viz: ${result.error.message}`);
  }

  if (result.status !== 0) {
    throw new Error(`pattern_viz exited with code ${result.status}: ${result.stderr}`);
  }

  // Parse JSON output from --fill mode
  const fillPatterns = JSON.parse(result.stdout);

  return {
    energy: p.energy,
    shape: p.shape,
    axisX: p.axisX,
    axisY: p.axisY,
    drift: p.drift,
    accent: p.accent,
    seed: p.seed,
    length: p.length,
    fillPatterns,
  };
}

/**
 * Parse CSV output from pattern_viz
 */
function parseCSV(csv, params) {
  const lines = csv.trim().split('\n');
  const header = lines[0].split(',');

  const steps = lines.slice(1).map(line => {
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
      metric: row.metric,
    };
  });

  return {
    params: {
      energy: params.energy,
      shape: params.shape,
      axisX: params.axisX,
      axisY: params.axisY,
      drift: params.drift,
      accent: params.accent,
      seed: params.seed,
      length: params.length,
    },
    steps,
    // Computed masks for easy comparison
    masks: {
      v1: steps.reduce((m, s) => s.v1 ? m | (1 << s.step) : m, 0),
      v2: steps.reduce((m, s) => s.v2 ? m | (1 << s.step) : m, 0),
      aux: steps.reduce((m, s) => s.aux ? m | (1 << s.step) : m, 0),
    },
    hits: {
      v1: steps.filter(s => s.v1).length,
      v2: steps.filter(s => s.v2).length,
      aux: steps.filter(s => s.aux).length,
    },
  };
}

/**
 * Generate parameter sweep data
 */
function generateSweep(paramName, values, baseParams = {}) {
  console.log(`  Generating ${paramName} sweep...`);

  return values.map(val => {
    const params = { ...baseParams };
    params[paramName] = val;
    return runPatternViz(params);
  });
}

/**
 * Generate seed variation test
 */
function generateSeedVariation(baseParams, numSeeds = 8) {
  const seeds = [
    0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCD1234,
    0x87654321, 0xFEEDFACE, 0xC0FFEE42, 0xBEEFCAFE,
  ].slice(0, numSeeds);

  const patterns = seeds.map(seed => runPatternViz({ ...baseParams, seed }));

  // Compute uniqueness
  const v1Masks = new Set(patterns.map(p => p.masks.v1));
  const v2Masks = new Set(patterns.map(p => p.masks.v2));
  const auxMasks = new Set(patterns.map(p => p.masks.aux));

  const variationScore = (unique, total) => total <= 1 ? 0 : (unique - 1) / (total - 1);

  return {
    params: baseParams,
    seeds: seeds.map(s => `0x${s.toString(16).toUpperCase()}`),
    patterns,
    variation: {
      v1: { unique: v1Masks.size, total: seeds.length, score: variationScore(v1Masks.size, seeds.length) },
      v2: { unique: v2Masks.size, total: seeds.length, score: variationScore(v2Masks.size, seeds.length) },
      aux: { unique: auxMasks.size, total: seeds.length, score: variationScore(auxMasks.size, seeds.length) },
    },
  };
}

/**
 * Generate fill pattern sweep across energy values
 */
function generateFillSweep() {
  console.log('  Generating fill pattern sweep...');

  const energyValues = [];
  for (let e = 0.0; e <= 1.0; e += 0.1) {
    energyValues.push(Math.round(e * 10) / 10); // Avoid floating point issues
  }

  return energyValues.map(energy => {
    const result = runPatternVizFill({ energy, shape: 0.3 });
    return {
      energy: result.energy,
      fillPatterns: result.fillPatterns,
    };
  });
}

/**
 * Define presets for demonstration
 */
const PRESETS = [
  { name: 'Minimal Techno', description: 'Sparse, meditative patterns', energy: 0.20, shape: 0.0, axisX: 0.0, axisY: 0.3, drift: 0.0, accent: 0.4 },
  { name: 'Four on Floor', description: 'Classic house kick pattern', energy: 0.23, shape: 0.0, axisX: 0.0, axisY: 0.3, drift: 0.0, accent: 0.5 },
  { name: 'Driving Techno', description: 'Energetic but controlled patterns', energy: 0.30, shape: 0.2, axisX: 0.6, axisY: 0.4, drift: 0.1, accent: 0.6 },
  { name: 'Syncopated Groove', description: 'Off-beat emphasis', energy: 0.5, shape: 0.5, axisX: 0.4, axisY: 0.6, drift: 0.3, accent: 0.5 },
  { name: 'Broken Beat', description: 'IDM-style complexity', energy: 0.6, shape: 0.7, axisX: 0.5, axisY: 0.7, drift: 0.5, accent: 0.6 },
  { name: 'Chaotic IDM', description: 'Wild, unpredictable patterns', energy: 0.8, shape: 0.9, axisX: 0.7, axisY: 0.8, drift: 0.8, accent: 0.7 },
  { name: 'Sparse Dub', description: 'Minimal, breathing space', energy: 0.2, shape: 0.3, axisX: 0.2, axisY: 0.2, drift: 0.2, accent: 0.3 },
  { name: 'Dense Gabber', description: 'Maximum intensity', energy: 1.0, shape: 0.4, axisX: 0.8, axisY: 0.5, drift: 0.2, accent: 0.9 },
];

// ============================================================================
// Main
// ============================================================================

console.log('Pattern Generation for DuoPulse v5 Eval Dashboard');
console.log('='.repeat(50));

// Check pattern_viz exists
if (!existsSync(PATTERN_VIZ)) {
  console.error(`Error: pattern_viz not found at ${PATTERN_VIZ}`);
  console.error('Run "make pattern-viz" first to build the tool.');
  process.exit(1);
}

console.log(`Using pattern_viz: ${PATTERN_VIZ}`);
console.log(`Output directory: ${OUTPUT_DIR}`);
console.log();

// 1. Generate parameter sweeps
console.log('Generating parameter sweeps...');

const sweeps = {
  shape: generateSweep('shape', [0.0, 0.15, 0.30, 0.50, 0.70, 0.85, 1.0], { energy: 0.5 }),
  energy: generateSweep('energy', [0.0, 0.2, 0.4, 0.6, 0.8, 1.0], { shape: 0.3 }),
  axisX: generateSweep('axisX', [0.0, 0.25, 0.5, 0.75, 1.0], { energy: 0.5, shape: 0.4 }),
  axisY: generateSweep('axisY', [0.0, 0.25, 0.5, 0.75, 1.0], { energy: 0.5, shape: 0.4 }),
  drift: generateSweep('drift', [0.0, 0.25, 0.5, 0.75, 1.0], { energy: 0.5, shape: 0.5 }),
  accent: generateSweep('accent', [0.0, 0.25, 0.5, 0.75, 1.0], { energy: 0.5, shape: 0.4 }),
};

writeFileSync(join(OUTPUT_DIR, 'sweeps.json'), JSON.stringify(sweeps, null, 2));
console.log('  Written sweeps.json');

// 1b. Generate cross-product ENERGY×SHAPE sweeps for zone coverage
console.log('Generating ENERGY zone sweeps...');

// ENERGY zone centers: MINIMAL=0.10, GROOVE=0.35, BUILD=0.60, PEAK=0.85
const ENERGY_ZONE_VALUES = [0.10, 0.35, 0.60, 0.85];
const SHAPE_VALUES = [0.0, 0.15, 0.30, 0.50, 0.70, 0.85, 1.0];

const energyZoneSweeps = {};
const energyZoneNames = ['minimal', 'groove', 'build', 'peak'];

for (let i = 0; i < ENERGY_ZONE_VALUES.length; i++) {
  const energy = ENERGY_ZONE_VALUES[i];
  const zoneName = energyZoneNames[i];
  console.log(`  Generating ${zoneName} zone sweep (energy=${energy})...`);

  energyZoneSweeps[zoneName] = SHAPE_VALUES.map(shape => {
    return runPatternViz({ energy, shape });
  });
}

writeFileSync(join(OUTPUT_DIR, 'energy-zone-sweeps.json'), JSON.stringify(energyZoneSweeps, null, 2));
console.log('  Written energy-zone-sweeps.json');

// 2. Generate presets
console.log('Generating presets...');

const presets = PRESETS.map(preset => ({
  ...preset,
  pattern: runPatternViz(preset),
}));

writeFileSync(join(OUTPUT_DIR, 'presets.json'), JSON.stringify(presets, null, 2));
console.log('  Written presets.json');

// 3. Generate seed variation tests
console.log('Generating seed variation tests...');

const seedTests = [];
const energyValues = [0.2, 0.4, 0.6, 0.8];
const shapeValues = [0.0, 0.15, 0.30, 0.50, 0.70, 0.85, 1.0];
const driftValues = [0.0, 0.25, 0.5, 0.75];

for (const energy of energyValues) {
  for (const shape of shapeValues) {
    for (const drift of driftValues) {
      seedTests.push(generateSeedVariation({ energy, shape, drift }));
    }
  }
}

// Compute overall scores
const v1Scores = seedTests.map(t => t.variation.v1.score);
const v2Scores = seedTests.map(t => t.variation.v2.score);
const auxScores = seedTests.map(t => t.variation.aux.score);

const avg = arr => arr.reduce((a, b) => a + b, 0) / arr.length;

const seedVariation = {
  tests: seedTests,
  summary: {
    v1: { avgScore: avg(v1Scores), minScore: Math.min(...v1Scores), maxScore: Math.max(...v1Scores) },
    v2: { avgScore: avg(v2Scores), minScore: Math.min(...v2Scores), maxScore: Math.max(...v2Scores) },
    aux: { avgScore: avg(auxScores), minScore: Math.min(...auxScores), maxScore: Math.max(...auxScores) },
    overall: (avg(v1Scores) + avg(v2Scores) + avg(auxScores)) / 3,
    pass: avg(v2Scores) >= 0.5,
  },
};

writeFileSync(join(OUTPUT_DIR, 'seed-variation.json'), JSON.stringify(seedVariation, null, 2));
console.log('  Written seed-variation.json');

// 4. Generate fill patterns
console.log('Generating fill patterns...');

const fills = {
  energySweep: generateFillSweep(),
};

writeFileSync(join(OUTPUT_DIR, 'fills.json'), JSON.stringify(fills, null, 2));
console.log('  Written fills.json');

// 5. Generate metadata
console.log('Generating metadata...');

const metadata = {
  generated: new Date().toISOString(),
  patternViz: PATTERN_VIZ,
  version: '1.0.0',
  patternLength: 64,
  numPresets: presets.length,
  numSeedTests: seedTests.length,
  sweepParams: Object.keys(sweeps),
};

writeFileSync(join(OUTPUT_DIR, 'metadata.json'), JSON.stringify(metadata, null, 2));
console.log('  Written metadata.json');

console.log();
console.log('Pattern generation complete!');
console.log(`Output: ${OUTPUT_DIR}`);
