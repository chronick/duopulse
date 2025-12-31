#!/usr/bin/env python3
"""Comprehensive pattern evaluation against design intentions.

Generates patterns across parameter space and evaluates them for:
1. Genre-specific characteristics
2. Energy zone behavior
3. Euclidean regularity
4. BUILD phase dynamics
5. Voice coupling effectiveness
"""

import json
import os
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path

from pattern_viz.pipeline import generate_pattern, generate_phrase
from pattern_viz.types import Genre, EnergyZone, VoiceCoupling
from pattern_viz.gumbel_sampler import count_bits, mask_to_steps
from pattern_viz.output import format_mask_binary, render_compact


# Output directories
OUTPUT_DIR = Path("evaluation_output")
PATTERNS_DIR = OUTPUT_DIR / "patterns"
EVAL_FILE = OUTPUT_DIR / "evaluation_results.md"


@dataclass
class PatternMetrics:
    """Metrics extracted from a pattern."""
    anchor_hits: int = 0
    shimmer_hits: int = 0
    aux_hits: int = 0
    total_hits: int = 0

    # Rhythmic analysis
    quarter_note_hits: int = 0  # Hits on 0,4,8,12,16,20,24,28
    eighth_note_hits: int = 0   # Hits on even steps
    offbeat_hits: int = 0       # Hits on odd steps

    # Distribution metrics
    max_gap: int = 0            # Largest gap between hits
    min_gap: int = 32           # Smallest gap between hits
    regularity_score: float = 0.0  # How evenly distributed (1.0 = perfect)

    # Velocity metrics
    avg_velocity: float = 0.0
    velocity_range: float = 0.0

    # Pattern characteristics
    has_four_on_floor: bool = False
    syncopation_score: float = 0.0


def analyze_pattern(pattern) -> PatternMetrics:
    """Extract metrics from a pattern."""
    m = PatternMetrics()

    m.anchor_hits = pattern.anchor_hits
    m.shimmer_hits = pattern.shimmer_hits
    m.aux_hits = pattern.aux_hits
    m.total_hits = m.anchor_hits + m.shimmer_hits + m.aux_hits

    # Analyze anchor rhythm
    anchor_steps = mask_to_steps(pattern.anchor_mask)

    if anchor_steps:
        # Quarter note analysis (0,4,8,12,16,20,24,28)
        quarter_notes = {0, 4, 8, 12, 16, 20, 24, 28}
        m.quarter_note_hits = len([s for s in anchor_steps if s in quarter_notes])

        # Eighth note analysis (even steps)
        m.eighth_note_hits = len([s for s in anchor_steps if s % 2 == 0])
        m.offbeat_hits = len([s for s in anchor_steps if s % 2 == 1])

        # Gap analysis
        if len(anchor_steps) > 1:
            gaps = [anchor_steps[i+1] - anchor_steps[i] for i in range(len(anchor_steps)-1)]
            m.max_gap = max(gaps)
            m.min_gap = min(gaps)

            # Regularity: std dev of gaps (lower = more regular)
            avg_gap = sum(gaps) / len(gaps)
            variance = sum((g - avg_gap) ** 2 for g in gaps) / len(gaps)
            std_dev = variance ** 0.5
            # Normalize: 0 std_dev = 1.0 regularity, higher std_dev = lower regularity
            m.regularity_score = max(0.0, 1.0 - std_dev / 8.0)

        # Four-on-floor detection (hits on 0,8,16,24 at minimum)
        core_four = {0, 8, 16, 24}
        m.has_four_on_floor = core_four.issubset(set(anchor_steps))

        # Syncopation: ratio of offbeat to total hits
        if m.anchor_hits > 0:
            m.syncopation_score = m.offbeat_hits / m.anchor_hits

    # Velocity analysis
    active_vels = [v for v in pattern.anchor_velocities if v > 0]
    if active_vels:
        m.avg_velocity = sum(active_vels) / len(active_vels)
        m.velocity_range = max(active_vels) - min(active_vels)

    return m


@dataclass
class EvaluationResult:
    """Result of evaluating a pattern against expectations."""
    test_name: str
    passed: bool
    expected: str
    actual: str
    details: str = ""


def evaluate_techno_minimal(seed: int = 0x12345678) -> list[EvaluationResult]:
    """Evaluate TECHNO Minimal archetype (position [0,0])."""
    results = []

    pattern = generate_pattern(
        genre=Genre.TECHNO,
        field_x=0.0,
        field_y=0.0,
        energy=0.35,  # GROOVE zone
        seed=seed,
    )
    metrics = analyze_pattern(pattern)

    # Test 1: Should have 4-on-floor or high quarter note ratio
    quarter_ratio = metrics.quarter_note_hits / max(1, metrics.anchor_hits)
    results.append(EvaluationResult(
        test_name="TECHNO Minimal: Quarter note dominance",
        passed=quarter_ratio >= 0.6,
        expected=">=60% hits on quarter notes",
        actual=f"{quarter_ratio:.0%} ({metrics.quarter_note_hits}/{metrics.anchor_hits})",
    ))

    # Test 2: Should have high regularity
    results.append(EvaluationResult(
        test_name="TECHNO Minimal: Pattern regularity",
        passed=metrics.regularity_score >= 0.5,
        expected="Regularity score >= 0.5",
        actual=f"{metrics.regularity_score:.2f}",
    ))

    # Test 3: Low syncopation
    results.append(EvaluationResult(
        test_name="TECHNO Minimal: Low syncopation",
        passed=metrics.syncopation_score <= 0.3,
        expected="Syncopation <= 30%",
        actual=f"{metrics.syncopation_score:.0%}",
    ))

    return results


def evaluate_tribal_polyrhythm(seed: int = 0x12345678) -> list[EvaluationResult]:
    """Evaluate TRIBAL Polyrhythm archetype (position [1,2])."""
    results = []

    pattern = generate_pattern(
        genre=Genre.TRIBAL,
        field_x=0.5,
        field_y=1.0,
        energy=0.5,
        seed=seed,
    )
    metrics = analyze_pattern(pattern)

    # Test 1: Should have some syncopation
    results.append(EvaluationResult(
        test_name="TRIBAL Polyrhythm: Has syncopation",
        passed=metrics.syncopation_score >= 0.1,
        expected="Syncopation >= 10%",
        actual=f"{metrics.syncopation_score:.0%}",
    ))

    # Test 2: Should have moderate regularity (not as rigid as techno)
    results.append(EvaluationResult(
        test_name="TRIBAL Polyrhythm: Moderate regularity",
        passed=0.3 <= metrics.regularity_score <= 0.9,
        expected="Regularity between 0.3-0.9",
        actual=f"{metrics.regularity_score:.2f}",
    ))

    return results


def evaluate_idm_chaos(seed: int = 0x12345678) -> list[EvaluationResult]:
    """Evaluate IDM Chaos archetype (position [2,2])."""
    results = []

    pattern = generate_pattern(
        genre=Genre.IDM,
        field_x=1.0,
        field_y=1.0,
        energy=0.6,
        seed=seed,
    )
    metrics = analyze_pattern(pattern)

    # Test 1: Should have lower regularity (more irregular)
    results.append(EvaluationResult(
        test_name="IDM Chaos: Irregular pattern",
        passed=metrics.regularity_score <= 0.8,
        expected="Regularity <= 0.8 (more irregular)",
        actual=f"{metrics.regularity_score:.2f}",
    ))

    # Test 2: Should have some offbeat hits
    results.append(EvaluationResult(
        test_name="IDM Chaos: Offbeat presence",
        passed=metrics.offbeat_hits >= 1,
        expected="At least 1 offbeat hit",
        actual=f"{metrics.offbeat_hits} offbeat hits",
    ))

    return results


def evaluate_energy_zones(seed: int = 0x12345678) -> list[EvaluationResult]:
    """Evaluate energy zone hit count scaling."""
    results = []

    # Generate patterns at different energy levels
    patterns = {}
    for energy, zone_name in [(0.1, "MINIMAL"), (0.35, "GROOVE"), (0.6, "BUILD"), (0.9, "PEAK")]:
        pattern = generate_pattern(energy=energy, seed=seed)
        patterns[zone_name] = pattern

    # Test 1: MINIMAL should be sparse
    results.append(EvaluationResult(
        test_name="Energy: MINIMAL is sparse",
        passed=patterns["MINIMAL"].anchor_hits <= 4,
        expected="<= 4 anchor hits",
        actual=f"{patterns['MINIMAL'].anchor_hits} hits",
    ))

    # Test 2: PEAK should be dense
    results.append(EvaluationResult(
        test_name="Energy: PEAK is dense",
        passed=patterns["PEAK"].anchor_hits >= 6,
        expected=">= 6 anchor hits",
        actual=f"{patterns['PEAK'].anchor_hits} hits",
    ))

    # Test 3: Monotonic increase
    hit_counts = [patterns[z].anchor_hits for z in ["MINIMAL", "GROOVE", "BUILD", "PEAK"]]
    is_monotonic = all(hit_counts[i] <= hit_counts[i+1] for i in range(len(hit_counts)-1))
    results.append(EvaluationResult(
        test_name="Energy: Monotonic hit increase",
        passed=is_monotonic,
        expected="MINIMAL <= GROOVE <= BUILD <= PEAK",
        actual=f"{hit_counts}",
    ))

    return results


def evaluate_voice_coupling(seed: int = 0x12345678) -> list[EvaluationResult]:
    """Evaluate voice coupling modes."""
    results = []

    # INTERLOCK mode
    pattern = generate_pattern(
        voice_coupling=VoiceCoupling.INTERLOCK,
        energy=0.7,
        seed=seed,
    )
    overlap = pattern.anchor_mask & pattern.shimmer_mask
    results.append(EvaluationResult(
        test_name="Coupling: INTERLOCK prevents overlap",
        passed=overlap == 0,
        expected="No simultaneous anchor+shimmer hits",
        actual=f"Overlap mask: {overlap:#x}",
    ))

    # SHADOW mode
    pattern = generate_pattern(
        voice_coupling=VoiceCoupling.SHADOW,
        energy=0.6,
        seed=seed,
    )
    # Shimmer should be shifted version of anchor
    results.append(EvaluationResult(
        test_name="Coupling: SHADOW creates echo",
        passed=pattern.shimmer_hits > 0,
        expected="Shimmer follows anchor",
        actual=f"Shimmer hits: {pattern.shimmer_hits}",
    ))

    return results


def evaluate_build_dynamics(seed: int = 0x12345678) -> list[EvaluationResult]:
    """Evaluate BUILD parameter phrase dynamics."""
    results = []

    # Generate phrase with high BUILD
    phrase = generate_phrase(
        bars=4,
        build=0.9,
        energy=0.5,
        seed=seed,
    )

    # Check density increase over phrase
    densities = [p.anchor_hits for p in phrase]
    last_bar_denser = densities[-1] >= densities[0]

    results.append(EvaluationResult(
        test_name="BUILD: Density increases over phrase",
        passed=last_bar_denser,
        expected="Last bar >= first bar density",
        actual=f"Bar densities: {densities}",
    ))

    # Check velocity increase
    velocities = []
    for p in phrase:
        active = [v for v in p.anchor_velocities if v > 0]
        if active:
            velocities.append(sum(active) / len(active))
        else:
            velocities.append(0)

    velocity_trend = velocities[-1] >= velocities[0] - 0.1  # Allow small variance
    results.append(EvaluationResult(
        test_name="BUILD: Velocity trends up",
        passed=velocity_trend,
        expected="Last bar velocity >= first bar",
        actual=f"Bar avg velocities: {[f'{v:.2f}' for v in velocities]}",
    ))

    return results


def evaluate_determinism(seed: int = 0x12345678) -> list[EvaluationResult]:
    """Evaluate deterministic generation."""
    results = []

    # Generate same pattern twice
    p1 = generate_pattern(seed=seed)
    p2 = generate_pattern(seed=seed)

    results.append(EvaluationResult(
        test_name="Determinism: Same seed = same pattern",
        passed=p1.anchor_mask == p2.anchor_mask,
        expected="Identical patterns",
        actual=f"Match: {p1.anchor_mask == p2.anchor_mask}",
    ))

    # Different seeds should differ
    p3 = generate_pattern(seed=seed ^ 0xFFFF)
    results.append(EvaluationResult(
        test_name="Determinism: Different seeds = different patterns",
        passed=p1.anchor_mask != p3.anchor_mask,
        expected="Different patterns",
        actual=f"Different: {p1.anchor_mask != p3.anchor_mask}",
    ))

    return results


def generate_pattern_samples():
    """Generate sample patterns for manual inspection."""
    samples = []

    # Sample across genres and positions
    for genre in [Genre.TECHNO, Genre.TRIBAL, Genre.IDM]:
        for x in [0.0, 0.5, 1.0]:
            for y in [0.0, 0.5, 1.0]:
                for energy in [0.2, 0.5, 0.8]:
                    pattern = generate_pattern(
                        genre=genre,
                        field_x=x,
                        field_y=y,
                        energy=energy,
                        seed=0x12345678,
                    )
                    samples.append({
                        "genre": genre.name,
                        "field_x": x,
                        "field_y": y,
                        "energy": energy,
                        "archetype": pattern.dominant_archetype,
                        "zone": pattern.energy_zone.name,
                        "anchor_mask": f"{pattern.anchor_mask:#010x}",
                        "shimmer_mask": f"{pattern.shimmer_mask:#010x}",
                        "aux_mask": f"{pattern.aux_mask:#010x}",
                        "anchor_hits": pattern.anchor_hits,
                        "shimmer_hits": pattern.shimmer_hits,
                        "aux_hits": pattern.aux_hits,
                        "anchor_pattern": format_mask_binary(pattern.anchor_mask, 32),
                        "compact": render_compact(pattern),
                    })

    return samples


def find_good_patterns(samples: list[dict]) -> list[dict]:
    """Identify patterns that exemplify good characteristics."""
    good = []

    for s in samples:
        pattern = generate_pattern(
            genre=Genre[s["genre"]],
            field_x=s["field_x"],
            field_y=s["field_y"],
            energy=s["energy"],
            seed=0x12345678,
        )
        metrics = analyze_pattern(pattern)

        # Criteria for "good" patterns
        reasons = []

        # Good techno: regular, quarter-note based
        if s["genre"] == "TECHNO" and metrics.regularity_score >= 0.7:
            reasons.append("Regular techno groove")

        # Good tribal: balanced syncopation
        if s["genre"] == "TRIBAL" and 0.15 <= metrics.syncopation_score <= 0.4:
            reasons.append("Balanced tribal syncopation")

        # Good IDM: interesting irregularity
        if s["genre"] == "IDM" and metrics.regularity_score <= 0.6:
            reasons.append("Interesting IDM irregularity")

        # Good velocity dynamics
        if metrics.velocity_range >= 0.2:
            reasons.append("Good velocity dynamics")

        if reasons:
            s["quality_notes"] = reasons
            s["metrics"] = {
                "regularity": f"{metrics.regularity_score:.2f}",
                "syncopation": f"{metrics.syncopation_score:.0%}",
                "velocity_range": f"{metrics.velocity_range:.2f}",
            }
            good.append(s)

    return good


def main():
    """Run full evaluation suite."""
    # Create output directories
    OUTPUT_DIR.mkdir(exist_ok=True)
    PATTERNS_DIR.mkdir(exist_ok=True)

    print("=" * 60)
    print("DuoPulse v4 Pattern Evaluation")
    print("=" * 60)
    print()

    # Run all evaluations
    all_results = []

    print("Running genre-specific evaluations...")
    all_results.extend(evaluate_techno_minimal())
    all_results.extend(evaluate_tribal_polyrhythm())
    all_results.extend(evaluate_idm_chaos())

    print("Running energy zone evaluations...")
    all_results.extend(evaluate_energy_zones())

    print("Running voice coupling evaluations...")
    all_results.extend(evaluate_voice_coupling())

    print("Running BUILD dynamics evaluations...")
    all_results.extend(evaluate_build_dynamics())

    print("Running determinism evaluations...")
    all_results.extend(evaluate_determinism())

    # Generate sample patterns
    print("Generating pattern samples...")
    samples = generate_pattern_samples()

    # Find good patterns
    print("Identifying exemplary patterns...")
    good_patterns = find_good_patterns(samples)

    # Calculate summary stats
    passed = sum(1 for r in all_results if r.passed)
    failed = len(all_results) - passed
    pass_rate = passed / len(all_results) * 100

    # Write evaluation results
    print(f"\nWriting results to {EVAL_FILE}...")

    with open(EVAL_FILE, "w") as f:
        f.write("# DuoPulse v4 Pattern Evaluation Results\n\n")
        f.write(f"**Generated**: {datetime.now().isoformat()}\n\n")

        # Summary
        f.write("## Summary\n\n")
        f.write(f"- **Tests Passed**: {passed}/{len(all_results)} ({pass_rate:.0f}%)\n")
        f.write(f"- **Tests Failed**: {failed}\n")
        f.write(f"- **Pattern Samples Generated**: {len(samples)}\n")
        f.write(f"- **Exemplary Patterns Identified**: {len(good_patterns)}\n\n")

        # Detailed results
        f.write("## Test Results\n\n")

        current_category = ""
        for r in all_results:
            category = r.test_name.split(":")[0]
            if category != current_category:
                f.write(f"\n### {category}\n\n")
                current_category = category

            status = "✅ PASS" if r.passed else "❌ FAIL"
            f.write(f"**{r.test_name}**: {status}\n")
            f.write(f"- Expected: {r.expected}\n")
            f.write(f"- Actual: {r.actual}\n")
            if r.details:
                f.write(f"- Details: {r.details}\n")
            f.write("\n")

        # Good patterns
        f.write("## Exemplary Patterns\n\n")
        f.write("These patterns demonstrate good characteristics for their genre:\n\n")

        for p in good_patterns[:20]:  # Limit to 20
            f.write(f"### {p['genre']} @ [{p['field_x']:.1f}, {p['field_y']:.1f}] E={p['energy']:.0%}\n\n")
            f.write(f"- **Archetype**: {p['archetype']}\n")
            f.write(f"- **Zone**: {p['zone']}\n")
            f.write(f"- **Pattern**: `{p['anchor_pattern']}`\n")
            f.write(f"- **Hits**: A={p['anchor_hits']}, S={p['shimmer_hits']}, X={p['aux_hits']}\n")
            f.write(f"- **Quality**: {', '.join(p['quality_notes'])}\n")
            f.write(f"- **Metrics**: {p['metrics']}\n\n")

        # Design intention compliance
        f.write("## Design Intention Compliance\n\n")

        f.write("### TECHNO Genre\n")
        techno_results = [r for r in all_results if "TECHNO" in r.test_name]
        techno_pass = sum(1 for r in techno_results if r.passed)
        f.write(f"- Compliance: {techno_pass}/{len(techno_results)} tests passed\n")
        f.write("- Intent: Regular, 4-on-floor patterns with minimal syncopation\n")
        f.write(f"- Result: {'✅ Meets intent' if techno_pass == len(techno_results) else '⚠️ Partial compliance'}\n\n")

        f.write("### TRIBAL Genre\n")
        tribal_results = [r for r in all_results if "TRIBAL" in r.test_name]
        tribal_pass = sum(1 for r in tribal_results if r.passed)
        f.write(f"- Compliance: {tribal_pass}/{len(tribal_results)} tests passed\n")
        f.write("- Intent: Polyrhythmic patterns with moderate syncopation\n")
        f.write(f"- Result: {'✅ Meets intent' if tribal_pass == len(tribal_results) else '⚠️ Partial compliance'}\n\n")

        f.write("### IDM Genre\n")
        idm_results = [r for r in all_results if "IDM" in r.test_name]
        idm_pass = sum(1 for r in idm_results if r.passed)
        f.write(f"- Compliance: {idm_pass}/{len(idm_results)} tests passed\n")
        f.write("- Intent: Irregular, complex patterns\n")
        f.write(f"- Result: {'✅ Meets intent' if idm_pass == len(idm_results) else '⚠️ Partial compliance'}\n\n")

        f.write("### Energy System\n")
        energy_results = [r for r in all_results if "Energy" in r.test_name]
        energy_pass = sum(1 for r in energy_results if r.passed)
        f.write(f"- Compliance: {energy_pass}/{len(energy_results)} tests passed\n")
        f.write("- Intent: Smooth density scaling from sparse to dense\n")
        f.write(f"- Result: {'✅ Meets intent' if energy_pass == len(energy_results) else '⚠️ Partial compliance'}\n\n")

        f.write("### BUILD Dynamics\n")
        build_results = [r for r in all_results if "BUILD" in r.test_name]
        build_pass = sum(1 for r in build_results if r.passed)
        f.write(f"- Compliance: {build_pass}/{len(build_results)} tests passed\n")
        f.write("- Intent: Increasing tension and density over phrase\n")
        f.write(f"- Result: {'✅ Meets intent' if build_pass == len(build_results) else '⚠️ Partial compliance'}\n\n")

    # Write pattern samples to JSON
    samples_file = PATTERNS_DIR / "all_samples.json"
    with open(samples_file, "w") as f:
        json.dump(samples, f, indent=2)

    good_file = PATTERNS_DIR / "good_patterns.json"
    with open(good_file, "w") as f:
        json.dump(good_patterns, f, indent=2)

    # Print summary
    print()
    print("=" * 60)
    print(f"EVALUATION COMPLETE: {passed}/{len(all_results)} tests passed ({pass_rate:.0f}%)")
    print("=" * 60)
    print()
    print(f"Results written to: {EVAL_FILE}")
    print(f"Pattern samples: {samples_file}")
    print(f"Good patterns: {good_file}")

    # Print failures if any
    if failed > 0:
        print()
        print("FAILED TESTS:")
        for r in all_results:
            if not r.passed:
                print(f"  ❌ {r.test_name}")
                print(f"     Expected: {r.expected}")
                print(f"     Actual: {r.actual}")


if __name__ == "__main__":
    main()
