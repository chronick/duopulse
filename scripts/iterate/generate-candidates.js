#!/usr/bin/env node
/**
 * generate-candidates.js
 *
 * Generates weight variation candidates for ensemble search.
 * Supports multiple variation strategies: random, gradient, crossover, mutation.
 *
 * Usage:
 *   node generate-candidates.js --count 4 --strategy random
 *   node generate-candidates.js --count 4 --strategy gradient --target syncopation
 *   node generate-candidates.js --count 2 --strategy crossover --parents a.json,b.json
 *   node generate-candidates.js --count 4 --strategy mutation
 *
 * Output:
 *   JSON array of candidate configurations to stdout
 */

const fs = require('fs');
const path = require('path');

const PROJECT_ROOT = path.join(__dirname, '../..');
const ENSEMBLE_CONFIG_PATH = path.join(PROJECT_ROOT, 'metrics/ensemble-config.json');
const BASELINE_WEIGHTS_PATH = path.join(PROJECT_ROOT, 'config/weights/baseline.json');
const SENSITIVITY_MATRIX_PATH = path.join(PROJECT_ROOT, 'metrics/sensitivity-matrix.json');

// Candidate ID letters
const CANDIDATE_IDS = ['A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'];

/**
 * Load JSON file with error handling
 */
function loadJSON(filePath, required = true) {
  if (!fs.existsSync(filePath)) {
    if (required) {
      console.error(`Error: Required file not found: ${filePath}`);
      process.exit(1);
    }
    return null;
  }
  return JSON.parse(fs.readFileSync(filePath, 'utf-8'));
}

/**
 * Convert baseline.json format to flat weight object
 */
function flattenWeights(config) {
  return {
    euclideanFadeStart: config.euclidean.fadeStart,
    euclideanFadeEnd: config.euclidean.fadeEnd,
    syncopationCenter: config.syncopation.center,
    syncopationWidth: config.syncopation.width,
    randomFadeStart: config.random.fadeStart,
    randomFadeEnd: config.random.fadeEnd,
  };
}

/**
 * Convert flat weight object to baseline.json format
 */
function unflattenWeights(flat, template) {
  return {
    ...template,
    euclidean: {
      ...template.euclidean,
      fadeStart: flat.euclideanFadeStart,
      fadeEnd: flat.euclideanFadeEnd,
    },
    syncopation: {
      center: flat.syncopationCenter,
      width: flat.syncopationWidth,
    },
    random: {
      fadeStart: flat.randomFadeStart,
      fadeEnd: flat.randomFadeEnd,
    },
  };
}

/**
 * Clamp value to bounds
 */
function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

/**
 * Apply weight bounds and constraints
 */
function applyConstraints(weights, bounds, constraints) {
  const result = { ...weights };

  // Apply bounds
  for (const [key, value] of Object.entries(result)) {
    if (bounds[key]) {
      result[key] = clamp(value, bounds[key].min, bounds[key].max);
    }
  }

  // Apply ordering constraints
  if (constraints.euclidean) {
    if (constraints.euclidean.fadeStart_lt_fadeEnd) {
      const minRange = constraints.euclidean.min_fade_range || 0.1;
      if (result.euclideanFadeStart >= result.euclideanFadeEnd - minRange) {
        result.euclideanFadeStart = result.euclideanFadeEnd - minRange;
        result.euclideanFadeStart = clamp(
          result.euclideanFadeStart,
          bounds.euclideanFadeStart.min,
          bounds.euclideanFadeStart.max
        );
      }
    }
  }

  if (constraints.random) {
    if (constraints.random.fadeStart_lt_fadeEnd) {
      const minRange = constraints.random.min_fade_range || 0.1;
      if (result.randomFadeStart >= result.randomFadeEnd - minRange) {
        result.randomFadeStart = result.randomFadeEnd - minRange;
        result.randomFadeStart = clamp(
          result.randomFadeStart,
          bounds.randomFadeStart.min,
          bounds.randomFadeStart.max
        );
      }
    }
  }

  return result;
}

/**
 * Generate random variation
 */
function randomVariation(base, scale, bounds, constraints) {
  const result = { ...base };

  for (const key of Object.keys(result)) {
    const delta = (Math.random() * 2 - 1) * scale;
    result[key] = result[key] + delta;
  }

  return applyConstraints(result, bounds, constraints);
}

/**
 * Generate gradient-directed variation (nudge toward improving target metric)
 */
function gradientVariation(base, target, sensitivity, scale, bounds, constraints) {
  const result = { ...base };

  // Map target metric to sensitivity matrix keys
  const metricToSensKey = {
    syncopation: 'syncopation',
    density: 'density',
    velocityRange: 'velocityRange',
    voiceSeparation: 'voiceSeparation',
    regularity: 'regularity',
  };

  const sensMetric = metricToSensKey[target] || target;

  // Map flat weight keys to sensitivity matrix parameter names
  const weightToSensParam = {
    euclideanFadeStart: 'shapeZone1End',
    euclideanFadeEnd: 'shapeCrossfade1End',
    syncopationCenter: 'shapeZone2aEnd',
    syncopationWidth: 'shapeCrossfade2End',
    randomFadeStart: 'shapeZone2bEnd',
    randomFadeEnd: 'shapeCrossfade3End',
  };

  if (sensitivity && sensitivity.matrix) {
    for (const [weightKey, sensParam] of Object.entries(weightToSensParam)) {
      const paramSens = sensitivity.matrix[sensParam];
      if (paramSens && paramSens[sensMetric] !== undefined) {
        const sens = paramSens[sensMetric];
        // Move in direction of positive sensitivity (improve metric)
        const direction = sens >= 0 ? 1 : -1;
        const magnitude = Math.abs(sens) > 0 ? scale : scale * 0.5;
        result[weightKey] += direction * magnitude * (0.5 + Math.random() * 0.5);
      } else {
        // No sensitivity data, random nudge
        result[weightKey] += (Math.random() * 2 - 1) * scale * 0.3;
      }
    }
  } else {
    // No sensitivity data available, fall back to random
    return randomVariation(base, scale, bounds, constraints);
  }

  return applyConstraints(result, bounds, constraints);
}

/**
 * Generate crossover from two parents
 */
function crossoverVariation(parent1, parent2, bounds, constraints) {
  const result = {};

  for (const key of Object.keys(parent1)) {
    // Blend with random weight
    const alpha = Math.random();
    result[key] = alpha * parent1[key] + (1 - alpha) * parent2[key];
  }

  return applyConstraints(result, bounds, constraints);
}

/**
 * Generate single-point mutation
 */
function mutationVariation(base, scale, bounds, constraints) {
  const result = { ...base };
  const keys = Object.keys(result);

  // Pick random weight to mutate
  const mutateKey = keys[Math.floor(Math.random() * keys.length)];

  // Apply larger variation to single parameter
  const delta = (Math.random() * 2 - 1) * scale * 3;
  result[mutateKey] = result[mutateKey] + delta;

  return applyConstraints(result, bounds, constraints);
}

/**
 * Parse command line arguments
 */
function parseArgs() {
  const args = process.argv.slice(2);
  const opts = {
    count: 4,
    strategy: 'random',
    target: null,
    parents: null,
    scale: null,
    baseWeights: null,
  };

  for (let i = 0; i < args.length; i++) {
    const arg = args[i];

    if (arg === '--count' || arg === '-n') {
      opts.count = parseInt(args[++i], 10);
    } else if (arg === '--strategy' || arg === '-s') {
      opts.strategy = args[++i];
    } else if (arg === '--target' || arg === '-t') {
      opts.target = args[++i];
    } else if (arg === '--parents' || arg === '-p') {
      opts.parents = args[++i].split(',');
    } else if (arg === '--scale') {
      opts.scale = parseFloat(args[++i]);
    } else if (arg === '--base' || arg === '-b') {
      opts.baseWeights = args[++i];
    } else if (arg === '--help' || arg === '-h') {
      console.log(`
Usage: generate-candidates.js [options]

Options:
  --count, -n <N>          Number of candidates to generate (default: 4)
  --strategy, -s <name>    Variation strategy: random, gradient, crossover, mutation
  --target, -t <metric>    Target metric for gradient strategy
  --parents, -p <a,b>      Parent config files for crossover strategy
  --scale <float>          Variation scale (default: from ensemble-config.json)
  --base, -b <file>        Base weights file (default: config/weights/baseline.json)
  --help, -h               Show this help

Strategies:
  random     - Each weight varied by +/- scale * random()
  gradient   - Nudge weights based on sensitivity to target metric
  crossover  - Blend two parent configurations
  mutation   - Perturb single random weight
`);
      process.exit(0);
    }
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
  const baselineWeightsFile = opts.baseWeights || BASELINE_WEIGHTS_PATH;
  const baselineWeights = loadJSON(baselineWeightsFile);
  const sensitivity = loadJSON(SENSITIVITY_MATRIX_PATH, false);

  // Extract configuration
  const bounds = ensembleConfig.weight_bounds;
  const constraints = ensembleConfig.constraints;
  const scale = opts.scale || ensembleConfig.search.variation_scale;

  // Convert baseline to flat format
  const base = flattenWeights(baselineWeights);

  // Load parents for crossover
  let parents = null;
  if (opts.strategy === 'crossover' && opts.parents) {
    parents = opts.parents.map(p => {
      const parentConfig = loadJSON(p);
      return flattenWeights(parentConfig);
    });
  }

  // Generate candidates
  const candidates = [];

  for (let i = 0; i < opts.count; i++) {
    let weights;
    let variationDesc;

    switch (opts.strategy) {
      case 'random':
        weights = randomVariation(base, scale, bounds, constraints);
        variationDesc = `base +/- ${(scale * 100).toFixed(0)}%`;
        break;

      case 'gradient':
        if (!opts.target) {
          console.error('Error: --target required for gradient strategy');
          process.exit(1);
        }
        weights = gradientVariation(base, opts.target, sensitivity, scale, bounds, constraints);
        variationDesc = `gradient toward ${opts.target}`;
        break;

      case 'crossover':
        if (!parents || parents.length < 2) {
          console.error('Error: --parents with 2 files required for crossover strategy');
          process.exit(1);
        }
        weights = crossoverVariation(parents[0], parents[1], bounds, constraints);
        variationDesc = 'crossover blend';
        break;

      case 'mutation':
        weights = mutationVariation(base, scale, bounds, constraints);
        variationDesc = 'single-point mutation';
        break;

      default:
        console.error(`Error: Unknown strategy: ${opts.strategy}`);
        process.exit(1);
    }

    // Round weights to 2 decimal places
    for (const key of Object.keys(weights)) {
      weights[key] = Math.round(weights[key] * 100) / 100;
    }

    candidates.push({
      id: CANDIDATE_IDS[i] || `C${i}`,
      weights,
      strategy: opts.strategy,
      variation: variationDesc,
      // Include full config for writing to file
      config: unflattenWeights(weights, baselineWeights),
    });
  }

  // Output JSON
  console.log(JSON.stringify(candidates, null, 2));
}

main();
