#!/usr/bin/env node
/**
 * Expressiveness Evaluation Script
 *
 * Computes Pentagon of Musicality metrics for pattern evaluation.
 * Matches Python generate-pattern-html.py output format.
 */

import { readFileSync, writeFileSync, existsSync } from 'fs';
import { dirname, join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const DATA_DIR = join(__dirname, 'public/data');

// LHL Metric weights for 32-step pattern (2 bars of 4/4 at 16th notes)
const METRIC_WEIGHTS_32 = [
  1.00, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
  0.90, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
  0.95, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
  0.90, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
];

// Pentagon metric definitions (matching Python PENTAGON_METRICS)
const PENTAGON_METRICS = {
  syncopation: {
    short: 'Sync',
    name: 'Syncopation',
    description: 'Syncopation creates groove and forward motion. Too little feels mechanical; too much loses the pulse.',
    targetByZone: { stable: '0.00-0.22', syncopated: '0.22-0.48', wild: '0.42-0.75' },
  },
  density: {
    short: 'Dens',
    name: 'Density',
    description: 'Density sets the energy level. Match it to ENERGY parameter for zone compliance.',
    targetByZone: { stable: '0.15-0.32', syncopated: '0.25-0.48', wild: '0.32-0.65' },
  },
  velocityRange: {
    short: 'VelRng',
    name: 'Velocity Range',
    description: 'Velocity variation humanizes patterns. Ghost notes add texture without changing rhythm.',
    targetByZone: { stable: '0.12-0.38', syncopated: '0.32-0.58', wild: '0.25-0.72' },
  },
  voiceSeparation: {
    short: 'VoiceSep',
    name: 'Voice Separation',
    description: 'Separation affects clarity vs. power. High DRIFT should increase separation.',
    targetByZone: { stable: '0.62-0.88', syncopated: '0.52-0.78', wild: '0.32-0.68' },
  },
  regularity: {
    short: 'Reg',
    name: 'Regularity',
    description: 'Regularity = danceability. Stable patterns need high regularity; wild patterns break it.',
    targetByZone: { stable: '0.72-1.00', syncopated: '0.42-0.68', wild: '0.12-0.48' },
  },
};

/**
 * Parse target range string like '0.00-0.22' into [min, max]
 */
function parseTargetRange(targetStr) {
  const parts = targetStr.split('-');
  if (parts.length === 2) {
    return [parseFloat(parts[0]), parseFloat(parts[1])];
  }
  return [0.0, 1.0];
}

/**
 * Check if value is in target range
 * Returns: { inRange, distance, statusText }
 */
function checkInRange(value, targetStr) {
  const [minVal, maxVal] = parseTargetRange(targetStr);

  if (minVal <= value && value <= maxVal) {
    return { inRange: true, distance: 0, statusText: `In range (${minVal.toFixed(2)}-${maxVal.toFixed(2)})` };
  }

  if (value < minVal) {
    const distance = minVal - value;
    return { inRange: false, distance, statusText: `Below target (need +${distance.toFixed(2)})` };
  }

  const distance = value - maxVal;
  return { inRange: false, distance, statusText: `Above target (need -${distance.toFixed(2)})` };
}

/**
 * Compute alignment score (0-1) for a single metric
 * 1.0 = perfectly centered, 0.0 = far outside
 */
function computeAlignmentScore(value, targetStr) {
  const [minVal, maxVal] = parseTargetRange(targetStr);
  const rangeCenter = (minVal + maxVal) / 2;
  const rangeWidth = (maxVal - minVal) / 2;

  const distanceFromCenter = Math.abs(value - rangeCenter);
  const tolerance = Math.max(rangeWidth * 2, 0.2);

  return Math.max(0, 1.0 - distanceFromCenter / tolerance);
}

function getShapeZone(shape) {
  if (shape < 0.3) return 'stable';
  if (shape < 0.7) return 'syncopated';
  return 'wild';
}

function computeSyncopation(v1Hits, length = 32) {
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

function computeDensity(v1Hits, v2Hits, auxHits, length = 32) {
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

function computeVoiceSeparation(v1Hits, v2Hits, auxHits, length = 32) {
  let overlap = 0;
  let totalActive = 0;

  for (let i = 0; i < length; i++) {
    const voicesActive = (v1Hits[i] ? 1 : 0) + (v2Hits[i] ? 1 : 0) + (auxHits[i] ? 1 : 0);
    if (voicesActive > 0) totalActive++;
    if (voicesActive >= 2) overlap++;
  }

  return totalActive > 0 ? 1.0 - (overlap / totalActive) : 0.5;
}

function computeRegularity(v1Hits, length = 32) {
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

function scoreMetric(raw, targetRange) {
  const [min, max] = targetRange;
  const center = (min + max) / 2;
  const width = (max - min) / 2;
  const distance = Math.abs(raw - center);
  return Math.max(0, 1.0 - (distance / width) ** 2);
}

function computePentagonMetrics(pattern) {
  const { steps, params } = pattern;
  const length = params.length;
  const shape = params.shape;
  const energy = params.energy;
  const accent = params.accent;

  const v1Hits = new Array(length).fill(false);
  const v2Hits = new Array(length).fill(false);
  const auxHits = new Array(length).fill(false);

  for (const s of steps) {
    v1Hits[s.step] = s.v1;
    v2Hits[s.step] = s.v2;
    auxHits[s.step] = s.aux;
  }

  const raw = {
    syncopation: computeSyncopation(v1Hits, length),
    density: computeDensity(v1Hits, v2Hits, auxHits, length),
    velocityRange: computeVelocityRange(steps),
    voiceSeparation: computeVoiceSeparation(v1Hits, v2Hits, auxHits, length),
    regularity: computeRegularity(v1Hits, length),
  };

  const zone = getShapeZone(shape);

  // Get target ranges as arrays for scoring
  const targets = {
    syncopation: parseTargetRange(PENTAGON_METRICS.syncopation.targetByZone[zone]),
    density: parseTargetRange(PENTAGON_METRICS.density.targetByZone[zone]),
    velocityRange: parseTargetRange(PENTAGON_METRICS.velocityRange.targetByZone[zone]),
    voiceSeparation: parseTargetRange(PENTAGON_METRICS.voiceSeparation.targetByZone[zone]),
    regularity: parseTargetRange(PENTAGON_METRICS.regularity.targetByZone[zone]),
  };

  // Adjust density target based on energy
  const densityRange = targets.density[1] - targets.density[0];
  const densityTarget = targets.density[0] + energy * densityRange;
  targets.density = [densityTarget - densityRange / 2.2, densityTarget + densityRange / 2.2];

  // Adjust velocity range based on accent
  const velZone = accent < 0.3 ? 'stable' : accent < 0.7 ? 'syncopated' : 'wild';
  targets.velocityRange = parseTargetRange(PENTAGON_METRICS.velocityRange.targetByZone[velZone]);

  const scores = {
    syncopation: scoreMetric(raw.syncopation, targets.syncopation),
    density: scoreMetric(raw.density, targets.density),
    velocityRange: scoreMetric(raw.velocityRange, targets.velocityRange),
    voiceSeparation: scoreMetric(raw.voiceSeparation, targets.voiceSeparation),
    regularity: scoreMetric(raw.regularity, targets.regularity),
  };

  const composite = (
    0.25 * scores.syncopation +
    0.15 * scores.density +
    0.20 * scores.velocityRange +
    0.20 * scores.voiceSeparation +
    0.20 * scores.regularity
  );

  return { raw, scores, composite, zone, targets };
}

// ============================================================================
// Main
// ============================================================================

console.log('Expressiveness Evaluation for DuoPulse v5');
console.log('='.repeat(50));

const sweepsPath = join(DATA_DIR, 'sweeps.json');
const presetsPath = join(DATA_DIR, 'presets.json');

if (!existsSync(sweepsPath)) {
  console.error(`Error: sweeps.json not found at ${sweepsPath}`);
  console.error('Run "npm run generate" first.');
  process.exit(1);
}

console.log('Loading pattern data...');
const sweeps = JSON.parse(readFileSync(sweepsPath, 'utf-8'));
const presets = JSON.parse(readFileSync(presetsPath, 'utf-8'));

// Compute metrics for all sweeps
console.log('Computing Pentagon metrics for sweeps...');

const sweepMetrics = {};
for (const [paramName, patterns] of Object.entries(sweeps)) {
  sweepMetrics[paramName] = patterns.map(pattern => ({
    params: pattern.params,
    hits: pattern.hits,
    pentagon: computePentagonMetrics(pattern),
  }));
}

writeFileSync(join(DATA_DIR, 'sweep-metrics.json'), JSON.stringify(sweepMetrics, null, 2));
console.log('  Written sweep-metrics.json');

// Compute metrics for presets
console.log('Computing Pentagon metrics for presets...');

const presetMetrics = presets.map(preset => ({
  name: preset.name,
  description: preset.description,
  params: preset.pattern.params,
  hits: preset.pattern.hits,
  pentagon: computePentagonMetrics(preset.pattern),
}));

writeFileSync(join(DATA_DIR, 'preset-metrics.json'), JSON.stringify(presetMetrics, null, 2));
console.log('  Written preset-metrics.json');

// Compute aggregate statistics (matching Python format)
console.log('Computing aggregate statistics...');

const allMetrics = Object.values(sweepMetrics).flat().map(m => m.pentagon);

// Group by zone and compute RAW metric averages (like Python does)
const byZone = { stable: [], syncopated: [], wild: [] };
for (const m of allMetrics) {
  byZone[m.zone].push(m);
}

function avgMetrics(metricsList) {
  if (!metricsList.length) return null;
  const n = metricsList.length;
  return {
    count: n,
    syncopation: metricsList.reduce((a, m) => a + m.raw.syncopation, 0) / n,
    density: metricsList.reduce((a, m) => a + m.raw.density, 0) / n,
    velocityRange: metricsList.reduce((a, m) => a + m.raw.velocityRange, 0) / n,
    voiceSeparation: metricsList.reduce((a, m) => a + m.raw.voiceSeparation, 0) / n,
    regularity: metricsList.reduce((a, m) => a + m.raw.regularity, 0) / n,
    composite: metricsList.reduce((a, m) => a + m.composite, 0) / n,
  };
}

const pentagonStats = {
  total: avgMetrics(allMetrics),
  stable: avgMetrics(byZone.stable),
  syncopated: avgMetrics(byZone.syncopated),
  wild: avgMetrics(byZone.wild),
};

// Compute alignment scores for each zone/metric combo
const allAlignmentScores = [];
const metricKeys = ['syncopation', 'density', 'velocityRange', 'voiceSeparation', 'regularity'];

for (const zone of ['stable', 'syncopated', 'wild']) {
  const zoneData = pentagonStats[zone];
  if (!zoneData) continue;

  for (const key of metricKeys) {
    const rawValue = zoneData[key];
    const targetStr = PENTAGON_METRICS[key].targetByZone[zone];
    const alignment = computeAlignmentScore(rawValue, targetStr);
    allAlignmentScores.push(alignment);
  }
}

const overallAlignment = allAlignmentScores.length > 0
  ? allAlignmentScores.reduce((a, b) => a + b, 0) / allAlignmentScores.length
  : 0;

// Load seed variation if exists
let seedSummary = null;
const seedPath = join(DATA_DIR, 'seed-variation.json');
if (existsSync(seedPath)) {
  const seedData = JSON.parse(readFileSync(seedPath, 'utf-8'));
  seedSummary = seedData.summary;
}

const expressiveness = {
  timestamp: new Date().toISOString(),
  pentagonStats,
  overallAlignment,
  alignmentStatus: overallAlignment >= 0.7 ? 'GOOD' : overallAlignment >= 0.5 ? 'FAIR' : 'POOR',
  totalPatterns: allMetrics.length,
  seedVariation: seedSummary,
  metricDefinitions: PENTAGON_METRICS,
  pass: overallAlignment >= 0.5 && (!seedSummary || seedSummary.pass),
};

writeFileSync(join(DATA_DIR, 'expressiveness.json'), JSON.stringify(expressiveness, null, 2));
console.log('  Written expressiveness.json');

// Print summary
console.log();
console.log('='.repeat(50));
console.log('PENTAGON OF MUSICALITY');
console.log('='.repeat(50));

console.log('\nRAW METRIC AVERAGES BY ZONE:');
console.log('           STABLE    SYNCOPATED    WILD');
for (const key of metricKeys) {
  const s = pentagonStats.stable?.[key] ?? '-';
  const sy = pentagonStats.syncopated?.[key] ?? '-';
  const w = pentagonStats.wild?.[key] ?? '-';
  const label = PENTAGON_METRICS[key].short.padEnd(10);
  console.log(`  ${label} ${typeof s === 'number' ? s.toFixed(2).padStart(6) : '  -   '}    ${typeof sy === 'number' ? sy.toFixed(2).padStart(6) : '  -   '}    ${typeof w === 'number' ? w.toFixed(2).padStart(6) : '  -   '}`);
}

console.log('\nZONE COMPLIANCE:');
for (const zone of ['stable', 'syncopated', 'wild']) {
  const z = pentagonStats[zone];
  if (z) {
    console.log(`  ${zone.toUpperCase().padEnd(12)} ${(z.composite * 100).toFixed(0).padStart(3)}% (n=${z.count})`);
  }
}

console.log(`\nOVERALL ALIGNMENT SCORE: ${(overallAlignment * 100).toFixed(1)}% [${expressiveness.alignmentStatus}]`);
console.log(`Total patterns analyzed: ${allMetrics.length}`);

if (seedSummary) {
  console.log('\nSEED VARIATION:');
  console.log(`  V1: ${(seedSummary.v1.avgScore * 100).toFixed(1)}%`);
  console.log(`  V2: ${(seedSummary.v2.avgScore * 100).toFixed(1)}%`);
  console.log(`  AUX: ${(seedSummary.aux.avgScore * 100).toFixed(1)}%`);
}

console.log();
console.log(`Result: [${expressiveness.pass ? 'PASS' : 'FAIL'}]`);
console.log();
