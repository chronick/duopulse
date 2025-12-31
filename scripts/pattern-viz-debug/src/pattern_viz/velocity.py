"""Velocity computation with PUNCH and BUILD parameters."""

from .types import PunchParams, BuildModifiers, BuildPhase, Voice, QUARTER_NOTE_MASK
from .gumbel_sampler import _hash_to_float


# Hash magic numbers for velocity
VEL_ACCENT_HASH_MAGIC = 0x41434E54  # "ACNT"
VEL_VARIATION_HASH_MAGIC = 0x56415249  # "VARI"

# Default accent masks per voice
DEFAULT_ACCENT_MASKS = {
    Voice.ANCHOR: 0x11111111,   # Steps 0, 4, 8, 12, etc.
    Voice.SHIMMER: 0x01010101,  # Steps 0, 8, 16, 24 (backbeats)
    Voice.AUX: 0x44444444,      # Steps 2, 6, 10, 14, etc. (offbeat 8ths)
}


def clamp(value: float, min_val: float, max_val: float) -> float:
    """Clamp value to range."""
    return max(min_val, min(max_val, value))


def compute_punch_params(punch: float) -> PunchParams:
    """Compute PUNCH parameter derived values.

    PUNCH = 0%: Flat dynamics (all similar velocity)
    PUNCH = 100%: Maximum dynamics (huge contrasts)

    Args:
        punch: PUNCH parameter (0.0-1.0)

    Returns:
        PunchParams with derived values
    """
    punch = clamp(punch, 0.0, 1.0)

    params = PunchParams()

    # Accent probability: 20% to 50%
    params.accent_probability = 0.20 + punch * 0.30

    # Velocity floor: 65% down to 30%
    # Low punch = high floor (flat), high punch = low floor (dynamics)
    params.velocity_floor = 0.65 - punch * 0.35

    # Accent boost: +15% to +45%
    params.accent_boost = 0.15 + punch * 0.30

    # Velocity variation: ±3% to ±15%
    params.velocity_variation = 0.03 + punch * 0.12

    return params


def compute_build_modifiers(build: float, phrase_progress: float) -> BuildModifiers:
    """Compute BUILD parameter modifiers based on phrase position.

    BUILD phases:
    - GROOVE (0-60%): Stable, no modification
    - BUILD (60-87.5%): Ramping density and velocity
    - FILL (87.5-100%): Maximum energy

    Args:
        build: BUILD parameter (0.0-1.0)
        phrase_progress: Position in phrase (0.0-1.0)

    Returns:
        BuildModifiers with phase and derived values
    """
    build = clamp(build, 0.0, 1.0)
    phrase_progress = clamp(phrase_progress, 0.0, 1.0)

    modifiers = BuildModifiers()
    modifiers.phrase_progress = phrase_progress

    if phrase_progress < 0.60:
        # GROOVE phase: stable, no modification
        modifiers.phase = BuildPhase.GROOVE
        modifiers.density_multiplier = 1.0
        modifiers.velocity_boost = 0.0
        modifiers.force_accents = False
    elif phrase_progress < 0.875:
        # BUILD phase: ramping
        modifiers.phase = BuildPhase.BUILD
        phase_progress = (phrase_progress - 0.60) / 0.275  # 0-1 within phase
        modifiers.density_multiplier = 1.0 + build * 0.35 * phase_progress
        modifiers.velocity_boost = build * 0.08 * phase_progress
        modifiers.force_accents = False
    else:
        # FILL phase: maximum energy
        modifiers.phase = BuildPhase.FILL
        modifiers.density_multiplier = 1.0 + build * 0.50
        modifiers.velocity_boost = build * 0.12
        modifiers.force_accents = build > 0.6

    modifiers.in_fill_zone = modifiers.phase == BuildPhase.FILL
    modifiers.fill_intensity = build if modifiers.in_fill_zone else 0.0

    return modifiers


def should_accent(
    step: int,
    accent_mask: int,
    accent_probability: float,
    build_mods: BuildModifiers,
    seed: int,
) -> bool:
    """Determine if a step should be accented.

    Args:
        step: Step index
        accent_mask: Bitmask of accent-eligible steps
        accent_probability: Probability of accent (from PUNCH)
        build_mods: BUILD modifiers
        seed: Random seed

    Returns:
        True if step should be accented
    """
    # Force all accents in FILL phase at high BUILD
    if build_mods.force_accents:
        return True

    # Check if step is accent-eligible
    if not (accent_mask & (1 << (step & 31))):
        return False

    # Apply probability
    roll = _hash_to_float(seed ^ VEL_ACCENT_HASH_MAGIC, step)
    return roll < accent_probability


def compute_velocity(
    punch_params: PunchParams,
    build_mods: BuildModifiers,
    is_accent: bool,
    seed: int,
    step: int,
) -> float:
    """Compute velocity for a single hit.

    Args:
        punch_params: PUNCH derived parameters
        build_mods: BUILD modifiers
        is_accent: Whether this hit is accented
        seed: Random seed
        step: Step index

    Returns:
        Velocity value (0.3-1.0)
    """
    # Start with velocity floor
    velocity = punch_params.velocity_floor

    # Apply BUILD velocity boost
    velocity += build_mods.velocity_boost

    # Add accent boost if accented
    if is_accent:
        velocity += punch_params.accent_boost

    # Apply fill zone boost
    if build_mods.in_fill_zone and build_mods.fill_intensity > 0:
        velocity += build_mods.fill_intensity * 0.15

    # Apply random variation
    if punch_params.velocity_variation > 0.001:
        var_roll = _hash_to_float(seed ^ VEL_VARIATION_HASH_MAGIC, step)
        variation = (var_roll - 0.5) * 2.0 * punch_params.velocity_variation
        velocity += variation

    # Clamp to valid range (min 0.30 for VCA audibility)
    return clamp(velocity, 0.30, 1.0)


def compute_step_velocity(
    voice: Voice,
    punch: float,
    build: float,
    phrase_progress: float,
    step: int,
    seed: int,
    accent_mask: int | None = None,
) -> float:
    """Convenience function to compute velocity for a step.

    Args:
        voice: Voice type (ANCHOR, SHIMMER, AUX)
        punch: PUNCH parameter (0.0-1.0)
        build: BUILD parameter (0.0-1.0)
        phrase_progress: Position in phrase (0.0-1.0)
        step: Step index
        seed: Random seed
        accent_mask: Override accent mask (uses default if None)

    Returns:
        Velocity value (0.3-1.0)
    """
    # Use default mask if none provided
    if accent_mask is None:
        accent_mask = DEFAULT_ACCENT_MASKS.get(voice, QUARTER_NOTE_MASK)

    # Compute parameters
    punch_params = compute_punch_params(punch)
    build_mods = compute_build_modifiers(build, phrase_progress)

    # Determine accent status
    is_accent = should_accent(step, accent_mask, punch_params.accent_probability, build_mods, seed)

    # Compute velocity
    return compute_velocity(punch_params, build_mods, is_accent, seed, step)


def get_default_accent_mask(voice: Voice) -> int:
    """Get default accent mask for voice."""
    return DEFAULT_ACCENT_MASKS.get(voice, QUARTER_NOTE_MASK)
