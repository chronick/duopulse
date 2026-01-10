# Pentagon of Musicality: Python Implementation

**Version**: 1.0
**Date**: 2026-01-09
**Target**: `scripts/evaluate-expressiveness.py`

## Integration Summary

Add to the existing `PatternMetrics` dataclass and `compute_metrics()` function.

---

## Data Structures

```python
@dataclass
class MusicMetrics:
    """Pentagon of Musicality metrics for a single pattern."""
    # Raw values (0-1)
    raw_syncopation: float = 0.0
    raw_dynamic_texture: float = 0.0
    raw_anticipation: float = 0.0
    raw_complexity: float = 0.0
    raw_interplay: float = 0.0

    # Scored values (0-1, context-aware)
    score_syncopation: float = 0.0
    score_dynamic_texture: float = 0.0
    score_anticipation: float = 0.0
    score_complexity: float = 0.0
    score_interplay: float = 0.0

    # Composite
    pentagon_area: float = 0.0
    balance: float = 0.0
    composite: float = 0.0
```

---

## Metric Weight Tables

```python
# Metric weights for 16-step and 32-step patterns
# Based on 4/4 time signature musical hierarchy

METRIC_WEIGHTS_16 = [
    1.00,  # Step 0:  Beat 1 (strongest)
    0.10,  # Step 1:  16th note
    0.40,  # Step 2:  8th note
    0.10,  # Step 3:  16th note (anticipation before beat 2)
    0.80,  # Step 4:  Beat 2
    0.10,  # Step 5:  16th note
    0.40,  # Step 6:  8th note
    0.10,  # Step 7:  16th note (anticipation before beat 3)
    0.90,  # Step 8:  Beat 3 (half-bar, strong)
    0.10,  # Step 9:  16th note
    0.40,  # Step 10: 8th note
    0.10,  # Step 11: 16th note (anticipation before beat 4)
    0.80,  # Step 12: Beat 4
    0.10,  # Step 13: 16th note
    0.40,  # Step 14: 8th note
    0.10,  # Step 15: 16th note (anticipation before beat 1)
]

METRIC_WEIGHTS_32 = [
    1.00, 0.05, 0.20, 0.05, 0.40, 0.05, 0.20, 0.10,  # Bar 1: beats 1-2
    0.80, 0.05, 0.20, 0.05, 0.40, 0.05, 0.20, 0.10,  # Bar 1: beats 3-4
    0.90, 0.05, 0.20, 0.05, 0.40, 0.05, 0.20, 0.10,  # Bar 2: beats 1-2
    0.80, 0.05, 0.20, 0.05, 0.40, 0.05, 0.20, 0.10,  # Bar 2: beats 3-4
]

# Anticipation positions (step before quarter notes)
ANTICIPATION_16 = {3, 7, 11, 15}
ANTICIPATION_32 = {3, 7, 11, 15, 19, 23, 27, 31}

def get_metric_weight(step: int, pattern_length: int) -> float:
    """Get metric weight for a step position."""
    if pattern_length == 16:
        return METRIC_WEIGHTS_16[step % 16]
    elif pattern_length == 32:
        return METRIC_WEIGHTS_32[step % 32]
    else:
        # Scale to 16-step
        normalized = step / pattern_length
        mapped = int(normalized * 16) % 16
        return METRIC_WEIGHTS_16[mapped]
```

---

## Metric 1: Syncopation

```python
def compute_syncopation(pattern: Pattern) -> float:
    """
    Compute LHL-inspired syncopation score.

    Measures tension from hits on weak beats followed by rests on strong beats.
    """
    length = pattern.length
    weights = [get_metric_weight(i, length) for i in range(length)]

    tension = 0.0
    max_tension = 0.0

    for step in pattern.steps:
        if not step.v1:  # Focus on V1 (anchor)
            continue

        # Check if next step is a rest
        next_step = (step.step + 1) % length
        next_has_hit = pattern.steps[next_step].v1

        current_weight = weights[step.step]
        next_weight = weights[next_step]

        if not next_has_hit:
            weight_diff = next_weight - current_weight
            if weight_diff > 0:
                tension += weight_diff

    # Calculate theoretical maximum (all weak-before-strong transitions)
    for i in range(length):
        next_i = (i + 1) % length
        weight_diff = weights[next_i] - weights[i]
        if weight_diff > 0:
            max_tension += weight_diff

    if max_tension == 0:
        return 0.0

    return min(1.0, tension / max_tension)


def score_syncopation(raw: float, shape: float) -> float:
    """Score syncopation with SHAPE-dependent optimal range."""
    if shape < 0.3:
        center, width = 0.22, 0.25
    elif shape < 0.7:
        center, width = 0.45, 0.30
    else:
        center, width = 0.62, 0.35

    distance = abs(raw - center)
    return max(0.0, 1.0 - (distance / width) ** 2)
```

---

## Metric 2: Dynamic Texture

```python
def compute_dynamic_texture(pattern: Pattern) -> float:
    """
    Compute dynamic texture from velocity distribution.

    Measures ghost/normal/accent balance and velocity variation.
    """
    # Collect all hit velocities (V1 and V2)
    velocities = []
    for step in pattern.steps:
        if step.v1 and step.v1_vel > 0:
            velocities.append(step.v1_vel)
        if step.v2 and step.v2_vel > 0:
            velocities.append(step.v2_vel)

    if len(velocities) < 2:
        return 0.0

    # Categorize
    ghosts = sum(1 for v in velocities if v < 0.4)
    accents = sum(1 for v in velocities if v >= 0.75)
    total = len(velocities)

    ghost_ratio = ghosts / total
    accent_ratio = accents / total

    # Velocity range
    vel_range = max(velocities) - min(velocities)

    # Velocity std dev
    mean_vel = sum(velocities) / total
    variance = sum((v - mean_vel) ** 2 for v in velocities) / total
    std_dev = variance ** 0.5
    normalized_std = min(1.0, std_dev / 0.25)

    # Balance score (optimal: ~20% ghosts, ~20% accents)
    balance = 1.0 - abs(ghost_ratio - 0.20) - abs(accent_ratio - 0.20)
    balance = max(0.0, balance)

    # Range score
    range_score = min(1.0, vel_range / 0.6)

    return 0.35 * balance + 0.35 * range_score + 0.30 * normalized_std


def score_dynamic_texture(raw: float, accent: float) -> float:
    """Score dynamic texture relative to ACCENT parameter."""
    if accent < 0.3:
        expected_min, expected_max = 0.10, 0.35
    elif accent < 0.7:
        expected_min, expected_max = 0.35, 0.65
    else:
        expected_min, expected_max = 0.55, 0.85

    center = (expected_min + expected_max) / 2
    width = expected_max - expected_min

    if raw < expected_min:
        penalty = ((expected_min - raw) / 0.3) ** 2
        return max(0.0, 1.0 - penalty)
    elif raw > expected_max:
        penalty = ((raw - expected_max) / 0.3) ** 2
        return max(0.0, 1.0 - penalty)
    else:
        distance = abs(raw - center)
        return 1.0 - (distance / width) * 0.3
```

---

## Metric 3: Anticipation

```python
def compute_anticipation(pattern: Pattern) -> float:
    """
    Compute anticipation score.

    Measures hits on positions immediately before strong beats.
    """
    length = pattern.length
    positions = ANTICIPATION_16 if length == 16 else ANTICIPATION_32

    total_hits = sum(1 for s in pattern.steps if s.v1)
    if total_hits == 0:
        return 0.0

    strong = 0  # Hit on anticipation, rest on following downbeat
    weak = 0    # Hit on anticipation, hit on following downbeat

    for pos in positions:
        if pos >= length:
            continue
        if pattern.steps[pos].v1:
            next_pos = (pos + 1) % length
            if not pattern.steps[next_pos].v1:
                strong += 1
            else:
                weak += 1

    # Strong anticipations worth 2x
    value = strong * 2 + weak
    max_value = len([p for p in positions if p < length]) * 2

    return min(1.0, value / max_value) if max_value > 0 else 0.0


def score_anticipation(raw: float, shape: float) -> float:
    """Score anticipation with SHAPE-dependent optimal range."""
    if shape < 0.3:
        center, width = 0.17, 0.20
    elif shape < 0.7:
        center, width = 0.38, 0.25
    else:
        center, width = 0.52, 0.35

    distance = abs(raw - center)
    return max(0.0, 1.0 - (distance / width) ** 2)
```

---

## Metric 4: Rhythmic Complexity

```python
def compute_complexity(pattern: Pattern) -> float:
    """
    Compute rhythmic complexity from inter-onset interval analysis.

    Measures IOI variance, non-divisibility, and distribution balance.
    """
    hit_positions = [s.step for s in pattern.steps if s.v1]
    length = pattern.length

    if len(hit_positions) < 2:
        return 0.0

    # Compute IOIs (including wrap-around)
    iois = []
    for i in range(len(hit_positions)):
        next_idx = (i + 1) % len(hit_positions)
        if next_idx == 0:
            ioi = (length - hit_positions[i]) + hit_positions[0]
        else:
            ioi = hit_positions[next_idx] - hit_positions[i]
        iois.append(ioi)

    # 1. IOI Variance
    mean_ioi = sum(iois) / len(iois)
    variance = sum((ioi - mean_ioi) ** 2 for ioi in iois) / len(iois)
    std_dev = variance ** 0.5
    max_std = length / 4
    ioi_score = min(1.0, std_dev / max_std)

    # 2. Non-divisibility
    common_divs = [2, 3, 4, 6, 8]
    irregular = sum(1 for ioi in iois if all(ioi % d != 0 for d in common_divs))
    irreg_score = irregular / len(iois)

    # 3. Distribution balance (first half vs second half)
    first_half = sum(1 for p in hit_positions if p < length // 2)
    imbalance = 2 * abs(first_half - len(hit_positions) / 2) / len(hit_positions)
    dist_score = 1.0 - imbalance ** 2

    return 0.45 * ioi_score + 0.30 * irreg_score + 0.25 * dist_score


def score_complexity(raw: float, shape: float) -> float:
    """Score rhythmic complexity with SHAPE-dependent expectations."""
    if shape < 0.3:
        center, width = 0.15, 0.25
    elif shape < 0.7:
        center, width = 0.38, 0.30
    else:
        center, width = 0.62, 0.35

    distance = abs(raw - center)
    return max(0.0, 1.0 - (distance / width) ** 2)
```

---

## Metric 5: Voice Interplay

```python
def compute_interplay(pattern: Pattern) -> float:
    """
    Compute voice interplay quality.

    Measures complementary rhythm, polyphonic distribution, and accent separation.
    """
    length = pattern.length

    # Extract hit arrays
    v1_hits = [s.v1 for s in pattern.steps]
    v2_hits = [s.v2 for s in pattern.steps]
    aux_hits = [s.aux for s in pattern.steps]
    v1_vels = [s.v1_vel for s in pattern.steps]
    v2_vels = [s.v2_vel for s in pattern.steps]

    # 1. Complementary rhythm (V2 fills V1 gaps)
    v1_gaps = [i for i in range(length) if not v1_hits[i]]
    v2_in_gaps = sum(1 for i in v1_gaps if v2_hits[i])
    v2_total = sum(v2_hits)

    if v2_total > 0:
        complement_ratio = v2_in_gaps / v2_total
        collisions = sum(1 for i in range(length) if v1_hits[i] and v2_hits[i])
        collision_penalty = collisions / v2_total
        complement_score = complement_ratio * (1.0 - collision_penalty * 0.5)
    else:
        complement_score = 0.0

    # 2. Polyphonic distribution
    single_voice = sum(
        1 for i in range(length)
        if (v1_hits[i] + v2_hits[i] + aux_hits[i]) == 1
    )
    multi_voice = sum(
        1 for i in range(length)
        if (v1_hits[i] + v2_hits[i] + aux_hits[i]) >= 2
    )
    total_active = single_voice + multi_voice

    if total_active > 0:
        single_ratio = single_voice / total_active
        poly_score = max(0.0, 1.0 - abs(single_ratio - 0.70) / 0.30)
    else:
        poly_score = 0.0

    # 3. Accent separation
    v1_accents = {i for i in range(length) if v1_hits[i] and v1_vels[i] >= 0.75}
    v2_accents = {i for i in range(length) if v2_hits[i] and v2_vels[i] >= 0.75}

    if v1_accents and v2_accents:
        overlap = len(v1_accents & v2_accents)
        total = len(v1_accents) + len(v2_accents)
        accent_score = max(0.0, 1.0 - (overlap * 2) / total)
    else:
        accent_score = 0.5

    return 0.45 * complement_score + 0.30 * poly_score + 0.25 * accent_score


def score_interplay(raw: float) -> float:
    """Score voice interplay (SHAPE-independent)."""
    center, width = 0.65, 0.35

    if raw < 0.30:
        return raw / 0.30 * 0.4

    distance = abs(raw - center)
    return max(0.0, 1.0 - (distance / width) ** 2)
```

---

## Pentagon Area Calculation

```python
import math

def compute_pentagon_area(scores: list[float]) -> float:
    """
    Compute normalized area of pentagon formed by 5 metric scores.

    Each score is 0-1, plotted as distance from center on radar chart.
    Returns normalized area [0, 1].
    """
    assert len(scores) == 5, "Pentagon requires exactly 5 scores"

    angle_step = 2 * math.pi / 5

    # Area using triangles from center
    area = 0
    for i in range(5):
        r1 = scores[i]
        r2 = scores[(i + 1) % 5]
        area += 0.5 * r1 * r2 * math.sin(angle_step)

    # Maximum area (all scores = 1.0)
    max_area = 5 * 0.5 * 1.0 * 1.0 * math.sin(angle_step)

    return area / max_area
```

---

## Full Metric Computation

```python
def compute_music_metrics(pattern: Pattern) -> MusicMetrics:
    """
    Compute all Pentagon of Musicality metrics for a pattern.
    """
    # Raw metrics
    raw_sync = compute_syncopation(pattern)
    raw_dyn = compute_dynamic_texture(pattern)
    raw_ant = compute_anticipation(pattern)
    raw_comp = compute_complexity(pattern)
    raw_inter = compute_interplay(pattern)

    # Scored metrics (context-aware)
    score_sync = score_syncopation(raw_sync, pattern.shape)
    score_dyn = score_dynamic_texture(raw_dyn, pattern.accent)
    score_ant = score_anticipation(raw_ant, pattern.shape)
    score_comp = score_complexity(raw_comp, pattern.shape)
    score_inter = score_interplay(raw_inter)

    scores = [score_sync, score_dyn, score_ant, score_comp, score_inter]

    # Pentagon area
    area = compute_pentagon_area(scores)

    # Balance (low variance = good balance)
    mean_score = sum(scores) / len(scores)
    variance = sum((s - mean_score) ** 2 for s in scores) / len(scores)
    balance = 1.0 - min(1.0, variance / 0.1)

    # Composite
    composite = area * 0.75 + balance * 0.25

    return MusicMetrics(
        raw_syncopation=raw_sync,
        raw_dynamic_texture=raw_dyn,
        raw_anticipation=raw_ant,
        raw_complexity=raw_comp,
        raw_interplay=raw_inter,
        score_syncopation=score_sync,
        score_dynamic_texture=score_dyn,
        score_anticipation=score_ant,
        score_complexity=score_comp,
        score_interplay=score_inter,
        pentagon_area=area,
        balance=balance,
        composite=composite,
    )
```

---

## Report Output Addition

Add to `generate_markdown_report()`:

```python
def format_pentagon_report(metrics: list[MusicMetrics]) -> str:
    """Generate Pentagon of Musicality section for report."""
    lines = []
    lines.append("## Pentagon of Musicality")
    lines.append("")

    # Average scores across all patterns
    avg = MusicMetrics(
        raw_syncopation=sum(m.raw_syncopation for m in metrics) / len(metrics),
        raw_dynamic_texture=sum(m.raw_dynamic_texture for m in metrics) / len(metrics),
        raw_anticipation=sum(m.raw_anticipation for m in metrics) / len(metrics),
        raw_complexity=sum(m.raw_complexity for m in metrics) / len(metrics),
        raw_interplay=sum(m.raw_interplay for m in metrics) / len(metrics),
        score_syncopation=sum(m.score_syncopation for m in metrics) / len(metrics),
        score_dynamic_texture=sum(m.score_dynamic_texture for m in metrics) / len(metrics),
        score_anticipation=sum(m.score_anticipation for m in metrics) / len(metrics),
        score_complexity=sum(m.score_complexity for m in metrics) / len(metrics),
        score_interplay=sum(m.score_interplay for m in metrics) / len(metrics),
        pentagon_area=sum(m.pentagon_area for m in metrics) / len(metrics),
        balance=sum(m.balance for m in metrics) / len(metrics),
        composite=sum(m.composite for m in metrics) / len(metrics),
    )

    lines.append("| Metric | Raw Value | Score | Target |")
    lines.append("|--------|-----------|-------|--------|")
    lines.append(f"| Syncopation | {avg.raw_syncopation:.2f} | {avg.score_syncopation:.0%} | 70%+ |")
    lines.append(f"| Dynamic Texture | {avg.raw_dynamic_texture:.2f} | {avg.score_dynamic_texture:.0%} | 70%+ |")
    lines.append(f"| Anticipation | {avg.raw_anticipation:.2f} | {avg.score_anticipation:.0%} | 70%+ |")
    lines.append(f"| Complexity | {avg.raw_complexity:.2f} | {avg.score_complexity:.0%} | 70%+ |")
    lines.append(f"| Voice Interplay | {avg.raw_interplay:.2f} | {avg.score_interplay:.0%} | 70%+ |")
    lines.append("")
    lines.append(f"**Pentagon Area**: {avg.pentagon_area:.0%}")
    lines.append(f"**Balance**: {avg.balance:.0%}")
    lines.append(f"**Composite Score**: {avg.composite:.0%}")
    lines.append("")

    # Pass/fail
    if avg.composite >= 0.65:
        lines.append("**Musicality Test**: PASS")
    else:
        lines.append("**Musicality Test**: FAIL - Composite below 65%")
        # Identify weak dimensions
        weak = []
        if avg.score_syncopation < 0.6: weak.append("Syncopation")
        if avg.score_dynamic_texture < 0.6: weak.append("Dynamic Texture")
        if avg.score_anticipation < 0.6: weak.append("Anticipation")
        if avg.score_complexity < 0.6: weak.append("Complexity")
        if avg.score_interplay < 0.6: weak.append("Voice Interplay")
        if weak:
            lines.append(f"   - Weak dimensions: {', '.join(weak)}")

    return "\n".join(lines)
```

---

## ASCII Radar Chart (Optional)

```python
def ascii_pentagon(scores: list[float], labels: list[str] = None) -> str:
    """
    Generate ASCII radar chart for 5 metrics.

    Example output:
             Sync
              /\
             /  \
        Dyn /    \ Ant
           /______\
          /        \
    Inter          Comp
    """
    if labels is None:
        labels = ["Sync", "Dyn", "Ant", "Comp", "Inter"]

    # Simplified ASCII representation
    lines = []
    lines.append(f"           {labels[0]}")
    lines.append(f"            {'*' if scores[0] > 0.5 else '.'}")
    lines.append(f"           / \\")
    lines.append(f"  {labels[1]} {'*' if scores[1] > 0.5 else '.'}   {'*' if scores[2] > 0.5 else '.'} {labels[2]}")
    lines.append(f"         /     \\")
    lines.append(f"        /_______\\")
    lines.append(f"       /         \\")
    lines.append(f"  {labels[4]} {'*' if scores[4] > 0.5 else '.'}       {'*' if scores[3] > 0.5 else '.'} {labels[3]}")

    return "\n".join(lines)
```

---

## Integration Checklist

1. [ ] Add `MusicMetrics` dataclass to data types section
2. [ ] Add metric weight tables (constants)
3. [ ] Implement 5 raw metric functions
4. [ ] Implement 5 scoring functions
5. [ ] Implement `compute_music_metrics()` orchestrator
6. [ ] Add `pentagon_area` and `composite` calculations
7. [ ] Update `generate_markdown_report()` with Pentagon section
8. [ ] Add JSON output fields for metrics
9. [ ] Add command-line flag `--musicality` to enable detailed output
10. [ ] Add pass/fail threshold for composite score
