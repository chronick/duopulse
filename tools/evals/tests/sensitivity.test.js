/**
 * Tests for sensitivity analysis calculations
 */

describe('Sensitivity Analysis', () => {
  // Helper functions extracted from compute-matrix.js for testing
  function computeSlope(xValues, yValues) {
    const n = xValues.length;
    if (n < 2) return 0;

    const meanX = xValues.reduce((a, b) => a + b, 0) / n;
    const meanY = yValues.reduce((a, b) => a + b, 0) / n;

    let numerator = 0;
    let denominator = 0;

    for (let i = 0; i < n; i++) {
      const dx = xValues[i] - meanX;
      const dy = yValues[i] - meanY;
      numerator += dx * dy;
      denominator += dx * dx;
    }

    return denominator !== 0 ? numerator / denominator : 0;
  }

  function normalizeSensitivity(slope, paramRange) {
    const expectedMetricRange = 1.0;
    const normalizedSlope = slope * paramRange / expectedMetricRange;
    return Math.max(-1, Math.min(1, normalizedSlope));
  }

  function computeRSquared(xValues, yValues, slope) {
    const n = xValues.length;
    if (n < 2) return 0;

    const meanX = xValues.reduce((a, b) => a + b, 0) / n;
    const meanY = yValues.reduce((a, b) => a + b, 0) / n;

    const predictions = xValues.map(x => meanY + slope * (x - meanX));
    const ssTotal = yValues.reduce((sum, y) => sum + (y - meanY) ** 2, 0);
    const ssResidual = yValues.reduce((sum, y, i) => sum + (y - predictions[i]) ** 2, 0);

    return ssTotal > 0 ? 1 - ssResidual / ssTotal : 0;
  }

  describe('computeSlope', () => {
    test('computes positive slope for increasing data', () => {
      const x = [0, 1, 2, 3, 4];
      const y = [0, 0.25, 0.5, 0.75, 1];
      const slope = computeSlope(x, y);
      expect(slope).toBeCloseTo(0.25, 5);
    });

    test('computes negative slope for decreasing data', () => {
      const x = [0, 1, 2, 3, 4];
      const y = [1, 0.75, 0.5, 0.25, 0];
      const slope = computeSlope(x, y);
      expect(slope).toBeCloseTo(-0.25, 5);
    });

    test('returns zero for flat data', () => {
      const x = [0, 1, 2, 3, 4];
      const y = [0.5, 0.5, 0.5, 0.5, 0.5];
      const slope = computeSlope(x, y);
      expect(slope).toBeCloseTo(0, 5);
    });

    test('returns zero for single point', () => {
      const slope = computeSlope([0.5], [0.5]);
      expect(slope).toBe(0);
    });

    test('returns zero for empty arrays', () => {
      const slope = computeSlope([], []);
      expect(slope).toBe(0);
    });

    test('handles noisy data', () => {
      // y = 0.5x + noise
      const x = [0, 1, 2, 3, 4];
      const y = [0.1, 0.4, 1.1, 1.4, 2.0];
      const slope = computeSlope(x, y);
      expect(slope).toBeGreaterThan(0.4);
      expect(slope).toBeLessThan(0.6);
    });
  });

  describe('normalizeSensitivity', () => {
    test('normalizes positive slope within range', () => {
      // Slope of 2.0 over parameter range 0.4 -> 0.8
      const normalized = normalizeSensitivity(2.0, 0.4);
      expect(normalized).toBeCloseTo(0.8, 5);
    });

    test('clamps to +1.0 for large positive values', () => {
      const normalized = normalizeSensitivity(10.0, 0.5);
      expect(normalized).toBe(1.0);
    });

    test('clamps to -1.0 for large negative values', () => {
      const normalized = normalizeSensitivity(-10.0, 0.5);
      expect(normalized).toBe(-1.0);
    });

    test('returns zero for zero slope', () => {
      const normalized = normalizeSensitivity(0, 0.4);
      expect(normalized).toBe(0);
    });

    test('scales with parameter range', () => {
      // Same slope, different ranges
      const narrow = normalizeSensitivity(2.0, 0.2);
      const wide = normalizeSensitivity(2.0, 0.4);
      expect(wide).toBeCloseTo(narrow * 2, 5);
    });
  });

  describe('computeRSquared', () => {
    test('returns 1.0 for perfect linear fit', () => {
      const x = [0, 1, 2, 3, 4];
      const y = [0, 0.25, 0.5, 0.75, 1];
      const slope = computeSlope(x, y);
      const r2 = computeRSquared(x, y, slope);
      expect(r2).toBeCloseTo(1.0, 5);
    });

    test('returns low value for poor fit', () => {
      const x = [0, 1, 2, 3, 4];
      const y = [0.5, 0.1, 0.9, 0.2, 0.7]; // Random-ish
      const slope = computeSlope(x, y);
      const r2 = computeRSquared(x, y, slope);
      expect(r2).toBeLessThan(0.3);
    });

    test('returns 0 for constant y values', () => {
      const x = [0, 1, 2, 3, 4];
      const y = [0.5, 0.5, 0.5, 0.5, 0.5];
      const slope = computeSlope(x, y);
      const r2 = computeRSquared(x, y, slope);
      expect(r2).toBe(0);
    });

    test('returns 0 for insufficient data', () => {
      const r2 = computeRSquared([1], [0.5], 0);
      expect(r2).toBe(0);
    });
  });

  describe('Lever Identification', () => {
    // Simulating the lever categorization logic
    function categorizeLever(sensitivity) {
      const abs = Math.abs(sensitivity);
      if (abs >= 0.3) return 'primary';
      if (abs >= 0.1) return 'secondary';
      return 'lowImpact';
    }

    test('classifies high sensitivity as primary', () => {
      expect(categorizeLever(0.5)).toBe('primary');
      expect(categorizeLever(-0.4)).toBe('primary');
      expect(categorizeLever(0.3)).toBe('primary');
    });

    test('classifies medium sensitivity as secondary', () => {
      expect(categorizeLever(0.2)).toBe('secondary');
      expect(categorizeLever(-0.15)).toBe('secondary');
      expect(categorizeLever(0.1)).toBe('secondary');
    });

    test('classifies low sensitivity as lowImpact', () => {
      expect(categorizeLever(0.05)).toBe('lowImpact');
      expect(categorizeLever(-0.09)).toBe('lowImpact');
      expect(categorizeLever(0)).toBe('lowImpact');
    });
  });

  describe('Integration scenarios', () => {
    test('typical syncopation parameter sweep', () => {
      // Simulated sweep: syncopationCenter from 0.3 to 0.7
      // Syncopation metric increases as center moves toward 0.5
      const x = [0.3, 0.4, 0.5, 0.6, 0.7];
      const y = [0.2, 0.35, 0.45, 0.4, 0.25]; // Bell curve response

      const slope = computeSlope(x, y);
      const normalized = normalizeSensitivity(slope, 0.4);

      // Should be near zero (bell curve has no linear trend)
      expect(Math.abs(normalized)).toBeLessThan(0.2);
    });

    test('typical randomFadeStart parameter sweep', () => {
      // Simulated sweep: randomFadeStart from 0.3 to 0.7
      // Regularity decreases as random algorithm kicks in earlier
      const x = [0.3, 0.4, 0.5, 0.6, 0.7];
      const y = [0.85, 0.75, 0.65, 0.55, 0.45]; // Decreasing regularity

      const slope = computeSlope(x, y);
      const normalized = normalizeSensitivity(slope, 0.4);

      // Should show strong negative sensitivity
      expect(normalized).toBeLessThan(-0.3);
    });
  });
});
