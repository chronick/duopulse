#!/usr/bin/env node
/**
 * Update baseline with new metrics
 *
 * Reads new metrics, compares to current baseline, and updates baseline.json
 * with proper version tagging and regression tracking.
 *
 * Environment variables:
 *   BASELINE_PATH: Path to baseline.json (default: metrics/baseline.json)
 *   REGRESSION_THRESHOLD: Max allowed regression (default: 0.02 = 2%)
 *
 * Usage:
 *   node scripts/update-baseline.js < new-metrics.json
 *   node scripts/update-baseline.js --metrics=path/to/metrics.json
 *   cat metrics.json | node scripts/update-baseline.js
 *
 * Exit codes:
 *   0: Baseline updated successfully
 *   1: Error occurred
 */

const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

// Configuration
const BASELINE_PATH = process.env.BASELINE_PATH || 'metrics/baseline.json';
const REGRESSION_THRESHOLD = parseFloat(process.env.REGRESSION_THRESHOLD || '0.02');

/**
 * Parse command line arguments
 */
function parseArgs() {
    const args = process.argv.slice(2);
    const result = { metricsPath: null };

    for (const arg of args) {
        if (arg.startsWith('--metrics=')) {
            result.metricsPath = arg.substring('--metrics='.length);
        } else if (arg === '--help' || arg === '-h') {
            console.log(`
Usage: node scripts/update-baseline.js [options]

Options:
  --metrics=PATH    Path to new metrics JSON file
  --help, -h        Show this help message

If --metrics is not provided, reads from stdin.

Environment variables:
  BASELINE_PATH         Path to baseline.json (default: metrics/baseline.json)
  REGRESSION_THRESHOLD  Max allowed regression (default: 0.02 = 2%)
`);
            process.exit(0);
        }
    }

    return result;
}

/**
 * Read JSON from stdin
 */
function readStdin() {
    return new Promise((resolve, reject) => {
        let data = '';
        process.stdin.setEncoding('utf8');

        process.stdin.on('readable', () => {
            let chunk;
            while ((chunk = process.stdin.read()) !== null) {
                data += chunk;
            }
        });

        process.stdin.on('end', () => {
            try {
                resolve(JSON.parse(data));
            } catch (err) {
                reject(new Error(`Failed to parse stdin as JSON: ${err.message}`));
            }
        });

        process.stdin.on('error', reject);

        // Timeout after 5 seconds if no input
        setTimeout(() => {
            if (data === '') {
                reject(new Error('No input received from stdin (timeout after 5s)'));
            }
        }, 5000);
    });
}

/**
 * Load JSON from file
 */
function loadJSON(filepath) {
    try {
        const content = fs.readFileSync(filepath, 'utf-8');
        return JSON.parse(content);
    } catch (err) {
        if (err.code === 'ENOENT') {
            return null;
        }
        throw new Error(`Error loading ${filepath}: ${err.message}`);
    }
}

/**
 * Save JSON to file
 */
function saveJSON(filepath, data) {
    const dir = path.dirname(filepath);
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir, { recursive: true });
    }
    fs.writeFileSync(filepath, JSON.stringify(data, null, 2) + '\n');
}

/**
 * Get current git commit hash
 */
function getGitCommit() {
    try {
        return execSync('git rev-parse HEAD', { encoding: 'utf-8' }).trim();
    } catch {
        return 'unknown';
    }
}

/**
 * Parse version tag and increment patch version
 * Format: baseline-v{major}.{minor}.{patch}
 */
function incrementVersion(currentTag) {
    if (!currentTag) {
        return 'baseline-v1.0.0';
    }

    const match = currentTag.match(/^baseline-v(\d+)\.(\d+)\.(\d+)$/);
    if (!match) {
        console.warn(`Warning: Could not parse version from tag "${currentTag}", starting fresh`);
        return 'baseline-v1.0.0';
    }

    const major = parseInt(match[1], 10);
    const minor = parseInt(match[2], 10);
    const patch = parseInt(match[3], 10);

    return `baseline-v${major}.${minor}.${patch + 1}`;
}

/**
 * Get overallAlignment score from metrics
 */
function getOverallAlignment(data) {
    // Handle both baseline format (wrapped in metrics) and raw evals format
    const metrics = data.metrics || data;
    return metrics.overallAlignment;
}

/**
 * Detect if new metrics represent a regression
 * Regression if: new_score < (baseline_score - threshold)
 */
function detectRegression(baseline, newMetrics) {
    const baselineScore = getOverallAlignment(baseline);
    const newScore = getOverallAlignment(newMetrics);

    if (baselineScore === undefined || newScore === undefined) {
        console.warn('Warning: Could not find overallAlignment scores');
        return { isRegression: false, baselineScore: null, newScore: null };
    }

    const threshold = baselineScore - REGRESSION_THRESHOLD;
    const isRegression = newScore < threshold;

    return {
        isRegression,
        baselineScore,
        newScore,
        delta: newScore - baselineScore,
        deltaPct: baselineScore !== 0 ? ((newScore - baselineScore) / baselineScore) * 100 : 0
    };
}

/**
 * Main function
 */
async function main() {
    const args = parseArgs();

    console.log('Updating baseline...');
    console.log(`  Baseline path: ${BASELINE_PATH}`);
    console.log(`  Regression threshold: ${REGRESSION_THRESHOLD * 100}%`);

    // Load new metrics
    let newMetrics;
    if (args.metricsPath) {
        console.log(`  Loading metrics from: ${args.metricsPath}`);
        newMetrics = loadJSON(args.metricsPath);
        if (!newMetrics) {
            console.error(`Error: Could not load metrics from ${args.metricsPath}`);
            process.exit(1);
        }
    } else {
        console.log('  Reading metrics from stdin...');
        try {
            newMetrics = await readStdin();
        } catch (err) {
            console.error(`Error: ${err.message}`);
            process.exit(1);
        }
    }

    // Load current baseline
    const currentBaseline = loadJSON(BASELINE_PATH);
    if (!currentBaseline) {
        console.log('  No existing baseline found, creating new one');
    }

    // Detect regression
    const regressionResult = currentBaseline
        ? detectRegression(currentBaseline, newMetrics)
        : { isRegression: false, baselineScore: null, newScore: getOverallAlignment(newMetrics) };

    console.log('');
    if (regressionResult.baselineScore !== null) {
        console.log(`  Baseline score: ${(regressionResult.baselineScore * 100).toFixed(2)}%`);
    }
    console.log(`  New score:      ${(regressionResult.newScore * 100).toFixed(2)}%`);
    if (regressionResult.delta !== undefined) {
        const deltaStr = regressionResult.delta >= 0 ? '+' : '';
        console.log(`  Delta:          ${deltaStr}${(regressionResult.delta * 100).toFixed(2)}% (${deltaStr}${regressionResult.deltaPct.toFixed(2)}%)`);
    }
    console.log('');

    // Generate new tag
    const currentTag = currentBaseline?.tag || null;
    const newTag = incrementVersion(currentTag);
    const newCommit = getGitCommit();
    const newTimestamp = new Date().toISOString();

    // Build new baseline
    let newBaseline;
    if (regressionResult.isRegression) {
        // Regression detected: increment counter, keep last_good unchanged
        const consecutiveRegressions = (currentBaseline?.consecutive_regressions || 0) + 1;
        console.log(`Regression detected! Consecutive regressions: ${consecutiveRegressions}`);

        newBaseline = {
            version: currentBaseline?.version || '1.0.0',
            generated_at: newTimestamp,
            commit: newCommit,
            tag: newTag,
            last_good: currentBaseline?.last_good || null,
            consecutive_regressions: consecutiveRegressions,
            metrics: newMetrics.metrics || newMetrics
        };
    } else {
        // No regression: update last_good to current baseline, reset counter
        console.log('No regression detected. Updating baseline.');

        // Create last_good from current baseline (if it exists)
        let lastGood = null;
        if (currentBaseline) {
            lastGood = {
                commit: currentBaseline.commit,
                tag: currentBaseline.tag,
                timestamp: currentBaseline.generated_at
            };
        }

        newBaseline = {
            version: currentBaseline?.version || '1.0.0',
            generated_at: newTimestamp,
            commit: newCommit,
            tag: newTag,
            last_good: lastGood,
            consecutive_regressions: 0,
            metrics: newMetrics.metrics || newMetrics
        };
    }

    // Save updated baseline
    saveJSON(BASELINE_PATH, newBaseline);
    console.log(`\nBaseline saved to ${BASELINE_PATH}`);

    // Output summary
    console.log('\n--- Summary ---');
    console.log(`Tag:                    ${newTag}`);
    console.log(`Commit:                 ${newCommit.substring(0, 8)}`);
    console.log(`Regression:             ${regressionResult.isRegression ? 'YES' : 'NO'}`);
    console.log(`Consecutive regressions: ${newBaseline.consecutive_regressions}`);
    if (newBaseline.last_good) {
        console.log(`Last good:              ${newBaseline.last_good.tag || newBaseline.last_good.commit?.substring(0, 8) || 'unknown'}`);
    }

    // Output machine-readable JSON to stdout for scripting
    const output = {
        tag: newTag,
        commit: newCommit,
        is_regression: regressionResult.isRegression,
        consecutive_regressions: newBaseline.consecutive_regressions,
        baseline_score: regressionResult.baselineScore,
        new_score: regressionResult.newScore,
        delta: regressionResult.delta,
        last_good: newBaseline.last_good
    };
    console.log('\n--- JSON Output ---');
    console.log(JSON.stringify(output, null, 2));

    process.exit(0);
}

main().catch(err => {
    console.error(`Error: ${err.message}`);
    process.exit(1);
});
