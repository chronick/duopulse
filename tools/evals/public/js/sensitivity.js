/**
 * Sensitivity Analysis Visualization
 *
 * Renders a heatmap showing how algorithm weights affect Pentagon metrics.
 * Color scale: blue (negative) -> white (neutral) -> red (positive)
 */

const METRICS = ['syncopation', 'density', 'velocityRange', 'voiceSeparation', 'regularity'];
const METRIC_NAMES = {
  syncopation: 'Syncopation',
  density: 'Density',
  velocityRange: 'Vel Range',
  voiceSeparation: 'Voice Sep',
  regularity: 'Regularity',
};

const PARAM_NAMES = {
  euclideanFadeStart: 'Eucl Start',
  euclideanFadeEnd: 'Eucl End',
  syncopationCenter: 'Sync Center',
  syncopationWidth: 'Sync Width',
  randomFadeStart: 'Rand Start',
  randomFadeEnd: 'Rand End',
};

/**
 * Convert sensitivity value (-1 to +1) to RGB color
 * Negative = blue, Zero = white, Positive = red
 */
function sensitivityToColor(value, alpha = 1.0) {
  // Clamp to [-1, 1]
  value = Math.max(-1, Math.min(1, value));

  let r, g, b;

  if (value < 0) {
    // Blue gradient: white -> blue
    const intensity = Math.abs(value);
    r = Math.round(255 * (1 - intensity * 0.8));
    g = Math.round(255 * (1 - intensity * 0.8));
    b = 255;
  } else {
    // Red gradient: white -> red
    const intensity = value;
    r = 255;
    g = Math.round(255 * (1 - intensity * 0.8));
    b = Math.round(255 * (1 - intensity * 0.8));
  }

  return `rgba(${r}, ${g}, ${b}, ${alpha})`;
}

/**
 * Format sensitivity value for display
 */
function formatSensitivity(value) {
  if (value === null || value === undefined) return '--';
  const sign = value >= 0 ? '+' : '';
  return sign + value.toFixed(2);
}

/**
 * Get text color for contrast against background
 */
function getTextColor(value) {
  const intensity = Math.abs(value);
  return intensity > 0.5 ? '#fff' : '#000';
}

/**
 * Render sensitivity heatmap as HTML
 */
export function renderSensitivityHeatmap(sensitivityData, onCellClick = null) {
  if (!sensitivityData || !sensitivityData.matrix) {
    return '<div class="error">No sensitivity data available. Run "make sensitivity-matrix" first.</div>';
  }

  const { matrix, levers, generated_at, baseline_commit } = sensitivityData;
  const params = Object.keys(matrix);

  // Build header row
  const headerCells = METRICS.map(m =>
    `<th class="metric-header">${METRIC_NAMES[m] || m}</th>`
  ).join('');

  // Build data rows
  const rows = params.map(param => {
    const paramLabel = PARAM_NAMES[param] || param;
    const cells = METRICS.map(metric => {
      const value = matrix[param]?.[metric] ?? null;
      const bgColor = value !== null ? sensitivityToColor(value, 0.85) : '#333';
      const textColor = value !== null ? getTextColor(value) : '#666';
      const displayValue = formatSensitivity(value);

      const clickAttr = onCellClick ? `data-param="${param}" data-metric="${metric}"` : '';
      const clickClass = onCellClick ? 'clickable' : '';

      return `
        <td class="sensitivity-cell ${clickClass}"
            style="background: ${bgColor}; color: ${textColor};"
            ${clickAttr}>
          ${displayValue}
        </td>
      `;
    }).join('');

    return `
      <tr>
        <td class="param-label">${paramLabel}</td>
        ${cells}
      </tr>
    `;
  }).join('');

  // Build lever summary
  const leverSummary = METRICS.map(metric => {
    const leverInfo = levers?.[metric];
    if (!leverInfo) return '';

    const primary = leverInfo.primary || [];
    const secondary = leverInfo.secondary || [];

    if (primary.length === 0 && secondary.length === 0) {
      return `<div class="lever-item"><strong>${METRIC_NAMES[metric]}:</strong> <span class="no-lever">no strong levers</span></div>`;
    }

    const primaryStr = primary.map(p => {
      const sign = p.sensitivity >= 0 ? '+' : '';
      return `<span class="lever-primary">${PARAM_NAMES[p.param] || p.param} (${sign}${p.sensitivity.toFixed(2)})</span>`;
    }).join(', ');

    const secondaryStr = secondary.length > 0
      ? ` <span class="lever-secondary">[also: ${secondary.map(p => PARAM_NAMES[p.param] || p.param).join(', ')}]</span>`
      : '';

    return `<div class="lever-item"><strong>${METRIC_NAMES[metric]}:</strong> ${primaryStr}${secondaryStr}</div>`;
  }).join('');

  return `
    <div class="sensitivity-heatmap">
      <div class="sensitivity-header">
        <span class="generated-info">Generated: ${new Date(generated_at).toLocaleString()}</span>
        <span class="commit-info">Baseline: ${baseline_commit?.substring(0, 8) || 'unknown'}</span>
      </div>

      <div class="color-legend">
        <span class="legend-label">Sensitivity:</span>
        <span class="legend-neg">-1.0 (decreases)</span>
        <span class="legend-bar"></span>
        <span class="legend-pos">+1.0 (increases)</span>
      </div>

      <table class="sensitivity-table">
        <thead>
          <tr>
            <th class="param-header">Parameter</th>
            ${headerCells}
          </tr>
        </thead>
        <tbody>
          ${rows}
        </tbody>
      </table>

      <div class="lever-summary">
        <h4>Lever Summary (primary influences)</h4>
        ${leverSummary}
      </div>
    </div>
  `;
}

/**
 * CSS styles for the sensitivity heatmap
 */
export const sensitivityStyles = `
  .sensitivity-heatmap {
    background: #1a1a1a;
    border-radius: 8px;
    padding: 20px;
    margin-bottom: 20px;
  }

  .sensitivity-header {
    display: flex;
    justify-content: space-between;
    margin-bottom: 16px;
    font-size: 12px;
    color: #666;
  }

  .color-legend {
    display: flex;
    align-items: center;
    gap: 12px;
    margin-bottom: 16px;
    font-size: 12px;
  }

  .legend-label {
    color: #888;
  }

  .legend-neg {
    color: #6699ff;
  }

  .legend-pos {
    color: #ff6666;
  }

  .legend-bar {
    width: 200px;
    height: 16px;
    background: linear-gradient(to right, #3366ff, #ffffff 50%, #ff3333);
    border-radius: 4px;
  }

  .sensitivity-table {
    width: 100%;
    border-collapse: collapse;
    margin-bottom: 20px;
  }

  .sensitivity-table th,
  .sensitivity-table td {
    padding: 10px 12px;
    text-align: center;
    border: 1px solid #333;
  }

  .param-header,
  .metric-header {
    background: #222;
    color: #888;
    font-weight: 600;
    font-size: 12px;
  }

  .param-label {
    text-align: left !important;
    background: #222;
    color: #ff6b35;
    font-weight: 500;
    font-size: 13px;
  }

  .sensitivity-cell {
    font-weight: 600;
    font-size: 14px;
    font-family: 'SF Mono', 'Monaco', monospace;
    transition: transform 0.1s, box-shadow 0.1s;
  }

  .sensitivity-cell.clickable {
    cursor: pointer;
  }

  .sensitivity-cell.clickable:hover {
    transform: scale(1.05);
    box-shadow: 0 0 8px rgba(255, 255, 255, 0.3);
    z-index: 1;
    position: relative;
  }

  .lever-summary {
    background: #222;
    border-radius: 6px;
    padding: 16px;
  }

  .lever-summary h4 {
    margin: 0 0 12px 0;
    color: #888;
    font-size: 14px;
  }

  .lever-item {
    margin-bottom: 8px;
    font-size: 13px;
    color: #ccc;
  }

  .lever-item strong {
    color: #ff6b35;
  }

  .lever-primary {
    color: #44ff44;
    font-weight: 500;
  }

  .lever-secondary {
    color: #888;
    font-size: 11px;
  }

  .no-lever {
    color: #666;
    font-style: italic;
  }
`;
