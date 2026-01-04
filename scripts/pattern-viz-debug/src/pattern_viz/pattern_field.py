"""Pattern field blending using winner-take-more softmax."""

import math
from dataclasses import dataclass

from .types import Genre, BlendedArchetype
from .archetypes import get_archetype, ArchetypeDNA


def _softmax_weights(distances: list[float], temperature: float = 0.5) -> list[float]:
    """Compute softmax weights with temperature.

    Lower temperature = more winner-take-all behavior.
    Higher temperature = more equal blending.

    Uses numerically stable softmax to avoid overflow.
    """
    if temperature <= 0:
        temperature = 0.01

    # Convert distances to negative scores (closer = higher score)
    # Using negative distance directly instead of 1/d to avoid large values
    scores = [-d / temperature for d in distances]

    # Numerically stable softmax: subtract max before exp
    max_score = max(scores)
    exp_weights = [math.exp(s - max_score) for s in scores]

    # Normalize to sum to 1
    total = sum(exp_weights)
    if total <= 0:
        return [1.0 / len(exp_weights)] * len(exp_weights)

    return [w / total for w in exp_weights]


def _blend_arrays(arrays: list[list[float]], weights: list[float]) -> list[float]:
    """Blend multiple arrays using weights."""
    if not arrays:
        return [0.0] * 32

    result = [0.0] * len(arrays[0])
    for arr, weight in zip(arrays, weights):
        for i, val in enumerate(arr):
            result[i] += val * weight
    return result


def blend_archetypes(
    genre: Genre,
    field_x: float,
    field_y: float,
    temperature: float = 0.5,
) -> BlendedArchetype:
    """Blend archetypes at field position using winner-take-more softmax.

    Args:
        genre: Genre bank (TECHNO, TRIBAL, IDM)
        field_x: X position (0.0-1.0, syncopation axis)
        field_y: Y position (0.0-1.0, complexity axis)
        temperature: Softmax temperature (lower = more winner-take-all)

    Returns:
        BlendedArchetype with interpolated weights and parameters
    """
    # Clamp inputs
    field_x = max(0.0, min(1.0, field_x))
    field_y = max(0.0, min(1.0, field_y))

    # Map to grid coordinates (0-2)
    grid_x = field_x * 2.0
    grid_y = field_y * 2.0

    # Find the four corner archetypes
    x0 = int(grid_x)
    y0 = int(grid_y)
    x1 = min(x0 + 1, 2)
    y1 = min(y0 + 1, 2)

    # Fractional position within cell
    fx = grid_x - x0
    fy = grid_y - y0

    # Get corner archetypes
    corners = [
        get_archetype(genre, x0, y0),  # Bottom-left
        get_archetype(genre, x1, y0),  # Bottom-right
        get_archetype(genre, x0, y1),  # Top-left
        get_archetype(genre, x1, y1),  # Top-right
    ]

    # Compute distances from current position to each corner
    distances = [
        math.sqrt(fx * fx + fy * fy),           # To bottom-left
        math.sqrt((1 - fx) ** 2 + fy * fy),     # To bottom-right
        math.sqrt(fx * fx + (1 - fy) ** 2),     # To top-left
        math.sqrt((1 - fx) ** 2 + (1 - fy) ** 2),  # To top-right
    ]

    # Compute softmax weights (winner-take-more)
    weights = _softmax_weights(distances, temperature)

    # Blend arrays
    blended = BlendedArchetype()
    blended.anchor_weights = _blend_arrays(
        [c.anchor_weights for c in corners], weights
    )
    blended.shimmer_weights = _blend_arrays(
        [c.shimmer_weights for c in corners], weights
    )
    blended.aux_weights = _blend_arrays(
        [c.aux_weights for c in corners], weights
    )

    # Blend scalar values
    blended.swing_amount = sum(
        c.swing_amount * w for c, w in zip(corners, weights)
    )
    blended.fill_multiplier = sum(
        c.fill_multiplier * w for c, w in zip(corners, weights)
    )

    # Use dominant archetype's discrete values
    dominant_idx = weights.index(max(weights))
    dominant = corners[dominant_idx]
    blended.accent_mask = dominant.accent_mask

    return blended


def get_dominant_archetype(
    genre: Genre,
    field_x: float,
    field_y: float,
) -> ArchetypeDNA:
    """Get the dominant (closest) archetype at field position.

    Useful for getting discrete properties like accent masks.
    """
    # Map to nearest grid position
    grid_x = round(field_x * 2.0)
    grid_y = round(field_y * 2.0)

    return get_archetype(genre, grid_x, grid_y)
