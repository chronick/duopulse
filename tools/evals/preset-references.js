/**
 * Preset Reference Patterns and Metric Expectations
 *
 * Defines canonical reference patterns and expected metric ranges for each preset.
 * Used by conformance scoring to evaluate how well generated patterns match
 * their intended musical character.
 *
 * Tolerance levels:
 * - strict: Stable zone presets - patterns must closely match expectations
 * - moderate: Syncopated zone presets - some variation allowed
 * - lax: Wild zone presets - wide variation acceptable
 */

/**
 * Reference patterns and metric expectations for all 8 presets.
 *
 * Pattern positions are 0-indexed 16th notes in a 64-step (4-bar) pattern:
 * - Steps 0, 16, 32, 48 = beat 1 of each bar (downbeats)
 * - Steps 8, 24, 40, 56 = beat 3 of each bar (backbeats)
 * - Steps 4, 12, 20, 28, 36, 44, 52, 60 = offbeats (8th notes)
 */
export const PRESET_REFERENCES = {
  'Minimal Techno': {
    // Sparse kicks on downbeats - less is more
    anchorPattern: [0, 16, 32, 48],
    shimmerPattern: null, // Any reasonable hi-hat pattern
    tolerance: 'strict',
    requiredHits: {
      anchor: [0, 32], // At least beat 1 of bars 1 and 3
    },
    metrics: {
      regularity: { min: 0.75, weight: 0.3 },
      density: { max: 0.30, weight: 0.4 },
      syncopation: { max: 0.15, weight: 0.3 },
    },
  },

  'Four on Floor': {
    // Classic techno kick pattern - kick on every quarter note
    anchorPattern: [0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60],
    shimmerPattern: null, // Any reasonable pattern
    tolerance: 'strict',
    requiredHits: {
      anchor: [0, 16, 32, 48], // Beat 1 of each bar minimum
    },
    metrics: {
      regularity: { min: 0.85, weight: 0.4 },
      syncopation: { max: 0.15, weight: 0.3 },
      density: { target: 0.25, tolerance: 0.15, weight: 0.3 },
    },
  },

  'Driving Techno': {
    // Energetic techno with strong pulse - regular but more hits than minimal
    anchorPattern: [0, 8, 16, 24, 32, 40, 48, 56], // Quarter notes on 8ths grid
    shimmerPattern: null,
    tolerance: 'strict',
    requiredHits: {
      anchor: [0, 16, 32, 48], // Downbeats required
    },
    metrics: {
      regularity: { min: 0.70, weight: 0.35 },
      density: { range: [0.20, 0.40], weight: 0.35 },
      syncopation: { max: 0.22, weight: 0.3 },
    },
  },

  'Syncopated Groove': {
    // Funky, off-beat feel with deliberate displacement
    anchorPattern: null, // Pattern varies, focus on metrics
    shimmerPattern: null,
    tolerance: 'moderate',
    requiredHits: null, // No strict position requirements
    metrics: {
      syncopation: { min: 0.25, weight: 0.5 },
      regularity: { range: [0.40, 0.70], weight: 0.3 },
      voiceSeparation: { min: 0.50, weight: 0.2 },
    },
  },

  'Sparse Dub': {
    // Minimal, spacious patterns with dub-style offbeat emphasis
    anchorPattern: null, // Sparse but syncopated
    shimmerPattern: null,
    tolerance: 'moderate',
    requiredHits: null,
    metrics: {
      density: { max: 0.35, weight: 0.35 },
      syncopation: { min: 0.22, weight: 0.35 },
      voiceSeparation: { min: 0.55, weight: 0.3 },
    },
  },

  'Dense Gabber': {
    // Aggressive, high-density patterns with driving energy
    anchorPattern: null, // Dense but still groovy
    shimmerPattern: null,
    tolerance: 'moderate',
    requiredHits: {
      anchor: [0, 32], // At least some downbeat anchoring
    },
    metrics: {
      density: { min: 0.40, weight: 0.4 },
      regularity: { range: [0.45, 0.75], weight: 0.3 },
      syncopation: { range: [0.15, 0.40], weight: 0.3 },
    },
  },

  'Broken Beat': {
    // Displaced, shuffled rhythms with irregular feel
    anchorPattern: null, // Intentionally irregular
    shimmerPattern: null,
    tolerance: 'moderate',
    requiredHits: null, // Broken = no strict requirements
    metrics: {
      syncopation: { min: 0.35, weight: 0.4 },
      regularity: { max: 0.55, weight: 0.3 },
      density: { range: [0.35, 0.65], weight: 0.3 },
    },
  },

  'Chaotic IDM': {
    // Glitchy, unpredictable patterns with high complexity
    anchorPattern: null, // Intentionally unpredictable
    shimmerPattern: null,
    tolerance: 'lax',
    requiredHits: null, // Chaos has no rules
    metrics: {
      syncopation: { min: 0.40, weight: 0.3 },
      regularity: { max: 0.45, weight: 0.3 },
      velocityRange: { min: 0.30, weight: 0.2 },
      density: { min: 0.45, weight: 0.2 },
    },
  },
};

/**
 * Pass thresholds by tolerance level.
 * These determine when a conformance score is considered "passing".
 */
export const PASS_THRESHOLDS = {
  strict: 0.75, // Stable patterns must closely match expectations
  moderate: 0.60, // Syncopated allows more variation
  lax: 0.50, // Wild is inherently chaotic, lower bar
};

/**
 * Get the pass threshold for a given tolerance level.
 * @param {string} tolerance - 'strict', 'moderate', or 'lax'
 * @returns {number} The pass threshold (0-1)
 */
export function getPassThreshold(tolerance) {
  return PASS_THRESHOLDS[tolerance] || 0.60;
}

/**
 * Compute pattern match score based on Hamming distance.
 * @param {number[]} generated - Array of step positions with hits in generated pattern
 * @param {number[]} reference - Array of step positions expected in reference pattern
 * @param {string} tolerance - 'strict', 'moderate', or 'lax'
 * @returns {number} Score from 0-1
 */
export function computePatternMatch(generated, reference, tolerance) {
  if (!reference) return 1.0; // No reference = any pattern OK

  // Convert arrays of hit positions to sets for comparison
  const generatedSet = new Set(generated);
  const referenceSet = new Set(reference);

  // Count differences (symmetric difference = Hamming distance for presence/absence)
  let hammingDistance = 0;
  for (const pos of referenceSet) {
    if (!generatedSet.has(pos)) hammingDistance++;
  }
  for (const pos of generatedSet) {
    if (!referenceSet.has(pos)) hammingDistance++;
  }

  const maxDistance = reference.length;

  const toleranceMultiplier = {
    strict: 1.0, // Exact match important
    moderate: 1.5, // Some variation allowed
    lax: 2.5, // Wide variation acceptable
  }[tolerance] || 1.0;

  return Math.max(0, 1.0 - hammingDistance / (maxDistance * toleranceMultiplier));
}

/**
 * Check if all required hit positions are present in the pattern.
 * @param {number[]} patternSteps - Array of step positions with hits
 * @param {number[]} requiredHits - Array of step positions that must have hits
 * @returns {number} 1 if all required hits present, 0 otherwise
 */
export function checkRequiredHits(patternSteps, requiredHits) {
  if (!requiredHits || requiredHits.length === 0) return 1;

  const patternSet = new Set(patternSteps);
  for (const pos of requiredHits) {
    if (!patternSet.has(pos)) return 0;
  }
  return 1;
}

/**
 * Compute metric conformance score based on min/max/target/range specs.
 * @param {number} actual - Actual metric value
 * @param {object} spec - Spec object with min/max/target/range and optional tolerance
 * @returns {number} Score from 0-1
 */
export function computeMetricConformance(actual, spec) {
  if (spec.target !== undefined) {
    const distance = Math.abs(actual - spec.target);
    return Math.max(0, 1.0 - distance / (spec.tolerance || 0.2));
  }
  if (spec.min !== undefined && actual < spec.min) {
    return Math.max(0, 1.0 - (spec.min - actual) * 3);
  }
  if (spec.max !== undefined && actual > spec.max) {
    return Math.max(0, 1.0 - (actual - spec.max) * 3);
  }
  if (spec.range !== undefined) {
    const [min, max] = spec.range;
    if (actual >= min && actual <= max) return 1.0;
    const distance = actual < min ? min - actual : actual - max;
    return Math.max(0, 1.0 - distance * 2);
  }
  return 1.0;
}

/**
 * Compute composite preset conformance score.
 * @param {string} presetName - Name of the preset
 * @param {object} pattern - Pattern object with masks and steps
 * @param {object} pentagonMetrics - Pentagon metrics object with raw values
 * @returns {object} Conformance result with score, breakdown, tolerance, pass, status
 */
export function computePresetConformance(presetName, pattern, pentagonMetrics) {
  const ref = PRESET_REFERENCES[presetName];
  if (!ref) return { score: 0.5, status: 'unknown', breakdown: {}, pass: false };

  let weightedScore = 0;
  let totalWeight = 0;
  const breakdown = {};

  // Pattern match score (weight 0.3)
  if (ref.anchorPattern) {
    // Extract anchor (v1) hit positions from pattern
    const anchorHits = pattern.steps
      ? pattern.steps.filter((s) => s.v1).map((s) => s.step)
      : [];
    const patternScore = computePatternMatch(
      anchorHits,
      ref.anchorPattern,
      ref.tolerance
    );
    breakdown.patternMatch = patternScore;
    weightedScore += patternScore * 0.3;
    totalWeight += 0.3;
  }

  // Required hits check (weight 0.2)
  if (ref.requiredHits && ref.requiredHits.anchor) {
    const anchorHits = pattern.steps
      ? pattern.steps.filter((s) => s.v1).map((s) => s.step)
      : [];
    const requiredScore = checkRequiredHits(anchorHits, ref.requiredHits.anchor);
    breakdown.requiredHits = requiredScore;
    weightedScore += requiredScore * 0.2;
    totalWeight += 0.2;
  }

  // Metric conformance (use spec weights)
  const rawMetrics = pentagonMetrics.raw || pentagonMetrics;
  for (const [metric, spec] of Object.entries(ref.metrics || {})) {
    const actual = rawMetrics[metric];
    if (actual !== undefined) {
      const score = computeMetricConformance(actual, spec);
      breakdown[metric] = score;
      weightedScore += score * spec.weight;
      totalWeight += spec.weight;
    }
  }

  const finalScore = totalWeight > 0 ? weightedScore / totalWeight : 0.5;

  return {
    score: finalScore,
    breakdown,
    tolerance: ref.tolerance,
    pass: finalScore >= getPassThreshold(ref.tolerance),
    status:
      finalScore >= 0.8
        ? 'excellent'
        : finalScore >= 0.6
          ? 'good'
          : finalScore >= 0.4
            ? 'fair'
            : 'poor',
  };
}
