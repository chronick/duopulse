#!/usr/bin/env node
/**
 * Lever Identification and Recommendations
 *
 * Analyzes sensitivity matrix to recommend parameter changes
 * for improving specific Pentagon metrics.
 *
 * Usage:
 *   node scripts/sensitivity/identify-levers.js [options]
 *
 * Options:
 *   --target <metric>   Target metric to improve (required)
 *   --matrix <path>     Path to sensitivity matrix (default: metrics/sensitivity-matrix.json)
 *   --direction <dir>   Direction: "increase" or "decrease" (default: increase)
 *   --format <fmt>      Output format: "text", "json", or "markdown" (default: text)
 */

import { readFileSync, existsSync } from 'fs';
import { dirname, join } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = join(__dirname, '../..');
const DEFAULT_MATRIX = join(PROJECT_ROOT, 'metrics/sensitivity-matrix.json');

const METRICS = ['syncopation', 'density', 'velocityRange', 'voiceSeparation', 'regularity'];

const METRIC_DESCRIPTIONS = {
  syncopation: 'Groove and forward motion',
  density: 'Overall pattern fullness',
  velocityRange: 'Dynamic variation between hits',
  voiceSeparation: 'Clarity vs voice overlap',
  regularity: 'Predictability/danceability',
};

// Parse CLI args
const args = process.argv.slice(2);
let targetMetric = null;
let matrixPath = DEFAULT_MATRIX;
let direction = 'increase';
let format = 'text';

for (let i = 0; i < args.length; i++) {
  if ((args[i] === '--target' || args[i] === '-t') && args[i + 1]) {
    targetMetric = args[++i];
  } else if (args[i] === '--matrix' && args[i + 1]) {
    matrixPath = args[++i];
  } else if (args[i] === '--direction' && args[i + 1]) {
    direction = args[++i];
  } else if (args[i] === '--format' && args[i + 1]) {
    format = args[++i];
  } else if (args[i] === '--help' || args[i] === '-h') {
    console.log(`
Lever Identification - Recommend parameter changes to improve metrics

Usage:
  node identify-levers.js --target <metric> [options]

Arguments:
  --target, -t <metric>  Target metric: ${METRICS.join(', ')}
  --matrix <path>        Path to sensitivity matrix JSON
  --direction <dir>      "increase" or "decrease" (default: increase)
  --format <fmt>         "text", "json", or "markdown" (default: text)
  --help, -h             Show this help

Examples:
  node identify-levers.js --target syncopation
  node identify-levers.js --target regularity --direction decrease
  node identify-levers.js --target density --format markdown
`);
    process.exit(0);
  }
}

if (!targetMetric) {
  console.error('Error: --target is required');
  console.error(`Available metrics: ${METRICS.join(', ')}`);
  process.exit(1);
}

// Normalize metric name
const normalizedMetric = METRICS.find(m =>
  m.toLowerCase() === targetMetric.toLowerCase() ||
  m.toLowerCase().startsWith(targetMetric.toLowerCase())
);

if (!normalizedMetric) {
  console.error(`Error: Unknown metric "${targetMetric}"`);
  console.error(`Available metrics: ${METRICS.join(', ')}`);
  process.exit(1);
}

targetMetric = normalizedMetric;

// Load matrix
if (!existsSync(matrixPath)) {
  console.error(`Error: Sensitivity matrix not found at ${matrixPath}`);
  console.error('Run "make sensitivity-matrix" first.');
  process.exit(1);
}

const matrixData = JSON.parse(readFileSync(matrixPath, 'utf-8'));
const { matrix, levers, details } = matrixData;

// Build recommendations
const multiplier = direction === 'decrease' ? -1 : 1;

// Sort parameters by effectiveness for target metric
const paramRankings = Object.entries(matrix)
  .map(([param, sensitivities]) => ({
    param,
    sensitivity: sensitivities[targetMetric],
    effectiveSensitivity: sensitivities[targetMetric] * multiplier,
    // Collateral impact on other metrics
    collateral: METRICS
      .filter(m => m !== targetMetric)
      .map(m => ({
        metric: m,
        impact: sensitivities[m],
      }))
      .filter(c => Math.abs(c.impact) >= 0.2),
  }))
  .sort((a, b) => b.effectiveSensitivity - a.effectiveSensitivity);

// Categorize recommendations
const recommended = paramRankings.filter(p => p.effectiveSensitivity >= 0.3);
const secondary = paramRankings.filter(p => p.effectiveSensitivity >= 0.1 && p.effectiveSensitivity < 0.3);
const avoid = paramRankings.filter(p => p.effectiveSensitivity < 0 || p.collateral.length > 1);

// Generate output
function formatText() {
  const lines = [];
  lines.push(`\n╔${'═'.repeat(60)}╗`);
  lines.push(`║  Lever Recommendations for ${targetMetric.toUpperCase().padEnd(28)} ║`);
  lines.push(`║  ${METRIC_DESCRIPTIONS[targetMetric].padEnd(56)} ║`);
  lines.push(`║  Direction: ${direction.padEnd(45)} ║`);
  lines.push(`╚${'═'.repeat(60)}╝`);
  lines.push('');

  if (recommended.length > 0) {
    lines.push('PRIMARY LEVERS (high impact):');
    for (const p of recommended) {
      const sign = p.sensitivity > 0 ? '+' : '';
      const action = p.effectiveSensitivity > 0 ? 'increase' : 'decrease';
      lines.push(`   * ${p.param} (${sign}${p.sensitivity.toFixed(2)}) -> ${action}`);
      if (p.collateral.length > 0) {
        const impacts = p.collateral.map(c => {
          const sign = c.impact > 0 ? '+' : '';
          return `${c.metric}:${sign}${c.impact.toFixed(2)}`;
        }).join(', ');
        lines.push(`     WARNING: Collateral: ${impacts}`);
      }
    }
    lines.push('');
  }

  if (secondary.length > 0) {
    lines.push('SECONDARY LEVERS (moderate impact):');
    for (const p of secondary) {
      const sign = p.sensitivity > 0 ? '+' : '';
      const action = p.effectiveSensitivity > 0 ? 'increase' : 'decrease';
      lines.push(`   * ${p.param} (${sign}${p.sensitivity.toFixed(2)}) -> ${action}`);
    }
    lines.push('');
  }

  if (avoid.length > 0) {
    lines.push('AVOID (negative or high collateral):');
    for (const p of avoid) {
      const sign = p.sensitivity > 0 ? '+' : '';
      let reason = p.effectiveSensitivity < 0 ? 'works against goal' : 'high collateral';
      if (p.collateral.length > 1) {
        const impacts = p.collateral.map(c => c.metric).join(', ');
        reason = `affects ${impacts}`;
      }
      lines.push(`   * ${p.param} (${sign}${p.sensitivity.toFixed(2)}) - ${reason}`);
    }
    lines.push('');
  }

  // Best single recommendation
  if (recommended.length > 0) {
    const best = recommended[0];
    const action = best.effectiveSensitivity > 0 ? 'Increase' : 'Decrease';
    lines.push('-'.repeat(62));
    lines.push(`RECOMMENDATION: ${action} ${best.param}`);
    if (details && details[best.param] && details[best.param][targetMetric]) {
      const d = details[best.param][targetMetric];
      lines.push(`   Expected ${targetMetric} range: ${d.metricRange.min.toFixed(3)} - ${d.metricRange.max.toFixed(3)}`);
    }
    lines.push('');
  }

  return lines.join('\n');
}

function formatMarkdown() {
  const lines = [];
  lines.push(`## Lever Recommendations: ${targetMetric}`);
  lines.push('');
  lines.push(`**Goal**: ${direction} ${targetMetric}`);
  lines.push(`**Description**: ${METRIC_DESCRIPTIONS[targetMetric]}`);
  lines.push('');

  if (recommended.length > 0) {
    lines.push('### Primary Levers');
    lines.push('');
    lines.push('| Parameter | Sensitivity | Action | Collateral |');
    lines.push('|-----------|-------------|--------|------------|');
    for (const p of recommended) {
      const sign = p.sensitivity > 0 ? '+' : '';
      const action = p.effectiveSensitivity > 0 ? 'increase' : 'decrease';
      const collateral = p.collateral.length > 0
        ? p.collateral.map(c => `${c.metric}(${c.impact > 0 ? '+' : ''}${c.impact.toFixed(2)})`).join(', ')
        : '-';
      lines.push(`| ${p.param} | ${sign}${p.sensitivity.toFixed(2)} | ${action} | ${collateral} |`);
    }
    lines.push('');
  }

  if (secondary.length > 0) {
    lines.push('### Secondary Levers');
    lines.push('');
    lines.push('| Parameter | Sensitivity | Action |');
    lines.push('|-----------|-------------|--------|');
    for (const p of secondary) {
      const sign = p.sensitivity > 0 ? '+' : '';
      const action = p.effectiveSensitivity > 0 ? 'increase' : 'decrease';
      lines.push(`| ${p.param} | ${sign}${p.sensitivity.toFixed(2)} | ${action} |`);
    }
    lines.push('');
  }

  return lines.join('\n');
}

function formatJson() {
  return JSON.stringify({
    targetMetric,
    direction,
    description: METRIC_DESCRIPTIONS[targetMetric],
    recommendations: {
      primary: recommended.map(p => ({
        param: p.param,
        sensitivity: p.sensitivity,
        action: p.effectiveSensitivity > 0 ? 'increase' : 'decrease',
        collateral: p.collateral,
      })),
      secondary: secondary.map(p => ({
        param: p.param,
        sensitivity: p.sensitivity,
        action: p.effectiveSensitivity > 0 ? 'increase' : 'decrease',
      })),
      avoid: avoid.map(p => p.param),
    },
    bestRecommendation: recommended.length > 0 ? {
      param: recommended[0].param,
      action: recommended[0].effectiveSensitivity > 0 ? 'increase' : 'decrease',
    } : null,
  }, null, 2);
}

// Output
switch (format) {
  case 'markdown':
  case 'md':
    console.log(formatMarkdown());
    break;
  case 'json':
    console.log(formatJson());
    break;
  default:
    console.log(formatText());
}
