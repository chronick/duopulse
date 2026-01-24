/**
 * Pentagon radar chart rendering
 */

import { METRIC_KEYS, METRIC_LABELS, ZONE_COLORS, ZONES } from './constants.js';
import { parseTargetRange } from './utils.js';

/**
 * Generate SVG for a pentagon radar chart with interactive zone tooltips
 * @param {Object} pentagonStats - Stats with total metrics and zone data
 * @param {Object} metricDefs - Metric definitions with target ranges
 * @param {number} size - SVG size in pixels
 * @returns {string} SVG markup wrapped in a container with tooltip
 */
export function renderPentagonRadarSVG(pentagonStats, metricDefs = {}, size = 380) {
  const cx = size / 2;
  const cy = size / 2;
  const r = size / 2 - 50;
  const angles = [0, 1, 2, 3, 4].map(i => (-90 + i * 72) * Math.PI / 180);

  const total = pentagonStats.total || {};
  const values = METRIC_KEYS.map(k => total[k] ?? 0.5);

  // Generate unique ID for this chart instance
  const chartId = `pentagon-${Date.now()}-${Math.random().toString(36).substr(2, 9)}`;

  let svg = `<svg width="${size}" height="${size}" viewBox="0 0 ${size} ${size}" xmlns="http://www.w3.org/2000/svg" class="pentagon-svg" id="${chartId}">`;

  // Defs for filters/effects
  svg += `<defs>
    <filter id="zone-glow-${chartId}" x="-50%" y="-50%" width="200%" height="200%">
      <feGaussianBlur stdDeviation="3" result="blur"/>
      <feMerge>
        <feMergeNode in="blur"/>
        <feMergeNode in="SourceGraphic"/>
      </feMerge>
    </filter>
  </defs>`;

  // Grid circles
  svg += renderGridCircles(cx, cy, r);

  // Zone bands (non-interactive background)
  svg += renderZoneBands(cx, cy, r, angles, metricDefs);

  // Interactive zone areas (invisible but hoverable)
  svg += renderInteractiveZones(cx, cy, r, angles, metricDefs, pentagonStats, chartId);

  // Axis lines and labels
  svg += renderAxes(cx, cy, r, angles);

  // Values polygon
  svg += renderValuesPolygon(cx, cy, r, angles, values);

  // Composite score in center
  const composite = total.composite ?? 0;
  svg += renderCenterScore(cx, cy, composite);

  svg += '</svg>';

  // Wrap in container with tooltip
  return `
    <div class="pentagon-chart-container" style="position: relative;">
      ${svg}
      <div class="pentagon-tooltip" id="tooltip-${chartId}" style="
        position: absolute;
        display: none;
        background: rgba(0, 0, 0, 0.9);
        border: 1px solid #444;
        border-radius: 6px;
        padding: 10px 14px;
        font-size: 12px;
        pointer-events: none;
        z-index: 100;
        min-width: 140px;
        box-shadow: 0 4px 12px rgba(0,0,0,0.5);
      "></div>
    </div>
    <script type="module">
      (function() {
        const chart = document.getElementById('${chartId}');
        const tooltip = document.getElementById('tooltip-${chartId}');
        if (!chart || !tooltip) return;

        chart.querySelectorAll('.zone-area').forEach(zone => {
          zone.addEventListener('mouseenter', (e) => {
            const zoneKey = zone.dataset.zone;
            const conformance = zone.dataset.conformance;
            const count = zone.dataset.count;
            const color = zone.dataset.color;

            tooltip.innerHTML = \`
              <div style="color: \${color}; font-weight: 600; font-size: 14px; margin-bottom: 6px;">
                \${zoneKey.toUpperCase()} ZONE
              </div>
              <div style="color: #ccc; margin-bottom: 4px;">
                Conformance: <span style="color: \${color}; font-weight: 600;">\${conformance}%</span>
              </div>
              <div style="color: #888; font-size: 11px;">
                \${count} patterns analyzed
              </div>
            \`;
            tooltip.style.display = 'block';
          });

          zone.addEventListener('mousemove', (e) => {
            const container = chart.closest('.pentagon-chart-container');
            const rect = container.getBoundingClientRect();
            const x = e.clientX - rect.left + 15;
            const y = e.clientY - rect.top - 10;
            tooltip.style.left = x + 'px';
            tooltip.style.top = y + 'px';
          });

          zone.addEventListener('mouseleave', () => {
            tooltip.style.display = 'none';
          });
        });
      })();
    </script>
  `;
}

function renderGridCircles(cx, cy, r) {
  let svg = '';
  for (const pct of [0.25, 0.5, 0.75, 1.0]) {
    svg += `<circle cx="${cx}" cy="${cy}" r="${r * pct}" fill="none" stroke="#333" stroke-width="1" opacity="0.5"/>`;
  }
  return svg;
}

function renderZoneBands(cx, cy, r, angles, metricDefs) {
  let svg = '';

  for (const [zone, color] of Object.entries(ZONE_COLORS)) {
    const points = [];
    for (let i = 0; i < 5; i++) {
      const key = METRIC_KEYS[i];
      const targetStr = metricDefs[key]?.targetByZone?.[zone] || '0.0-1.0';
      const [, maxVal] = parseTargetRange(targetStr);
      const px = cx + r * maxVal * Math.cos(angles[i]);
      const py = cy + r * maxVal * Math.sin(angles[i]);
      points.push(`${px},${py}`);
    }
    svg += `<polygon points="${points.join(' ')}" fill="none" stroke="${color}" stroke-width="1" stroke-dasharray="4,4" opacity="0.4"/>`;
  }

  return svg;
}

function renderInteractiveZones(cx, cy, r, angles, metricDefs, pentagonStats, chartId) {
  let svg = '';

  // Render zones from outermost (wild) to innermost (stable) so inner zones are on top
  const zoneOrder = ['wild', 'syncopated', 'stable'];

  for (const zone of zoneOrder) {
    const color = ZONE_COLORS[zone];
    const zoneData = pentagonStats[zone] || {};
    const conformance = zoneData.composite !== undefined ? Math.round(zoneData.composite * 100) : 0;
    const count = zoneData.count || 0;

    const points = [];
    for (let i = 0; i < 5; i++) {
      const key = METRIC_KEYS[i];
      const targetStr = metricDefs[key]?.targetByZone?.[zone] || '0.0-1.0';
      const [, maxVal] = parseTargetRange(targetStr);
      const px = cx + r * maxVal * Math.cos(angles[i]);
      const py = cy + r * maxVal * Math.sin(angles[i]);
      points.push(`${px},${py}`);
    }

    svg += `<polygon
      class="zone-area"
      points="${points.join(' ')}"
      fill="${color}"
      fill-opacity="0"
      stroke="transparent"
      stroke-width="12"
      data-zone="${zone}"
      data-conformance="${conformance}"
      data-count="${count}"
      data-color="${color}"
      style="cursor: pointer; transition: fill-opacity 0.2s;"
      onmouseenter="this.style.fillOpacity='0.15'; this.style.filter='url(#zone-glow-${chartId})';"
      onmouseleave="this.style.fillOpacity='0'; this.style.filter='none';"
    />`;
  }

  return svg;
}

function renderAxes(cx, cy, r, angles) {
  let svg = '';

  for (let i = 0; i < 5; i++) {
    const x = cx + r * Math.cos(angles[i]);
    const y = cy + r * Math.sin(angles[i]);
    svg += `<line x1="${cx}" y1="${cy}" x2="${x}" y2="${y}" stroke="#444" stroke-width="1"/>`;

    const lx = cx + (r + 30) * Math.cos(angles[i]);
    const ly = cy + (r + 30) * Math.sin(angles[i]);
    svg += `<text x="${lx}" y="${ly + 4}" text-anchor="middle" fill="#fff" font-size="12" font-weight="600">${METRIC_LABELS[i]}</text>`;
  }

  return svg;
}

function renderValuesPolygon(cx, cy, r, angles, values) {
  let svg = '';
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

  return svg;
}

function renderCenterScore(cx, cy, composite) {
  let svg = '';
  svg += `<text x="${cx}" y="${cy + 8}" text-anchor="middle" fill="#fff" font-size="32" font-weight="700">${Math.round(composite * 100)}%</text>`;
  svg += `<text x="${cx}" y="${cy + 28}" text-anchor="middle" fill="#666" font-size="11">composite</text>`;
  return svg;
}
