# Trill Diagnostics (Non-Metric)

**Version**: 1.0
**Date**: 2026-01-09
**Status**: Diagnostic tool - NOT a musicality metric

> **Implementation Note**: True sub-step trills (32nd notes within a 16th-note grid) would require a 32 or 64-step grid resolution to produce half/quarter-sized triggers. This is **not yet implemented** in the pattern generator. Current "trills" are consecutive 16th-note hits, which create a similar perceptual effect at higher tempos. Future work may add sub-step resolution for authentic flams and rolls.

## Why Trills Are Not a Metric Axis

Trills are a **compositional technique**, not a **perceptual dimension**. The Pentagon metrics measure how a pattern *feels*:
- Syncopation = tension from metric displacement
- Density = activity level
- Velocity Range = dynamic contrast
- Voice Separation = voice independence
- Regularity = temporal predictability

"Trill count" measures *what the pattern contains*, not *how it affects perception*. A pattern with 0.7 syncopation has meaning (moderate tension). A pattern with 0.7 "trill score" has no inherent musical meaning - is that good? Bad? It depends entirely on context.

### How Trills Manifest in Existing Metrics

Trills affect the pentagon metrics indirectly:

| Trill Characteristic | Affected Metric | Effect |
|---------------------|-----------------|--------|
| Consecutive hits (IOI=1) | Regularity | Decreases (higher gap variance) |
| Trills on weak beats | Syncopation | Increases |
| Trills on strong beats | Syncopation | Neutral/decreases |
| Shaped velocity envelope | Velocity Range | Increases |
| V1+V2 trill collision | Voice Separation | Decreases |

## Trill Diagnostics API

For algorithm tuning and debugging, a separate diagnostic system provides trill visibility without polluting the musicality framework.

### Data Structures

```python
from dataclasses import dataclass
from typing import Optional


@dataclass
class Trill:
    """A detected trill (consecutive hits)."""
    voice: str  # 'v1', 'v2', 'aux'
    start_pos: int  # First step of trill
    length: int  # Number of consecutive hits (2-4 typical)
    velocities: list[float]  # Velocity of each hit
    envelope_type: str  # 'flat', 'crescendo', 'decrescendo', 'accented'


@dataclass
class TrillDiagnostics:
    """Complete trill analysis for a pattern."""
    # Detection
    trills: list[Trill]
    total_trill_hits: int
    total_single_hits: int

    # Density
    trill_density: float  # trill_hits / total_hits

    # Distribution
    v1_trills: int
    v2_trills: int
    aux_trills: int

    # Placement quality
    on_anticipation: int  # Trills on steps 7,15,23,31
    on_downbeat: int  # Trills starting on 0,8,16,24
    placement_score: float  # Higher = better placement

    # Envelope analysis
    flat_envelope_count: int
    shaped_envelope_count: int
    envelope_variety: float  # Higher = more varied envelopes

    # Collision detection
    voice_collisions: int  # Trills overlapping between voices

    # Warnings
    warnings: list[str]
```

### Detection Algorithm

```python
def detect_trills(
    hits: list[bool],
    velocities: list[float],
    voice: str,
    min_length: int = 2,
    max_length: int = 4
) -> list[Trill]:
    """
    Detect trills (consecutive hits) in a single voice.

    Args:
        hits: Boolean hit pattern
        velocities: Velocity for each step
        voice: Voice identifier ('v1', 'v2', 'aux')
        min_length: Minimum consecutive hits to count as trill (default 2)
        max_length: Maximum trill length to detect (default 4)

    Returns:
        List of detected Trill objects
    """
    trills = []
    i = 0
    pattern_length = len(hits)

    while i < pattern_length:
        if hits[i]:
            # Count consecutive hits
            run_length = 1
            run_vels = [velocities[i]]

            while (i + run_length < pattern_length and
                   hits[i + run_length] and
                   run_length < max_length):
                run_vels.append(velocities[i + run_length])
                run_length += 1

            if run_length >= min_length:
                trills.append(Trill(
                    voice=voice,
                    start_pos=i,
                    length=run_length,
                    velocities=run_vels,
                    envelope_type=classify_envelope(run_vels),
                ))

            i += run_length
        else:
            i += 1

    return trills


def classify_envelope(velocities: list[float]) -> str:
    """
    Classify velocity envelope of a trill.

    Returns: 'flat', 'crescendo', 'decrescendo', or 'accented'
    """
    if len(velocities) < 2:
        return 'flat'

    vel_range = max(velocities) - min(velocities)

    # Flat: all velocities within 0.15 of each other
    if vel_range < 0.15:
        return 'flat'

    # Check for monotonic increase/decrease
    is_increasing = all(velocities[i] <= velocities[i+1] for i in range(len(velocities)-1))
    is_decreasing = all(velocities[i] >= velocities[i+1] for i in range(len(velocities)-1))

    if is_increasing:
        return 'crescendo'
    elif is_decreasing:
        return 'decrescendo'

    # First hit is loudest = accented (flam style)
    if velocities[0] == max(velocities) and vel_range > 0.2:
        return 'accented'

    return 'flat'  # Default for irregular patterns
```

### Full Analysis

```python
ANTICIPATION_POSITIONS = {7, 15, 23, 31}  # 16th before quarter notes
DOWNBEAT_POSITIONS = {0, 8, 16, 24}  # Strong beats


def analyze_trills(
    v1_hits: list[bool],
    v2_hits: list[bool],
    aux_hits: list[bool],
    v1_vels: list[float],
    v2_vels: list[float],
    aux_vels: list[float],
    pattern_length: int = 32
) -> TrillDiagnostics:
    """Complete trill analysis for diagnostic purposes."""

    # Detect trills in each voice
    v1_trills = detect_trills(v1_hits, v1_vels, 'v1')
    v2_trills = detect_trills(v2_hits, v2_vels, 'v2')
    aux_trills = detect_trills(aux_hits, aux_vels, 'aux')

    all_trills = v1_trills + v2_trills + aux_trills

    # Calculate totals
    total_trill_hits = sum(t.length for t in all_trills)
    total_hits = sum(v1_hits) + sum(v2_hits) + sum(aux_hits)
    total_single_hits = total_hits - total_trill_hits

    # Density
    trill_density = total_trill_hits / total_hits if total_hits > 0 else 0.0

    # Placement analysis
    on_anticipation = sum(1 for t in all_trills if t.start_pos in ANTICIPATION_POSITIONS)
    on_downbeat = sum(1 for t in all_trills if t.start_pos in DOWNBEAT_POSITIONS)

    # Placement score: anticipation = good, random = neutral, downbeat for V1 = bad
    good_placements = on_anticipation
    bad_placements = sum(1 for t in v1_trills if t.start_pos in DOWNBEAT_POSITIONS)  # Kick rolls
    placement_score = (good_placements - bad_placements * 0.5) / max(1, len(all_trills))
    placement_score = max(0.0, min(1.0, placement_score + 0.5))  # Normalize to 0-1

    # Envelope analysis
    flat_count = sum(1 for t in all_trills if t.envelope_type == 'flat')
    shaped_count = len(all_trills) - flat_count
    envelope_types = set(t.envelope_type for t in all_trills)
    envelope_variety = len(envelope_types) / 4.0 if all_trills else 0.0

    # Collision detection
    collisions = 0
    for t1 in v1_trills:
        t1_steps = set(range(t1.start_pos, t1.start_pos + t1.length))
        for t2 in v2_trills + aux_trills:
            t2_steps = set(range(t2.start_pos, t2.start_pos + t2.length))
            if t1_steps & t2_steps:  # Overlap
                collisions += 1

    # Generate warnings
    warnings = []

    if len(v1_trills) > 0 and any(t.length > 2 for t in v1_trills):
        warnings.append("V1 (kick) has trills longer than 2 hits - physically unrealistic")

    if trill_density > 0.5:
        warnings.append(f"High trill density ({trill_density:.0%}) - pattern may sound chaotic")

    if flat_count > shaped_count and len(all_trills) > 3:
        warnings.append("Most trills have flat velocity - consider adding dynamics")

    if collisions > 2:
        warnings.append(f"{collisions} voice collisions during trills - may cause muddiness")

    return TrillDiagnostics(
        trills=all_trills,
        total_trill_hits=total_trill_hits,
        total_single_hits=total_single_hits,
        trill_density=trill_density,
        v1_trills=len(v1_trills),
        v2_trills=len(v2_trills),
        aux_trills=len(aux_trills),
        on_anticipation=on_anticipation,
        on_downbeat=on_downbeat,
        placement_score=placement_score,
        flat_envelope_count=flat_count,
        shaped_envelope_count=shaped_count,
        envelope_variety=envelope_variety,
        voice_collisions=collisions,
        warnings=warnings,
    )
```

## Voice-Specific Guidelines

### V1 (Anchor/Kick)

**Recommendation**: Trills generally inappropriate

| Trill Type | Appropriateness | Reason |
|------------|-----------------|--------|
| 2-hit flam | OK at phrase end | Adds weight |
| 3+ hit roll | Avoid | Kicks don't physically roll |
| Exception | SHAPE > 0.8 | Breakcore/gabber territory |

### V2 (Shimmer/Hats)

**Recommendation**: Trills natural and expected

| Trill Type | When to Use |
|------------|-------------|
| 2-hit flam | Accent points |
| 3-hit roll | Building tension |
| 4-hit buzz | Phrase endings, fills |
| Crescendo envelope | Building toward downbeat |
| Decrescendo envelope | Fading out of phrase |

### AUX (Accent/Snare)

**Recommendation**: Trills on anticipation positions

| Position | Appropriateness |
|----------|-----------------|
| Step 15 (before bar 2) | Excellent - classic fill |
| Step 31 (before bar 1) | Excellent - phrase connector |
| Steps 7, 23 | Good - mid-phrase tension |
| Random positions | Avoid - breaks groove |

## SHAPE-Zone Trill Expectations

| SHAPE Zone | V1 Trills | V2 Trills | AUX Trills | Total Density |
|------------|-----------|-----------|------------|---------------|
| Stable (0-30%) | 0 | 0-2 flams | 0-1 phrase-end | < 10% |
| Syncopated (30-70%) | 0 | 2-4 mixed | 1-2 anticipation | 10-25% |
| Wild (70-100%) | 0-1 (chaos) | Unrestricted | Unrestricted | 15-40% |

## Integration with Expressiveness Report

The trill diagnostics should be included in the expressiveness report as a separate section, NOT as a Pentagon metric:

```python
def generate_trill_report_section(diag: TrillDiagnostics) -> str:
    """Generate markdown section for trill analysis."""
    lines = [
        "## Trill Analysis (Diagnostic)",
        "",
        f"**Total trills detected**: {len(diag.trills)}",
        f"**Trill density**: {diag.trill_density:.0%}",
        "",
        "### Distribution by Voice",
        f"- V1 (kick): {diag.v1_trills}",
        f"- V2 (hats): {diag.v2_trills}",
        f"- AUX (snare): {diag.aux_trills}",
        "",
        "### Placement Quality",
        f"- On anticipation positions: {diag.on_anticipation}",
        f"- On downbeats: {diag.on_downbeat}",
        f"- Placement score: {diag.placement_score:.2f}",
        "",
        "### Envelope Analysis",
        f"- Flat envelopes: {diag.flat_envelope_count}",
        f"- Shaped envelopes: {diag.shaped_envelope_count}",
        f"- Envelope variety: {diag.envelope_variety:.0%}",
        "",
    ]

    if diag.warnings:
        lines.extend([
            "### Warnings",
            "",
        ])
        for warning in diag.warnings:
            lines.append(f"- {warning}")
        lines.append("")

    return "\n".join(lines)
```

## Summary

Trills are a valuable technique for IDM expressiveness, but they are NOT a musicality dimension. They:

1. **Manifest through existing metrics** (Regularity, Syncopation, Velocity Range)
2. **Are context-dependent** (no universal "optimal" value)
3. **Should be tracked separately** for algorithm tuning

The `TrillDiagnostics` class provides visibility without polluting the Pentagon framework.
