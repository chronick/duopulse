"""Euclidean rhythm generation using Bjorklund algorithm."""

from .types import Genre, EnergyZone
from .gumbel_sampler import select_hits_gumbel


def generate_euclidean(hits: int, steps: int) -> int:
    """Generate Euclidean rhythm: k hits distributed evenly across n steps.

    Uses Bjorklund algorithm (bucket-fill approximation).

    Args:
        hits: Number of hits to place
        steps: Total number of steps

    Returns:
        Bitmask with evenly distributed hits
    """
    # Clamp inputs
    hits = max(0, hits)
    steps = max(1, min(32, steps))
    hits = min(hits, steps)

    # Special cases
    if hits == 0:
        return 0
    if hits >= steps:
        return (1 << steps) - 1

    # Bjorklund: distribute hits evenly using bucket-fill
    pattern = 0
    placed_hits = 0
    accumulator = 0.0
    step_size = hits / steps

    for i in range(steps):
        accumulator += step_size
        if accumulator >= 1.0 and placed_hits < hits:
            pattern |= 1 << i
            placed_hits += 1
            accumulator -= 1.0

    # If rounding left some unplaced, add at end
    for i in range(steps - 1, -1, -1):
        if placed_hits >= hits:
            break
        if not (pattern & (1 << i)):
            pattern |= 1 << i
            placed_hits += 1

    return pattern


def rotate_pattern(pattern: int, offset: int, steps: int) -> int:
    """Rotate pattern by offset steps.

    Args:
        pattern: Bitmask to rotate
        offset: Steps to rotate right
        steps: Pattern length

    Returns:
        Rotated pattern
    """
    steps = max(1, min(32, steps))

    # Normalize offset
    offset = offset % steps
    if offset < 0:
        offset += steps

    # Create mask for valid bits
    mask = (1 << steps) - 1

    # Rotate right
    rotated = ((pattern >> offset) | (pattern << (steps - offset))) & mask
    return rotated


def get_genre_euclidean_ratio(
    genre: Genre, field_x: float, zone: EnergyZone
) -> float:
    """Get genre-specific Euclidean blend ratio.

    The ratio determines how much of the pattern uses Euclidean distribution
    versus pure probabilistic selection.

    Args:
        genre: Genre bank
        field_x: X position (0.0-1.0, syncopation axis)
        zone: Energy zone

    Returns:
        Euclidean ratio (0.0 = pure probabilistic, 1.0 = pure Euclidean)
    """
    # Only active in MINIMAL and GROOVE zones
    if zone not in (EnergyZone.MINIMAL, EnergyZone.GROOVE):
        return 0.0

    field_x = max(0.0, min(1.0, field_x))

    # Genre-specific base ratios at Field X = 0
    if genre == Genre.TECHNO:
        base_ratio = 0.70  # Strong Euclidean for 4-on-floor
    elif genre == Genre.TRIBAL:
        base_ratio = 0.40  # Moderate for polyrhythmic balance
    else:  # IDM
        base_ratio = 0.0   # No Euclidean (maximum irregularity)

    # Taper ratio with Field X: at X=1.0, ratio reduces by 70%
    taper = 1.0 - 0.7 * field_x
    effective_ratio = base_ratio * taper

    return max(0.0, min(1.0, effective_ratio))


def blend_euclidean_with_weights(
    budget: int,
    steps: int,
    weights: list[float],
    eligibility: int,
    euclidean_ratio: float,
    seed: int,
) -> int:
    """Blend Euclidean foundation with probabilistic weights.

    Args:
        budget: Number of hits to select
        steps: Pattern length
        weights: Weight for each step
        eligibility: Bitmask of eligible steps
        euclidean_ratio: Blend ratio (0.0 = pure Gumbel, 1.0 = pure Euclidean)
        seed: Random seed

    Returns:
        Bitmask of selected hits
    """
    euclidean_ratio = max(0.0, min(1.0, euclidean_ratio))
    budget = max(0, budget)
    steps = max(1, min(32, steps))

    if budget == 0:
        return 0

    # Pure Gumbel if ratio near 0
    if euclidean_ratio < 0.01:
        return select_hits_gumbel(weights, eligibility, budget, seed, steps)

    # Pure Euclidean if ratio near 1
    if euclidean_ratio > 0.99:
        euclidean = generate_euclidean(budget, steps)
        rotation = seed % steps
        rotated = rotate_pattern(euclidean, rotation, steps)
        return rotated & eligibility

    # Hybrid: reserve Euclidean hits, fill remainder from Gumbel
    euclidean_hits = int(budget * euclidean_ratio)
    gumbel_hits = budget - euclidean_hits

    # Generate Euclidean pattern and rotate
    euclidean_pattern = generate_euclidean(euclidean_hits, steps)
    rotation = seed % steps
    euclidean_pattern = rotate_pattern(euclidean_pattern, rotation, steps)
    euclidean_pattern &= eligibility

    # Select remaining hits from Gumbel, excluding Euclidean positions
    remaining_eligibility = eligibility & ~euclidean_pattern
    gumbel_pattern = select_hits_gumbel(
        weights, remaining_eligibility, gumbel_hits, seed ^ 0xE0C1, steps
    )

    # Combine patterns
    return euclidean_pattern | gumbel_pattern


# Common Euclidean patterns for reference
EUCLIDEAN_PATTERNS = {
    # (hits, steps): description
    (4, 16): "4-on-floor (classic techno kick)",
    (3, 8): "Tresillo (3-3-2 pattern)",
    (5, 8): "Cinquillo (Cuban rhythm)",
    (5, 16): "Bossa nova bass",
    (7, 16): "West African bell pattern",
    (3, 16): "Sparse accent pattern",
}


def describe_euclidean(hits: int, steps: int) -> str:
    """Get description of Euclidean pattern."""
    key = (hits, steps)
    if key in EUCLIDEAN_PATTERNS:
        return EUCLIDEAN_PATTERNS[key]
    return f"E({hits},{steps})"
