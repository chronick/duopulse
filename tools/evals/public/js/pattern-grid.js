/**
 * Pattern grid rendering
 */

import { VOICES } from './constants.js';

/**
 * Render a single step cell
 * @param {Object} step - Step data
 * @param {Object} voice - Voice config
 * @param {number} index - Step index
 * @returns {string} HTML string
 */
function renderStep(step, voice, index) {
  const isHit = step[voice.key];
  const vel = step[voice.velKey];
  const isDownbeat = index % 4 === 0;
  const opacity = isHit ? 0.4 + vel * 0.6 : 1;

  const style = isHit
    ? `--step-color: var(--${voice.className}-color); opacity: ${opacity.toFixed(2)};`
    : `--step-dim: var(--${voice.className}-dim);`;

  const classes = ['step', isHit ? 'hit' : 'empty', isDownbeat ? 'downbeat' : '']
    .filter(Boolean)
    .join(' ');

  return `<div class="${classes}" style="${style}"></div>`;
}

/**
 * Render a voice row
 * @param {Object} voice - Voice config
 * @param {Array} steps - Step data array
 * @returns {string} HTML string
 */
function renderVoiceRow(voice, steps) {
  const stepsHtml = steps.map((s, i) => renderStep(s, voice, i)).join('');

  return `
    <div class="pattern-row">
      <div class="voice-label ${voice.className}">${voice.name}</div>
      <div class="pattern-steps">${stepsHtml}</div>
    </div>
  `;
}

/**
 * Render hit counts summary
 * @param {Object} hits - Hit counts by voice
 * @param {number} length - Pattern length
 * @returns {string} HTML string
 */
function renderHitCounts(hits, length) {
  return `
    <div class="hit-counts">
      <span class="hit-count v1">V1: ${hits.v1}/${length}</span>
      <span class="hit-count v2">V2: ${hits.v2}/${length}</span>
      <span class="hit-count aux">AUX: ${hits.aux}/${length}</span>
    </div>
  `;
}

/**
 * Render fill state indicator row
 * @param {number} length - Pattern length
 * @param {boolean} fillActive - Whether fill is active
 * @returns {string} HTML string
 */
function renderFillStateRow(length, fillActive = false) {
  // Render same number of indicators as pattern steps
  const indicators = Array.from({ length }, (_, i) => {
    const stateClass = fillActive ? 'active' : 'inactive';
    const isDownbeat = i % 4 === 0;
    return `<div class="fill-state-indicator ${stateClass} ${isDownbeat ? 'downbeat' : ''}"></div>`;
  }).join('');

  return `
    <div class="pattern-row fill-state-row">
      <div class="voice-label fill-state-label">FILL</div>
      <div class="pattern-steps fill-state-steps">${indicators}</div>
    </div>
  `;
}

/**
 * Render a complete pattern grid
 * @param {Object} pattern - Pattern data with steps, params, hits
 * @param {Object} options - Rendering options
 * @param {boolean} options.compact - Hide hit counts
 * @param {string} options.patternId - ID for selection
 * @param {string} options.name - Display name
 * @param {string} options.selectedId - Currently selected pattern ID
 * @param {boolean} options.showFillState - Show fill state indicator (default: true)
 * @param {boolean} options.fillActive - Whether fill is active (default: false)
 * @returns {string} HTML string
 */
export function renderPatternGrid(pattern, options = {}) {
  const {
    compact = false,
    patternId = null,
    name = '',
    selectedId = null,
    showFillState = true,
    fillActive = false,
  } = options;
  const { steps, params, hits } = pattern;
  const length = params.length;

  const fillStateHtml = showFillState ? renderFillStateRow(length, fillActive) : '';
  const rows = VOICES.map(voice => renderVoiceRow(voice, steps)).join('');
  const hitCountsHtml = compact ? '' : renderHitCounts(hits, length);

  const isSelected = patternId && patternId === selectedId;
  const classes = [
    'pattern-grid',
    patternId ? 'selectable' : '',
    isSelected ? 'selected' : '',
  ].filter(Boolean).join(' ');

  const dataAttr = patternId
    ? `data-pattern-id="${patternId}" data-pattern-name="${name}"`
    : '';

  return `<div class="${classes}" ${dataAttr}>${rows}${fillStateHtml}</div>${hitCountsHtml}`;
}

/**
 * Extract pattern from click event on pattern grid
 * @param {Event} e - Click event
 * @returns {Object|null} { patternId, patternName } or null
 */
export function getPatternFromClick(e) {
  const grid = e.target.closest('.pattern-grid.selectable');
  if (!grid) return null;

  return {
    patternId: grid.dataset.patternId,
    patternName: grid.dataset.patternName || 'Pattern',
  };
}
