/**
 * Tests for utility functions
 */

import {
  parseTargetRange,
  checkInRange,
  computeAlignmentScore,
  getStatusColor,
  getZoneColor,
  formatPercent,
  clamp,
} from '../public/js/utils.js';

describe('parseTargetRange', () => {
  test('parses valid range string', () => {
    expect(parseTargetRange('0.2-0.5')).toEqual([0.2, 0.5]);
  });

  test('parses zero to one range', () => {
    expect(parseTargetRange('0.0-1.0')).toEqual([0.0, 1.0]);
  });

  test('returns default for invalid string', () => {
    expect(parseTargetRange('invalid')).toEqual([0.0, 1.0]);
  });

  test('returns default for empty string', () => {
    expect(parseTargetRange('')).toEqual([0.0, 1.0]);
  });

  test('returns default for malformed negative range (known limitation)', () => {
    // parseTargetRange uses simple split('-') which doesn't handle negative numbers
    // This is acceptable since all our ranges are 0.0-1.0
    expect(parseTargetRange('-0.5-0.5')).toEqual([0.0, 1.0]);
  });
});

describe('checkInRange', () => {
  test('returns true when value is in range', () => {
    expect(checkInRange(0.3, '0.2-0.5')).toBe(true);
  });

  test('returns true when value equals min', () => {
    expect(checkInRange(0.2, '0.2-0.5')).toBe(true);
  });

  test('returns true when value equals max', () => {
    expect(checkInRange(0.5, '0.2-0.5')).toBe(true);
  });

  test('returns false when value is below range', () => {
    expect(checkInRange(0.1, '0.2-0.5')).toBe(false);
  });

  test('returns false when value is above range', () => {
    expect(checkInRange(0.6, '0.2-0.5')).toBe(false);
  });
});

describe('computeAlignmentScore', () => {
  test('returns 1.0 for value at center of range', () => {
    const score = computeAlignmentScore(0.35, '0.2-0.5');
    expect(score).toBeCloseTo(1.0, 1);
  });

  test('returns high score for value in range', () => {
    const score = computeAlignmentScore(0.3, '0.2-0.5');
    expect(score).toBeGreaterThan(0.5);
  });

  test('returns lower score for value outside range', () => {
    const score = computeAlignmentScore(0.1, '0.2-0.5');
    expect(score).toBeLessThan(1.0);
  });

  test('returns 0 for value far from range', () => {
    const score = computeAlignmentScore(1.0, '0.0-0.2');
    expect(score).toBe(0);
  });

  test('never returns negative', () => {
    const score = computeAlignmentScore(5.0, '0.0-0.1');
    expect(score).toBeGreaterThanOrEqual(0);
  });
});

describe('getStatusColor', () => {
  test('returns green for in-range values', () => {
    expect(getStatusColor(true, 1.0)).toBe('#44ff44');
  });

  test('returns yellow for close but out-of-range values', () => {
    expect(getStatusColor(false, 0.6)).toBe('#ffaa44');
  });

  test('returns red for far out-of-range values', () => {
    expect(getStatusColor(false, 0.3)).toBe('#ff4444');
  });
});

describe('getZoneColor', () => {
  test('returns correct color for stable zone', () => {
    expect(getZoneColor('stable')).toBe('#44aa44');
  });

  test('returns correct color for syncopated zone', () => {
    expect(getZoneColor('syncopated')).toBe('#aaaa44');
  });

  test('returns correct color for wild zone', () => {
    expect(getZoneColor('wild')).toBe('#aa4444');
  });

  test('returns fallback for unknown zone', () => {
    expect(getZoneColor('unknown')).toBe('#888888');
  });
});

describe('formatPercent', () => {
  test('formats 0.5 as 50%', () => {
    expect(formatPercent(0.5)).toBe('50%');
  });

  test('formats 1.0 as 100%', () => {
    expect(formatPercent(1.0)).toBe('100%');
  });

  test('formats with decimals', () => {
    expect(formatPercent(0.333, 1)).toBe('33.3%');
  });

  test('rounds correctly', () => {
    expect(formatPercent(0.999, 0)).toBe('100%');
  });
});

describe('clamp', () => {
  test('returns value when in range', () => {
    expect(clamp(5, 0, 10)).toBe(5);
  });

  test('returns min when value below', () => {
    expect(clamp(-5, 0, 10)).toBe(0);
  });

  test('returns max when value above', () => {
    expect(clamp(15, 0, 10)).toBe(10);
  });

  test('handles equal min and max', () => {
    expect(clamp(5, 3, 3)).toBe(3);
  });
});
