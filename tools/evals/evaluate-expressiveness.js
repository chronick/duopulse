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
import { computePresetConformance } from './preset-references.js';

const __dirname = dirname(fileURLToPath(import.meta.url));
const DATA_DIR = join(__dirname, 'public/data');

// LHL Metric weights for 64-step pattern (4 bars of 4/4 at 16th notes)
// Pattern repeats every 2 bars (32 steps)
const METRIC_WEIGHTS_64 = [
  // Bar 1
  1.00, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
  // Bar 2
  0.90, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
  // Bar 3 (repeat of bar 1 pattern, slightly less emphasis)
  0.95, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
  // Bar 4 (repeat of bar 2 pattern)
  0.90, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
  // Bar 5 (repeat of bar 1 pattern)
  1.00, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
  // Bar 6 (repeat of bar 2 pattern)
  0.90, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
  // Bar 7 (repeat of bar 3 pattern)
  0.95, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
  // Bar 8 (repeat of bar 4 pattern)
  0.90, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,
];

// Pentagon metric definitions (matching Python PENTAGON_METRICS)
// Target ranges aligned with spec design intent (06-shape.md, 08-complement.md)
const PENTAGON_METRICS = {
  syncopation: {
    short: 'Sync',
    name: 'Syncopation',
    // Spec: Syncopated zone (30-70%) designed for "funk, displaced, tension"
    // Target revised from 0.70-1.00 to 0.55-0.85 (iteration 2026-01-23-003: rotation ceiling found)
    description: 'Syncopation creates groove and forward motion. Syncopated zone is designed for maximum displacement.',
    targetByZone: { stable: '0.00-0.20', syncopated: '0.55-0.85', wild: '0.60-1.00' },
  },
  density: {
    short: 'Dens',
    name: 'Density',
    // Note: Density is ENERGY-dependent, not SHAPE-dependent (iteration 2026-01-21-001)
    // It's excluded from SHAPE zone scoring and evaluated in ENERGY zones instead.
    description: 'Density is controlled by ENERGY parameter. Excluded from SHAPE zone scoring - see ENERGY zone metrics.',
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
    // Spec: COMPLEMENT relationship fills gaps, preventing overlap by design - HIGH separation expected
    description: 'Separation measures voice independence. COMPLEMENT gap-filling creates high separation by design.',
    targetByZone: { stable: '0.75-0.95', syncopated: '0.70-0.95', wild: '0.65-0.95' },
  },
  regularity: {
    short: 'Reg',
    name: 'Regularity',
    // Iteration 2026-01-20-008: Wild zone target revised from 0.12-0.48 to 0.55-0.85
    // The fixed hit budget (K) in Gumbel selection creates gap uniformity regardless of weight variance.
    // Wild zone "chaos" manifests in hit position randomness, not gap irregularity.
    description: 'Regularity = danceability. Stable patterns need high regularity; wild patterns break it.',
    targetByZone: { stable: '0.72-1.00', syncopated: '0.42-0.68', wild: '0.55-0.85' },
  },
};

// Fill metric definitions for evaluating fill pattern quality
const FILL_METRICS = {
  fillDensityRamp: {
    short: 'DensRamp',
    name: 'Fill Density Ramp',
    description: 'Fills should build from sparse to dense',
    targetByZone: {
      stable: '0.60-0.85',
      syncopated: '0.70-0.95',
      wild: '0.80-1.00',
    },
  },
  fillVelocityBuild: {
    short: 'VelBuild',
    name: 'Fill Velocity Build',
    description: 'Velocity should crescendo through fill',
    targetByZone: {
      stable: '0.50-0.75',
      syncopated: '0.60-0.85',
      wild: '0.70-0.95',
    },
  },
  fillAccentPlacement: {
    short: 'AccPlace',
    name: 'Fill Accent Placement',
    description: 'Accents should land on strong beats. Fills build toward downbeats.',
    targetByZone: {
      stable: '0.80-1.00',
      syncopated: '0.70-1.00',
      wild: '0.55-0.95',
    },
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

function computeSyncopation(v1Hits, length = 64) {
  const weights = METRIC_WEIGHTS_64.slice(0, length);
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

  // Note: Density excluded from SHAPE zone composite scoring (iteration 2026-01-21-001)
  // Density is controlled by ENERGY parameter, not SHAPE. Including it in SHAPE zone
  // scoring was a design misalignment - SHAPE sweeps at ENERGY=0.5 always have ~0.50 density
  // regardless of SHAPE zone. Density is evaluated separately in ENERGY zone metrics.
  const composite = (
    0.30 * scores.syncopation +    // was 0.25, now carries density's weight
    0.25 * scores.velocityRange +  // was 0.20
    0.25 * scores.voiceSeparation + // was 0.20
    0.20 * scores.regularity        // unchanged
  );

  return { raw, scores, composite, zone, targets };
}

// ============================================================================
// Multi-Seed Evaluation Functions
// ============================================================================

/**
 * Compute average of an array
 */
function average(arr) {
  if (!arr || arr.length === 0) return 0;
  return arr.reduce((a, b) => a + b, 0) / arr.length;
}

/**
 * Compute standard deviation of an array
 */
function stddev(arr) {
  if (!arr || arr.length < 2) return 0;
  const avg = average(arr);
  const squaredDiffs = arr.map(x => (x - avg) ** 2);
  return Math.sqrt(average(squaredDiffs));
}

/**
 * Compute pentagon metrics for raw step data (used for seed patterns)
 * Similar to computePentagonMetrics but takes raw data structure
 */
function computePentagonMetricsFromSteps(steps, params) {
  const length = params.length || 64;
  const shape = params.shape;
  const energy = params.energy;
  const accent = params.accent || 0.5;

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

  // Get target ranges
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
    0.30 * scores.syncopation +
    0.25 * scores.velocityRange +
    0.25 * scores.voiceSeparation +
    0.20 * scores.regularity
  );

  return { raw, scores, composite, zone, targets };
}

/**
 * Compute pentagon metrics averaged across multiple seeds
 * Also computes stability (inverse of variance) for each metric
 */
function computeMultiSeedPentagonMetrics(pattern) {
  const { seedPatterns, params } = pattern;

  // If no seedPatterns, fall back to single-seed computation
  if (!seedPatterns || seedPatterns.length === 0) {
    return {
      ...computePentagonMetrics(pattern),
      stability: null,
      perSeed: null,
    };
  }

  // Compute metrics for each seed's pattern
  const seedMetrics = seedPatterns.map(sp =>
    computePentagonMetricsFromSteps(sp.steps, params)
  );

  // Average raw metrics across seeds
  const avgRaw = {
    syncopation: average(seedMetrics.map(m => m.raw.syncopation)),
    density: average(seedMetrics.map(m => m.raw.density)),
    velocityRange: average(seedMetrics.map(m => m.raw.velocityRange)),
    voiceSeparation: average(seedMetrics.map(m => m.raw.voiceSeparation)),
    regularity: average(seedMetrics.map(m => m.raw.regularity)),
  };

  // Average scores across seeds
  const avgScores = {
    syncopation: average(seedMetrics.map(m => m.scores.syncopation)),
    density: average(seedMetrics.map(m => m.scores.density)),
    velocityRange: average(seedMetrics.map(m => m.scores.velocityRange)),
    voiceSeparation: average(seedMetrics.map(m => m.scores.voiceSeparation)),
    regularity: average(seedMetrics.map(m => m.scores.regularity)),
  };

  const avgComposite = average(seedMetrics.map(m => m.composite));

  // Compute stability (1 - normalized stddev) for each metric
  // High stability (close to 1) means the metric is consistent across seeds
  const stability = {
    syncopation: Math.max(0, 1 - stddev(seedMetrics.map(m => m.raw.syncopation)) * 2),
    density: Math.max(0, 1 - stddev(seedMetrics.map(m => m.raw.density)) * 2),
    velocityRange: Math.max(0, 1 - stddev(seedMetrics.map(m => m.raw.velocityRange)) * 2),
    voiceSeparation: Math.max(0, 1 - stddev(seedMetrics.map(m => m.raw.voiceSeparation)) * 2),
    regularity: Math.max(0, 1 - stddev(seedMetrics.map(m => m.raw.regularity)) * 2),
  };
  stability.overall = average(Object.values(stability));

  // Per-seed breakdown
  const perSeed = seedPatterns.map((sp, i) => ({
    seed: sp.seed,
    seedHex: sp.seedHex,
    composite: seedMetrics[i].composite,
    raw: seedMetrics[i].raw,
  }));

  const zone = seedMetrics[0]?.zone || getShapeZone(params.shape);

  return {
    raw: avgRaw,
    scores: avgScores,
    composite: avgComposite,
    zone,
    targets: seedMetrics[0]?.targets,
    stability,
    perSeed,
    numSeeds: seedPatterns.length,
  };
}

// ============================================================================
// Fill Metric Computation Functions
// ============================================================================

/**
 * Get fill pattern at specific progress value
 */
function getFillAtProgress(fillPatterns, targetProgress) {
  return fillPatterns.find(p => Math.abs(p.params.fillProgress - targetProgress) < 0.01);
}

/**
 * Compute fill density ramp metric
 * Compare hit density between fillProgress 0.25 and 1.0
 * Returns 0-1 value where higher = better ramp
 */
function computeFillDensityRamp(fillPatterns, patternLength = 32) {
  const early = getFillAtProgress(fillPatterns, 0.25);
  const late = getFillAtProgress(fillPatterns, 1.0);

  if (!early || !late) return 0.5;

  const earlyDensity = early.hitCounts.total / patternLength;
  const lateDensity = late.hitCounts.total / patternLength;

  // Compute ramp as increase in density
  // Normalize: 0 density change = 0.5, max expected increase (~0.5) = 1.0
  const ramp = lateDensity - earlyDensity;

  // Normalize to 0-1 range. Expect ramps from 0 to ~0.5 for good fills
  return Math.max(0, Math.min(1.0, 0.5 + ramp));
}

/**
 * Compute fill velocity build metric
 * Compare average velocity between fillProgress 0.25 and 1.0
 * Returns 0-1 value where higher = better velocity crescendo
 */
function computeFillVelocityBuild(fillPatterns) {
  const early = getFillAtProgress(fillPatterns, 0.25);
  const late = getFillAtProgress(fillPatterns, 1.0);

  if (!early || !late) return 0.5;

  // Compute average velocity for each fill stage
  const avgVel = (fillSteps) => {
    if (!fillSteps || fillSteps.length === 0) return 0;
    const vels = fillSteps.map(s =>
      s.anchorVel || s.shimmerVel || s.auxVel || 0
    );
    return vels.reduce((a, b) => a + b, 0) / vels.length;
  };

  const earlyVel = avgVel(early.fillSteps);
  const lateVel = avgVel(late.fillSteps);

  // Velocity build: difference in average velocity
  // Normalize: expect ~0.1-0.3 increase for good fills
  const build = lateVel - earlyVel;

  // Map to 0-1 range: 0 build = 0.5, 0.3 build = 1.0
  return Math.max(0, Math.min(1.0, 0.5 + build * 1.67));
}

/**
 * Compute fill accent placement metric
 * Check if high-velocity hits (>0.8) land on downbeats in late fill
 * Downbeats are steps 0, 4, 8, 12, 16, 20, 24, 28 (every 4th step)
 * Returns 0-1 value where higher = better accent placement
 */
function computeFillAccentPlacement(fillPatterns) {
  // Include downbeats for both 32-step and 64-step patterns
  const DOWNBEATS = new Set([0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60]);
  const ACCENT_THRESHOLD = 0.8;

  // Check late fill (progress 0.75 and 1.0)
  const late75 = getFillAtProgress(fillPatterns, 0.75);
  const late100 = getFillAtProgress(fillPatterns, 1.0);

  const latePatterns = [late75, late100].filter(Boolean);
  if (latePatterns.length === 0) return 0.5;

  let accentedDownbeats = 0;
  let totalAccents = 0;

  for (const pattern of latePatterns) {
    for (const step of (pattern.fillSteps || [])) {
      const vel = step.anchorVel || step.shimmerVel || step.auxVel || 0;
      if (vel >= ACCENT_THRESHOLD) {
        totalAccents++;
        if (DOWNBEATS.has(step.step)) {
          accentedDownbeats++;
        }
      }
    }
  }

  if (totalAccents === 0) return 0.5;

  // Return ratio of accents on downbeats
  return accentedDownbeats / totalAccents;
}

/**
 * Evaluate a fill pattern set and return fill metrics
 */
function evaluateFillPattern(fillPatterns, shape = 0.3) {
  const zone = getShapeZone(shape);

  const raw = {
    fillDensityRamp: computeFillDensityRamp(fillPatterns),
    fillVelocityBuild: computeFillVelocityBuild(fillPatterns),
    fillAccentPlacement: computeFillAccentPlacement(fillPatterns),
  };

  // Compute alignment scores for each metric
  const scores = {};
  const fillMetricKeys = Object.keys(FILL_METRICS);

  for (const key of fillMetricKeys) {
    const targetStr = FILL_METRICS[key].targetByZone[zone];
    scores[key] = computeAlignmentScore(raw[key], targetStr);
  }

  // Composite score (equal weighting)
  const composite = (scores.fillDensityRamp + scores.fillVelocityBuild + scores.fillAccentPlacement) / 3;

  return { raw, scores, composite, zone };
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

// Compute metrics for all sweeps (multi-seed averaging)
console.log('Computing Pentagon metrics for sweeps (multi-seed)...');

const sweepMetrics = {};
for (const [paramName, patterns] of Object.entries(sweeps)) {
  sweepMetrics[paramName] = patterns.map(pattern => ({
    params: pattern.params,
    hits: pattern.hits,
    pentagon: computeMultiSeedPentagonMetrics(pattern),
  }));
}

writeFileSync(join(DATA_DIR, 'sweep-metrics.json'), JSON.stringify(sweepMetrics, null, 2));
console.log('  Written sweep-metrics.json');

// Compute metrics for ENERGY zone sweeps (multi-seed averaging)
console.log('Computing Pentagon metrics for ENERGY zones (multi-seed)...');

const energyZoneSweepsPath = join(DATA_DIR, 'energy-zone-sweeps.json');
let energyZoneMetrics = null;

if (existsSync(energyZoneSweepsPath)) {
  const energyZoneSweeps = JSON.parse(readFileSync(energyZoneSweepsPath, 'utf-8'));

  energyZoneMetrics = {};
  for (const [zoneName, patterns] of Object.entries(energyZoneSweeps)) {
    energyZoneMetrics[zoneName] = patterns.map(pattern => ({
      params: pattern.params,
      hits: pattern.hits,
      pentagon: computeMultiSeedPentagonMetrics(pattern),
    }));
  }

  writeFileSync(join(DATA_DIR, 'energy-zone-metrics.json'), JSON.stringify(energyZoneMetrics, null, 2));
  console.log('  Written energy-zone-metrics.json');
}

// Compute metrics for presets (multi-seed averaging)
console.log('Computing Pentagon metrics for presets (multi-seed)...');

const presetMetrics = presets.map(preset => {
  const pentagon = computeMultiSeedPentagonMetrics(preset.pattern);
  const conformance = computePresetConformance(preset.name, preset.pattern, pentagon);
  return {
    name: preset.name,
    description: preset.description,
    params: preset.pattern.params,
    hits: preset.pattern.hits,
    pentagon,
    conformance,
  };
});

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

// Compute aggregate stability metrics across all patterns
console.log('Computing metric stability across seeds...');

function avgStability(metricsList, metricKey) {
  const stabilityValues = metricsList
    .filter(m => m.stability && m.stability[metricKey] !== undefined)
    .map(m => m.stability[metricKey]);
  if (stabilityValues.length === 0) return null;
  return stabilityValues.reduce((a, b) => a + b, 0) / stabilityValues.length;
}

const metricStability = {
  syncopation: avgStability(allMetrics, 'syncopation'),
  density: avgStability(allMetrics, 'density'),
  velocityRange: avgStability(allMetrics, 'velocityRange'),
  voiceSeparation: avgStability(allMetrics, 'voiceSeparation'),
  regularity: avgStability(allMetrics, 'regularity'),
};
metricStability.overall = avgStability(allMetrics, 'overall');

// Compute per-seed pentagon scores (average composite per seed across all patterns)
const numSeeds = allMetrics[0]?.numSeeds || 1;
const perSeedPentagonScore = [];
if (numSeeds > 1) {
  for (let i = 0; i < numSeeds; i++) {
    const seedComposites = allMetrics
      .filter(m => m.perSeed && m.perSeed[i])
      .map(m => m.perSeed[i].composite);
    if (seedComposites.length > 0) {
      const avgComposite = seedComposites.reduce((a, b) => a + b, 0) / seedComposites.length;
      const seedInfo = allMetrics.find(m => m.perSeed && m.perSeed[i])?.perSeed[i];
      perSeedPentagonScore.push({
        seed: seedInfo?.seedHex || `Seed ${i}`,
        score: avgComposite,
      });
    }
  }
}

// Compute alignment scores for each zone/metric combo
// Note: Density excluded from SHAPE zone alignment (iteration 2026-01-21-001)
// Density is ENERGY-dependent and is evaluated separately in ENERGY zone metrics.
const allAlignmentScores = [];
const metricKeys = ['syncopation', 'velocityRange', 'voiceSeparation', 'regularity'];

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

const pentagonScore = allAlignmentScores.length > 0
  ? allAlignmentScores.reduce((a, b) => a + b, 0) / allAlignmentScores.length
  : 0;

// Compute overall conformance score from presets
const conformanceScores = presetMetrics.map(p => p.conformance.score);
const conformanceScore = conformanceScores.length > 0
  ? conformanceScores.reduce((a, b) => a + b, 0) / conformanceScores.length
  : 0;
const conformancePassCount = presetMetrics.filter(p => p.conformance.pass).length;

// Combined alignment: pentagon (60%) + conformance (40%)
const overallAlignment = (pentagonScore * 0.6) + (conformanceScore * 0.4);

// Load seed variation if exists
let seedSummary = null;
const seedPath = join(DATA_DIR, 'seed-variation.json');
if (existsSync(seedPath)) {
  const seedData = JSON.parse(readFileSync(seedPath, 'utf-8'));
  seedSummary = seedData.summary;
}

// Load and evaluate fill patterns if exists
let fillMetrics = null;
const fillsPath = join(DATA_DIR, 'fills.json');
if (existsSync(fillsPath)) {
  console.log('Computing fill metrics...');
  const fills = JSON.parse(readFileSync(fillsPath, 'utf-8'));

  // Evaluate each energy sweep entry
  const fillEvaluations = fills.energySweep.map(entry => {
    const shape = entry.fillPatterns[0]?.params?.shape ?? 0.3;
    const metrics = evaluateFillPattern(entry.fillPatterns, shape);
    return {
      energy: entry.energy,
      shape,
      metrics,
    };
  });

  // Compute aggregate fill statistics
  const avgFillMetric = (key) => {
    const values = fillEvaluations.map(e => e.metrics.raw[key]);
    return values.reduce((a, b) => a + b, 0) / values.length;
  };

  const avgFillScore = (key) => {
    const values = fillEvaluations.map(e => e.metrics.scores[key]);
    return values.reduce((a, b) => a + b, 0) / values.length;
  };

  const fillComposite = fillEvaluations.reduce((a, e) => a + e.metrics.composite, 0) / fillEvaluations.length;

  fillMetrics = {
    evaluations: fillEvaluations,
    summary: {
      fillDensityRamp: { raw: avgFillMetric('fillDensityRamp'), score: avgFillScore('fillDensityRamp') },
      fillVelocityBuild: { raw: avgFillMetric('fillVelocityBuild'), score: avgFillScore('fillVelocityBuild') },
      fillAccentPlacement: { raw: avgFillMetric('fillAccentPlacement'), score: avgFillScore('fillAccentPlacement') },
      composite: fillComposite,
      pass: fillComposite >= 0.5,
    },
  };

  writeFileSync(join(DATA_DIR, 'fill-metrics.json'), JSON.stringify(fillMetrics, null, 2));
  console.log('  Written fill-metrics.json');
}

// Compute ENERGY zone aggregate statistics
let energyZoneStats = null;
if (energyZoneMetrics) {
  console.log('Computing ENERGY zone aggregate statistics...');

  energyZoneStats = {};
  for (const [zoneName, metrics] of Object.entries(energyZoneMetrics)) {
    const zoneMetrics = metrics.map(m => m.pentagon);
    energyZoneStats[zoneName] = avgMetrics(zoneMetrics);
  }

  writeFileSync(join(DATA_DIR, 'energy-zone-stats.json'), JSON.stringify(energyZoneStats, null, 2));
  console.log('  Written energy-zone-stats.json');
}

const expressiveness = {
  timestamp: new Date().toISOString(),
  pentagonStats,
  pentagonScore,
  conformance: {
    score: conformanceScore,
    passCount: conformancePassCount,
    totalPresets: presetMetrics.length,
    presetBreakdown: presetMetrics.map(p => ({
      name: p.name,
      score: p.conformance.score,
      status: p.conformance.status,
      pass: p.conformance.pass,
      tolerance: p.conformance.tolerance,
    })),
  },
  overallAlignment,
  alignmentStatus: overallAlignment >= 0.7 ? 'GOOD' : overallAlignment >= 0.5 ? 'FAIR' : 'POOR',
  totalPatterns: allMetrics.length,
  multiSeed: {
    enabled: numSeeds > 1,
    numSeeds,
    metricStability,
    perSeedPentagonScore,
  },
  seedVariation: seedSummary,
  fillMetrics: fillMetrics?.summary ?? null,
  energyZoneStats,
  metricDefinitions: PENTAGON_METRICS,
  fillMetricDefinitions: FILL_METRICS,
  pass: overallAlignment >= 0.5 && (!seedSummary || seedSummary.pass) && (!fillMetrics || fillMetrics.summary.pass),
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

console.log(`\nPENTAGON SCORE: ${(pentagonScore * 100).toFixed(1)}%`);
console.log(`CONFORMANCE SCORE: ${(conformanceScore * 100).toFixed(1)}% (${conformancePassCount}/${presetMetrics.length} presets pass)`);
console.log(`OVERALL ALIGNMENT: ${(overallAlignment * 100).toFixed(1)}% [${expressiveness.alignmentStatus}] (pentagon 60% + conformance 40%)`);
console.log(`Total patterns analyzed: ${allMetrics.length}`);

console.log('\nPRESET CONFORMANCE:');
for (const preset of presetMetrics) {
  const c = preset.conformance;
  const passStr = c.pass ? 'PASS' : 'FAIL';
  console.log(`  ${preset.name.padEnd(18)} ${(c.score * 100).toFixed(0).padStart(3)}% [${c.status.padEnd(9)}] ${passStr}`);
}

if (seedSummary) {
  console.log('\nSEED VARIATION:');
  console.log(`  V1: ${(seedSummary.v1.avgScore * 100).toFixed(1)}%`);
  console.log(`  V2: ${(seedSummary.v2.avgScore * 100).toFixed(1)}%`);
  console.log(`  AUX: ${(seedSummary.aux.avgScore * 100).toFixed(1)}%`);
}

// Multi-seed stability metrics
if (numSeeds > 1 && metricStability.overall !== null) {
  console.log(`\nMETRIC STABILITY (across ${numSeeds} seeds):`);
  const fmtPct = (v) => v !== null ? `${(v * 100).toFixed(0)}%` : 'N/A';
  console.log(`  Syncopation:      ${fmtPct(metricStability.syncopation).padStart(4)} stable`);
  console.log(`  Density:          ${fmtPct(metricStability.density).padStart(4)} stable`);
  console.log(`  Velocity Range:   ${fmtPct(metricStability.velocityRange).padStart(4)} stable`);
  console.log(`  Voice Separation: ${fmtPct(metricStability.voiceSeparation).padStart(4)} stable`);
  console.log(`  Regularity:       ${fmtPct(metricStability.regularity).padStart(4)} stable`);
  console.log(`  Overall:          ${fmtPct(metricStability.overall).padStart(4)} stable`);

  if (perSeedPentagonScore.length > 0) {
    console.log('\nPER-SEED PENTAGON SCORES:');
    for (const { seed, score } of perSeedPentagonScore) {
      console.log(`  ${seed.padEnd(12)} ${(score * 100).toFixed(1)}%`);
    }
  }
}

if (fillMetrics) {
  console.log('\nFILL METRICS:');
  const fm = fillMetrics.summary;
  console.log(`  Density Ramp:     ${fm.fillDensityRamp.raw.toFixed(2)} (score: ${(fm.fillDensityRamp.score * 100).toFixed(0)}%)`);
  console.log(`  Velocity Build:   ${fm.fillVelocityBuild.raw.toFixed(2)} (score: ${(fm.fillVelocityBuild.score * 100).toFixed(0)}%)`);
  console.log(`  Accent Placement: ${fm.fillAccentPlacement.raw.toFixed(2)} (score: ${(fm.fillAccentPlacement.score * 100).toFixed(0)}%)`);
  console.log(`  Composite:        ${(fm.composite * 100).toFixed(1)}% [${fm.pass ? 'PASS' : 'FAIL'}]`);
}

if (energyZoneStats) {
  console.log('\nPENTAGON BY ENERGY ZONE:');
  console.log('           MINIMAL   GROOVE    BUILD     PEAK');
  for (const key of metricKeys) {
    const m = energyZoneStats.minimal?.[key] ?? '-';
    const g = energyZoneStats.groove?.[key] ?? '-';
    const b = energyZoneStats.build?.[key] ?? '-';
    const p = energyZoneStats.peak?.[key] ?? '-';
    const label = PENTAGON_METRICS[key].short.padEnd(10);
    console.log(`  ${label} ${typeof m === 'number' ? m.toFixed(2).padStart(6) : '  -   '}    ${typeof g === 'number' ? g.toFixed(2).padStart(6) : '  -   '}    ${typeof b === 'number' ? b.toFixed(2).padStart(6) : '  -   '}    ${typeof p === 'number' ? p.toFixed(2).padStart(6) : '  -   '}`);
  }
}

console.log();
console.log(`Result: [${expressiveness.pass ? 'PASS' : 'FAIL'}]`);
console.log();
