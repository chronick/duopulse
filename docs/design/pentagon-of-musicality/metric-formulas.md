# Pentagon of Musicality: Metric Formulas (v2)

**Version**: 2.0
**Date**: 2026-01-09
**Status**: Revised per critic feedback - orthogonal axes

## Overview

Five **orthogonal** metrics that capture independent dimensions of rhythmic musicality. Each is normalized to 0-1 with SHAPE-zone-dependent target ranges.

**Key Changes from v1**:
- Collapsed overlapping metrics (Anticipation absorbed into Syncopation)
- Split conflated metrics (Dynamic Texture → Density + Velocity Range)
- Renamed for clarity (Voice Interplay → Voice Separation, Complexity → Regularity)
- Weighted sum instead of pentagon area for composite

---

## Metric 1: Syncopation (LHL Model)

### Concept

Measures deviation from the metric grid using the Longuet-Higgins & Lee model. Syncopation occurs when a note on a weak metric position is NOT followed by a note on the subsequent stronger position.

### Metric Weight Table (32-step at 16th resolution)

```python
# Metric hierarchy for 32-step pattern (2 bars of 4/4 at 16th notes)
# Higher weight = stronger metric position
METRIC_WEIGHTS_32 = [
    1.00,  # 0:  Bar 1, Beat 1 (strongest)
    0.10,  # 1:  16th
    0.40,  # 2:  8th
    0.10,  # 3:  16th (anticipation of beat 2)
    0.80,  # 4:  Beat 2
    0.10,  # 5:  16th
    0.40,  # 6:  8th
    0.10,  # 7:  16th (anticipation of beat 3)
    0.90,  # 8:  Beat 3 (half-bar)
    0.10,  # 9:  16th
    0.40,  # 10: 8th
    0.10,  # 11: 16th (anticipation of beat 4)
    0.80,  # 12: Beat 4
    0.10,  # 13: 16th
    0.40,  # 14: 8th
    0.10,  # 15: 16th (anticipation of bar 2)
    0.95,  # 16: Bar 2, Beat 1
    0.10,  # 17: 16th
    0.40,  # 18: 8th
    0.10,  # 19: 16th
    0.80,  # 20: Beat 2
    0.10,  # 21: 16th
    0.40,  # 22: 8th
    0.10,  # 23: 16th
    0.90,  # 24: Beat 3
    0.10,  # 25: 16th
    0.40,  # 26: 8th
    0.10,  # 27: 16th
    0.80,  # 28: Beat 4
    0.10,  # 29: 16th
    0.40,  # 30: 8th
    0.10,  # 31: 16th (anticipation of next bar)
]
```

### Formula

```python
def compute_syncopation(hits: list[bool], pattern_length: int = 32) -> float:
    """
    Compute LHL syncopation score.

    Syncopation = sum of tension created by weak-to-strong transitions
    where a hit on a weak position is followed by a rest on a stronger position.
    """
    weights = METRIC_WEIGHTS_32[:pattern_length]

    syncopation_tension = 0.0
    max_possible_tension = 0.0

    for i in range(pattern_length):
        if hits[i]:
            next_pos = (i + 1) % pattern_length
            weight_diff = weights[next_pos] - weights[i]

            # Syncopation occurs when weak beat hit is followed by strong beat rest
            if weight_diff > 0 and not hits[next_pos]:
                syncopation_tension += weight_diff

            # Track maximum possible for normalization
            max_possible_tension += max(0, weights[(i + 1) % pattern_length] - weights[i])

    if max_possible_tension == 0:
        return 0.0

    return min(1.0, syncopation_tension / max_possible_tension)
```

### SHAPE-Zone Scoring

```python
def score_syncopation(raw: float, shape: float) -> float:
    """Score syncopation relative to SHAPE zone expectation."""
    if shape < 0.3:
        # Stable zone: low syncopation expected
        target_center, target_width = 0.12, 0.20
    elif shape < 0.7:
        # Syncopated zone: moderate syncopation expected
        target_center, target_width = 0.38, 0.25
    else:
        # Wild zone: high syncopation acceptable
        target_center, target_width = 0.60, 0.35

    distance = abs(raw - target_center)
    return max(0.0, 1.0 - (distance / target_width) ** 2)
```

---

## Metric 2: Density

### Concept

Measures overall rhythmic activity - what fraction of steps have hits. Simple but orthogonal to all other metrics.

### Formula

```python
def compute_density(
    v1_hits: list[bool],
    v2_hits: list[bool],
    aux_hits: list[bool],
    pattern_length: int = 32
) -> float:
    """
    Compute pattern density as fraction of active steps.

    A step is "active" if any voice has a hit.
    """
    active_steps = sum(
        1 for i in range(pattern_length)
        if v1_hits[i] or v2_hits[i] or aux_hits[i]
    )
    return active_steps / pattern_length
```

### Per-Voice Density (Diagnostic)

```python
def compute_voice_densities(
    v1_hits: list[bool],
    v2_hits: list[bool],
    aux_hits: list[bool],
    pattern_length: int = 32
) -> dict[str, float]:
    """Per-voice density breakdown."""
    return {
        'v1': sum(v1_hits) / pattern_length,
        'v2': sum(v2_hits) / pattern_length,
        'aux': sum(aux_hits) / pattern_length,
        'combined': compute_density(v1_hits, v2_hits, aux_hits, pattern_length),
    }
```

### SHAPE-Zone Scoring

```python
def score_density(raw: float, shape: float, energy: float) -> float:
    """
    Score density relative to SHAPE and ENERGY.

    ENERGY is the primary driver of density.
    SHAPE modifies the acceptable range.
    """
    # Base target from ENERGY
    energy_target = 0.15 + energy * 0.45  # Range: 0.15 to 0.60

    # SHAPE widens acceptable range at extremes
    if shape < 0.3:
        width = 0.15
    elif shape < 0.7:
        width = 0.20
    else:
        width = 0.30  # Wild zone: density is less constrained

    distance = abs(raw - energy_target)
    return max(0.0, 1.0 - (distance / width) ** 2)
```

---

## Metric 3: Velocity Range

### Concept

Measures dynamic contrast - the spread between quietest and loudest hits. High velocity range = expressive dynamics (ghost notes + accents). Low range = flat/mechanical.

### Formula

```python
def compute_velocity_range(
    v1_hits: list[bool],
    v2_hits: list[bool],
    aux_hits: list[bool],
    v1_vels: list[float],
    v2_vels: list[float],
    aux_vels: list[float]
) -> float:
    """
    Compute velocity range across all voices.

    Returns: max(velocity) - min(velocity) for all hits
    """
    all_velocities = []

    for i in range(len(v1_hits)):
        if v1_hits[i] and v1_vels[i] > 0:
            all_velocities.append(v1_vels[i])
        if v2_hits[i] and v2_vels[i] > 0:
            all_velocities.append(v2_vels[i])
        if aux_hits[i] and aux_vels[i] > 0:
            all_velocities.append(aux_vels[i])

    if len(all_velocities) < 2:
        return 0.0

    return max(all_velocities) - min(all_velocities)
```

### Ghost/Accent Analysis (Diagnostic)

```python
def analyze_velocity_distribution(velocities: list[float]) -> dict:
    """
    Categorize hits by velocity level.

    Ghost: < 0.4
    Normal: 0.4 - 0.75
    Accent: > 0.75
    """
    if not velocities:
        return {'ghost_ratio': 0, 'normal_ratio': 0, 'accent_ratio': 0}

    ghosts = sum(1 for v in velocities if v < 0.4)
    accents = sum(1 for v in velocities if v > 0.75)
    normals = len(velocities) - ghosts - accents
    total = len(velocities)

    return {
        'ghost_ratio': ghosts / total,
        'normal_ratio': normals / total,
        'accent_ratio': accents / total,
    }
```

### SHAPE-Zone Scoring

```python
def score_velocity_range(raw: float, accent: float) -> float:
    """
    Score velocity range relative to ACCENT parameter.

    ACCENT controls expected dynamic contrast.
    """
    # Target range based on ACCENT
    if accent < 0.3:
        target_center, width = 0.20, 0.25  # Low accent = flat OK
    elif accent < 0.7:
        target_center, width = 0.45, 0.25  # Medium = moderate dynamics
    else:
        target_center, width = 0.65, 0.30  # High accent = strong contrast

    distance = abs(raw - target_center)
    return max(0.0, 1.0 - (distance / width) ** 2)
```

---

## Metric 4: Voice Separation

### Concept

Measures how independently the voices operate. High separation = voices fill each other's gaps (complementary). Low separation = voices stack on same steps (reinforcing).

### Formula

```python
def compute_voice_separation(
    v1_hits: list[bool],
    v2_hits: list[bool],
    aux_hits: list[bool],
    pattern_length: int = 32
) -> float:
    """
    Compute voice separation as inverse of overlap.

    Overlap = steps where 2+ voices hit simultaneously
    Separation = 1 - (overlap / total_active_steps)
    """
    overlap_count = 0
    total_active = 0

    for i in range(pattern_length):
        voices_active = v1_hits[i] + v2_hits[i] + aux_hits[i]
        if voices_active > 0:
            total_active += 1
        if voices_active >= 2:
            overlap_count += 1

    if total_active == 0:
        return 0.5  # Neutral for empty pattern

    overlap_ratio = overlap_count / total_active
    return 1.0 - overlap_ratio
```

### Pairwise Separation (Diagnostic)

```python
def compute_pairwise_separation(
    v1_hits: list[bool],
    v2_hits: list[bool],
    aux_hits: list[bool]
) -> dict[str, float]:
    """Separation between each voice pair."""
    def pair_sep(a: list[bool], b: list[bool]) -> float:
        a_total = sum(a)
        if a_total == 0:
            return 1.0
        overlap = sum(1 for i in range(len(a)) if a[i] and b[i])
        return 1.0 - (overlap / a_total)

    return {
        'v1_v2': pair_sep(v1_hits, v2_hits),
        'v1_aux': pair_sep(v1_hits, aux_hits),
        'v2_aux': pair_sep(v2_hits, aux_hits),
    }
```

### SHAPE-Zone Scoring

```python
def score_voice_separation(raw: float, shape: float) -> float:
    """
    Score voice separation.

    Stable patterns benefit from some reinforcement (lower separation OK).
    Wild patterns can have more overlap for density.
    """
    if shape < 0.3:
        # Stable: moderate-high separation (complementary rhythm)
        target_center, width = 0.75, 0.25
    elif shape < 0.7:
        # Syncopated: moderate separation
        target_center, width = 0.65, 0.25
    else:
        # Wild: lower separation OK (stacking for intensity)
        target_center, width = 0.50, 0.30

    distance = abs(raw - target_center)
    return max(0.0, 1.0 - (distance / width) ** 2)
```

---

## Metric 5: Regularity

### Concept

Measures temporal predictability - how consistent are the gaps between hits? High regularity = evenly-spaced (euclidean). Low regularity = irregular/broken.

This is the **inverse** of v1's "Rhythmic Complexity" - clearer semantics where high = predictable.

### Formula

```python
def compute_regularity(hits: list[bool], pattern_length: int = 32) -> float:
    """
    Compute regularity as inverse of gap variance.

    Uses coefficient of variation (CV) = std_dev / mean
    Regularity = 1 - min(1, CV)

    Perfect regularity (all equal gaps) = 1.0
    High variance (irregular gaps) = low regularity
    """
    hit_positions = [i for i, h in enumerate(hits) if h]

    if len(hit_positions) < 2:
        return 0.5  # Neutral for sparse patterns

    # Compute inter-onset intervals (including wrap-around)
    gaps = []
    for i in range(len(hit_positions)):
        next_idx = (i + 1) % len(hit_positions)
        if next_idx == 0:
            gap = (pattern_length - hit_positions[i]) + hit_positions[0]
        else:
            gap = hit_positions[next_idx] - hit_positions[i]
        gaps.append(gap)

    mean_gap = sum(gaps) / len(gaps)
    if mean_gap == 0:
        return 1.0

    variance = sum((g - mean_gap) ** 2 for g in gaps) / len(gaps)
    std_dev = variance ** 0.5
    cv = std_dev / mean_gap  # Coefficient of variation

    # Normalize: CV > 1.0 is very irregular
    return max(0.0, 1.0 - min(1.0, cv))
```

### Gap Analysis (Diagnostic)

```python
def analyze_gaps(hits: list[bool], pattern_length: int = 32) -> dict:
    """Detailed gap analysis for diagnostics."""
    hit_positions = [i for i, h in enumerate(hits) if h]

    if len(hit_positions) < 2:
        return {'min': 0, 'max': 0, 'mean': 0, 'std': 0, 'unique_gaps': 0}

    gaps = []
    for i in range(len(hit_positions)):
        next_idx = (i + 1) % len(hit_positions)
        if next_idx == 0:
            gap = (pattern_length - hit_positions[i]) + hit_positions[0]
        else:
            gap = hit_positions[next_idx] - hit_positions[i]
        gaps.append(gap)

    mean_gap = sum(gaps) / len(gaps)
    variance = sum((g - mean_gap) ** 2 for g in gaps) / len(gaps)

    return {
        'min': min(gaps),
        'max': max(gaps),
        'mean': mean_gap,
        'std': variance ** 0.5,
        'unique_gaps': len(set(gaps)),
        'gaps': gaps,
    }
```

### SHAPE-Zone Scoring

```python
def score_regularity(raw: float, shape: float) -> float:
    """
    Score regularity relative to SHAPE zone.

    Stable zone: HIGH regularity expected (euclidean, predictable)
    Wild zone: LOW regularity expected (broken, chaotic)
    """
    if shape < 0.3:
        # Stable: high regularity expected
        target_center, width = 0.85, 0.20
    elif shape < 0.7:
        # Syncopated: moderate regularity
        target_center, width = 0.55, 0.25
    else:
        # Wild: low regularity OK
        target_center, width = 0.30, 0.30

    distance = abs(raw - target_center)
    return max(0.0, 1.0 - (distance / width) ** 2)
```

---

## Composite Score: Weighted Sum

### Why Not Pentagon Area?

Pentagon area = `0.5 * sum(v[i] * v[i+1] * sin(72°))` has problematic behavior:
- Zero on ANY axis collapses area toward zero
- A pattern scoring (1,1,0,0,0) has lower area than (0.5,0.5,0.5,0.5,0.5)
- This punishes specialization and rewards mediocrity

### Weighted Sum Formula

```python
def compute_composite_score(
    syncopation_score: float,
    density_score: float,
    velocity_range_score: float,
    voice_separation_score: float,
    regularity_score: float
) -> float:
    """
    Compute weighted composite score.

    Weights reflect musical importance:
    - Syncopation: 0.25 (highest - defines groove character)
    - Density: 0.15 (lowest - less perceptually salient)
    - Others: 0.20 each
    """
    return (
        0.25 * syncopation_score +
        0.15 * density_score +
        0.20 * velocity_range_score +
        0.20 * voice_separation_score +
        0.20 * regularity_score
    )
```

### Full Musicality Analysis

```python
@dataclass
class MusalityScores:
    """Complete musicality analysis results."""
    # Raw values (0-1, not zone-adjusted)
    raw_syncopation: float
    raw_density: float
    raw_velocity_range: float
    raw_voice_separation: float
    raw_regularity: float

    # Scored values (0-1, zone-adjusted)
    score_syncopation: float
    score_density: float
    score_velocity_range: float
    score_voice_separation: float
    score_regularity: float

    # Composite
    composite: float

    # Zone validation
    shape_zone: str  # 'stable', 'syncopated', 'wild'
    zone_compliance: float  # How well pattern matches zone expectations


def compute_musicality(
    pattern: Pattern,
    shape: float,
    energy: float,
    accent: float
) -> MusalityScores:
    """Compute full musicality analysis."""
    # Raw metrics
    raw_sync = compute_syncopation(pattern.v1_hits)
    raw_dens = compute_density(pattern.v1_hits, pattern.v2_hits, pattern.aux_hits)
    raw_vel = compute_velocity_range(
        pattern.v1_hits, pattern.v2_hits, pattern.aux_hits,
        pattern.v1_vels, pattern.v2_vels, pattern.aux_vels
    )
    raw_sep = compute_voice_separation(pattern.v1_hits, pattern.v2_hits, pattern.aux_hits)
    raw_reg = compute_regularity(pattern.v1_hits)

    # Scored metrics
    score_sync = score_syncopation(raw_sync, shape)
    score_dens = score_density(raw_dens, shape, energy)
    score_vel = score_velocity_range(raw_vel, accent)
    score_sep = score_voice_separation(raw_sep, shape)
    score_reg = score_regularity(raw_reg, shape)

    # Composite
    composite = compute_composite_score(
        score_sync, score_dens, score_vel, score_sep, score_reg
    )

    # Zone
    if shape < 0.3:
        zone = 'stable'
    elif shape < 0.7:
        zone = 'syncopated'
    else:
        zone = 'wild'

    # Zone compliance = average of all scores (they're already zone-adjusted)
    zone_compliance = (score_sync + score_dens + score_vel + score_sep + score_reg) / 5

    return MusalityScores(
        raw_syncopation=raw_sync,
        raw_density=raw_dens,
        raw_velocity_range=raw_vel,
        raw_voice_separation=raw_sep,
        raw_regularity=raw_reg,
        score_syncopation=score_sync,
        score_density=score_dens,
        score_velocity_range=score_vel,
        score_voice_separation=score_sep,
        score_regularity=score_reg,
        composite=composite,
        shape_zone=zone,
        zone_compliance=zone_compliance,
    )
```

---

## Summary Table

| Metric | Raw Range | Measures | Primary Driver |
|--------|-----------|----------|----------------|
| Syncopation | 0.0-1.0 | Metric grid deviation | SHAPE |
| Density | 0.0-1.0 | Activity level | ENERGY |
| Velocity Range | 0.0-1.0 | Dynamic contrast | ACCENT |
| Voice Separation | 0.0-1.0 | Voice independence | SHAPE |
| Regularity | 0.0-1.0 | Gap consistency | SHAPE |

---

## References

- Longuet-Higgins HC, Lee CS. "The Rhythmic Interpretation of Monophonic Music." Music Perception. 1984.
- Huron D. "Sweet Anticipation: Music and the Psychology of Expectation." MIT Press. 2006.
- Witek et al. "Syncopation, Body-Movement and Pleasure in Groove Music." PLoS ONE. 2014.
