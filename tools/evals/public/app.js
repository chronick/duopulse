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
};

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

  container.innerHTML = `
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
          <div class="label-desc">Hill-climbing metric (1.0 = all metrics in target ranges)</div>
        </div>
        <div class="alignment-value">
          <span style="color: ${alignmentColor}; font-size: 48px; font-weight: 700;">${(exp.overallAlignment * 100).toFixed(1)}%</span>
          <span style="color: ${alignmentColor}; font-size: 18px; margin-left: 12px;">[${exp.alignmentStatus}]</span>
        </div>
        <div class="alignment-count">${exp.totalPatterns} patterns analyzed</div>
      </div>
    </div>

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

    const card = document.createElement('div');
    card.className = 'preset-card';
    card.innerHTML = `
      <h3>${preset.name}</h3>
      <p class="preset-desc">${preset.description}</p>
      <div class="preset-params">${paramsHtml}</div>
      ${renderPattern(patternData, { patternId, name: preset.name })}
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
    const paramValue = pattern.params[paramName];
    const patternId = `sweep-${paramName}-${i}`;
    const name = `${paramName.toUpperCase()} ${formatPercent(paramValue)}`;
    const zone = metric.pentagon.zone;
    const zoneColor = ZONES.find(z => z.key === zone)?.color || '#888';

    return `
      <div class="sweep-item">
        <div class="sweep-value">${paramName.toUpperCase()} = ${paramValue.toFixed(2)}</div>
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

  container.innerHTML = renderSensitivityHeatmap(data.sensitivity);
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
      case 'sensitivity': renderSensitivityView(); break;
    }
  }

  $$('.nav-btn').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.view === viewName);
  });
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
  $$('.nav-btn').forEach(btn => {
    btn.addEventListener('click', () => showView(btn.dataset.view));
  });

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

  showView('overview');
}

init();
