/**
 * Tests for pattern grid rendering
 */

import { renderPatternGrid, getPatternFromClick } from '../public/js/pattern-grid.js';

const mockPattern = {
  params: { length: 16 },
  hits: { v1: 4, v2: 3, aux: 8 },
  steps: [
    { v1: true, v1Vel: 1.0, v2: false, v2Vel: 0, aux: true, auxVel: 0.8 },
    { v1: false, v1Vel: 0, v2: true, v2Vel: 0.7, aux: true, auxVel: 0.6 },
    { v1: false, v1Vel: 0, v2: false, v2Vel: 0, aux: false, auxVel: 0 },
    { v1: false, v1Vel: 0, v2: false, v2Vel: 0, aux: true, auxVel: 0.5 },
    { v1: true, v1Vel: 0.9, v2: false, v2Vel: 0, aux: true, auxVel: 0.7 },
    { v1: false, v1Vel: 0, v2: true, v2Vel: 0.6, aux: true, auxVel: 0.6 },
    { v1: false, v1Vel: 0, v2: false, v2Vel: 0, aux: false, auxVel: 0 },
    { v1: false, v1Vel: 0, v2: false, v2Vel: 0, aux: true, auxVel: 0.4 },
    { v1: true, v1Vel: 0.8, v2: false, v2Vel: 0, aux: true, auxVel: 0.8 },
    { v1: false, v1Vel: 0, v2: true, v2Vel: 0.5, aux: true, auxVel: 0.5 },
    { v1: false, v1Vel: 0, v2: false, v2Vel: 0, aux: false, auxVel: 0 },
    { v1: false, v1Vel: 0, v2: false, v2Vel: 0, aux: false, auxVel: 0 },
    { v1: true, v1Vel: 0.7, v2: false, v2Vel: 0, aux: false, auxVel: 0 },
    { v1: false, v1Vel: 0, v2: false, v2Vel: 0, aux: false, auxVel: 0 },
    { v1: false, v1Vel: 0, v2: false, v2Vel: 0, aux: false, auxVel: 0 },
    { v1: false, v1Vel: 0, v2: false, v2Vel: 0, aux: false, auxVel: 0 },
  ],
};

describe('renderPatternGrid', () => {
  test('renders pattern grid container', () => {
    const html = renderPatternGrid(mockPattern);
    expect(html).toContain('class="pattern-grid"');
  });

  test('renders all three voice rows', () => {
    const html = renderPatternGrid(mockPattern);
    expect(html).toContain('voice-label v1');
    expect(html).toContain('voice-label v2');
    expect(html).toContain('voice-label aux');
  });

  test('renders hit counts by default', () => {
    const html = renderPatternGrid(mockPattern);
    expect(html).toContain('hit-counts');
    expect(html).toContain('V1: 4/16');
    expect(html).toContain('V2: 3/16');
    expect(html).toContain('AUX: 8/16');
  });

  test('hides hit counts when compact', () => {
    const html = renderPatternGrid(mockPattern, { compact: true });
    expect(html).not.toContain('hit-counts');
  });

  test('adds selectable class when patternId provided', () => {
    const html = renderPatternGrid(mockPattern, { patternId: 'test-1' });
    expect(html).toContain('selectable');
  });

  test('adds data attributes when patternId provided', () => {
    const html = renderPatternGrid(mockPattern, { patternId: 'test-1', name: 'Test Pattern' });
    expect(html).toContain('data-pattern-id="test-1"');
    expect(html).toContain('data-pattern-name="Test Pattern"');
  });

  test('adds selected class when selectedId matches', () => {
    const html = renderPatternGrid(mockPattern, { patternId: 'test-1', selectedId: 'test-1' });
    expect(html).toContain('selected');
  });

  test('does not add selected class when selectedId differs', () => {
    const html = renderPatternGrid(mockPattern, { patternId: 'test-1', selectedId: 'test-2' });
    expect(html).not.toContain('selected');
  });

  test('marks hit steps correctly', () => {
    const html = renderPatternGrid(mockPattern);
    expect(html).toContain('step hit');
    expect(html).toContain('step empty');
  });

  test('marks downbeats correctly', () => {
    const html = renderPatternGrid(mockPattern);
    expect(html).toContain('downbeat');
  });
});

describe('getPatternFromClick', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div class="pattern-grid selectable" data-pattern-id="preset-0" data-pattern-name="Four on Floor">
        <div class="step hit"></div>
      </div>
      <div class="pattern-grid">
        <div class="step hit"></div>
      </div>
    `;
  });

  test('returns pattern info from selectable grid click', () => {
    const target = document.querySelector('.pattern-grid.selectable .step');
    const result = getPatternFromClick({ target });

    expect(result).toEqual({
      patternId: 'preset-0',
      patternName: 'Four on Floor',
    });
  });

  test('returns null for non-selectable grid click', () => {
    const target = document.querySelector('.pattern-grid:not(.selectable) .step');
    const result = getPatternFromClick({ target });

    expect(result).toBeNull();
  });

  test('returns null for click outside grid', () => {
    const result = getPatternFromClick({ target: document.body });
    expect(result).toBeNull();
  });

  test('uses default name if not provided', () => {
    document.body.innerHTML = `
      <div class="pattern-grid selectable" data-pattern-id="test-1">
        <div class="step"></div>
      </div>
    `;
    const target = document.querySelector('.step');
    const result = getPatternFromClick({ target });

    expect(result.patternName).toBe('Pattern');
  });
});
