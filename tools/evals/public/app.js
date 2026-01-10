/**
 * DuoPulse Pattern Evaluation Dashboard
 * Frontend JavaScript - fetches JSON and renders pattern visualizations
 */

// State
let data = {
  metadata: null,
  presets: null,
  presetMetrics: null,
  sweeps: null,
  sweepMetrics: null,
  seedVariation: null,
  expressiveness: null,
};

let currentView = 'overview';

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

    return true;
  } catch (err) {
    console.error('Error loading data:', err);
    return false;
  }
}

// ============================================================================
// Utility Functions
// ============================================================================

function parseTargetRange(targetStr) {
  const parts = targetStr.split('-');
  if (parts.length === 2) {
    return [parseFloat(parts[0]), parseFloat(parts[1])];
  }
  return [0.0, 1.0];
}

function checkInRange(value, targetStr) {
  const [minVal, maxVal] = parseTargetRange(targetStr);
  return minVal <= value && value <= maxVal;
}

function computeAlignmentScore(value, targetStr) {
  const [minVal, maxVal] = parseTargetRange(targetStr);
  const rangeCenter = (minVal + maxVal) / 2;
  const rangeWidth = (maxVal - minVal) / 2;
  const distanceFromCenter = Math.abs(value - rangeCenter);
  const tolerance = Math.max(rangeWidth * 2, 0.2);
  return Math.max(0, 1.0 - distanceFromCenter / tolerance);
}

// ============================================================================
// Pentagon SVG Rendering
// ============================================================================

function renderPentagonRadarSVG(pentagonStats, size = 380) {
  const cx = size / 2;
  const cy = size / 2;
  const r = size / 2 - 50;

  const metricKeys = ['syncopation', 'density', 'velocityRange', 'voiceSeparation', 'regularity'];
  const labels = ['Sync', 'Dens', 'VelRng', 'VoiceSep', 'Reg'];
  const angles = [0, 1, 2, 3, 4].map(i => (-90 + i * 72) * Math.PI / 180);

  const total = pentagonStats.total || {};
  const values = metricKeys.map(k => total[k] ?? 0.5);

  // Grid circles
  let svg = `<svg width="${size}" height="${size}" viewBox="0 0 ${size} ${size}" xmlns="http://www.w3.org/2000/svg">`;

  for (const pct of [0.25, 0.5, 0.75, 1.0]) {
    svg += `<circle cx="${cx}" cy="${cy}" r="${r * pct}" fill="none" stroke="#333" stroke-width="1" opacity="0.5"/>`;
  }

  // Zone bands (dashed outlines)
  const zoneColors = { stable: '#44aa44', syncopated: '#aaaa44', wild: '#aa4444' };
  const defs = data.expressiveness?.metricDefinitions || {};

  for (const [zone, color] of Object.entries(zoneColors)) {
    const points = [];
    for (let i = 0; i < 5; i++) {
      const key = metricKeys[i];
      const targetStr = defs[key]?.targetByZone?.[zone] || '0.0-1.0';
      const [, maxVal] = parseTargetRange(targetStr);
      const px = cx + r * maxVal * Math.cos(angles[i]);
      const py = cy + r * maxVal * Math.sin(angles[i]);
      points.push(`${px},${py}`);
    }
    svg += `<polygon points="${points.join(' ')}" fill="none" stroke="${color}" stroke-width="1" stroke-dasharray="4,4" opacity="0.4"/>`;
  }

  // Axis lines and labels
  for (let i = 0; i < 5; i++) {
    const x = cx + r * Math.cos(angles[i]);
    const y = cy + r * Math.sin(angles[i]);
    svg += `<line x1="${cx}" y1="${cy}" x2="${x}" y2="${y}" stroke="#444" stroke-width="1"/>`;

    const lx = cx + (r + 30) * Math.cos(angles[i]);
    const ly = cy + (r + 30) * Math.sin(angles[i]);
    svg += `<text x="${lx}" y="${ly + 4}" text-anchor="middle" fill="#fff" font-size="12" font-weight="600">${labels[i]}</text>`;
  }

  // Values polygon
  const points = [];
  for (let i = 0; i < 5; i++) {
    const px = cx + r * values[i] * Math.cos(angles[i]);
    const py = cy + r * values[i] * Math.sin(angles[i]);
    points.push(`${px},${py}`);
  }
  svg += `<polygon points="${points.join(' ')}" fill="#4ecdc4" fill-opacity="0.3" stroke="#4ecdc4" stroke-width="2"/>`;

  // Dots at vertices
  for (let i = 0; i < 5; i++) {
    const px = cx + r * values[i] * Math.cos(angles[i]);
    const py = cy + r * values[i] * Math.sin(angles[i]);
    svg += `<circle cx="${px}" cy="${py}" r="5" fill="#4ecdc4"/>`;
  }

  // Composite in center
  const composite = total.composite ?? 0;
  svg += `<text x="${cx}" y="${cy + 8}" text-anchor="middle" fill="#fff" font-size="32" font-weight="700">${Math.round(composite * 100)}%</text>`;
  svg += `<text x="${cx}" y="${cy + 28}" text-anchor="middle" fill="#666" font-size="11">composite</text>`;

  svg += '</svg>';
  return svg;
}

// ============================================================================
// Pattern Grid Rendering
// ============================================================================

function renderPatternGrid(pattern, compact = false) {
  const { steps, params } = pattern;
  const length = params.length;

  const voices = [
    { name: 'V1', key: 'v1', velKey: 'v1Vel', className: 'v1' },
    { name: 'V2', key: 'v2', velKey: 'v2Vel', className: 'v2' },
    { name: 'AUX', key: 'aux', velKey: 'auxVel', className: 'aux' },
  ];

  const rows = voices.map(voice => {
    const stepsHtml = steps.map((s, i) => {
      const isHit = s[voice.key];
      const vel = s[voice.velKey];
      const isDownbeat = i % 4 === 0;
      const opacity = isHit ? 0.4 + vel * 0.6 : 1;

      const style = isHit
        ? `--step-color: var(--${voice.className}-color); opacity: ${opacity.toFixed(2)};`
        : `--step-dim: var(--${voice.className}-dim);`;

      const classes = ['step', isHit ? 'hit' : 'empty', isDownbeat ? 'downbeat' : ''].filter(Boolean).join(' ');
      return `<div class="${classes}" style="${style}"></div>`;
    }).join('');

    return `
      <div class="pattern-row">
        <div class="voice-label ${voice.className}">${voice.name}</div>
        <div class="pattern-steps">${stepsHtml}</div>
      </div>
    `;
  }).join('');

  const hitCounts = `
    <div class="hit-counts">
      <span class="hit-count v1">V1: ${pattern.hits.v1}/${length}</span>
      <span class="hit-count v2">V2: ${pattern.hits.v2}/${length}</span>
      <span class="hit-count aux">AUX: ${pattern.hits.aux}/${length}</span>
    </div>
  `;

  return `<div class="pattern-grid">${rows}</div>${compact ? '' : hitCounts}`;
}

// ============================================================================
// Overview View (Pentagon Summary - matching Python layout)
// ============================================================================

function renderOverviewView() {
  const container = $('#overview-container');
  const exp = data.expressiveness;
  const stats = exp.pentagonStats;
  const defs = exp.metricDefinitions;

  const metricKeys = ['syncopation', 'density', 'velocityRange', 'voiceSeparation', 'regularity'];
  const zones = [
    { key: 'stable', label: 'STABLE', range: 'SHAPE 0-30%', color: '#44aa44' },
    { key: 'syncopated', label: 'SYNCOPATED', range: 'SHAPE 30-70%', color: '#aaaa44' },
    { key: 'wild', label: 'WILD', range: 'SHAPE 70-100%', color: '#aa4444' },
  ];

  // Build table rows
  let tableRows = '';
  for (const key of metricKeys) {
    const meta = defs[key];
    const cells = zones.map(zone => {
      const zoneData = stats[zone.key];
      if (!zoneData) return '<td style="text-align: center; color: #444;">--</td>';

      const value = zoneData[key];
      const targetStr = meta.targetByZone[zone.key];
      const inRange = checkInRange(value, targetStr);
      const alignment = computeAlignmentScore(value, targetStr);

      let color = '#ff4444';
      if (inRange) color = '#44ff44';
      else if (alignment > 0.5) color = '#ffaa44';

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
  const complianceCells = zones.map(zone => {
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
          ${renderPentagonRadarSVG(stats, 380)}
        </div>

        <div class="pentagon-table-wrapper">
          <table class="pentagon-table">
            <thead>
              <tr style="border-bottom: 1px solid #333;">
                <th style="text-align: left; padding: 8px 12px; color: #888;">Metric</th>
                ${zones.map(z => `
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
        ${['v1', 'v2', 'aux'].map(v => {
          const score = data.seedVariation.summary[v].avgScore;
          const color = v === 'v1' ? 'var(--v1-color)' : v === 'v2' ? 'var(--v2-color)' : 'var(--aux-color)';
          return `
            <div class="seed-bar-item">
              <span class="seed-label" style="color: ${color}">${v.toUpperCase()}</span>
              <div class="seed-bar">
                <div class="seed-bar-fill" style="width: ${score * 100}%; background: ${color};"></div>
              </div>
              <span class="seed-value">${Math.round(score * 100)}%</span>
            </div>
          `;
        }).join('')}
      </div>
      <div class="seed-status ${data.seedVariation.summary.pass ? 'pass' : 'fail'}">
        ${data.seedVariation.summary.pass ? 'PASS' : 'FAIL'} - Overall: ${Math.round(data.seedVariation.summary.overall * 100)}%
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
      ${renderPatternGrid(patternData)}
    `;

    container.appendChild(card);
  });
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

    return `
      <div class="sweep-item">
        <div class="sweep-value">${paramName.toUpperCase()} = ${paramValue.toFixed(2)}</div>
        ${renderPatternGrid(pattern, true)}
        <div style="font-size: 11px; color: var(--text-secondary); margin-top: 8px;">
          Zone: <span style="color: ${metric.pentagon.zone === 'stable' ? '#44aa44' : metric.pentagon.zone === 'syncopated' ? '#aaaa44' : '#aa4444'}">${metric.pentagon.zone}</span>
          | Score: <span style="color: ${metric.pentagon.composite > 0.5 ? '#44ff44' : '#ff6666'}">${Math.round(metric.pentagon.composite * 100)}%</span>
        </div>
      </div>
    `;
  }).join('');
}

// ============================================================================
// Seeds View
// ============================================================================

function renderSeedsView() {
  const summary = data.seedVariation.summary;
  const tests = data.seedVariation.tests;

  const summaryHtml = `
    <div class="summary-grid">
      ${['v1', 'v2', 'aux'].map(v => {
        const s = summary[v];
        const color = v === 'v1' ? 'v1' : v === 'v2' ? 'v2' : 'aux';
        return `
          <div class="summary-item">
            <div class="label">${v.toUpperCase()} Variation</div>
            <div class="value ${color}">${Math.round(s.avgScore * 100)}%</div>
            <div class="variation-bar-container">
              <div class="variation-bar">
                <div class="variation-bar-fill ${s.avgScore >= 0.5 ? 'pass' : 'fail'}" style="width: ${s.avgScore * 100}%"></div>
              </div>
            </div>
          </div>
        `;
      }).join('')}
      <div class="summary-item">
        <div class="label">Overall</div>
        <div class="value" style="color: ${summary.pass ? 'var(--success)' : 'var(--error)'}">${Math.round(summary.overall * 100)}%</div>
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
            <div class="score"><span style="color: var(--v1-color)">V1:</span> <span style="color: ${t.variation.v1.score >= 0.5 ? 'var(--success)' : 'var(--error)'}">${Math.round(t.variation.v1.score * 100)}%</span></div>
            <div class="score"><span style="color: var(--v2-color)">V2:</span> <span style="color: ${t.variation.v2.score >= 0.5 ? 'var(--success)' : 'var(--error)'}">${Math.round(t.variation.v2.score * 100)}%</span></div>
            <div class="score"><span style="color: var(--aux-color)">AUX:</span> <span style="color: ${t.variation.aux.score >= 0.5 ? 'var(--success)' : 'var(--error)'}">${Math.round(t.variation.aux.score * 100)}%</span></div>
          </div>
        </div>
      `).join('')}
    </div>
    ${lowVariation.length > 20 ? `<p style="margin-top: 12px; color: var(--text-secondary);">... and ${lowVariation.length - 20} more</p>` : ''}
  `;
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
    }
  }

  $$('.nav-btn').forEach(btn => {
    btn.classList.toggle('active', btn.dataset.view === viewName);
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

  showView('overview');
}

init();
