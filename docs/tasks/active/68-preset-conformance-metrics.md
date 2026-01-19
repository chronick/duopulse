---
id: 68
slug: preset-conformance-metrics
title: "Preset Conformance Metrics"
status: pending
created_date: 2026-01-19
updated_date: 2026-01-19
branch: feature/preset-conformance-metrics
spec_refs: ["Appendix: Style Preset Map", "14. Testing Requirements"]
---

# Task 68: Preset Conformance Metrics

## Objective

Add a preset conformance metric system that grounds evaluation in how well generated patterns match the expected characteristics for each preset name (e.g., "Four on Floor" should produce kicks on beats 0, 4, 8, 12; "Minimal Techno" should be sparse and regular). **Conformance scores will contribute to the overall alignment metric**, providing a direct measure of how well presets achieve their intended musical character.

## Context

Current state analysis shows:
- **Overall alignment is POOR** (39.6% per baseline.json)
- **Presets don't match their names**: "Four on Floor" shows syncopation=0 but regularity=0.55 (should be ~1.0)
- **Most presets have composite scores below 0.2**, indicating they fail to produce patterns matching their descriptions
- The existing Pentagon metrics evaluate general musicality but not preset-specific correctness

The current presets and their issues:
| Preset | Current Composite | Key Problems |
|--------|------------------|--------------|
| Minimal Techno | 0.18 | Density too high (0.41 vs 0.15-0.32), regularity too low |
| Four on Floor | 0.08 | Doesn't produce 4-on-floor kick pattern, regularity=0.55 |
| Driving Techno | 0.13 | Regularity=0.28 (should be high for techno) |
| Syncopated Groove | 0.14 | Syncopation=0 (should be >0.22 for syncopated zone) |
| Broken Beat | 0.10 | Syncopation=0, should have displaced rhythms |
| Chaotic IDM | 0.20 | Syncopation=0, regularity too high (0.64 vs 0.12-0.48) |
| Sparse Dub | 0.18 | Syncopation=0 for syncopated zone preset |
| Dense Gabber | 0.40 | Best performer, still below 0.5 threshold |

## Implementation Plan

### Phase 1: Reference Pattern Definitions

Define canonical reference patterns for each preset based on their musical meaning:

```javascript
const PRESET_REFERENCES = {
  'Four on Floor': {
    anchorPattern: [0, 8, 16, 24, 32, 40, 48, 56], // Kicks on every quarter note
    shimmerPattern: null, // Any reasonable pattern
    tolerance: 'strict', // Stable zone = strict conformance
    requiredHits: { anchor: [0, 16, 32, 48] }, // Beat 1 of each bar minimum
    metrics: {
      regularity: { min: 0.85, weight: 0.4 },
      syncopation: { max: 0.15, weight: 0.3 },
      density: { target: 0.25, tolerance: 0.15, weight: 0.3 }
    }
  },
  'Minimal Techno': {
    anchorPattern: [0, 16, 32, 48], // Sparse kicks on downbeats
    tolerance: 'strict',
    requiredHits: { anchor: [0, 32] }, // At least beat 1 of bars 1 and 3
    metrics: {
      regularity: { min: 0.75, weight: 0.3 },
      density: { max: 0.30, weight: 0.4 },
      syncopation: { max: 0.15, weight: 0.3 }
    }
  },
  'Syncopated Groove': {
    tolerance: 'moderate',
    metrics: {
      syncopation: { min: 0.25, weight: 0.5 },
      regularity: { range: [0.40, 0.70], weight: 0.3 },
      voiceSeparation: { min: 0.50, weight: 0.2 }
    }
  },
  'Broken Beat': {
    tolerance: 'moderate',
    metrics: {
      syncopation: { min: 0.35, weight: 0.4 },
      regularity: { max: 0.55, weight: 0.3 },
      density: { range: [0.35, 0.65], weight: 0.3 }
    }
  },
  'Chaotic IDM': {
    tolerance: 'lax',
    metrics: {
      syncopation: { min: 0.40, weight: 0.3 },
      regularity: { max: 0.45, weight: 0.3 },
      velocityRange: { min: 0.30, weight: 0.2 },
      density: { min: 0.45, weight: 0.2 }
    }
  },
  // ... etc
};
```

### Phase 2: Conformance Scoring Functions

```javascript
// Pattern match scoring (0-1 based on Hamming distance to reference)
function computePatternMatch(generated, reference, tolerance) {
  if (!reference) return 1.0; // No reference = any pattern OK
  
  const hammingDistance = countDifferentBits(generated, reference);
  const maxDistance = reference.length;
  
  const toleranceMultiplier = {
    strict: 1.0,   // Exact match important
    moderate: 1.5, // Some variation allowed
    lax: 2.5       // Wide variation acceptable
  }[tolerance];
  
  return Math.max(0, 1.0 - (hammingDistance / (maxDistance * toleranceMultiplier)));
}

// Required hits check (0 or 1 - pass/fail)
function checkRequiredHits(pattern, requiredHits) {
  for (const pos of requiredHits) {
    if (!pattern[pos]) return 0;
  }
  return 1;
}

// Metric conformance (0-1 based on distance from target)
function computeMetricConformance(actual, spec) {
  if (spec.target !== undefined) {
    const distance = Math.abs(actual - spec.target);
    return Math.max(0, 1.0 - distance / (spec.tolerance || 0.2));
  }
  if (spec.min !== undefined && actual < spec.min) {
    return Math.max(0, 1.0 - (spec.min - actual) * 3);
  }
  if (spec.max !== undefined && actual > spec.max) {
    return Math.max(0, 1.0 - (actual - spec.max) * 3);
  }
  if (spec.range !== undefined) {
    const [min, max] = spec.range;
    if (actual >= min && actual <= max) return 1.0;
    const distance = actual < min ? min - actual : actual - max;
    return Math.max(0, 1.0 - distance * 2);
  }
  return 1.0;
}
```

### Phase 3: Composite Conformance Score

```javascript
function computePresetConformance(presetName, pattern, pentagonMetrics) {
  const ref = PRESET_REFERENCES[presetName];
  if (!ref) return { score: 0.5, status: 'unknown' }; // No reference defined
  
  let weightedScore = 0;
  let totalWeight = 0;
  const breakdown = {};
  
  // Pattern match score
  if (ref.anchorPattern) {
    const patternScore = computePatternMatch(
      pattern.masks.v1,
      ref.anchorPattern,
      ref.tolerance
    );
    breakdown.patternMatch = patternScore;
    weightedScore += patternScore * 0.3;
    totalWeight += 0.3;
  }
  
  // Required hits check
  if (ref.requiredHits) {
    const requiredScore = checkRequiredHits(
      pattern.steps.filter(s => s.v1).map(s => s.step),
      ref.requiredHits.anchor || []
    );
    breakdown.requiredHits = requiredScore;
    weightedScore += requiredScore * 0.2;
    totalWeight += 0.2;
  }
  
  // Metric conformance
  for (const [metric, spec] of Object.entries(ref.metrics || {})) {
    const actual = pentagonMetrics.raw[metric];
    const score = computeMetricConformance(actual, spec);
    breakdown[metric] = score;
    weightedScore += score * spec.weight;
    totalWeight += spec.weight;
  }
  
  const finalScore = totalWeight > 0 ? weightedScore / totalWeight : 0.5;
  
  return {
    score: finalScore,
    breakdown,
    tolerance: ref.tolerance,
    pass: finalScore >= getPassThreshold(ref.tolerance),
    status: finalScore >= 0.8 ? 'excellent' : 
            finalScore >= 0.6 ? 'good' :
            finalScore >= 0.4 ? 'fair' : 'poor'
  };
}

function getPassThreshold(tolerance) {
  return { strict: 0.75, moderate: 0.60, lax: 0.50 }[tolerance] || 0.60;
}
```

### Phase 4: Integration with Evaluation Pipeline

Add to `evaluate-expressiveness.js`:
- Import preset reference definitions
- Compute conformance for each preset pattern
- Add to `preset-metrics.json` output
- Add overall preset conformance score to `expressiveness.json`
- **Integrate conformance into overall alignment calculation** - conformance should be a weighted component alongside pentagon metrics
- Report conformance in summary output

### Phase 5: Dashboard Visualization

Add to dashboard:
- Conformance score per preset (bar chart)
- Breakdown of what's failing (pattern match, metrics, required hits)
- Color coding: green (pass), yellow (close), red (fail)

## Subtasks

- [x] Define reference patterns for all 8 presets in `preset-references.js`
- [x] Implement `computePatternMatch()` function with tolerance levels
- [x] Implement `checkRequiredHits()` function
- [x] Implement `computeMetricConformance()` function
- [x] Implement `computePresetConformance()` composite function
- [x] Integrate into `evaluate-expressiveness.js`
- [x] **Add conformance to overall alignment calculation** (weighted 40% conformance, 60% pentagon)
- [x] Add conformance data to output files (preset-metrics.json, expressiveness.json)
- [x] Update dashboard to display conformance metrics
- [x] Add unit tests for conformance functions
- [x] All tests pass

## Acceptance Criteria

- [ ] Each preset has a defined reference pattern or metric expectations
- [ ] Conformance scores are computed and output alongside pentagon metrics
- [ ] **Conformance contributes to overall alignment calculation** with appropriate weighting
- [ ] Tolerance levels (strict/moderate/lax) are respected per preset zone
- [ ] Dashboard shows conformance breakdown per preset
- [ ] Overall preset conformance score is added to expressiveness summary
- [ ] "Four on Floor" conformance correctly penalizes non-4-on-floor patterns
- [ ] Stable zone presets (Minimal Techno, Four on Floor, Driving Techno) use strict tolerance
- [ ] Wild zone presets (Chaotic IDM) use lax tolerance
- [ ] Overall alignment score improves when presets better match their expected characteristics
- [ ] All tests pass
- [ ] No new warnings
- [ ] Code review passes

## Implementation Notes

### Files to Modify

1. **New file**: `tools/evals/preset-references.js` - Define reference patterns
2. **Modify**: `tools/evals/evaluate-expressiveness.js` - Add conformance computation
3. **Modify**: `tools/evals/public/app.js` - Dashboard conformance display
4. **New file**: `tools/evals/tests/conformance.test.js` - Unit tests

### Tolerance Level Mapping

| Preset | SHAPE Zone | Tolerance |
|--------|------------|-----------|
| Minimal Techno | stable (0.15) | strict |
| Four on Floor | stable (0.0) | strict |
| Driving Techno | stable (0.2) | strict |
| Syncopated Groove | syncopated (0.5) | moderate |
| Sparse Dub | syncopated (0.3) | moderate |
| Dense Gabber | syncopated (0.4) | moderate |
| Broken Beat | wild (0.7) | moderate |
| Chaotic IDM | wild (0.9) | lax |

### Pass Thresholds

- **strict**: 0.75 (stable patterns must closely match expectations)
- **moderate**: 0.60 (syncopated allows more variation)
- **lax**: 0.50 (wild is inherently chaotic, lower bar)

### Overall Alignment Integration

Conformance should be integrated into the overall alignment calculation as a weighted component:

```javascript
// Current alignment uses pentagon metrics exclusively
// New alignment should combine pentagon metrics + conformance

function computeOverallAlignment(presets) {
  let pentagonScore = 0;
  let conformanceScore = 0;

  for (const preset of presets) {
    // Existing pentagon alignment
    pentagonScore += computePentagonAlignment(preset);

    // New conformance alignment
    conformanceScore += preset.conformance.score;
  }

  pentagonScore /= presets.length;
  conformanceScore /= presets.length;

  // Weighted combination
  // Start with 60/40 pentagon/conformance split
  const overallAlignment = (pentagonScore * 0.6) + (conformanceScore * 0.4);

  return {
    overall: overallAlignment,
    pentagon: pentagonScore,
    conformance: conformanceScore
  };
}
```

**Rationale**: Pentagon metrics measure general musical quality (regularity, syncopation, etc.), while conformance measures preset-specific correctness. Both are important for alignment, but pentagon metrics capture broader musical principles, so they get higher weight initially. This weighting can be tuned during iteration.

### Dependencies

This task depends on the existing pattern generation and Pentagon metrics infrastructure:
- `tools/evals/generate-patterns.js` (generates preset data)
- `tools/evals/evaluate-expressiveness.js` (Pentagon metrics)
- `metrics/baseline.json` (current baseline for comparison)

No blocking dependencies on other tasks.
