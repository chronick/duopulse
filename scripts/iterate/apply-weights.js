#!/usr/bin/env node
/**
 * apply-weights.js
 *
 * Applies weight configurations from JSON to inc/algorithm_config.h.
 * Updates constexpr values in the header file while preserving formatting.
 *
 * Usage:
 *   node apply-weights.js config/weights/winner-A.json
 *   node apply-weights.js --weights weights.json --output inc/algorithm_config.h
 *   node apply-weights.js --weights weights.json --dry-run
 *
 * Input JSON format (flat):
 *   {
 *     "euclideanFadeStart": 0.28,
 *     "euclideanFadeEnd": 0.45,
 *     "syncopationCenter": 0.52,
 *     "syncopationWidth": 0.25,
 *     "randomFadeStart": 0.72,
 *     "randomFadeEnd": 0.88
 *   }
 *
 * Also accepts baseline.json nested format with euclidean/syncopation/random keys.
 */

const fs = require('fs');
const path = require('path');

const PROJECT_ROOT = path.join(__dirname, '../..');
const DEFAULT_HEADER_PATH = path.join(PROJECT_ROOT, 'inc/algorithm_config.h');

// Mapping from JSON keys to C++ constexpr names
const WEIGHT_TO_CONSTEXPR = {
  euclideanFadeStart: 'kEuclideanFadeStart',
  euclideanFadeEnd: 'kEuclideanFadeEnd',
  syncopationCenter: 'kSyncopationCenter',
  syncopationWidth: 'kSyncopationWidth',
  randomFadeStart: 'kRandomFadeStart',
  randomFadeEnd: 'kRandomFadeEnd',
};

// Valid bounds for weight values
const WEIGHT_BOUNDS = {
  euclideanFadeStart: { min: 0.0, max: 1.0 },
  euclideanFadeEnd: { min: 0.0, max: 1.0 },
  syncopationCenter: { min: 0.0, max: 1.0 },
  syncopationWidth: { min: 0.05, max: 0.5 },
  randomFadeStart: { min: 0.0, max: 1.0 },
  randomFadeEnd: { min: 0.0, max: 1.0 },
};

/**
 * Load JSON file with error handling
 */
function loadJSON(filePath) {
  if (!fs.existsSync(filePath)) {
    console.error(`Error: File not found: ${filePath}`);
    process.exit(1);
  }
  try {
    return JSON.parse(fs.readFileSync(filePath, 'utf-8'));
  } catch (e) {
    console.error(`Error: Invalid JSON in ${filePath}: ${e.message}`);
    process.exit(1);
  }
}

/**
 * Detect and normalize weight format
 * Accepts both flat format and nested baseline.json format
 */
function normalizeWeights(config) {
  // Check if nested format (has euclidean/syncopation/random keys)
  if (config.euclidean && config.syncopation && config.random) {
    return {
      euclideanFadeStart: config.euclidean.fadeStart,
      euclideanFadeEnd: config.euclidean.fadeEnd,
      syncopationCenter: config.syncopation.center,
      syncopationWidth: config.syncopation.width,
      randomFadeStart: config.random.fadeStart,
      randomFadeEnd: config.random.fadeEnd,
    };
  }

  // Already flat format or partial - return as-is
  return config;
}

/**
 * Validate weight values are within bounds
 */
function validateWeights(weights) {
  const errors = [];

  for (const [key, value] of Object.entries(weights)) {
    if (!WEIGHT_BOUNDS[key]) {
      continue; // Skip unknown keys (might be metadata like "version")
    }

    if (typeof value !== 'number') {
      errors.push(`${key}: expected number, got ${typeof value}`);
      continue;
    }

    const bounds = WEIGHT_BOUNDS[key];
    if (value < bounds.min || value > bounds.max) {
      errors.push(`${key}: ${value} out of bounds [${bounds.min}, ${bounds.max}]`);
    }
  }

  // Check ordering constraints
  if (weights.euclideanFadeStart !== undefined && weights.euclideanFadeEnd !== undefined) {
    if (weights.euclideanFadeStart >= weights.euclideanFadeEnd) {
      errors.push(`euclideanFadeStart (${weights.euclideanFadeStart}) must be < euclideanFadeEnd (${weights.euclideanFadeEnd})`);
    }
  }

  if (weights.randomFadeStart !== undefined && weights.randomFadeEnd !== undefined) {
    if (weights.randomFadeStart >= weights.randomFadeEnd) {
      errors.push(`randomFadeStart (${weights.randomFadeStart}) must be < randomFadeEnd (${weights.randomFadeEnd})`);
    }
  }

  return errors;
}

/**
 * Update a single constexpr value in the header content
 */
function updateConstexpr(content, constexprName, newValue) {
  // Match: constexpr float kName = 0.30f;
  // With optional comments and varying whitespace
  const pattern = new RegExp(
    `(constexpr\\s+float\\s+${constexprName}\\s*=\\s*)([0-9.]+)f(\\s*;)`,
    'g'
  );

  // Format value to 2 decimal places with 'f' suffix
  const formattedValue = newValue.toFixed(2);

  let matched = false;
  const updated = content.replace(pattern, (match, prefix, oldValue, suffix) => {
    matched = true;
    return `${prefix}${formattedValue}f${suffix}`;
  });

  return { content: updated, matched, oldValue: matched ? content.match(pattern)?.[0] : null };
}

/**
 * Apply all weight changes to header content
 */
function applyWeightsToHeader(headerContent, weights) {
  let content = headerContent;
  const changes = [];
  const notFound = [];

  for (const [jsonKey, value] of Object.entries(weights)) {
    const constexprName = WEIGHT_TO_CONSTEXPR[jsonKey];
    if (!constexprName) {
      continue; // Skip metadata keys
    }

    // Extract old value for reporting
    const oldValueMatch = content.match(
      new RegExp(`constexpr\\s+float\\s+${constexprName}\\s*=\\s*([0-9.]+)f`)
    );
    const oldValue = oldValueMatch ? parseFloat(oldValueMatch[1]) : null;

    const result = updateConstexpr(content, constexprName, value);
    content = result.content;

    if (result.matched) {
      changes.push({
        key: jsonKey,
        constexpr: constexprName,
        oldValue: oldValue,
        newValue: value,
        changed: oldValue !== null && Math.abs(oldValue - value) > 0.001,
      });
    } else {
      notFound.push(constexprName);
    }
  }

  return { content, changes, notFound };
}

/**
 * Create backup of file
 */
function createBackup(filePath) {
  const backupPath = `${filePath}.backup`;
  fs.copyFileSync(filePath, backupPath);
  return backupPath;
}

/**
 * Parse command line arguments
 */
function parseArgs() {
  const args = process.argv.slice(2);
  const opts = {
    weightsFile: null,
    outputFile: DEFAULT_HEADER_PATH,
    dryRun: false,
    noBackup: false,
    quiet: false,
  };

  for (let i = 0; i < args.length; i++) {
    const arg = args[i];

    if (arg === '--weights' || arg === '-w') {
      opts.weightsFile = args[++i];
    } else if (arg === '--output' || arg === '-o') {
      opts.outputFile = args[++i];
    } else if (arg === '--dry-run' || arg === '-n') {
      opts.dryRun = true;
    } else if (arg === '--no-backup') {
      opts.noBackup = true;
    } else if (arg === '--quiet' || arg === '-q') {
      opts.quiet = true;
    } else if (arg === '--help' || arg === '-h') {
      console.log(`
Usage: apply-weights.js [options] [weights.json]

Apply weight configurations from JSON to algorithm_config.h.

Arguments:
  weights.json           JSON file with weight values (positional)

Options:
  --weights, -w <file>   JSON file with weight values
  --output, -o <file>    Header file to update (default: inc/algorithm_config.h)
  --dry-run, -n          Show changes without writing
  --no-backup            Don't create .backup file
  --quiet, -q            Minimal output
  --help, -h             Show this help

Input Format (flat):
  {
    "euclideanFadeStart": 0.28,
    "euclideanFadeEnd": 0.45,
    "syncopationCenter": 0.52,
    "syncopationWidth": 0.25,
    "randomFadeStart": 0.72,
    "randomFadeEnd": 0.88
  }

Also accepts baseline.json nested format with euclidean/syncopation/random keys.
`);
      process.exit(0);
    } else if (!arg.startsWith('-')) {
      // Positional argument - treat as weights file
      opts.weightsFile = arg;
    }
  }

  return opts;
}

/**
 * Main
 */
function main() {
  const opts = parseArgs();

  if (!opts.weightsFile) {
    console.error('Error: No weights file specified');
    console.error('Usage: apply-weights.js [--weights] <weights.json>');
    process.exit(1);
  }

  // Resolve paths
  const weightsPath = path.isAbsolute(opts.weightsFile)
    ? opts.weightsFile
    : path.join(process.cwd(), opts.weightsFile);
  const headerPath = path.isAbsolute(opts.outputFile)
    ? opts.outputFile
    : path.join(PROJECT_ROOT, opts.outputFile);

  // Load and normalize weights
  const rawConfig = loadJSON(weightsPath);
  const weights = normalizeWeights(rawConfig);

  // Validate weights
  const validationErrors = validateWeights(weights);
  if (validationErrors.length > 0) {
    console.error('Validation errors:');
    validationErrors.forEach(err => console.error(`  - ${err}`));
    process.exit(1);
  }

  // Read header file
  if (!fs.existsSync(headerPath)) {
    console.error(`Error: Header file not found: ${headerPath}`);
    process.exit(1);
  }
  const headerContent = fs.readFileSync(headerPath, 'utf-8');

  // Apply changes
  const { content, changes, notFound } = applyWeightsToHeader(headerContent, weights);

  // Report
  if (!opts.quiet) {
    console.log(`Applying weights from: ${weightsPath}`);
    console.log(`Target header: ${headerPath}`);
    console.log('');

    if (changes.length > 0) {
      console.log('Changes:');
      for (const change of changes) {
        const status = change.changed ? 'CHANGED' : 'unchanged';
        const arrow = change.changed ? '->' : '==';
        console.log(
          `  ${change.constexpr}: ${change.oldValue?.toFixed(2)} ${arrow} ${change.newValue.toFixed(2)} [${status}]`
        );
      }
    }

    if (notFound.length > 0) {
      console.log('');
      console.log('Not found in header (skipped):');
      notFound.forEach(name => console.log(`  - ${name}`));
    }

    const actualChanges = changes.filter(c => c.changed);
    console.log('');
    console.log(`Summary: ${actualChanges.length} values changed, ${changes.length - actualChanges.length} unchanged`);
  }

  // Write output
  if (opts.dryRun) {
    if (!opts.quiet) {
      console.log('');
      console.log('Dry run - no changes written');
    }
  } else {
    // Create backup unless disabled
    if (!opts.noBackup) {
      const backupPath = createBackup(headerPath);
      if (!opts.quiet) {
        console.log(`Backup created: ${backupPath}`);
      }
    }

    // Write updated header
    fs.writeFileSync(headerPath, content, 'utf-8');

    if (!opts.quiet) {
      console.log(`Updated: ${headerPath}`);
    }
  }

  // Exit with success
  process.exit(0);
}

main();
