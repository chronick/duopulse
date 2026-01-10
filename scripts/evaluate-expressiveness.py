#!/usr/bin/env python3
"""
Expressiveness Evaluation Script

Evaluates pattern expressiveness across parameter sweeps to identify:
1. Parameter regions with low variation (convergence zones)
2. Seed-dependent uniqueness (do different seeds produce different patterns?)
3. Musical interest metrics (distribution, gaps, syncopation)

This creates a feedback loop for iterating on pattern generation algorithms.

Usage:
    python scripts/evaluate-expressiveness.py [--output report.md]
    make expressiveness-report

Output:
    - Console summary with pass/fail indicators
    - Markdown report with detailed analysis
    - JSON data for further processing
"""

import subprocess
import csv
import io
import json
import argparse
import math
from pathlib import Path
from dataclasses import dataclass, field, asdict
from typing import Optional
from collections import defaultdict
import sys


# =============================================================================
# Data Types
# =============================================================================

@dataclass
class Step:
    """Single step in a pattern."""
    step: int
    v1: bool
    v2: bool
    aux: bool
    v1_vel: float
    v2_vel: float
    aux_vel: float
    metric: float


@dataclass
class Pattern:
    """A complete pattern with parameters and steps."""
    energy: float
    shape: float
    axis_x: float
    axis_y: float
    drift: float
    accent: float
    seed: int
    length: int
    steps: list[Step]

    @property
    def v1_mask(self) -> int:
        """Bitmask of V1 hits."""
        mask = 0
        for s in self.steps:
            if s.v1:
                mask |= (1 << s.step)
        return mask

    @property
    def v2_mask(self) -> int:
        """Bitmask of V2 hits."""
        mask = 0
        for s in self.steps:
            if s.v2:
                mask |= (1 << s.step)
        return mask

    @property
    def aux_mask(self) -> int:
        """Bitmask of AUX hits."""
        mask = 0
        for s in self.steps:
            if s.aux:
                mask |= (1 << s.step)
        return mask

    @property
    def v1_hits(self) -> int:
        return sum(1 for s in self.steps if s.v1)

    @property
    def v2_hits(self) -> int:
        return sum(1 for s in self.steps if s.v2)

    @property
    def aux_hits(self) -> int:
        return sum(1 for s in self.steps if s.aux)


@dataclass
class PatternMetrics:
    """Computed metrics for a single pattern."""
    # Hit counts
    v1_hits: int = 0
    v2_hits: int = 0
    aux_hits: int = 0

    # Masks (for uniqueness comparison)
    v1_mask: int = 0
    v2_mask: int = 0
    aux_mask: int = 0

    # Rhythmic analysis
    quarter_note_hits: int = 0  # Hits on 0,4,8,12,16,20,24,28
    offbeat_hits: int = 0       # Hits on odd steps (V1)
    syncopation_ratio: float = 0.0  # offbeat / total

    # Gap analysis (V1)
    gaps: list[int] = field(default_factory=list)
    max_gap: int = 0
    min_gap: int = 32
    avg_gap: float = 0.0
    gap_variance: float = 0.0
    regularity_score: float = 0.0  # 0 = irregular, 1 = perfectly regular

    # Velocity metrics
    v1_avg_velocity: float = 0.0
    v1_velocity_range: float = 0.0
    v2_avg_velocity: float = 0.0
    v2_velocity_range: float = 0.0


# =============================================================================
# Pentagon of Musicality Metrics (v2)
# =============================================================================

# LHL Metric weights for 32-step pattern (2 bars of 4/4 at 16th notes)
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


@dataclass
class PentagonMetrics:
    """Pentagon of Musicality - 5 orthogonal metrics for pattern evaluation."""
    # Raw values (0-1, not zone-adjusted)
    raw_syncopation: float = 0.0
    raw_density: float = 0.0
    raw_velocity_range: float = 0.0
    raw_voice_separation: float = 0.0
    raw_regularity: float = 0.0

    # Scored values (0-1, zone-adjusted based on SHAPE/ENERGY/ACCENT)
    score_syncopation: float = 0.0
    score_density: float = 0.0
    score_velocity_range: float = 0.0
    score_voice_separation: float = 0.0
    score_regularity: float = 0.0

    # Composite scores
    composite: float = 0.0
    zone_compliance: float = 0.0
    shape_zone: str = "stable"


def compute_syncopation(hits: list[bool], pattern_length: int = 32) -> float:
    """
    Compute LHL syncopation score.

    Syncopation = tension from weak-to-strong transitions where
    a hit on a weak position is followed by a rest on a stronger position.
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


def compute_density(
    v1_hits: list[bool],
    v2_hits: list[bool],
    aux_hits: list[bool],
    pattern_length: int = 32
) -> float:
    """Compute pattern density as fraction of active steps."""
    active_steps = sum(
        1 for i in range(pattern_length)
        if v1_hits[i] or v2_hits[i] or aux_hits[i]
    )
    return active_steps / pattern_length


def compute_velocity_range(
    v1_hits: list[bool],
    v2_hits: list[bool],
    aux_hits: list[bool],
    v1_vels: list[float],
    v2_vels: list[float],
    aux_vels: list[float]
) -> float:
    """Compute velocity range across all voices."""
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


def compute_voice_separation(
    v1_hits: list[bool],
    v2_hits: list[bool],
    aux_hits: list[bool],
    pattern_length: int = 32
) -> float:
    """Compute voice separation as inverse of overlap."""
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


def compute_regularity(hits: list[bool], pattern_length: int = 32) -> float:
    """
    Compute regularity as inverse of gap variance.

    Uses coefficient of variation (CV) = std_dev / mean
    Regularity = 1 - min(1, CV)
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


def score_syncopation(raw: float, shape: float) -> float:
    """Score syncopation relative to SHAPE zone expectation."""
    # Tighter ranges for more discriminating evaluation
    # Stable: 0.00-0.22, Syncopated: 0.22-0.48, Wild: 0.42-0.75
    if shape < 0.3:
        target_center, target_width = 0.11, 0.14  # center=0.11, range=0.00-0.22
    elif shape < 0.7:
        target_center, target_width = 0.35, 0.16  # center=0.35, range=0.22-0.48
    else:
        target_center, target_width = 0.58, 0.20  # center=0.58, range=0.42-0.75

    distance = abs(raw - target_center)
    return max(0.0, 1.0 - (distance / target_width) ** 2)


def score_density(raw: float, shape: float, energy: float) -> float:
    """Score density relative to SHAPE and ENERGY."""
    # Tighter ranges: Stable: 0.15-0.32, Syncopated: 0.25-0.48, Wild: 0.32-0.65
    # ENERGY scales within these bounds
    if shape < 0.3:
        base_min, base_max = 0.15, 0.32
    elif shape < 0.7:
        base_min, base_max = 0.25, 0.48
    else:
        base_min, base_max = 0.32, 0.65

    # Energy scales the target within the zone's range
    zone_range = base_max - base_min
    energy_target = base_min + energy * zone_range
    width = zone_range / 2.2  # Tighter width

    distance = abs(raw - energy_target)
    return max(0.0, 1.0 - (distance / width) ** 2)


def score_velocity_range(raw: float, accent: float) -> float:
    """Score velocity range relative to ACCENT parameter."""
    # Tighter ranges: Low: 0.12-0.38, Med: 0.32-0.58, High: 0.25-0.72
    if accent < 0.3:
        target_center, width = 0.25, 0.16  # center=0.25, range=0.12-0.38
    elif accent < 0.7:
        target_center, width = 0.45, 0.16  # center=0.45, range=0.32-0.58
    else:
        target_center, width = 0.48, 0.28  # center=0.48, range=0.25-0.72

    distance = abs(raw - target_center)
    return max(0.0, 1.0 - (distance / width) ** 2)


def score_voice_separation(raw: float, shape: float) -> float:
    """Score voice separation."""
    # Tighter ranges: Stable: 0.62-0.88, Syncopated: 0.52-0.78, Wild: 0.32-0.68
    if shape < 0.3:
        target_center, width = 0.75, 0.16  # center=0.75, range=0.62-0.88
    elif shape < 0.7:
        target_center, width = 0.65, 0.16  # center=0.65, range=0.52-0.78
    else:
        target_center, width = 0.50, 0.22  # center=0.50, range=0.32-0.68

    distance = abs(raw - target_center)
    return max(0.0, 1.0 - (distance / width) ** 2)


def score_regularity(raw: float, shape: float) -> float:
    """Score regularity relative to SHAPE zone."""
    # Tighter ranges: Stable: 0.72-1.0, Syncopated: 0.42-0.68, Wild: 0.12-0.48
    if shape < 0.3:
        target_center, width = 0.86, 0.18  # center=0.86, range=0.72-1.0
    elif shape < 0.7:
        target_center, width = 0.55, 0.16  # center=0.55, range=0.42-0.68
    else:
        target_center, width = 0.30, 0.22  # center=0.30, range=0.12-0.48

    distance = abs(raw - target_center)
    return max(0.0, 1.0 - (distance / width) ** 2)


def compute_pentagon_metrics(
    pattern: "Pattern",
    shape: float = 0.5,
    energy: float = 0.5,
    accent: float = 0.5,
) -> PentagonMetrics:
    """Compute full Pentagon of Musicality analysis."""
    # Extract hit and velocity lists
    v1_hits = [s.v1 for s in pattern.steps]
    v2_hits = [s.v2 for s in pattern.steps]
    aux_hits = [s.aux for s in pattern.steps]
    v1_vels = [s.v1_vel for s in pattern.steps]
    v2_vels = [s.v2_vel for s in pattern.steps]
    aux_vels = [s.aux_vel for s in pattern.steps]
    pattern_length = pattern.length

    # Compute raw metrics
    raw_sync = compute_syncopation(v1_hits, pattern_length)
    raw_dens = compute_density(v1_hits, v2_hits, aux_hits, pattern_length)
    raw_vel = compute_velocity_range(v1_hits, v2_hits, aux_hits, v1_vels, v2_vels, aux_vels)
    raw_sep = compute_voice_separation(v1_hits, v2_hits, aux_hits, pattern_length)
    raw_reg = compute_regularity(v1_hits, pattern_length)

    # Compute scored metrics
    s_sync = score_syncopation(raw_sync, shape)
    s_dens = score_density(raw_dens, shape, energy)
    s_vel = score_velocity_range(raw_vel, accent)
    s_sep = score_voice_separation(raw_sep, shape)
    s_reg = score_regularity(raw_reg, shape)

    # Composite (weighted sum)
    composite = (
        0.25 * s_sync +
        0.15 * s_dens +
        0.20 * s_vel +
        0.20 * s_sep +
        0.20 * s_reg
    )

    # Zone
    if shape < 0.3:
        zone = "stable"
    elif shape < 0.7:
        zone = "syncopated"
    else:
        zone = "wild"

    # Zone compliance = average of all scores
    zone_compliance = (s_sync + s_dens + s_vel + s_sep + s_reg) / 5

    return PentagonMetrics(
        raw_syncopation=raw_sync,
        raw_density=raw_dens,
        raw_velocity_range=raw_vel,
        raw_voice_separation=raw_sep,
        raw_regularity=raw_reg,
        score_syncopation=s_sync,
        score_density=s_dens,
        score_velocity_range=s_vel,
        score_voice_separation=s_sep,
        score_regularity=s_reg,
        composite=composite,
        zone_compliance=zone_compliance,
        shape_zone=zone,
    )


@dataclass
class SweepResult:
    """Results from sweeping a parameter."""
    param_name: str
    param_values: list[float]
    patterns: list[Pattern]
    metrics: list[PatternMetrics]


@dataclass
class SeedVariationResult:
    """Results from testing seed variation at fixed parameters."""
    energy: float
    shape: float
    drift: float
    seeds: list[int]
    patterns: list[Pattern]
    unique_v1_masks: int
    unique_v2_masks: int
    unique_aux_masks: int
    v1_variation_score: float  # 0 = all same, 1 = all different
    v2_variation_score: float
    aux_variation_score: float


@dataclass
class ExpressivenessReport:
    """Complete expressiveness evaluation report."""
    # Seed variation tests (critical)
    seed_variation_tests: list[SeedVariationResult] = field(default_factory=list)

    # Parameter sweeps
    shape_sweep: Optional[SweepResult] = None
    energy_sweep: Optional[SweepResult] = None
    drift_sweep: Optional[SweepResult] = None

    # Problem zones
    low_variation_zones: list[dict] = field(default_factory=list)
    convergence_patterns: list[dict] = field(default_factory=list)

    # Summary scores
    overall_seed_variation: float = 0.0
    v1_expressiveness: float = 0.0
    v2_expressiveness: float = 0.0
    aux_expressiveness: float = 0.0


# =============================================================================
# Pattern Generation
# =============================================================================

def run_pattern_viz(
    pattern_viz_path: Path,
    energy: float = 0.5,
    shape: float = 0.3,
    axis_x: float = 0.5,
    axis_y: float = 0.5,
    drift: float = 0.0,
    accent: float = 0.5,
    seed: int = 0xDEADBEEF,
    length: int = 32,
) -> Pattern:
    """Run pattern_viz and parse CSV output."""
    cmd = [
        str(pattern_viz_path),
        f"--energy={energy:.2f}",
        f"--shape={shape:.2f}",
        f"--axis-x={axis_x:.2f}",
        f"--axis-y={axis_y:.2f}",
        f"--drift={drift:.2f}",
        f"--accent={accent:.2f}",
        f"--seed={seed}",
        f"--length={length}",
        "--format=csv",
    ]

    result = subprocess.run(cmd, capture_output=True, text=True, check=True)
    reader = csv.DictReader(io.StringIO(result.stdout))

    steps = []
    for row in reader:
        steps.append(Step(
            step=int(row["step"]),
            v1=row["v1"] == "1",
            v2=row["v2"] == "1",
            aux=row["aux"] == "1",
            v1_vel=float(row["v1_vel"]),
            v2_vel=float(row["v2_vel"]),
            aux_vel=float(row["aux_vel"]),
            metric=float(row["metric"]),
        ))

    return Pattern(
        energy=energy,
        shape=shape,
        axis_x=axis_x,
        axis_y=axis_y,
        drift=drift,
        accent=accent,
        seed=seed,
        length=length,
        steps=steps,
    )


# =============================================================================
# Metrics Computation
# =============================================================================

def compute_metrics(pattern: Pattern) -> PatternMetrics:
    """Compute all metrics for a pattern."""
    m = PatternMetrics()

    m.v1_hits = pattern.v1_hits
    m.v2_hits = pattern.v2_hits
    m.aux_hits = pattern.aux_hits
    m.v1_mask = pattern.v1_mask
    m.v2_mask = pattern.v2_mask
    m.aux_mask = pattern.aux_mask

    # Get V1 hit positions
    v1_positions = [s.step for s in pattern.steps if s.v1]

    if v1_positions:
        # Quarter note analysis (0,4,8,12,16,20,24,28)
        quarter_notes = {0, 4, 8, 12, 16, 20, 24, 28}
        m.quarter_note_hits = len([p for p in v1_positions if p in quarter_notes])

        # Offbeat analysis (odd steps)
        m.offbeat_hits = len([p for p in v1_positions if p % 2 == 1])
        m.syncopation_ratio = m.offbeat_hits / len(v1_positions) if v1_positions else 0.0

        # Gap analysis
        if len(v1_positions) > 1:
            m.gaps = [v1_positions[i+1] - v1_positions[i]
                      for i in range(len(v1_positions)-1)]
            m.max_gap = max(m.gaps)
            m.min_gap = min(m.gaps)
            m.avg_gap = sum(m.gaps) / len(m.gaps)
            m.gap_variance = sum((g - m.avg_gap) ** 2 for g in m.gaps) / len(m.gaps)
            std_dev = m.gap_variance ** 0.5
            m.regularity_score = max(0.0, 1.0 - std_dev / 8.0)

    # Velocity analysis (V1)
    v1_vels = [s.v1_vel for s in pattern.steps if s.v1 and s.v1_vel > 0]
    if v1_vels:
        m.v1_avg_velocity = sum(v1_vels) / len(v1_vels)
        m.v1_velocity_range = max(v1_vels) - min(v1_vels)

    # Velocity analysis (V2)
    v2_vels = [s.v2_vel for s in pattern.steps if s.v2 and s.v2_vel > 0]
    if v2_vels:
        m.v2_avg_velocity = sum(v2_vels) / len(v2_vels)
        m.v2_velocity_range = max(v2_vels) - min(v2_vels)

    return m


# =============================================================================
# Expressiveness Tests
# =============================================================================

def test_seed_variation(
    pattern_viz: Path,
    energy: float,
    shape: float,
    drift: float,
    num_seeds: int = 8,
) -> SeedVariationResult:
    """Test pattern variation across different seeds at fixed parameters."""
    # Use diverse seed values
    seeds = [
        0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCD1234,
        0x87654321, 0xFEEDFACE, 0xC0FFEE42, 0xBEEFCAFE,
    ][:num_seeds]

    patterns = []
    for seed in seeds:
        pattern = run_pattern_viz(
            pattern_viz,
            energy=energy,
            shape=shape,
            drift=drift,
            seed=seed,
        )
        patterns.append(pattern)

    # Count unique masks
    v1_masks = {p.v1_mask for p in patterns}
    v2_masks = {p.v2_mask for p in patterns}
    aux_masks = {p.aux_mask for p in patterns}

    # Compute variation scores (0 = all same, 1 = all different)
    def variation_score(unique_count: int, total: int) -> float:
        if total <= 1:
            return 0.0
        return (unique_count - 1) / (total - 1)

    return SeedVariationResult(
        energy=energy,
        shape=shape,
        drift=drift,
        seeds=seeds,
        patterns=patterns,
        unique_v1_masks=len(v1_masks),
        unique_v2_masks=len(v2_masks),
        unique_aux_masks=len(aux_masks),
        v1_variation_score=variation_score(len(v1_masks), len(seeds)),
        v2_variation_score=variation_score(len(v2_masks), len(seeds)),
        aux_variation_score=variation_score(len(aux_masks), len(seeds)),
    )


def sweep_parameter(
    pattern_viz: Path,
    param_name: str,
    values: list[float],
    base_energy: float = 0.5,
    base_shape: float = 0.3,
    base_drift: float = 0.0,
    seed: int = 0xDEADBEEF,
) -> SweepResult:
    """Sweep a parameter and collect patterns/metrics."""
    patterns = []
    metrics = []

    for val in values:
        kwargs = {
            "pattern_viz_path": pattern_viz,
            "energy": base_energy,
            "shape": base_shape,
            "drift": base_drift,
            "seed": seed,
        }

        if param_name == "energy":
            kwargs["energy"] = val
        elif param_name == "shape":
            kwargs["shape"] = val
        elif param_name == "drift":
            kwargs["drift"] = val
        elif param_name == "axis_x":
            kwargs["axis_x"] = val
        elif param_name == "axis_y":
            kwargs["axis_y"] = val

        pattern = run_pattern_viz(**kwargs)
        patterns.append(pattern)
        metrics.append(compute_metrics(pattern))

    return SweepResult(
        param_name=param_name,
        param_values=values,
        patterns=patterns,
        metrics=metrics,
    )


def find_low_variation_zones(seed_tests: list[SeedVariationResult]) -> list[dict]:
    """Identify parameter regions where seed variation is low."""
    zones = []
    for test in seed_tests:
        # V2 (shimmer) variation is the critical one
        if test.v2_variation_score < 0.5:
            zones.append({
                "energy": test.energy,
                "shape": test.shape,
                "drift": test.drift,
                "v1_score": test.v1_variation_score,
                "v2_score": test.v2_variation_score,
                "aux_score": test.aux_variation_score,
                "severity": "CRITICAL" if test.v2_variation_score < 0.25 else "WARNING",
                "issue": f"V2 only has {test.unique_v2_masks} unique patterns from {len(test.seeds)} seeds",
            })
    return zones


def find_convergence_patterns(seed_tests: list[SeedVariationResult]) -> list[dict]:
    """Find specific patterns where multiple seeds converge."""
    convergences = []
    for test in seed_tests:
        if test.unique_v2_masks < len(test.seeds):
            # Group seeds by their V2 mask
            mask_to_seeds = defaultdict(list)
            for i, p in enumerate(test.patterns):
                mask_to_seeds[p.v2_mask].append(test.seeds[i])

            for mask, seeds in mask_to_seeds.items():
                if len(seeds) > 1:
                    convergences.append({
                        "energy": test.energy,
                        "shape": test.shape,
                        "drift": test.drift,
                        "v2_mask": f"0x{mask:08X}",
                        "seeds": [f"0x{s:08X}" for s in seeds],
                        "count": len(seeds),
                    })
    return convergences


# =============================================================================
# Report Generation
# =============================================================================

def generate_markdown_report(report: ExpressivenessReport) -> str:
    """Generate a detailed markdown report."""
    lines = []
    lines.append("# Expressiveness Evaluation Report")
    lines.append("")
    lines.append("This report evaluates pattern expressiveness across parameter sweeps.")
    lines.append("")

    # Executive Summary
    lines.append("## Executive Summary")
    lines.append("")
    lines.append(f"- **Overall Seed Variation Score**: {report.overall_seed_variation:.1%}")
    lines.append(f"- **V1 (Anchor) Expressiveness**: {report.v1_expressiveness:.1%}")
    lines.append(f"- **V2 (Shimmer) Expressiveness**: {report.v2_expressiveness:.1%}")
    lines.append(f"- **AUX Expressiveness**: {report.aux_expressiveness:.1%}")
    lines.append("")

    # Pass/Fail
    v2_pass = report.v2_expressiveness >= 0.5
    lines.append(f"**V2 Variation Test**: {'PASS' if v2_pass else 'FAIL'}")
    if not v2_pass:
        lines.append("")
        lines.append("> CRITICAL: Shimmer (V2) patterns do not vary sufficiently across seeds.")
        lines.append("> This means the reseed gesture feels 'broken' to users.")
    lines.append("")

    # Seed Variation Details
    lines.append("## Seed Variation Tests")
    lines.append("")
    lines.append("| Energy | Shape | Drift | V1 Unique | V2 Unique | AUX Unique | V2 Score |")
    lines.append("|--------|-------|-------|-----------|-----------|------------|----------|")

    for test in report.seed_variation_tests:
        status = "OK" if test.v2_variation_score >= 0.5 else "LOW"
        lines.append(
            f"| {test.energy:.2f} | {test.shape:.2f} | {test.drift:.2f} | "
            f"{test.unique_v1_masks}/{len(test.seeds)} | "
            f"{test.unique_v2_masks}/{len(test.seeds)} | "
            f"{test.unique_aux_masks}/{len(test.seeds)} | "
            f"{test.v2_variation_score:.0%} {status} |"
        )
    lines.append("")

    # Low Variation Zones
    if report.low_variation_zones:
        lines.append("## Low Variation Zones (Problem Areas)")
        lines.append("")
        lines.append("These parameter regions have insufficient seed variation:")
        lines.append("")
        for zone in report.low_variation_zones:
            lines.append(f"### {zone['severity']}: E={zone['energy']:.2f} S={zone['shape']:.2f} D={zone['drift']:.2f}")
            lines.append("")
            lines.append(f"- V1 Score: {zone['v1_score']:.0%}")
            lines.append(f"- V2 Score: {zone['v2_score']:.0%}")
            lines.append(f"- AUX Score: {zone['aux_score']:.0%}")
            lines.append(f"- Issue: {zone['issue']}")
            lines.append("")

    # Convergence Patterns
    if report.convergence_patterns:
        lines.append("## Convergence Patterns (Identical Outputs)")
        lines.append("")
        lines.append("These specific patterns are produced by multiple seeds:")
        lines.append("")
        for conv in report.convergence_patterns[:10]:  # Limit to 10
            lines.append(f"- **{conv['v2_mask']}** produced by {conv['count']} seeds at "
                        f"E={conv['energy']:.2f} S={conv['shape']:.2f} D={conv['drift']:.2f}")
            lines.append(f"  - Seeds: {', '.join(conv['seeds'])}")
        lines.append("")

    # Pentagon of Musicality Analysis
    lines.append("## Pentagon of Musicality Analysis")
    lines.append("")
    lines.append("Five orthogonal metrics measuring pattern musicality:")
    lines.append("- **Syncopation**: Tension from metric displacement (LHL model)")
    lines.append("- **Density**: Activity level (hits per step)")
    lines.append("- **Velocity Range**: Dynamic contrast")
    lines.append("- **Voice Separation**: Voice independence (1 - overlap)")
    lines.append("- **Regularity**: Temporal predictability (1 - CV of gaps)")
    lines.append("")

    if report.shape_sweep:
        lines.append("### Pentagon Metrics vs SHAPE")
        lines.append("")
        lines.append("| SHAPE | Zone | Sync | Dens | VelRng | VoiceSep | Reg | Composite |")
        lines.append("|-------|------|------|------|--------|----------|-----|-----------|")
        for i, val in enumerate(report.shape_sweep.param_values):
            p = report.shape_sweep.patterns[i]
            pm = compute_pentagon_metrics(p, p.shape, p.energy, p.accent)
            lines.append(f"| {val:.2f} | {pm.shape_zone[:4]} | "
                        f"{pm.raw_syncopation:.2f} | {pm.raw_density:.2f} | "
                        f"{pm.raw_velocity_range:.2f} | {pm.raw_voice_separation:.2f} | "
                        f"{pm.raw_regularity:.2f} | {pm.composite:.0%} |")
        lines.append("")

    # Parameter Sweep Analysis
    if report.shape_sweep:
        lines.append("## SHAPE Parameter Sweep")
        lines.append("")
        lines.append("| SHAPE | V1 Hits | V2 Hits | Regularity | Syncopation |")
        lines.append("|-------|---------|---------|------------|-------------|")
        for i, val in enumerate(report.shape_sweep.param_values):
            m = report.shape_sweep.metrics[i]
            lines.append(f"| {val:.2f} | {m.v1_hits} | {m.v2_hits} | "
                        f"{m.regularity_score:.2f} | {m.syncopation_ratio:.0%} |")
        lines.append("")

    if report.energy_sweep:
        lines.append("## ENERGY Parameter Sweep")
        lines.append("")
        lines.append("| ENERGY | V1 Hits | V2 Hits | V1 Mask | V2 Mask |")
        lines.append("|--------|---------|---------|---------|---------|")
        for i, val in enumerate(report.energy_sweep.param_values):
            m = report.energy_sweep.metrics[i]
            lines.append(f"| {val:.2f} | {m.v1_hits} | {m.v2_hits} | "
                        f"0x{m.v1_mask:08X} | 0x{m.v2_mask:08X} |")
        lines.append("")

    # Recommendations
    lines.append("## Recommendations")
    lines.append("")
    if report.v2_expressiveness < 0.5:
        lines.append("### Priority 1: Fix V2 (Shimmer) Variation")
        lines.append("")
        lines.append("The shimmer voice does not vary enough with different seeds. Recommended fixes:")
        lines.append("")
        lines.append("1. **Inject seed into even-spacing placement** (Low risk)")
        lines.append("   - Add seed-based micro-jitter to `PlaceEvenlySpaced()`")
        lines.append("   - Preserves overall structure while adding variation")
        lines.append("")
        lines.append("2. **Raise default DRIFT from 0.0 to 0.25** (Medium risk)")
        lines.append("   - Enables seed-sensitive weighted placement")
        lines.append("   - Matches user expectation that reseed changes pattern")
        lines.append("")
        lines.append("3. **Use Gumbel selection within gaps** (Structural fix)")
        lines.append("   - Apply same selection mechanism to shimmer as anchor")
        lines.append("   - Most comprehensive solution")
        lines.append("")
    else:
        lines.append("No critical issues found. Pattern expressiveness is adequate.")
        lines.append("")
        lines.append("Consider monitoring these metrics after future changes:")
        lines.append("- V2 variation score should stay above 50%")
        lines.append("- Syncopation should increase with SHAPE")
        lines.append("- Hit counts should scale with ENERGY")

    lines.append("")
    lines.append("---")
    lines.append("*Generated by evaluate-expressiveness.py*")

    return "\n".join(lines)


def print_console_summary(report: ExpressivenessReport):
    """Print a concise console summary."""
    print("=" * 60)
    print("EXPRESSIVENESS EVALUATION SUMMARY")
    print("=" * 60)
    print()

    # Overall scores
    print(f"Overall Seed Variation: {report.overall_seed_variation:.0%}")
    print(f"  V1 (Anchor):  {report.v1_expressiveness:.0%}")
    print(f"  V2 (Shimmer): {report.v2_expressiveness:.0%}")
    print(f"  AUX:          {report.aux_expressiveness:.0%}")
    print()

    # Pass/Fail
    v2_pass = report.v2_expressiveness >= 0.5
    v1_pass = report.v1_expressiveness >= 0.5

    if v2_pass and v1_pass:
        print("[PASS] Expressiveness is adequate")
    else:
        print("[FAIL] Expressiveness issues detected:")
        if not v2_pass:
            print("       - V2 (Shimmer) variation is too low")
        if not v1_pass:
            print("       - V1 (Anchor) variation is too low")
    print()

    # Problem zones
    if report.low_variation_zones:
        print("LOW VARIATION ZONES:")
        for zone in report.low_variation_zones[:5]:
            print(f"  [{zone['severity']}] E={zone['energy']:.2f} S={zone['shape']:.2f} D={zone['drift']:.2f}")
            print(f"           V2 score: {zone['v2_score']:.0%}")
    print()

    # Convergence patterns
    if report.convergence_patterns:
        print(f"CONVERGENCE PATTERNS: {len(report.convergence_patterns)} found")
        for conv in report.convergence_patterns[:3]:
            print(f"  {conv['v2_mask']}: {conv['count']} seeds produce same pattern")
    print()


# =============================================================================
# Main Entry Point
# =============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="Evaluate pattern expressiveness across parameter space"
    )
    parser.add_argument(
        "--pattern-viz",
        type=Path,
        default=Path("build/pattern_viz"),
        help="Path to pattern_viz binary (default: build/pattern_viz)"
    )
    parser.add_argument(
        "--output", "-o",
        type=Path,
        default=Path("docs/design/expressiveness-report.md"),
        help="Output markdown report path"
    )
    parser.add_argument(
        "--json",
        type=Path,
        help="Also output JSON data (optional)"
    )
    parser.add_argument(
        "--quick",
        action="store_true",
        help="Run quick evaluation (fewer parameter combinations)"
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Print detailed progress"
    )
    args = parser.parse_args()

    # Check pattern_viz exists
    if not args.pattern_viz.exists():
        print(f"Error: pattern_viz not found at {args.pattern_viz}")
        print("Run 'make pattern-viz' first to build the tool.")
        return 1

    print(f"Using pattern_viz: {args.pattern_viz}")
    print()

    report = ExpressivenessReport()

    # ==========================================================================
    # Seed Variation Tests (Critical)
    # ==========================================================================
    print("Running seed variation tests...")

    # Test grid: combinations of energy, shape, drift
    if args.quick:
        energy_values = [0.3, 0.6]
        shape_values = [0.15, 0.50, 0.85]
        drift_values = [0.0, 0.5]
    else:
        energy_values = [0.2, 0.4, 0.6, 0.8]
        shape_values = [0.0, 0.15, 0.30, 0.50, 0.70, 0.85, 1.0]
        drift_values = [0.0, 0.25, 0.5, 0.75]

    total_tests = len(energy_values) * len(shape_values) * len(drift_values)
    test_num = 0

    for energy in energy_values:
        for shape in shape_values:
            for drift in drift_values:
                test_num += 1
                if args.verbose:
                    print(f"  [{test_num}/{total_tests}] E={energy:.2f} S={shape:.2f} D={drift:.2f}")

                result = test_seed_variation(
                    args.pattern_viz,
                    energy=energy,
                    shape=shape,
                    drift=drift,
                    num_seeds=8 if not args.quick else 4,
                )
                report.seed_variation_tests.append(result)

    print(f"  Completed {len(report.seed_variation_tests)} seed variation tests")
    print()

    # ==========================================================================
    # Parameter Sweeps
    # ==========================================================================
    print("Running parameter sweeps...")

    # SHAPE sweep
    shape_values = [0.0, 0.15, 0.30, 0.50, 0.70, 0.85, 1.0]
    report.shape_sweep = sweep_parameter(
        args.pattern_viz,
        "shape",
        shape_values,
        base_energy=0.5,
    )
    print("  - SHAPE sweep complete")

    # ENERGY sweep
    energy_values = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]
    report.energy_sweep = sweep_parameter(
        args.pattern_viz,
        "energy",
        energy_values,
        base_shape=0.3,
    )
    print("  - ENERGY sweep complete")

    # DRIFT sweep
    drift_values = [0.0, 0.25, 0.5, 0.75, 1.0]
    report.drift_sweep = sweep_parameter(
        args.pattern_viz,
        "drift",
        drift_values,
        base_energy=0.5,
        base_shape=0.3,
    )
    print("  - DRIFT sweep complete")
    print()

    # ==========================================================================
    # Analysis
    # ==========================================================================
    print("Analyzing results...")

    # Find problem zones
    report.low_variation_zones = find_low_variation_zones(report.seed_variation_tests)
    report.convergence_patterns = find_convergence_patterns(report.seed_variation_tests)

    # Compute overall scores (average across all seed tests)
    v1_scores = [t.v1_variation_score for t in report.seed_variation_tests]
    v2_scores = [t.v2_variation_score for t in report.seed_variation_tests]
    aux_scores = [t.aux_variation_score for t in report.seed_variation_tests]

    report.v1_expressiveness = sum(v1_scores) / len(v1_scores) if v1_scores else 0.0
    report.v2_expressiveness = sum(v2_scores) / len(v2_scores) if v2_scores else 0.0
    report.aux_expressiveness = sum(aux_scores) / len(aux_scores) if aux_scores else 0.0
    report.overall_seed_variation = (
        report.v1_expressiveness + report.v2_expressiveness + report.aux_expressiveness
    ) / 3.0

    print()

    # ==========================================================================
    # Output
    # ==========================================================================

    # Console summary
    print_console_summary(report)

    # Markdown report
    markdown = generate_markdown_report(report)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(markdown)
    print(f"Report written to: {args.output}")

    # JSON output (optional)
    if args.json:
        # Convert to JSON-serializable format
        json_data = {
            "overall_seed_variation": report.overall_seed_variation,
            "v1_expressiveness": report.v1_expressiveness,
            "v2_expressiveness": report.v2_expressiveness,
            "aux_expressiveness": report.aux_expressiveness,
            "low_variation_zones": report.low_variation_zones,
            "convergence_patterns": report.convergence_patterns,
            "seed_tests": [
                {
                    "energy": t.energy,
                    "shape": t.shape,
                    "drift": t.drift,
                    "v1_score": t.v1_variation_score,
                    "v2_score": t.v2_variation_score,
                    "aux_score": t.aux_variation_score,
                    "unique_v1": t.unique_v1_masks,
                    "unique_v2": t.unique_v2_masks,
                    "unique_aux": t.unique_aux_masks,
                }
                for t in report.seed_variation_tests
            ],
        }
        args.json.write_text(json.dumps(json_data, indent=2))
        print(f"JSON data written to: {args.json}")

    # Exit code based on pass/fail
    return 0 if report.v2_expressiveness >= 0.5 else 1


if __name__ == "__main__":
    sys.exit(main())
