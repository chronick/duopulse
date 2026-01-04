"""Gumbel Top-K selection algorithm for deterministic weighted sampling."""

import math
from .types import EnergyZone


# Small epsilon to avoid log(0)
EPSILON = 1e-6

# Minimum score (for initialization)
MIN_SCORE = -1e9


def _hash_step(seed: int, step: int) -> int:
    """Generate deterministic hash from seed and step.

    Uses a fast mixing function for good distribution.
    """
    # Combine seed and step using multiplication with golden ratio prime
    h = seed & 0xFFFFFFFF
    h ^= (step * 0x9E3779B9) & 0xFFFFFFFF
    h = (h ^ (h >> 16)) & 0xFFFFFFFF
    h = (h * 0x85EBCA6B) & 0xFFFFFFFF
    h = (h ^ (h >> 13)) & 0xFFFFFFFF
    h = (h * 0xC2B2AE35) & 0xFFFFFFFF
    h = (h ^ (h >> 16)) & 0xFFFFFFFF
    return h


def _hash_to_float(seed: int, step: int) -> float:
    """Convert hash to float in (0, 1)."""
    h = _hash_step(seed, step)
    # Use top 24 bits for precision
    result = (h >> 8) / 16777216.0
    # Clamp to avoid exactly 0 or 1
    return max(EPSILON, min(1.0 - EPSILON, result))


def _uniform_to_gumbel(uniform: float) -> float:
    """Convert uniform [0,1] to Gumbel distribution."""
    uniform = max(EPSILON, min(1.0 - EPSILON, uniform))
    return -math.log(-math.log(uniform))


def _compute_gumbel_scores(
    weights: list[float], seed: int, pattern_length: int
) -> list[float]:
    """Compute Gumbel scores for all steps.

    Score = log(weight) + Gumbel(seed, step)
    """
    scores = []
    for step in range(pattern_length):
        weight = weights[step] if step < len(weights) else 0.0

        if weight < EPSILON:
            scores.append(MIN_SCORE)
            continue

        uniform = _hash_to_float(seed, step)
        gumbel = _uniform_to_gumbel(uniform)
        scores.append(math.log(weight) + gumbel)

    return scores


def _check_spacing_valid(
    selected_mask: int, candidate_step: int, min_spacing: int, pattern_length: int
) -> bool:
    """Check if candidate step satisfies spacing constraint."""
    if min_spacing <= 0 or selected_mask == 0:
        return True

    for step in range(pattern_length):
        if not (selected_mask & (1 << step)):
            continue

        # Compute minimum distance (circular)
        dist = abs(candidate_step - step)
        wrap_dist = pattern_length - dist
        min_dist = min(dist, wrap_dist)

        if min_dist < min_spacing:
            return False

    return True


def _find_best_step(
    scores: list[float],
    eligibility_mask: int,
    selected_mask: int,
    pattern_length: int,
    min_spacing: int,
) -> int:
    """Find the highest-scoring eligible step that satisfies spacing."""
    best_score = MIN_SCORE
    best_step = -1

    for step in range(min(pattern_length, 32)):
        # Check eligibility
        if not (eligibility_mask & (1 << step)):
            continue

        # Check already selected
        if selected_mask & (1 << step):
            continue

        # Check spacing constraint
        if not _check_spacing_valid(selected_mask, step, min_spacing, pattern_length):
            continue

        # Check if this is the best so far
        if scores[step] > best_score:
            best_score = scores[step]
            best_step = step

    return best_step


def get_min_spacing_for_zone(zone: EnergyZone) -> int:
    """Get minimum step spacing for energy zone."""
    if zone == EnergyZone.MINIMAL:
        return 4  # Sparse patterns need more spacing
    elif zone == EnergyZone.GROOVE:
        return 2  # Moderate spacing
    elif zone == EnergyZone.BUILD:
        return 1  # Tight patterns allowed
    else:  # PEAK
        return 0  # No spacing constraint


def select_hits_gumbel(
    weights: list[float],
    eligibility_mask: int,
    target_count: int,
    seed: int,
    pattern_length: int = 32,
    min_spacing: int = 0,
) -> int:
    """Select hits using Gumbel Top-K algorithm.

    This provides deterministic weighted sampling without replacement.

    Args:
        weights: Weight for each step (0.0-1.0)
        eligibility_mask: Bitmask of eligible steps
        target_count: Number of hits to select
        seed: Random seed for determinism
        pattern_length: Pattern length (max 32)
        min_spacing: Minimum steps between selected hits

    Returns:
        Bitmask of selected steps
    """
    pattern_length = min(pattern_length, 32)

    if target_count <= 0 or eligibility_mask == 0:
        return 0

    # Limit target count
    target_count = min(target_count, 16)

    # Compute Gumbel scores
    scores = _compute_gumbel_scores(weights, seed, pattern_length)

    # Greedily select top-K steps respecting spacing
    selected_mask = 0
    selected_count = 0

    # First pass: try with spacing constraints
    while selected_count < target_count:
        best_step = _find_best_step(
            scores, eligibility_mask, selected_mask, pattern_length, min_spacing
        )

        if best_step < 0:
            break

        selected_mask |= 1 << best_step
        selected_count += 1

    # Second pass: relaxed spacing if needed
    if selected_count < target_count and min_spacing > 0:
        relaxed_spacing = min_spacing // 2

        while selected_count < target_count:
            best_step = _find_best_step(
                scores, eligibility_mask, selected_mask, pattern_length, relaxed_spacing
            )

            if best_step < 0:
                break

            selected_mask |= 1 << best_step
            selected_count += 1

    # Final pass: no spacing constraint
    if selected_count < target_count:
        while selected_count < target_count:
            best_step = _find_best_step(
                scores, eligibility_mask, selected_mask, pattern_length, 0
            )

            if best_step < 0:
                break

            selected_mask |= 1 << best_step
            selected_count += 1

    return selected_mask


def count_bits(mask: int) -> int:
    """Count set bits in mask (Brian Kernighan's algorithm)."""
    count = 0
    while mask:
        mask &= mask - 1
        count += 1
    return count


def mask_to_steps(mask: int, pattern_length: int = 32) -> list[int]:
    """Convert bitmask to list of step indices."""
    steps = []
    for i in range(pattern_length):
        if mask & (1 << i):
            steps.append(i)
    return steps


def steps_to_mask(steps: list[int]) -> int:
    """Convert list of step indices to bitmask."""
    mask = 0
    for step in steps:
        if 0 <= step < 32:
            mask |= 1 << step
    return mask
