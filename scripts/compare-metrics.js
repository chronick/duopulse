#!/usr/bin/env node
/**
 * Compare PR metrics against baseline
 *
 * Reads baseline from metrics/baseline.json and PR metrics from evals output,
 * calculates deltas, and outputs comparison results.
 *
 * Environment variables:
 *   BASELINE_PATH: Path to baseline.json (default: metrics/baseline.json)
 *   PR_METRICS_PATH: Path to PR metrics (default: tools/evals/public/data/expressiveness.json)
 *   REGRESSION_THRESHOLD: Max allowed regression (default: 0.02 = 2%)
 *   OUTPUT_PATH: Where to write comparison (default: metrics-comparison.json)
 *   GITHUB_OUTPUT: GitHub Actions output file (for setting outputs)
 *
 * Usage:
 *   node scripts/compare-metrics.js
 *
 * Exit codes:
 *   0: No regressions beyond threshold
 *   1: Regressions detected (or error)
 */

const fs = require('fs');
const path = require('path');

// Configuration
const BASELINE_PATH = process.env.BASELINE_PATH || 'metrics/baseline.json';
const PR_METRICS_PATH = process.env.PR_METRICS_PATH || 'tools/evals/public/data/expressiveness.json';
const REGRESSION_THRESHOLD = parseFloat(process.env.REGRESSION_THRESHOLD || '0.02');
const OUTPUT_PATH = process.env.OUTPUT_PATH || 'metrics-comparison.json';
const GITHUB_OUTPUT = process.env.GITHUB_OUTPUT;

// Pentagon metrics to compare
const METRICS = ['syncopation', 'density', 'velocityRange', 'voiceSeparation', 'regularity'];
const ZONES = ['total', 'stable', 'syncopated', 'wild'];
const ENERGY_ZONES = ['minimal', 'groove', 'build', 'peak'];

function loadJSON(filepath) {
    try {
        const content = fs.readFileSync(filepath, 'utf-8');
        return JSON.parse(content);
    } catch (err) {
        console.error(`Error loading ${filepath}: ${err.message}`);
        process.exit(1);
    }
}

function getMetricValue(data, zone, metric, isEnergyZone = false) {
    // Handle both baseline format (wrapped in metrics) and raw evals format
    const metrics = data.metrics || data;

    // For ENERGY zones, look in energyZoneStats
    if (isEnergyZone) {
        if (metrics.energyZoneStats && metrics.energyZoneStats[zone]) {
            return metrics.energyZoneStats[zone][metric];
        }
        return null;
    }

    if (metrics.pentagonStats && metrics.pentagonStats[zone]) {
        return metrics.pentagonStats[zone][metric];
    }

    // Fallback for flat structure
    if (metrics[zone] && metrics[zone][metric] !== undefined) {
        return metrics[zone][metric];
    }

    return null;
}

function compareMetrics(baseline, prMetrics) {
    const results = [];
    let hasRegression = false;

    // Compare SHAPE zones (standard Pentagon metrics)
    for (const zone of ZONES) {
        for (const metric of METRICS) {
            const baselineValue = getMetricValue(baseline, zone, metric, false);
            const prValue = getMetricValue(prMetrics, zone, metric, false);

            if (baselineValue === null || prValue === null) {
                console.warn(`Warning: Missing ${metric} for zone ${zone}`);
                continue;
            }

            const delta = prValue - baselineValue;
            const deltaPct = baselineValue !== 0 ? (delta / baselineValue) * 100 : 0;

            // Determine status
            // For most metrics, higher is better. Negative delta is regression.
            // Exception: regularity might be "worse" if too high (patterns too predictable)
            // But for simplicity, we treat all metrics as "higher is better" for now.
            let status = 'unchanged';
            if (deltaPct > 1) {
                status = 'improved';
            } else if (deltaPct < -REGRESSION_THRESHOLD * 100) {
                status = 'regression';
                hasRegression = true;
            }

            results.push({
                name: metric,
                zone: zone,
                zoneType: 'shape',
                baseline: baselineValue,
                pr: prValue,
                delta: delta,
                delta_pct: deltaPct,
                status: status
            });
        }
    }

    // Compare ENERGY zones (if available)
    const hasEnergyZones = (baseline.metrics || baseline).energyZoneStats || (prMetrics.metrics || prMetrics).energyZoneStats;

    if (hasEnergyZones) {
        for (const zone of ENERGY_ZONES) {
            for (const metric of METRICS) {
                const baselineValue = getMetricValue(baseline, zone, metric, true);
                const prValue = getMetricValue(prMetrics, zone, metric, true);

                if (baselineValue === null || prValue === null) {
                    // Silently skip - energy zones may not be in older baselines
                    continue;
                }

                const delta = prValue - baselineValue;
                const deltaPct = baselineValue !== 0 ? (delta / baselineValue) * 100 : 0;

                let status = 'unchanged';
                if (deltaPct > 1) {
                    status = 'improved';
                } else if (deltaPct < -REGRESSION_THRESHOLD * 100) {
                    status = 'regression';
                    hasRegression = true;
                }

                results.push({
                    name: metric,
                    zone: `energy:${zone}`,
                    zoneType: 'energy',
                    baseline: baselineValue,
                    pr: prValue,
                    delta: delta,
                    delta_pct: deltaPct,
                    status: status
                });
            }
        }
    }

    return { results, hasRegression };
}

function generateMarkdownTable(comparison) {
    const lines = [];

    lines.push('## Pentagon Metrics Comparison');
    lines.push('');
    lines.push(`**Baseline**: \`${comparison.baseline_commit.substring(0, 8)}\` (${comparison.baseline_date})`);
    lines.push(`**PR**: this PR`);
    lines.push(`**Threshold**: ${REGRESSION_THRESHOLD * 100}% regression`);
    lines.push('');
    lines.push('| Metric | Zone | Baseline | PR | Delta | Status |');
    lines.push('|--------|------|----------|-----|-------|--------|');

    // Only show total zone for brevity, or show all if there are regressions
    const showAll = comparison.has_regression;
    const metrics = comparison.metrics.filter(m =>
        showAll || m.zone === 'total' || m.status !== 'unchanged'
    );

    for (const m of metrics) {
        const statusIcon = m.status === 'regression' ? '⚠️' :
                          m.status === 'improved' ? '✅' : '➖';
        const deltaStr = m.delta_pct >= 0 ?
            `+${m.delta_pct.toFixed(2)}%` :
            `${m.delta_pct.toFixed(2)}%`;

        lines.push(`| ${m.name} | ${m.zone} | ${m.baseline.toFixed(3)} | ${m.pr.toFixed(3)} | ${deltaStr} | ${statusIcon} |`);
    }

    lines.push('');

    if (comparison.has_regression) {
        const regressionCount = comparison.metrics.filter(m => m.status === 'regression').length;
        lines.push(`**Result**: ⚠️ ${regressionCount} regression(s) detected`);
        lines.push('');
        lines.push('> Add `allow-regression` label to bypass regression check.');
    } else {
        lines.push('**Result**: ✅ No regressions');
    }

    return lines.join('\n');
}

function main() {
    console.log('Comparing PR metrics to baseline...');
    console.log(`  Baseline: ${BASELINE_PATH}`);
    console.log(`  PR metrics: ${PR_METRICS_PATH}`);
    console.log(`  Threshold: ${REGRESSION_THRESHOLD * 100}%`);

    // Load data
    const baseline = loadJSON(BASELINE_PATH);
    const prMetrics = loadJSON(PR_METRICS_PATH);

    // Extract baseline tracking info
    const consecutiveRegressions = baseline.consecutive_regressions || 0;
    const lastGood = baseline.last_good || null;
    const baselineTag = baseline.tag || null;

    // Compare
    const { results, hasRegression } = compareMetrics(baseline, prMetrics);

    // Determine consecutive regression status
    // If we have a regression AND there was already at least 1 consecutive regression,
    // this would be 2+ in a row
    const wouldBeConsecutive = hasRegression && consecutiveRegressions >= 1;
    const consecutiveCount = hasRegression ? consecutiveRegressions + 1 : 0;
    const suggestRollback = consecutiveCount >= 2;

    // Build output
    const comparison = {
        baseline_commit: baseline.commit || 'unknown',
        baseline_date: baseline.generated_at || 'unknown',
        baseline_info: {
            tag: baselineTag,
            consecutive_regressions: consecutiveRegressions
        },
        pr_commit: process.env.GITHUB_SHA || 'local',
        threshold: REGRESSION_THRESHOLD,
        has_regression: hasRegression,
        regression_detected: hasRegression,
        consecutive_regression_warning: wouldBeConsecutive,
        suggest_rollback: suggestRollback,
        metrics: results
    };

    // Generate markdown
    comparison.markdown = generateMarkdownTable(comparison);

    // Save comparison
    fs.writeFileSync(OUTPUT_PATH, JSON.stringify(comparison, null, 2));
    console.log(`\nComparison saved to ${OUTPUT_PATH}`);

    // Print summary
    console.log('\n' + comparison.markdown);

    // Set GitHub outputs if available
    if (GITHUB_OUTPUT) {
        fs.appendFileSync(GITHUB_OUTPUT, `has_regression=${hasRegression}\n`);
        fs.appendFileSync(GITHUB_OUTPUT, `consecutive_count=${consecutiveCount}\n`);
        fs.appendFileSync(GITHUB_OUTPUT, `suggest_rollback=${suggestRollback}\n`);
        // Escape markdown for multiline output
        const escapedMarkdown = comparison.markdown
            .replace(/%/g, '%25')
            .replace(/\n/g, '%0A')
            .replace(/\r/g, '%0D');
        fs.appendFileSync(GITHUB_OUTPUT, `markdown=${escapedMarkdown}\n`);
    }

    // Exit with error if regression detected
    if (hasRegression) {
        console.log('\n' + '='.repeat(60));
        console.log('  REGRESSION DETECTED');
        console.log('='.repeat(60));

        if (wouldBeConsecutive) {
            console.log('');
            console.log('WARNING: CONSECUTIVE REGRESSION DETECTED (' + consecutiveCount + ' in a row)');
            console.log('This is the ' + consecutiveCount + getOrdinalSuffix(consecutiveCount) + ' consecutive regression.');
        }

        if (suggestRollback) {
            console.log('');
            console.log('RECOMMENDATION: Consider using /rollback to revert to last known good baseline.');
            if (lastGood) {
                console.log('');
                console.log('Last good baseline:');
                console.log('  Tag: ' + (lastGood.tag || 'none'));
                console.log('  Commit: ' + (lastGood.commit ? lastGood.commit.substring(0, 8) : 'unknown'));
                console.log('  Timestamp: ' + (lastGood.timestamp || 'unknown'));
            }
        }

        console.log('');
        process.exit(1);
    } else {
        console.log('\n' + '='.repeat(60));
        console.log('  No regressions detected');
        console.log('='.repeat(60) + '\n');
        process.exit(0);
    }
}

/**
 * Get ordinal suffix for a number (1st, 2nd, 3rd, etc.)
 */
function getOrdinalSuffix(n) {
    const s = ['th', 'st', 'nd', 'rd'];
    const v = n % 100;
    return s[(v - 20) % 10] || s[v] || s[0];
}

main();
