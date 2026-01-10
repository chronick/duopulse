# Design Session: Pentagon of Musicality Metrics

**Session ID**: pentagon-of-musicality
**Status**: Design Complete (v2 - Revised per Critic Feedback)
**Created**: 2026-01-09
**Revised**: 2026-01-09
**Author**: Design Iteration Architect

## Problem Statement

The expressiveness evaluation script (`scripts/evaluate-expressiveness.py`) currently measures:
- Hit counts and masks
- Syncopation ratio (simple offbeat percentage)
- Gap analysis
- Velocity metrics (average and range)

**Goal**: Add a "Pentagon of Musicality" - 5 normalized (0-1) metrics that capture **orthogonal** dimensions of rhythmic interest, suitable for radar chart visualization.

## Design Principles (v2)

1. **Orthogonality**: Each metric measures an independent dimension - no double-counting
2. **Technique vs. Dimension**: Metrics measure *perceptual effects*, not *compositional techniques*
3. **Weighted Sum over Area**: Use weighted sum for composite score (area calculation punishes zeros catastrophically)
4. **SHAPE-Zone Validation**: Metrics validate that patterns match expected behavior for their SHAPE zone

## The Five Orthogonal Metrics (Revised)

| Metric | Measures | Formula Basis | Independence |
|--------|----------|---------------|--------------|
| **Syncopation** | Deviation from metric grid | LHL-weighted weak-position hits | Measures *where* hits fall |
| **Density** | Overall activity level | `hits / pattern_length` | Measures *how many* hits |
| **Velocity Range** | Dynamic contrast | `max(vel) - min(vel)` | Measures *loudness variation* |
| **Voice Separation** | Multi-voice independence | `1 - overlap_ratio` | Measures *voice relationship* |
| **Regularity** | Temporal predictability | `1 - CV(gaps)` | Measures *spacing consistency* |

### Why These Are Orthogonal

These five dimensions span distinct musical qualities:

| Combination | Musical Result |
|-------------|----------------|
| High syncopation + High regularity | Reggae one-drop (displaced but even) |
| High density + Low syncopation | Four-on-floor techno |
| High velocity range + Low density | Sparse with dynamics |
| High voice separation + Low regularity | Polyrhythmic IDM |
| Low syncopation + Low regularity + High density | Breakbeat |

### Previous Metrics Collapsed

The v1 design had overlapping metrics that were collapsed:

| v1 Metric | Issue | Resolution |
|-----------|-------|------------|
| Syncopation | Overlapped with Anticipation | Keep Syncopation (LHL model) |
| Anticipation | Form of syncopation (Huron 2006) | Absorbed into Syncopation |
| Rhythmic Complexity | Overlapped with Regularity | Replaced by Regularity (inverse) |
| Dynamic Texture | Conflated density + variation | Split: Density + Velocity Range |
| Voice Interplay | Kept | Renamed: Voice Separation |

## Target Ranges by SHAPE Zone (v2 - Tightened)

| Metric | Stable (0-30%) | Syncopated (30-70%) | Wild (70-100%) |
|--------|----------------|---------------------|----------------|
| Syncopation | 0.00-0.22 | 0.22-0.48 | 0.42-0.75 |
| Density | 0.15-0.32 | 0.25-0.48 | 0.32-0.65 |
| Velocity Range | 0.12-0.38 | 0.32-0.58 | 0.25-0.72 |
| Voice Separation | 0.62-0.88 | 0.52-0.78 | 0.32-0.68 |
| Regularity | 0.72-1.00 | 0.42-0.68 | 0.12-0.48 |

**Note**: Ranges tightened from v1 for more discriminating hill-climbing feedback.

## Composite Scoring

**Weighted Sum** (not pentagon area):

```python
composite = (
    0.25 * syncopation_score +
    0.15 * density_score +
    0.20 * velocity_range_score +
    0.20 * voice_separation_score +
    0.20 * regularity_score
)
```

Weights reflect musical importance:
- Syncopation highest (defines groove character)
- Density lowest (less perceptually salient)
- Others equal

## Trills: Diagnostic Only

Trills are NOT a metric axis. They are a **compositional technique** that manifests through existing metrics:
- Trills increase Syncopation (if on weak beats)
- Trills decrease Regularity (IOI variance increases)
- Trills may affect Velocity Range (if shaped envelopes)

A separate `TrillDiagnostics` class provides visibility without polluting the musicality framework:

```python
@dataclass
class TrillDiagnostics:
    consecutive_hit_count: int
    max_run_length: int
    trill_positions: list[int]
    voice_distribution: dict[str, int]
```

## Session Files

| File | Purpose |
|------|---------|
| `README.md` | This file - session overview |
| `metric-formulas.md` | Detailed metric definitions and formulas |
| `trill-diagnostics.md` | Non-metric trill analysis (diagnostic only) |

## Key Design Decisions

1. **Orthogonality over intuition**: Collapsed overlapping metrics even though "Anticipation" felt distinct
2. **Weighted sum over area**: Pentagon area punishes zeros; weighted sum is more robust
3. **Regularity as inverse of Complexity**: Clearer semantics (high = predictable, low = chaotic)
4. **Trills as diagnostic**: Technique measurement separated from musicality evaluation
5. **SHAPE-zone validation**: Metrics validate zone promises, not absolute quality
