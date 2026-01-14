/**
 * Pentagon radar chart rendering
 */

import { METRIC_KEYS, METRIC_LABELS, ZONE_COLORS } from './constants.js';
import { parseTargetRange } from './utils.js';

/**
 * Generate SVG for a pentagon radar chart
 * @param {Object} pentagonStats - Stats with total metrics and zone data
 * @param {Object} metricDefs - Metric definitions with target ranges
 * @param {number} size - SVG size in pixels
 * @returns {string} SVG markup
 */
export function renderPentagonRadarSVG(pentagonStats, metricDefs = {}, size = 380) {
  const cx = size / 2;
  const cy = size / 2;
  const r = size / 2 - 50;
  const angles = [0, 1, 2, 3, 4].map(i => (-90 + i * 72) * Math.PI / 180);

  const total = pentagonStats.total || {};
  const values = METRIC_KEYS.map(k => total[k] ?? 0.5);

  let svg = `<svg width="${size}" height="${size}" viewBox="0 0 ${size} ${size}" xmlns="http://www.w3.org/2000/svg">`;

  // Grid circles
  svg += renderGridCircles(cx, cy, r);

  // Zone bands
  svg += renderZoneBands(cx, cy, r, angles, metricDefs);

  // Axis lines and labels
  svg += renderAxes(cx, cy, r, angles);

  // Values polygon
  svg += renderValuesPolygon(cx, cy, r, angles, values);

  // Composite score in center
  const composite = total.composite ?? 0;
  svg += renderCenterScore(cx, cy, composite);

  svg += '</svg>';
  return svg;
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
