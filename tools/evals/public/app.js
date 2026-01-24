/**
 * DuoPulse Pattern Evaluation Dashboard
 * Frontend JavaScript - fetches JSON and renders pattern visualizations
 */

import { Player, createPlayerUI } from './player.js';
import { parseTargetRange, checkInRange, computeAlignmentScore, getStatusColor, formatPercent } from './js/utils.js';
import { METRIC_KEYS, ZONES, VOICE_KEYS } from './js/constants.js';
import { renderPatternGrid, getPatternFromClick } from './js/pattern-grid.js';
import { renderPentagonRadarSVG } from './js/pentagon.js';
import { renderSensitivityHeatmap, sensitivityStyles } from './js/sensitivity.js';
import { renderTimelineChart, timelineStyles } from './js/timeline.js';

// State
let data = {
  metadata: null,
  presets: null,
  presetMetrics: null,
  sweeps: null,
  sweepMetrics: null,
  seedVariation: null,
  expressiveness: null,
  sensitivity: null,
  fills: null,
  metricsHistory: null,
  iterations: null,
};

// Iterations state
let filteredIterations = [];

let currentView = 'overview';
let player = null;
let playerUI = null;
let selectedPatternId = null;

const $ = (sel) => document.querySelector(sel);
const $$ = (sel) => document.querySelectorAll(sel);

// ============================================================================
// Data Loading
// ============================================================================

async function loadData() {
  const files = [
    'metadata.json',
    'presets.json',
    'preset-metrics.json',
    'sweeps.json',
    'sweep-metrics.json',
    'seed-variation.json',
    'expressiveness.json',
  ];

  try {
    const results = await Promise.all(
      files.map(async (file) => {
        const res = await fetch(`data/${file}`);
        if (!res.ok) throw new Error(`Failed to load ${file}`);
        return res.json();
      })
    );

    [
      data.metadata,
      data.presets,
      data.presetMetrics,
      data.sweeps,
      data.sweepMetrics,
      data.seedVariation,
      data.expressiveness,
    ] = results;

    // Try to load sensitivity data (optional - may not exist yet)
    try {
      const sensitivityRes = await fetch('data/sensitivity.json');
      if (sensitivityRes.ok) {
        data.sensitivity = await sensitivityRes.json();
      }
    } catch (e) {
      console.log('Sensitivity data not available (run "make sensitivity-matrix" to generate)');
    }

    // Try to load fills data (optional - may not exist yet)
    try {
      const fillsRes = await fetch('data/fills.json');
      if (fillsRes.ok) {
        data.fills = await fillsRes.json();
      }
    } catch (e) {
      console.log('Fills data not available (run "make evals" to generate)');
    }

    // Try to load metrics history (optional - may not exist yet)
    try {
      const historyRes = await fetch('data/metrics-history.json');
      if (historyRes.ok) {
        data.metricsHistory = await historyRes.json();
      }
    } catch (e) {
      console.log('Metrics history not available (run "make metrics-history" to generate)');
    }

    // Try to load iterations data (optional - may not exist yet)
    try {
      const iterationsRes = await fetch('data/iterations.json');
      if (iterationsRes.ok) {
        data.iterations = await iterationsRes.json();
        filteredIterations = data.iterations.iterations || [];
      }
    } catch (e) {
      console.log('Iterations data not available');
    }

    return true;
  } catch (err) {
    console.error('Error loading data:', err);
    return false;
  }
}


// ============================================================================
// Pattern Selection
// ============================================================================

function handlePatternClick(e) {
  const result = getPatternFromClick(e);
  if (!result) return;

  const { patternId, patternName } = result;
  let pattern = null;

  // Check presets
  if (patternId.startsWith('preset-')) {
    const idx = parseInt(patternId.replace('preset-', ''), 10);
    pattern = data.presets[idx]?.pattern;
  }
  // Check sweeps
  else if (patternId.startsWith('sweep-')) {
    const [, param, idx] = patternId.match(/sweep-(\w+)-(\d+)/);
    pattern = data.sweeps[param]?.[parseInt(idx, 10)];
  }

  if (pattern && player) {
    selectedPatternId = patternId;
    player.setPattern(pattern);
    playerUI?.updatePatternInfo(pattern, patternName);
    playerUI?.updateStepCount(pattern.params?.length || 16);

    // Update selection visuals
    $$('.pattern-grid.selected').forEach(g => g.classList.remove('selected'));
    e.target.closest('.pattern-grid').classList.add('selected');
  }
}

function attachPatternListeners() {
  document.removeEventListener('click', handlePatternClick);
  document.addEventListener('click', handlePatternClick);
}

// Helper to render pattern with current selection state
function renderPattern(pattern, options = {}) {
  return renderPatternGrid(pattern, { ...options, selectedId: selectedPatternId });
}

// ============================================================================
// Overview View (Pentagon Summary - matching Python layout)
// ============================================================================

function renderOverviewView() {
  const container = $('#overview-container');
  const exp = data.expressiveness;
  const stats = exp.pentagonStats;
  const defs = exp.metricDefinitions;

  // Build table rows
  let tableRows = '';
  for (const key of METRIC_KEYS) {
    const meta = defs[key];
    const cells = ZONES.map(zone => {
      const zoneData = stats[zone.key];
      if (!zoneData) return '<td style="text-align: center; color: #444;">--</td>';

      const value = zoneData[key];
      const targetStr = meta.targetByZone[zone.key];
      const inRange = checkInRange(value, targetStr);
      const alignment = computeAlignmentScore(value, targetStr);
      const color = getStatusColor(inRange, alignment);

      return `
        <td style="text-align: center; padding: 10px 12px;">
          <span style="color: ${color}; font-weight: 600; font-size: 14px;">${value.toFixed(2)}</span><br>
          <span style="color: #555; font-size: 10px;">target: ${targetStr}</span>
        </td>
      `;
    }).join('');

    tableRows += `
      <tr style="border-bottom: 1px solid #2a2a2a;">
        <td style="padding: 10px 12px; max-width: 300px;">
          <span style="color: #ff6b35; font-weight: 500;">${meta.name}</span><br>
          <span style="color: #666; font-size: 10px;">${meta.description}</span>
        </td>
        ${cells}
      </tr>
    `;
  }

  // Zone compliance row
  const complianceCells = ZONES.map(zone => {
    const zoneData = stats[zone.key];
    if (!zoneData) return '<td style="text-align: center; color: #444;">--</td>';

    return `
      <td style="text-align: center; padding: 10px 12px;">
        <span style="color: ${zone.color}; font-weight: 600; font-size: 14px;">${Math.round(zoneData.composite * 100)}%</span><br>
        <span style="color: #555; font-size: 10px;">n=${zoneData.count}</span>
      </td>
    `;
  }).join('');

  // Alignment status color
  const alignmentColor = exp.alignmentStatus === 'GOOD' ? '#44ff44'
    : exp.alignmentStatus === 'FAIR' ? '#ffaa44' : '#ff4444';

  // Inject timeline styles if not already present
  if (!document.getElementById('timeline-styles')) {
    const styleEl = document.createElement('style');
    styleEl.id = 'timeline-styles';
    styleEl.textContent = timelineStyles;
    document.head.appendChild(styleEl);
  }

  // Render metrics timeline if available
  const timelineHtml = data.metricsHistory
    ? `<div class="metrics-timeline-section">
        ${renderTimelineChart(data.metricsHistory, 1000, 300)}
        <div style="text-align: center; margin-top: 1rem;">
          <a href="iterations/index.html" style="color: var(--accent-color, #64b5f6); text-decoration: none; font-size: 0.95rem;">
            View All Iterations →
          </a>
        </div>
      </div>`
    : '';

  container.innerHTML = `
    ${timelineHtml}

    <div class="pentagon-overview">
      <h3>Pentagon of Musicality -- Overview</h3>

      <div class="pentagon-flex">
        <div class="pentagon-chart">
          <div class="chart-label">ALL PATTERNS AVERAGE</div>
          ${renderPentagonRadarSVG(stats, defs, 380)}
        </div>

        <div class="pentagon-table-wrapper">
          <table class="pentagon-table">
            <thead>
              <tr style="border-bottom: 1px solid #333;">
                <th style="text-align: left; padding: 8px 12px; color: #888;">Metric</th>
                ${ZONES.map(z => `
                  <th style="text-align: center; padding: 8px 12px;">
                    <span style="color: ${z.color}; font-weight: 600;">${z.label}</span><br>
                    <span style="color: #555; font-size: 10px;">${z.range}</span>
                  </th>
                `).join('')}
              </tr>
            </thead>
            <tbody>
              ${tableRows}
              <tr style="border-top: 2px solid #333;">
                <td style="padding: 10px 12px; color: #fff; font-weight: 600;">Zone Compliance</td>
                ${complianceCells}
              </tr>
            </tbody>
          </table>
        </div>
      </div>

      <div class="alignment-section">
        <div class="alignment-label">
          <div class="label-title">OVERALL ALIGNMENT SCORE</div>
          <div class="label-desc">Combined metric (60% pentagon + 40% conformance)</div>
        </div>
        <div class="alignment-value">
          <span style="color: ${alignmentColor}; font-size: 48px; font-weight: 700;">${(exp.overallAlignment * 100).toFixed(1)}%</span>
          <span style="color: ${alignmentColor}; font-size: 18px; margin-left: 12px;">[${exp.alignmentStatus}]</span>
        </div>
        <div class="alignment-count">${exp.totalPatterns} patterns analyzed</div>
      </div>

      ${exp.pentagonScore !== undefined && exp.conformance ? `
      <div class="alignment-breakdown">
        <div class="breakdown-item">
          <span class="breakdown-label">Pentagon Score</span>
          <span class="breakdown-value" style="color: ${exp.pentagonScore >= 0.7 ? 'var(--success)' : exp.pentagonScore >= 0.4 ? 'var(--warning)' : 'var(--error)'};">${(exp.pentagonScore * 100).toFixed(1)}%</span>
          <span class="breakdown-weight">(60%)</span>
        </div>
        <div class="breakdown-item">
          <span class="breakdown-label">Conformance Score</span>
          <span class="breakdown-value" style="color: ${exp.conformance.score >= 0.7 ? 'var(--success)' : exp.conformance.score >= 0.4 ? 'var(--warning)' : 'var(--error)'};">${(exp.conformance.score * 100).toFixed(1)}%</span>
          <span class="breakdown-weight">(40%)</span>
        </div>
      </div>
      ` : ''}
    </div>

    ${exp.conformance && exp.conformance.presetBreakdown ? `
    <div class="conformance-section">
      <h3>Preset Conformance</h3>
      <div class="conformance-overview">
        Overall: <span style="color: ${exp.conformance.score >= 0.7 ? 'var(--success)' : exp.conformance.score >= 0.4 ? 'var(--warning)' : 'var(--error)'}; font-weight: 700;">${(exp.conformance.score * 100).toFixed(1)}%</span>
        <span style="color: var(--text-dim); margin-left: 12px;">(${exp.conformance.passCount}/${exp.conformance.presetBreakdown.length} presets pass)</span>
      </div>
      <div class="conformance-bars">
        ${exp.conformance.presetBreakdown.map(p => {
          const barColor = p.pass ? 'var(--success)' : p.score >= 0.5 ? 'var(--warning)' : 'var(--error)';
          const statusText = p.pass ? 'PASS' : p.score >= 0.5 ? 'CLOSE' : 'FAIL';
          const statusClass = p.pass ? 'pass' : p.score >= 0.5 ? 'warn' : 'fail';
          return `
            <div class="conformance-bar-item">
              <span class="conformance-preset-label">${p.name}</span>
              <div class="conformance-bar-track">
                <div class="conformance-bar-fill" style="width: ${(p.score * 100).toFixed(0)}%; background: ${barColor};"></div>
              </div>
              <span class="conformance-score">${(p.score * 100).toFixed(0)}%</span>
              <span class="conformance-status ${statusClass}">${statusText}</span>
            </div>
          `;
        }).join('')}
      </div>
      <div class="conformance-rating ${exp.conformance.score >= 0.8 ? 'excellent' : exp.conformance.score >= 0.6 ? 'good' : exp.conformance.score >= 0.4 ? 'fair' : 'poor'}">
        ${exp.conformance.score >= 0.8 ? 'EXCELLENT' : exp.conformance.score >= 0.6 ? 'GOOD' : exp.conformance.score >= 0.4 ? 'FAIR' : 'POOR'} - ${exp.conformance.score >= 0.8 ? 'All presets performing well' : exp.conformance.score >= 0.6 ? 'Most presets performing adequately' : exp.conformance.score >= 0.4 ? 'Some presets need attention' : 'Significant conformance issues'}
      </div>
    </div>
    ` : ''}

    ${data.seedVariation ? `
    <div class="seed-summary-section">
      <h3>Seed Variation</h3>
      <div class="seed-bars">
        ${VOICE_KEYS.map(v => {
          const score = data.seedVariation.summary[v].avgScore;
          return `
            <div class="seed-bar-item">
              <span class="seed-label" style="color: var(--${v}-color)">${v.toUpperCase()}</span>
              <div class="seed-bar">
                <div class="seed-bar-fill" style="width: ${formatPercent(score)}; background: var(--${v}-color);"></div>
              </div>
              <span class="seed-value">${formatPercent(score)}</span>
            </div>
          `;
        }).join('')}
      </div>
      <div class="seed-status ${data.seedVariation.summary.pass ? 'pass' : 'fail'}">
        ${data.seedVariation.summary.pass ? 'PASS' : 'FAIL'} - Overall: ${formatPercent(data.seedVariation.summary.overall)}
      </div>
    </div>
    ` : ''}
  `;
}

// ============================================================================
// Presets View
// ============================================================================

function renderPresetsView() {
  const container = $('#presets-container');
  container.innerHTML = '';

  data.presetMetrics.forEach((preset, i) => {
    const patternData = data.presets[i].pattern;
    const patternId = `preset-${i}`;

    const paramsHtml = Object.entries(preset.params)
      .filter(([k]) => !['seed', 'length'].includes(k))
      .map(([k, v]) => `<span class="param"><span class="param-name">${k.toUpperCase()}:</span> <span class="param-value">${v.toFixed(2)}</span></span>`)
      .join('');

    // Build conformance section
    const conformance = preset.conformance || {};
    const score = conformance.score !== undefined ? conformance.score : 0.5;
    const scorePercent = Math.round(score * 100);
    const status = conformance.status || 'unknown';
    const tolerance = conformance.tolerance || 'moderate';
    const pass = conformance.pass;
    const breakdown = conformance.breakdown || {};

    const passClass = pass ? 'pass' : 'fail';
    const statusClass = status;

    // Build breakdown items
    const breakdownHtml = Object.entries(breakdown)
      .map(([metric, metricScore]) => {
        const metricPercent = Math.round(metricScore * 100);
        const metricClass = metricScore >= 0.7 ? 'pass' : metricScore >= 0.4 ? 'warn' : 'fail';
        return `
          <div class="conformance-breakdown-item">
            <span class="conformance-metric-name">${metric}</span>
            <span class="conformance-metric-score ${metricClass}">${metricPercent}%</span>
          </div>
        `;
      })
      .join('');

    const conformanceHtml = `
      <div class="preset-conformance">
        <div class="conformance-header">
          <span class="conformance-label">Conformance:</span>
          <span class="conformance-score ${passClass}">${scorePercent}%</span>
          <span class="conformance-status-badge ${statusClass}">${status}</span>
          <span class="conformance-tolerance">(${tolerance})</span>
        </div>
        ${breakdownHtml ? `<div class="conformance-breakdown">${breakdownHtml}</div>` : ''}
      </div>
    `;

    const card = document.createElement('div');
    card.className = 'preset-card';
    card.innerHTML = `
      <h3>${preset.name}</h3>
      <p class="preset-desc">${preset.description}</p>
      <div class="preset-params">${paramsHtml}</div>
      ${renderPattern(patternData, { patternId, name: preset.name })}
      ${conformanceHtml}
    `;

    container.appendChild(card);
  });

  attachPatternListeners();
}

// ============================================================================
// Sweeps View
// ============================================================================

function renderSweepsView() {
  const container = $('#sweep-container');
  const select = $('#sweep-select');
  const paramName = select.value;

  const patterns = data.sweeps[paramName];
  const metrics = data.sweepMetrics[paramName];

  container.innerHTML = patterns.map((pattern, i) => {
    const metric = metrics[i];
    const params = pattern.params;
    const patternId = `sweep-${paramName}-${i}`;
    const name = `${paramName.toUpperCase()} ${formatPercent(params[paramName])}`;
    const zone = metric.pentagon.zone;
    const zoneColor = ZONES.find(z => z.key === zone)?.color || '#888';

    // Display all parameters, highlight the swept one
    const paramDisplay = Object.keys(params)
      .filter(k => k !== 'seed' && k !== 'length') // Skip seed and length
      .map(key => {
        const value = params[key].toFixed(2);
        const isSwept = key === paramName;
        const style = isSwept ? 'color: var(--warning); font-weight: 700;' : 'color: var(--text-dim);';
        return `<span style="${style}">${key.toUpperCase()}: ${value}</span>`;
      })
      .join(' · ');

    return `
      <div class="sweep-item">
        <div class="sweep-params" style="font-size: 11px; margin-bottom: 8px;">${paramDisplay}</div>
        ${renderPattern(pattern, { compact: true, patternId, name })}
        <div style="font-size: 11px; color: var(--text-secondary); margin-top: 8px;">
          Zone: <span style="color: ${zoneColor}">${zone}</span>
          | Score: <span style="color: ${metric.pentagon.composite > 0.5 ? 'var(--success)' : 'var(--error)'}">${formatPercent(metric.pentagon.composite)}</span>
        </div>
      </div>
    `;
  }).join('');

  attachPatternListeners();
}

// ============================================================================
// Seeds View
// ============================================================================

function renderSeedsView() {
  const summary = data.seedVariation.summary;
  const tests = data.seedVariation.tests;

  const summaryHtml = `
    <div class="summary-grid">
      ${VOICE_KEYS.map(v => {
        const s = summary[v];
        return `
          <div class="summary-item">
            <div class="label">${v.toUpperCase()} Variation</div>
            <div class="value ${v}">${formatPercent(s.avgScore)}</div>
            <div class="variation-bar-container">
              <div class="variation-bar">
                <div class="variation-bar-fill ${s.avgScore >= 0.5 ? 'pass' : 'fail'}" style="width: ${formatPercent(s.avgScore)}"></div>
              </div>
            </div>
          </div>
        `;
      }).join('')}
      <div class="summary-item">
        <div class="label">Overall</div>
        <div class="value" style="color: ${summary.pass ? 'var(--success)' : 'var(--error)'}">${formatPercent(summary.overall)}</div>
      </div>
    </div>
    <div class="summary-status ${summary.pass ? 'pass' : 'fail'}">
      ${summary.pass ? 'PASS - Seed variation is adequate' : 'FAIL - V2 (Shimmer) variation is too low'}
    </div>
  `;

  $('#seeds-summary').innerHTML = summaryHtml;

  const lowVariation = tests.filter(t => t.variation.v2.score < 0.5);

  $('#seeds-container').innerHTML = `
    <h3 style="margin-bottom: 12px;">Low Variation Zones (${lowVariation.length} found)</h3>
    <div class="seed-test-grid">
      ${lowVariation.slice(0, 20).map(t => `
        <div class="seed-test-item">
          <div class="params">E=${t.params.energy.toFixed(2)} S=${t.params.shape.toFixed(2)} D=${t.params.drift.toFixed(2)}</div>
          <div class="scores">
            ${VOICE_KEYS.map(v => {
              const score = t.variation[v].score;
              return `<div class="score"><span style="color: var(--${v}-color)">${v.toUpperCase()}:</span> <span style="color: ${score >= 0.5 ? 'var(--success)' : 'var(--error)'}">${formatPercent(score)}</span></div>`;
            }).join('')}
          </div>
        </div>
      `).join('')}
    </div>
    ${lowVariation.length > 20 ? `<p style="margin-top: 12px; color: var(--text-secondary);">... and ${lowVariation.length - 20} more</p>` : ''}
  `;
}

// ============================================================================
// Sensitivity View
// ============================================================================

function renderSensitivityView() {
  const container = $('#sensitivity-container');

  // Inject styles if not already present
  if (!document.getElementById('sensitivity-styles')) {
    const styleEl = document.createElement('style');
    styleEl.id = 'sensitivity-styles';
    styleEl.textContent = sensitivityStyles;
    document.head.appendChild(styleEl);
  }

  // Render heatmap with clickable cells
  const dummyCallback = () => {}; // Pass callback to make cells clickable
  container.innerHTML = renderSensitivityHeatmap(data.sensitivity, dummyCallback);

  // Attach click handlers via event delegation
  container.querySelectorAll('.sensitivity-cell.clickable').forEach(cell => {
    cell.addEventListener('click', (e) => {
      const param = e.target.dataset.param;
      const metric = e.target.dataset.metric;
      alert(`Sweep curve for ${param} → ${metric}\n\n(Feature coming soon: Interactive sweep visualization)`);
    });
  });
}

// ============================================================================
// Fills View
// ============================================================================

/**
 * Compute fill metrics from fill patterns
 * @param {Array} fillPatterns - Array of fill patterns at different progress points
 * @returns {Object} Computed metrics
 */
function computeFillMetrics(fillPatterns) {
  if (!fillPatterns || fillPatterns.length < 2) {
    return { densityRamp: 0, velocityBuild: 0, accentPlacement: 0, composite: 0 };
  }

  // Density Ramp: Check if density increases with progress
  const densities = fillPatterns.map(fp => fp.hitCounts.total);
  let densityIncreases = 0;
  for (let i = 1; i < densities.length; i++) {
    if (densities[i] >= densities[i - 1]) densityIncreases++;
  }
  const densityRamp = densityIncreases / (densities.length - 1);

  // Velocity Build: Check if average velocity increases with progress
  const avgVelocities = fillPatterns.map(fp => {
    let totalVel = 0;
    let count = 0;
    for (const step of fp.fillSteps) {
      if (step.anchorVel) { totalVel += step.anchorVel; count++; }
      if (step.shimmerVel) { totalVel += step.shimmerVel; count++; }
      if (step.auxVel) { totalVel += step.auxVel; count++; }
    }
    return count > 0 ? totalVel / count : 0;
  });

  let velocityIncreases = 0;
  for (let i = 1; i < avgVelocities.length; i++) {
    if (avgVelocities[i] >= avgVelocities[i - 1]) velocityIncreases++;
  }
  const velocityBuild = velocityIncreases / (avgVelocities.length - 1);

  // Accent Placement: Check if final progress has highest velocities
  const lastPattern = fillPatterns[fillPatterns.length - 1];
  const hasHighVelocity = lastPattern.fillSteps.some(s =>
    (s.anchorVel && s.anchorVel > 0.9) ||
    (s.shimmerVel && s.shimmerVel > 0.9) ||
    (s.auxVel && s.auxVel > 0.9)
  );
  const accentPlacement = hasHighVelocity ? 1.0 : 0.5;

  // Composite score
  const composite = (densityRamp + velocityBuild + accentPlacement) / 3;

  return { densityRamp, velocityBuild, accentPlacement, composite };
}

/**
 * Render a fill pattern grid (32 steps with progress labels)
 * @param {Array} fillPatterns - Array of fill patterns at different progress points
 * @param {number} energy - Energy level for display
 * @returns {string} HTML string
 */
function renderFillPatternGrid(fillPatterns, energy) {
  const progressLabels = [0.25, 0.5, 0.75, 1.0];

  // Create a 32-step grid (fill patterns can be longer)
  const numSteps = 32;

  // Build fill state indicator (shows fill is active across entire pattern)
  const fillStateIndicator = `
    <div class="fill-row fill-state-row">
      <div class="fill-progress-label">Fill State</div>
      <div class="fill-steps">
        ${Array.from({ length: numSteps }, (_, i) =>
          `<div class="fill-state-indicator active" title="Fill active"></div>`
        ).join('')}
      </div>
      <div class="fill-hit-count"></div>
    </div>
  `;

  // Build rows for each progress point
  const rows = fillPatterns.map((fp, idx) => {
    const progress = progressLabels[idx] || fp.params.fillProgress;

    // Create step cells
    const stepCells = [];
    for (let s = 0; s < numSteps; s++) {
      const stepData = fp.fillSteps.find(fs => fs.step === s);
      const isDownbeat = s % 4 === 0;

      if (stepData) {
        // Determine which voices hit on this step
        const voices = [];
        if (stepData.anchor) voices.push({ key: 'anchor', vel: stepData.anchorVel, color: 'var(--v1-color)' });
        if (stepData.shimmer) voices.push({ key: 'shimmer', vel: stepData.shimmerVel, color: 'var(--v2-color)' });
        if (stepData.aux) voices.push({ key: 'aux', vel: stepData.auxVel, color: 'var(--aux-color)' });

        // Use primary voice color, or multi-voice indicator
        const primaryVoice = voices[0];
        const opacity = primaryVoice ? (0.4 + primaryVoice.vel * 0.6).toFixed(2) : 1;
        const bgColor = voices.length > 1 ? '#fff' : (primaryVoice?.color || '#888');
        const tooltip = voices.map(v => `${v.key}: ${(v.vel * 100).toFixed(0)}%`).join(', ');

        stepCells.push(`
          <div class="fill-step hit ${isDownbeat ? 'downbeat' : ''}"
               style="background: ${bgColor}; opacity: ${opacity};"
               title="${tooltip}">
            ${voices.length > 1 ? '<span class="multi-hit"></span>' : ''}
          </div>
        `);
      } else {
        stepCells.push(`
          <div class="fill-step empty ${isDownbeat ? 'downbeat' : ''}" style="background: #333;"></div>
        `);
      }
    }

    const hitCount = fp.hitCounts.total;
    return `
      <div class="fill-row">
        <div class="fill-progress-label">${(progress * 100).toFixed(0)}%</div>
        <div class="fill-steps">${stepCells.join('')}</div>
        <div class="fill-hit-count">${hitCount} hits</div>
      </div>
    `;
  }).join('');

  return `
    <div class="fill-pattern-grid">
      <div class="fill-grid-header">
        <span class="fill-energy-label">ENERGY = ${energy.toFixed(2)}</span>
      </div>
      ${fillStateIndicator}
      ${rows}
      <div class="fill-step-numbers">
        <div class="fill-progress-label"></div>
        <div class="fill-steps">
          ${Array.from({ length: numSteps }, (_, i) =>
            i % 4 === 0 ? `<div class="fill-step-num">${i}</div>` : '<div class="fill-step-num"></div>'
          ).join('')}
        </div>
        <div class="fill-hit-count"></div>
      </div>
    </div>
  `;
}

/**
 * Render fill metrics as bar graphs
 * @param {Object} metrics - Fill metrics object
 * @returns {string} HTML string
 */
function renderFillMetricsDisplay(metrics) {
  const items = [
    { key: 'densityRamp', label: 'Density Ramp', desc: 'Hits increase with progress' },
    { key: 'velocityBuild', label: 'Velocity Build', desc: 'Velocity increases with progress' },
    { key: 'accentPlacement', label: 'Accent Placement', desc: 'Strong accents at climax' },
    { key: 'composite', label: 'Composite Score', desc: 'Overall fill quality' },
  ];

  const bars = items.map(item => {
    const value = metrics[item.key];
    const percent = (value * 100).toFixed(0);
    const barColor = value >= 0.7 ? 'var(--success)' : value >= 0.4 ? 'var(--warning)' : 'var(--error)';

    return `
      <div class="fill-metric-item">
        <div class="fill-metric-header">
          <span class="fill-metric-label">${item.label}</span>
          <span class="fill-metric-value" style="color: ${barColor}">${percent}%</span>
        </div>
        <div class="fill-metric-desc">${item.desc}</div>
        <div class="fill-metric-bar">
          <div class="fill-metric-bar-fill" style="width: ${percent}%; background: ${barColor};"></div>
        </div>
      </div>
    `;
  }).join('');

  return `<div class="fill-metrics">${bars}</div>`;
}

function renderFillsView() {
  const metricsContainer = $('#fill-metrics-container');
  const gridContainer = $('#fill-grid-container');
  const select = $('#fill-energy-select');

  if (!data.fills || !data.fills.energySweep) {
    metricsContainer.innerHTML = '<p class="no-data">Fill data not available. Run <code>make evals</code> to generate.</p>';
    gridContainer.innerHTML = '';
    return;
  }

  const energySweep = data.fills.energySweep;

  // Populate energy select if empty
  if (select.options.length === 0) {
    energySweep.forEach((entry, i) => {
      const opt = document.createElement('option');
      opt.value = i;
      opt.textContent = `ENERGY = ${entry.energy.toFixed(2)}`;
      select.appendChild(opt);
    });

    select.addEventListener('change', () => renderFillsView());
  }

  const selectedIdx = parseInt(select.value, 10) || 0;
  const selectedEntry = energySweep[selectedIdx];
  const fillPatterns = selectedEntry.fillPatterns;

  // Extract params from first fill pattern (all have same base params except fillProgress)
  const baseParams = fillPatterns[0]?.params || {};

  // Compute metrics for current energy level
  const metrics = computeFillMetrics(fillPatterns);

  // Compute aggregate metrics across all energy levels
  const allMetrics = energySweep.map(e => computeFillMetrics(e.fillPatterns));
  const avgMetrics = {
    densityRamp: allMetrics.reduce((s, m) => s + m.densityRamp, 0) / allMetrics.length,
    velocityBuild: allMetrics.reduce((s, m) => s + m.velocityBuild, 0) / allMetrics.length,
    accentPlacement: allMetrics.reduce((s, m) => s + m.accentPlacement, 0) / allMetrics.length,
    composite: allMetrics.reduce((s, m) => s + m.composite, 0) / allMetrics.length,
  };

  // Render aggregate metrics
  metricsContainer.innerHTML = `
    <div class="fill-metrics-section">
      <h3>Fill System Metrics (All Energy Levels)</h3>
      ${renderFillMetricsDisplay(avgMetrics)}
    </div>
  `;

  // Render fill configuration and explanation
  const fillConfigHtml = `
    <div class="fill-config-section">
      <h3>Fill Pattern Configuration</h3>
      <p class="fill-description">
        Fill patterns are transitional sequences that build tension before returning to the main groove.
        The system generates 4 variations per energy level (25%, 50%, 75%, 100% progress),
        progressively adding density, velocity, and accents as fillProgress increases.
      </p>
      <div class="fill-params">
        <div class="param-row">
          <span class="param-label">Base Parameters:</span>
          <span class="param-values">
            SHAPE: ${baseParams.shape?.toFixed(2) || '--'}
            | AXIS-X: ${baseParams.axisX?.toFixed(2) || '--'}
            | AXIS-Y: ${baseParams.axisY?.toFixed(2) || '--'}
            | DRIFT: ${baseParams.drift?.toFixed(2) || '--'}
            | ACCENT: ${baseParams.accent?.toFixed(2) || '--'}
          </span>
        </div>
        <div class="param-row">
          <span class="param-label">Energy Sweep:</span>
          <span class="param-values">
            Sweeping ENERGY from 0.0 to 1.0 (currently viewing: ${selectedEntry.energy.toFixed(2)})
          </span>
        </div>
      </div>
      <p class="fill-note">
        <strong>Note:</strong> Fill state is indicated by a horizontal line above/below patterns in other views.
        High line = fill active, low line = normal pattern.
      </p>
    </div>
  `;

  // Render fill pattern grid for selected energy
  gridContainer.innerHTML = fillConfigHtml + renderFillPatternGrid(fillPatterns, selectedEntry.energy);
}

// ============================================================================
// Iterations View
// ============================================================================

function renderIterationsView() {
  // Render summary stats
  const summary = data.iterations?.summary || { total: 0, successful: 0, failed: 0, successRate: 0 };
  $('#stat-total').textContent = summary.total;
  $('#stat-success').textContent = summary.successful;
  $('#stat-failed').textContent = summary.failed;
  $('#stat-rate').textContent = `${(summary.successRate * 100).toFixed(1)}%`;

  // Render timeline
  renderIterationsTimeline();

  // Render iteration cards
  renderIterationCards();

  // Setup event listeners
  setupIterationsEventListeners();
}

function renderIterationsTimeline() {
  const container = $('#iterations-timeline-chart');

  if (!data.metricsHistory || !data.metricsHistory.timeline || data.metricsHistory.timeline.length < 2) {
    container.innerHTML = `
      <div class="no-timeline-data">
        <p>Not enough historical data to display timeline</p>
        <p class="hint">Run evaluations and tag baselines to track progress over time</p>
      </div>
    `;
    return;
  }

  container.innerHTML = renderTimelineChart(data.metricsHistory, 900, 280);
}

function renderIterationCards() {
  const container = $('#iterations-list');

  if (!filteredIterations || filteredIterations.length === 0) {
    container.innerHTML = `
      <div class="no-iterations">
        <p>No iterations found.</p>
        <p class="hint">Run <code>/iterate "goal"</code> to start hill-climbing.</p>
      </div>
    `;
    return;
  }

  const cards = filteredIterations.map(createIterationCard).join('');
  container.innerHTML = cards;

  // Add event listeners to cards
  $$('.card-header').forEach(header => {
    header.addEventListener('click', toggleIterationCardDetails);
  });

  $$('.toggle-details').forEach(btn => {
    btn.addEventListener('click', (e) => {
      e.stopPropagation();
      toggleIterationCardDetails.call(btn.closest('.card-header'), e);
    });
  });
}

function createIterationCard(iteration) {
  const statusClass = iteration.status || 'unknown';
  const statusLabel = iteration.status || 'Unknown';

  // Lever info
  const leverInfo = iteration.lever ? `
    <div class="card-lever">
      ${iteration.lever.primary}: ${iteration.lever.oldValue !== null ? iteration.lever.oldValue.toFixed(2) : '?'}
      → ${iteration.lever.newValue !== null ? iteration.lever.newValue.toFixed(2) : '?'}
      ${iteration.lever.delta ? `(${iteration.lever.delta})` : ''}
    </div>
  ` : '';

  // Metrics info
  const metricsInfo = iteration.metrics ? `
    <div class="card-metrics">
      ${iteration.metrics.target ? `
        <div class="metric-item">
          <div class="metric-label">Target Metric</div>
          <div class="metric-value ${iteration.metrics.target.delta?.startsWith('+') ? 'positive' : (iteration.metrics.target.delta?.startsWith('-') ? 'negative' : '')}">
            ${iteration.metrics.target.delta}
          </div>
        </div>
      ` : ''}
      ${iteration.metrics.maxRegression ? `
        <div class="metric-item">
          <div class="metric-label">Max Regression</div>
          <div class="metric-value ${iteration.metrics.maxRegression.startsWith('-') ? 'negative' : 'positive'}">
            ${iteration.metrics.maxRegression}
          </div>
        </div>
      ` : ''}
    </div>
  ` : '';

  // Links
  const links = [];
  if (iteration.pr && iteration.pr !== 'null') {
    links.push(`<a href="https://github.com/chronick/duopulse/pull/${iteration.pr.replace('#', '')}" class="card-link" target="_blank">View PR ${iteration.pr}</a>`);
  }
  if (iteration.commit && iteration.commit !== 'null') {
    links.push(`<a href="https://github.com/chronick/duopulse/commit/${iteration.commit}" class="card-link" target="_blank"><code>${iteration.commit.substring(0, 7)}</code></a>`);
  }
  if (iteration.branch && iteration.branch !== 'null') {
    links.push(`<span class="card-link"><code>${iteration.branch}</code></span>`);
  }

  return `
    <div class="iteration-card ${statusClass}" id="${iteration.id}">
      <div class="card-header">
        <div class="card-title-row">
          <div class="card-id">#${iteration.id}</div>
          <div class="card-status ${statusClass}">${statusLabel}</div>
        </div>
        <h3 class="card-goal">${escapeHTML(iteration.goal)}</h3>
        ${leverInfo}
        ${metricsInfo}
        ${links.length > 0 ? `<div class="card-links">${links.join('')}</div>` : ''}
      </div>

      <div class="card-details" id="details-${iteration.id}">
        ${createIterationCardDetails(iteration)}
      </div>

      <button class="toggle-details">
        <span class="toggle-text">Show Details</span>
        <span class="toggle-icon">▼</span>
      </button>
    </div>
  `;
}

function createIterationCardDetails(iteration) {
  const sections = [];

  if (iteration.sections?.goal) {
    sections.push(`
      <div class="card-details-section">
        <h4>Goal</h4>
        <pre>${escapeHTML(iteration.sections.goal)}</pre>
      </div>
    `);
  }

  if (iteration.sections?.decision) {
    sections.push(`
      <div class="card-details-section">
        <h4>Decision</h4>
        <pre>${escapeHTML(iteration.sections.decision)}</pre>
      </div>
    `);
  }

  if (iteration.sections?.notes) {
    sections.push(`
      <div class="card-details-section">
        <h4>Notes</h4>
        <pre>${escapeHTML(iteration.sections.notes)}</pre>
      </div>
    `);
  }

  if (sections.length === 0) {
    return '<p>No additional details available</p>';
  }

  return sections.join('');
}

function toggleIterationCardDetails(e) {
  const card = e.currentTarget.closest('.iteration-card');
  const details = card.querySelector('.card-details');
  const toggleBtn = card.querySelector('.toggle-details');
  const toggleText = toggleBtn.querySelector('.toggle-text');
  const toggleIcon = toggleBtn.querySelector('.toggle-icon');

  if (details.classList.contains('expanded')) {
    details.classList.remove('expanded');
    toggleText.textContent = 'Show Details';
    toggleIcon.classList.remove('expanded');
  } else {
    details.classList.add('expanded');
    toggleText.textContent = 'Hide Details';
    toggleIcon.classList.add('expanded');
  }
}

function setupIterationsEventListeners() {
  const searchInput = $('#search-iterations');
  const filterStatus = $('#filter-status');
  const sortSelect = $('#sort-iterations');

  // Remove existing listeners by replacing elements (simple approach)
  if (searchInput && !searchInput.dataset.listenerAttached) {
    searchInput.addEventListener('input', applyIterationsFilters);
    searchInput.dataset.listenerAttached = 'true';
  }

  if (filterStatus && !filterStatus.dataset.listenerAttached) {
    filterStatus.addEventListener('change', applyIterationsFilters);
    filterStatus.dataset.listenerAttached = 'true';
  }

  if (sortSelect && !sortSelect.dataset.listenerAttached) {
    sortSelect.addEventListener('change', applyIterationsSort);
    sortSelect.dataset.listenerAttached = 'true';
  }
}

function applyIterationsFilters() {
  const searchTerm = ($('#search-iterations')?.value || '').toLowerCase();
  const statusFilter = $('#filter-status')?.value || 'all';

  filteredIterations = (data.iterations?.iterations || []).filter(iteration => {
    // Status filter
    if (statusFilter !== 'all' && iteration.status !== statusFilter) {
      return false;
    }

    // Search filter
    if (searchTerm) {
      const searchableText = [
        iteration.id,
        iteration.goal,
        iteration.lever?.primary,
        iteration.sections?.goal,
        iteration.sections?.decision,
        iteration.sections?.notes
      ].filter(Boolean).join(' ').toLowerCase();

      if (!searchableText.includes(searchTerm)) {
        return false;
      }
    }

    return true;
  });

  applyIterationsSort();
}

function applyIterationsSort() {
  const sortBy = $('#sort-iterations')?.value || 'newest';

  switch (sortBy) {
    case 'oldest':
      filteredIterations.sort((a, b) => a.id.localeCompare(b.id));
      break;
    case 'impact':
      filteredIterations.sort((a, b) => {
        const deltaA = parseFloat(a.metrics?.target?.delta?.replace('%', '') || '0');
        const deltaB = parseFloat(b.metrics?.target?.delta?.replace('%', '') || '0');
        return deltaB - deltaA;
      });
      break;
    case 'newest':
    default:
      filteredIterations.sort((a, b) => b.id.localeCompare(a.id));
      break;
  }

  renderIterationCards();
}

function escapeHTML(text) {
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

// ============================================================================
// Navigation
// ============================================================================

function showView(viewName) {
  $$('.view').forEach(v => v.hidden = true);

  const view = $(`#view-${viewName}`);
  if (view) {
    view.hidden = false;
    currentView = viewName;

    switch (viewName) {
      case 'overview': renderOverviewView(); break;
      case 'presets': renderPresetsView(); break;
      case 'sweeps': renderSweepsView(); break;
      case 'seeds': renderSeedsView(); break;
      case 'fills': renderFillsView(); break;
      case 'iterations': renderIterationsView(); break;
    }
  }

  // Show player sidebar only on pages with clickable patterns
  const playerSidebar = $('.player-sidebar');
  const showPlayer = viewName === 'presets' || viewName === 'sweeps';
  if (playerSidebar) {
    playerSidebar.style.display = showPlayer ? '' : 'none';
  }

  $$('.nav-btn').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.view === viewName);
  });

  // Update URL hash without triggering hashchange
  const hash = viewName === 'overview' ? '' : `#${viewName}`;
  if (window.location.hash !== hash) {
    history.replaceState(null, '', hash || window.location.pathname);
  }
}

function getViewFromHash() {
  const hash = window.location.hash.slice(1); // Remove the '#'
  const validViews = ['overview', 'presets', 'sweeps', 'seeds', 'fills', 'iterations'];
  return validViews.includes(hash) ? hash : 'overview';
}

function handleRouteChange() {
  const viewName = getViewFromHash();
  showView(viewName);
}

// ============================================================================
// Player Initialization
// ============================================================================

async function initPlayer() {
  player = new Player();
  await player.init();

  const container = $('#player-container');
  playerUI = createPlayerUI(player, container);

  // Keyboard shortcuts
  document.addEventListener('keydown', (e) => {
    // Ignore if typing in input
    if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT') return;

    switch (e.code) {
      case 'Space':
        e.preventDefault();
        player.toggle();
        break;
      case 'Escape':
        player.stop();
        break;
      case 'Digit1':
        player.toggleMute('v1');
        document.querySelector('#mute-v1')?.classList.toggle('muted', player.getMute('v1'));
        document.querySelector('#mute-v1 .mute-state').textContent = player.getMute('v1') ? 'OFF' : 'ON';
        break;
      case 'Digit2':
        player.toggleMute('v2');
        document.querySelector('#mute-v2')?.classList.toggle('muted', player.getMute('v2'));
        document.querySelector('#mute-v2 .mute-state').textContent = player.getMute('v2') ? 'OFF' : 'ON';
        break;
      case 'Digit3':
        player.toggleMute('aux');
        document.querySelector('#mute-aux')?.classList.toggle('muted', player.getMute('aux'));
        document.querySelector('#mute-aux .mute-state').textContent = player.getMute('aux') ? 'OFF' : 'ON';
        break;
    }
  });
}

// ============================================================================
// Initialization
// ============================================================================

async function init() {
  // Navigation buttons update hash
  $$('.nav-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const viewName = btn.dataset.view;
      const hash = viewName === 'overview' ? '' : `#${viewName}`;
      window.location.hash = hash;
    });
  });

  // Listen for hash changes (back/forward navigation)
  window.addEventListener('hashchange', handleRouteChange);

  $('#sweep-select').addEventListener('change', () => {
    if (currentView === 'sweeps') renderSweepsView();
  });

  const success = await loadData();
  $('#loading').hidden = true;

  if (!success) {
    $('#error').hidden = false;
    $('#error-message').textContent = 'Failed to load pattern data.';
    return;
  }

  if (data.metadata) {
    $('#gen-time').textContent = new Date(data.metadata.generated).toLocaleString();
    const totalPatterns = data.seedVariation?.tests?.length * 8 || 0;
    $('#num-patterns').textContent = totalPatterns;
  }

  // Initialize player
  await initPlayer();

  // Route to initial view based on URL hash
  handleRouteChange();
}

init();
