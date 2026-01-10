/**
 * Utility functions for pattern evaluation
 */

/**
 * Parse a target range string like "0.2-0.5" into [min, max]
 * @param {string} targetStr - Range string in format "min-max"
 * @returns {[number, number]} Tuple of [min, max]
 */
export function parseTargetRange(targetStr) {
  const parts = targetStr.split('-');
  if (parts.length === 2) {
    return [parseFloat(parts[0]), parseFloat(parts[1])];
  }
  return [0.0, 1.0];
}

/**
 * Check if a value is within a target range
 * @param {number} value - Value to check
 * @param {string} targetStr - Range string in format "min-max"
 * @returns {boolean}
 */
export function checkInRange(value, targetStr) {
  const [minVal, maxVal] = parseTargetRange(targetStr);
  return minVal <= value && value <= maxVal;
}

/**
 * Compute alignment score (0-1) measuring how close a value is to target range
 * @param {number} value - Value to score
 * @param {string} targetStr - Range string in format "min-max"
 * @returns {number} Score from 0 (far from range) to 1 (in range center)
 */
export function computeAlignmentScore(value, targetStr) {
  const [minVal, maxVal] = parseTargetRange(targetStr);
  const rangeCenter = (minVal + maxVal) / 2;
  const rangeWidth = (maxVal - minVal) / 2;
  const distanceFromCenter = Math.abs(value - rangeCenter);
  const tolerance = Math.max(rangeWidth * 2, 0.2);
  return Math.max(0, 1.0 - distanceFromCenter / tolerance);
}

/**
 * Get status color based on alignment score
 * @param {boolean} inRange - Whether value is in target range
 * @param {number} alignment - Alignment score (0-1)
 * @returns {string} CSS color
 */
export function getStatusColor(inRange, alignment) {
  if (inRange) return '#44ff44';
  if (alignment > 0.5) return '#ffaa44';
  return '#ff4444';
}

/**
 * Get zone color by zone key
 * @param {string} zone - Zone key (stable, syncopated, wild)
 * @returns {string} CSS color
 */
export function getZoneColor(zone) {
  const colors = {
    stable: '#44aa44',
    syncopated: '#aaaa44',
    wild: '#aa4444',
  };
  return colors[zone] || '#888888';
}

/**
 * Get voice color CSS variable reference
 * @param {string} voice - Voice key (v1, v2, aux)
 * @returns {string} CSS variable reference
 */
export function getVoiceColor(voice) {
  const colors = {
    v1: 'var(--v1-color)',
    v2: 'var(--v2-color)',
    aux: 'var(--aux-color)',
  };
  return colors[voice] || 'var(--text-primary)';
}

/**
 * Format a number as a percentage string
 * @param {number} value - Value to format (0-1 scale)
 * @param {number} decimals - Number of decimal places
 * @returns {string}
 */
export function formatPercent(value, decimals = 0) {
  return `${(value * 100).toFixed(decimals)}%`;
}

/**
 * Clamp a number between min and max
 * @param {number} value
 * @param {number} min
 * @param {number} max
 * @returns {number}
 */
export function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}
