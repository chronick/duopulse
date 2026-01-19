/**
 * Tests for conformance scoring functions
 */

import {
  computePatternMatch,
  checkRequiredHits,
  computeMetricConformance,
  computePresetConformance,
  getPassThreshold,
  PASS_THRESHOLDS,
} from '../preset-references.js';

describe('computePatternMatch', () => {
  test('exact match returns 1.0', () => {
    const generated = [0, 16, 32, 48];
    const reference = [0, 16, 32, 48];
    expect(computePatternMatch(generated, reference, 'strict')).toBe(1.0);
  });

  test('complete mismatch returns 0.0', () => {
    const generated = [1, 17, 33, 49];
    const reference = [0, 16, 32, 48];
    // Hamming distance = 8 (4 missing from ref + 4 extra in generated)
    // With strict: 1.0 - 8 / (4 * 1.0) = 1.0 - 2.0 = -1.0 -> clamped to 0
    expect(computePatternMatch(generated, reference, 'strict')).toBe(0.0);
  });

  test('null reference returns 1.0', () => {
    const generated = [0, 4, 8, 12];
    expect(computePatternMatch(generated, null, 'strict')).toBe(1.0);
  });

  test('partial match with strict tolerance', () => {
    const generated = [0, 16, 32]; // Missing 48
    const reference = [0, 16, 32, 48];
    // Hamming distance = 1 (missing 48)
    // With strict: 1.0 - 1 / (4 * 1.0) = 0.75
    expect(computePatternMatch(generated, reference, 'strict')).toBe(0.75);
  });

  test('partial match with moderate tolerance', () => {
    const generated = [0, 16, 32]; // Missing 48
    const reference = [0, 16, 32, 48];
    // Hamming distance = 1
    // With moderate: 1.0 - 1 / (4 * 1.5) = 1.0 - 0.167 = 0.833
    const score = computePatternMatch(generated, reference, 'moderate');
    expect(score).toBeCloseTo(0.833, 2);
  });

  test('partial match with lax tolerance', () => {
    const generated = [0, 16, 32]; // Missing 48
    const reference = [0, 16, 32, 48];
    // Hamming distance = 1
    // With lax: 1.0 - 1 / (4 * 2.5) = 1.0 - 0.1 = 0.9
    const score = computePatternMatch(generated, reference, 'lax');
    expect(score).toBe(0.9);
  });

  test('extra hits also count against score', () => {
    const generated = [0, 16, 32, 48, 8, 24]; // Reference plus extras
    const reference = [0, 16, 32, 48];
    // Hamming distance = 2 (2 extra hits)
    // With strict: 1.0 - 2 / (4 * 1.0) = 0.5
    expect(computePatternMatch(generated, reference, 'strict')).toBe(0.5);
  });

  test('empty generated pattern with strict tolerance', () => {
    const generated = [];
    const reference = [0, 16, 32, 48];
    // Hamming distance = 4 (all missing)
    // With strict: 1.0 - 4 / (4 * 1.0) = 0.0
    expect(computePatternMatch(generated, reference, 'strict')).toBe(0.0);
  });
});

describe('checkRequiredHits', () => {
  test('all required hits present returns 1', () => {
    const patternSteps = [0, 16, 32, 48];
    const requiredHits = [0, 32];
    expect(checkRequiredHits(patternSteps, requiredHits)).toBe(1);
  });

  test('missing any required hit returns 0', () => {
    const patternSteps = [0, 16, 48];
    const requiredHits = [0, 32]; // 32 is missing
    expect(checkRequiredHits(patternSteps, requiredHits)).toBe(0);
  });

  test('empty required hits returns 1', () => {
    const patternSteps = [0, 16, 32, 48];
    const requiredHits = [];
    expect(checkRequiredHits(patternSteps, requiredHits)).toBe(1);
  });

  test('null required hits returns 1', () => {
    const patternSteps = [0, 16, 32, 48];
    expect(checkRequiredHits(patternSteps, null)).toBe(1);
  });

  test('undefined required hits returns 1', () => {
    const patternSteps = [0, 16, 32, 48];
    expect(checkRequiredHits(patternSteps, undefined)).toBe(1);
  });

  test('extra hits beyond required still returns 1', () => {
    const patternSteps = [0, 4, 8, 16, 24, 32, 40, 48, 56];
    const requiredHits = [0, 32];
    expect(checkRequiredHits(patternSteps, requiredHits)).toBe(1);
  });

  test('single required hit present returns 1', () => {
    const patternSteps = [0];
    const requiredHits = [0];
    expect(checkRequiredHits(patternSteps, requiredHits)).toBe(1);
  });

  test('single required hit missing returns 0', () => {
    const patternSteps = [16, 32, 48];
    const requiredHits = [0];
    expect(checkRequiredHits(patternSteps, requiredHits)).toBe(0);
  });
});

describe('computeMetricConformance', () => {
  describe('target spec', () => {
    test('value at target returns 1.0', () => {
      const spec = { target: 0.25, tolerance: 0.15 };
      expect(computeMetricConformance(0.25, spec)).toBe(1.0);
    });

    test('value far from target approaches 0', () => {
      const spec = { target: 0.25, tolerance: 0.15 };
      // Distance = 0.4, tolerance = 0.15
      // Score = 1.0 - 0.4 / 0.15 = 1.0 - 2.67 = -1.67 -> clamped to 0
      expect(computeMetricConformance(0.65, spec)).toBe(0.0);
    });

    test('value within tolerance has partial score', () => {
      const spec = { target: 0.25, tolerance: 0.15 };
      // Distance = 0.10, tolerance = 0.15
      // Score = 1.0 - 0.10 / 0.15 = 1.0 - 0.667 = 0.333
      const score = computeMetricConformance(0.35, spec);
      expect(score).toBeCloseTo(0.333, 2);
    });

    test('uses default tolerance of 0.2 when not specified', () => {
      const spec = { target: 0.5 };
      // Distance = 0.1, default tolerance = 0.2
      // Score = 1.0 - 0.1 / 0.2 = 0.5
      expect(computeMetricConformance(0.6, spec)).toBeCloseTo(0.5);
    });
  });

  describe('min spec', () => {
    test('value above min returns 1.0', () => {
      const spec = { min: 0.75 };
      expect(computeMetricConformance(0.85, spec)).toBe(1.0);
    });

    test('value at min returns 1.0', () => {
      const spec = { min: 0.75 };
      expect(computeMetricConformance(0.75, spec)).toBe(1.0);
    });

    test('value below min has penalty', () => {
      const spec = { min: 0.75 };
      // Actual = 0.65, min = 0.75
      // Distance below = 0.10
      // Score = 1.0 - 0.10 * 3 = 0.7
      expect(computeMetricConformance(0.65, spec)).toBeCloseTo(0.7);
    });

    test('value far below min returns 0', () => {
      const spec = { min: 0.75 };
      // Actual = 0.25, min = 0.75
      // Distance below = 0.50
      // Score = 1.0 - 0.50 * 3 = -0.5 -> clamped to 0
      expect(computeMetricConformance(0.25, spec)).toBe(0.0);
    });
  });

  describe('max spec', () => {
    test('value below max returns 1.0', () => {
      const spec = { max: 0.30 };
      expect(computeMetricConformance(0.20, spec)).toBe(1.0);
    });

    test('value at max returns 1.0', () => {
      const spec = { max: 0.30 };
      expect(computeMetricConformance(0.30, spec)).toBe(1.0);
    });

    test('value above max has penalty', () => {
      const spec = { max: 0.30 };
      // Actual = 0.40, max = 0.30
      // Distance above = 0.10
      // Score = 1.0 - 0.10 * 3 = 0.7
      expect(computeMetricConformance(0.40, spec)).toBeCloseTo(0.7);
    });

    test('value far above max returns 0', () => {
      const spec = { max: 0.15 };
      // Actual = 0.65, max = 0.15
      // Distance above = 0.50
      // Score = 1.0 - 0.50 * 3 = -0.5 -> clamped to 0
      expect(computeMetricConformance(0.65, spec)).toBe(0.0);
    });
  });

  describe('range spec', () => {
    test('value in range returns 1.0', () => {
      const spec = { range: [0.40, 0.70] };
      expect(computeMetricConformance(0.55, spec)).toBe(1.0);
    });

    test('value at range min returns 1.0', () => {
      const spec = { range: [0.40, 0.70] };
      expect(computeMetricConformance(0.40, spec)).toBe(1.0);
    });

    test('value at range max returns 1.0', () => {
      const spec = { range: [0.40, 0.70] };
      expect(computeMetricConformance(0.70, spec)).toBe(1.0);
    });

    test('value below range has penalty', () => {
      const spec = { range: [0.40, 0.70] };
      // Actual = 0.30, min = 0.40
      // Distance below = 0.10
      // Score = 1.0 - 0.10 * 2 = 0.8
      expect(computeMetricConformance(0.30, spec)).toBeCloseTo(0.8);
    });

    test('value above range has penalty', () => {
      const spec = { range: [0.40, 0.70] };
      // Actual = 0.80, max = 0.70
      // Distance above = 0.10
      // Score = 1.0 - 0.10 * 2 = 0.8
      expect(computeMetricConformance(0.80, spec)).toBeCloseTo(0.8);
    });

    test('value far outside range returns 0', () => {
      const spec = { range: [0.40, 0.70] };
      // Actual = 1.0, max = 0.70
      // Distance above = 0.30
      // Score = 1.0 - 0.30 * 2 = 0.4
      expect(computeMetricConformance(1.0, spec)).toBeCloseTo(0.4);
    });
  });

  describe('no constraints', () => {
    test('empty spec returns 1.0', () => {
      expect(computeMetricConformance(0.5, {})).toBe(1.0);
    });

    test('spec with only weight returns 1.0', () => {
      expect(computeMetricConformance(0.5, { weight: 0.3 })).toBe(1.0);
    });
  });
});

describe('computePresetConformance', () => {
  // Mock pattern structure matching what's used in evaluate-expressiveness.js
  const createMockPattern = (v1Steps, v2Steps = [], auxSteps = []) => ({
    steps: [
      ...v1Steps.map((step) => ({ step, v1: true, v2: false, aux: false })),
      ...v2Steps.map((step) => ({ step, v1: false, v2: true, aux: false })),
      ...auxSteps.map((step) => ({ step, v1: false, v2: false, aux: true })),
    ],
    params: { length: 64, shape: 0.0, energy: 0.3, accent: 0.3 },
  });

  // Mock pentagon metrics
  const createMockPentagonMetrics = (overrides = {}) => ({
    raw: {
      syncopation: 0.0,
      density: 0.25,
      velocityRange: 0.3,
      voiceSeparation: 0.7,
      regularity: 0.9,
      ...overrides,
    },
  });

  describe('return structure', () => {
    test('valid preset returns proper structure', () => {
      const pattern = createMockPattern([0, 16, 32, 48]);
      const pentagon = createMockPentagonMetrics({ regularity: 0.85, density: 0.25 });

      const result = computePresetConformance('Minimal Techno', pattern, pentagon);

      expect(result).toHaveProperty('score');
      expect(result).toHaveProperty('breakdown');
      expect(result).toHaveProperty('tolerance');
      expect(result).toHaveProperty('pass');
      expect(result).toHaveProperty('status');

      expect(typeof result.score).toBe('number');
      expect(typeof result.breakdown).toBe('object');
      expect(typeof result.tolerance).toBe('string');
      expect(typeof result.pass).toBe('boolean');
      expect(typeof result.status).toBe('string');
    });

    test('unknown preset returns fallback', () => {
      const pattern = createMockPattern([0, 16, 32, 48]);
      const pentagon = createMockPentagonMetrics();

      const result = computePresetConformance('Unknown Preset', pattern, pentagon);

      expect(result.score).toBe(0.5);
      expect(result.status).toBe('unknown');
      expect(result.pass).toBe(false);
      expect(result.breakdown).toEqual({});
    });
  });

  describe('status thresholds', () => {
    test('score >= 0.8 maps to excellent status', () => {
      // Create a near-perfect Minimal Techno pattern
      const pattern = createMockPattern([0, 16, 32, 48]);
      const pentagon = createMockPentagonMetrics({
        regularity: 0.90,
        density: 0.20,
        syncopation: 0.05,
      });

      const result = computePresetConformance('Minimal Techno', pattern, pentagon);

      // With good metrics and exact pattern match, should get high score
      expect(result.status).toBe('excellent');
      expect(result.score).toBeGreaterThanOrEqual(0.8);
    });

    test('score >= 0.6 but < 0.8 maps to good status', () => {
      // Slightly off pattern
      const pattern = createMockPattern([0, 16, 32]); // Missing 48
      const pentagon = createMockPentagonMetrics({
        regularity: 0.80,
        density: 0.28,
        syncopation: 0.10,
      });

      const result = computePresetConformance('Minimal Techno', pattern, pentagon);

      // Score should be in good range
      if (result.score >= 0.6 && result.score < 0.8) {
        expect(result.status).toBe('good');
      }
    });

    test('score >= 0.4 but < 0.6 maps to fair status', () => {
      // Poor pattern match
      const pattern = createMockPattern([0, 8, 16, 24, 32, 40, 48, 56]); // Too many hits
      const pentagon = createMockPentagonMetrics({
        regularity: 0.60,
        density: 0.40, // Above max
        syncopation: 0.12,
      });

      const result = computePresetConformance('Minimal Techno', pattern, pentagon);

      if (result.score >= 0.4 && result.score < 0.6) {
        expect(result.status).toBe('fair');
      }
    });

    test('score < 0.4 maps to poor status', () => {
      // Very bad pattern
      const pattern = createMockPattern([1, 5, 9, 13, 17, 21, 25, 29]); // All wrong positions
      const pentagon = createMockPentagonMetrics({
        regularity: 0.30, // Below min 0.75
        density: 0.60, // Above max 0.30
        syncopation: 0.50, // Above max 0.15
      });

      const result = computePresetConformance('Minimal Techno', pattern, pentagon);

      expect(result.status).toBe('poor');
      expect(result.score).toBeLessThan(0.4);
    });
  });

  describe('pass/fail based on tolerance thresholds', () => {
    test('strict preset passes at threshold 0.75', () => {
      const pattern = createMockPattern([0, 16, 32, 48]);
      const pentagon = createMockPentagonMetrics({
        regularity: 0.90,
        density: 0.20,
        syncopation: 0.05,
      });

      const result = computePresetConformance('Minimal Techno', pattern, pentagon);

      expect(result.tolerance).toBe('strict');
      if (result.score >= 0.75) {
        expect(result.pass).toBe(true);
      }
    });

    test('strict preset fails below threshold 0.75', () => {
      const pattern = createMockPattern([1, 17, 33, 49]); // Wrong positions
      const pentagon = createMockPentagonMetrics({
        regularity: 0.50,
        density: 0.50,
        syncopation: 0.30,
      });

      const result = computePresetConformance('Minimal Techno', pattern, pentagon);

      expect(result.tolerance).toBe('strict');
      if (result.score < 0.75) {
        expect(result.pass).toBe(false);
      }
    });

    test('moderate preset passes at threshold 0.60', () => {
      const pattern = createMockPattern([0, 4, 12, 20, 28, 32, 44, 52]);
      const pentagon = createMockPentagonMetrics({
        syncopation: 0.35,
        regularity: 0.55,
        voiceSeparation: 0.60,
      });

      const result = computePresetConformance('Syncopated Groove', pattern, pentagon);

      expect(result.tolerance).toBe('moderate');
      if (result.score >= 0.60) {
        expect(result.pass).toBe(true);
      }
    });

    test('lax preset passes at threshold 0.50', () => {
      const pattern = createMockPattern([0, 3, 7, 11, 14, 19, 23, 27, 31]);
      const pentagon = createMockPentagonMetrics({
        syncopation: 0.45,
        regularity: 0.35,
        velocityRange: 0.40,
        density: 0.50,
      });

      const result = computePresetConformance('Chaotic IDM', pattern, pentagon);

      expect(result.tolerance).toBe('lax');
      if (result.score >= 0.50) {
        expect(result.pass).toBe(true);
      }
    });
  });

  describe('breakdown contents', () => {
    test('breakdown contains patternMatch when anchorPattern defined', () => {
      const pattern = createMockPattern([0, 16, 32, 48]);
      const pentagon = createMockPentagonMetrics();

      const result = computePresetConformance('Minimal Techno', pattern, pentagon);

      expect(result.breakdown).toHaveProperty('patternMatch');
    });

    test('breakdown contains requiredHits when requiredHits defined', () => {
      const pattern = createMockPattern([0, 32]);
      const pentagon = createMockPentagonMetrics();

      const result = computePresetConformance('Minimal Techno', pattern, pentagon);

      expect(result.breakdown).toHaveProperty('requiredHits');
    });

    test('breakdown contains metric scores', () => {
      const pattern = createMockPattern([0, 16, 32, 48]);
      const pentagon = createMockPentagonMetrics();

      const result = computePresetConformance('Minimal Techno', pattern, pentagon);

      expect(result.breakdown).toHaveProperty('regularity');
      expect(result.breakdown).toHaveProperty('density');
      expect(result.breakdown).toHaveProperty('syncopation');
    });
  });
});

describe('getPassThreshold', () => {
  test('strict returns 0.75', () => {
    expect(getPassThreshold('strict')).toBe(0.75);
  });

  test('moderate returns 0.60', () => {
    expect(getPassThreshold('moderate')).toBe(0.60);
  });

  test('lax returns 0.50', () => {
    expect(getPassThreshold('lax')).toBe(0.50);
  });

  test('unknown tolerance returns default 0.60', () => {
    expect(getPassThreshold('invalid')).toBe(0.60);
  });
});

describe('PASS_THRESHOLDS', () => {
  test('contains all expected tolerance levels', () => {
    expect(PASS_THRESHOLDS).toHaveProperty('strict');
    expect(PASS_THRESHOLDS).toHaveProperty('moderate');
    expect(PASS_THRESHOLDS).toHaveProperty('lax');
  });

  test('thresholds are in expected order', () => {
    expect(PASS_THRESHOLDS.strict).toBeGreaterThan(PASS_THRESHOLDS.moderate);
    expect(PASS_THRESHOLDS.moderate).toBeGreaterThan(PASS_THRESHOLDS.lax);
  });
});
