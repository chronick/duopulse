"""Complete pattern generation pipeline."""

from dataclasses import dataclass, field

from .types import (
    Genre,
    EnergyZone,
    VoiceCoupling,
    Voice,
    ControlState,
    StepOutput,
    BlendedArchetype,
)
from .archetypes import ArchetypeDNA
from .pattern_field import blend_archetypes, get_dominant_archetype
from .hit_budget import compute_energy_zone, compute_bar_budget, BarBudget
from .gumbel_sampler import (
    select_hits_gumbel,
    get_min_spacing_for_zone,
    count_bits,
    mask_to_steps,
)
from .euclidean import get_genre_euclidean_ratio, blend_euclidean_with_weights
from .velocity import (
    compute_punch_params,
    compute_build_modifiers,
    should_accent,
    compute_velocity,
    get_default_accent_mask,
    PunchParams,
    BuildModifiers,
)


@dataclass
class PatternOutput:
    """Complete pattern output for one bar."""
    # Hit masks
    anchor_mask: int = 0
    shimmer_mask: int = 0
    aux_mask: int = 0

    # Velocities per step (only set for steps with hits)
    anchor_velocities: list[float] = field(default_factory=lambda: [0.0] * 32)
    shimmer_velocities: list[float] = field(default_factory=lambda: [0.0] * 32)
    aux_velocities: list[float] = field(default_factory=lambda: [0.0] * 32)

    # Pattern info
    pattern_length: int = 32
    anchor_hits: int = 0
    shimmer_hits: int = 0
    aux_hits: int = 0

    # Archetype info
    dominant_archetype: str = ""
    blended_swing: float = 0.0

    # Zone info
    energy_zone: EnergyZone = EnergyZone.GROOVE

    def get_step(self, step: int) -> StepOutput:
        """Get output for a specific step."""
        out = StepOutput()
        out.step = step
        out.anchor_fires = bool(self.anchor_mask & (1 << step))
        out.shimmer_fires = bool(self.shimmer_mask & (1 << step))
        out.aux_fires = bool(self.aux_mask & (1 << step))
        out.anchor_velocity = self.anchor_velocities[step]
        out.shimmer_velocity = self.shimmer_velocities[step]
        out.aux_velocity = self.aux_velocities[step]
        return out


def _apply_voice_coupling(
    anchor_mask: int,
    shimmer_mask: int,
    coupling: VoiceCoupling,
    pattern_length: int,
) -> int:
    """Apply voice coupling to shimmer mask.

    Args:
        anchor_mask: Anchor hit mask
        shimmer_mask: Shimmer hit mask (will be modified)
        coupling: Voice coupling mode
        pattern_length: Pattern length

    Returns:
        Modified shimmer mask
    """
    if coupling == VoiceCoupling.INDEPENDENT:
        # No modification
        return shimmer_mask

    elif coupling == VoiceCoupling.INTERLOCK:
        # Suppress simultaneous hits
        return shimmer_mask & ~anchor_mask

    else:  # SHADOW
        # Shimmer echoes anchor with 1-step delay
        length_mask = (1 << pattern_length) - 1 if pattern_length < 32 else 0xFFFFFFFF
        shifted = ((anchor_mask << 1) | (anchor_mask >> (pattern_length - 1))) & length_mask
        return shifted


def generate_pattern(
    state: ControlState | None = None,
    energy: float = 0.5,
    build: float = 0.5,
    field_x: float = 0.5,
    field_y: float = 0.5,
    punch: float = 0.5,
    genre: Genre = Genre.TECHNO,
    drift: float = 0.5,
    balance: float = 0.5,
    pattern_length: int = 32,
    phrase_progress: float = 0.0,
    swing: float = 0.5,
    voice_coupling: VoiceCoupling = VoiceCoupling.INDEPENDENT,
    seed: int = 0x12345678,
) -> PatternOutput:
    """Generate a complete drum pattern.

    Can be called with either a ControlState object or individual parameters.

    Args:
        state: Optional ControlState with all parameters
        energy: Energy level (0.0-1.0)
        build: BUILD parameter (0.0-1.0)
        field_x: Field X position (0.0-1.0)
        field_y: Field Y position (0.0-1.0)
        punch: PUNCH parameter (0.0-1.0)
        genre: Genre bank
        drift: DRIFT parameter (0.0-1.0)
        balance: Voice balance (0.0-1.0)
        pattern_length: Steps per bar
        phrase_progress: Position in phrase (0.0-1.0)
        swing: Swing amount (0.0-1.0)
        voice_coupling: Voice coupling mode
        seed: Random seed for determinism

    Returns:
        PatternOutput with hit masks and velocities
    """
    # Use state if provided, otherwise use individual params
    if state is not None:
        energy = state.energy
        build = state.build
        field_x = state.field_x
        field_y = state.field_y
        punch = state.punch
        genre = state.genre
        drift = state.drift
        balance = state.balance
        pattern_length = state.pattern_length
        phrase_progress = state.phrase_progress
        swing = state.swing
        voice_coupling = state.voice_coupling
        seed = state.seed

    # Clamp inputs
    energy = max(0.0, min(1.0, energy))
    build = max(0.0, min(1.0, build))
    field_x = max(0.0, min(1.0, field_x))
    field_y = max(0.0, min(1.0, field_y))
    punch = max(0.0, min(1.0, punch))
    balance = max(0.0, min(1.0, balance))
    pattern_length = max(16, min(64, pattern_length))

    # Clamp to 32 for bitmask operations
    clamped_length = min(pattern_length, 32)

    # Initialize output
    output = PatternOutput()
    output.pattern_length = clamped_length

    # 1. Compute energy zone
    zone = compute_energy_zone(energy)
    output.energy_zone = zone

    # 2. Blend archetypes at field position
    blended = blend_archetypes(genre, field_x, field_y)
    dominant = get_dominant_archetype(genre, field_x, field_y)
    output.dominant_archetype = dominant.name
    output.blended_swing = blended.swing_amount

    # 3. Compute BUILD modifiers
    build_mods = compute_build_modifiers(build, phrase_progress)

    # 4. Compute hit budgets
    budget = compute_bar_budget(
        energy=energy,
        balance=balance,
        zone=zone,
        pattern_length=clamped_length,
        build_multiplier=build_mods.density_multiplier,
        flavor=field_x,
    )

    # 5. Get Euclidean ratio for genre
    euclidean_ratio = get_genre_euclidean_ratio(genre, field_x, zone)

    # 6. Get min spacing for zone
    min_spacing = get_min_spacing_for_zone(zone)

    # 7. Generate anchor hits
    if euclidean_ratio > 0.01:
        output.anchor_mask = blend_euclidean_with_weights(
            budget=budget.anchor_hits,
            steps=clamped_length,
            weights=blended.anchor_weights,
            eligibility=budget.anchor_eligibility,
            euclidean_ratio=euclidean_ratio,
            seed=seed,
        )
    else:
        output.anchor_mask = select_hits_gumbel(
            weights=blended.anchor_weights,
            eligibility_mask=budget.anchor_eligibility,
            target_count=budget.anchor_hits,
            seed=seed,
            pattern_length=clamped_length,
            min_spacing=min_spacing,
        )

    # 8. Generate shimmer hits
    shimmer_seed = seed ^ 0x12345
    if euclidean_ratio > 0.01:
        output.shimmer_mask = blend_euclidean_with_weights(
            budget=budget.shimmer_hits,
            steps=clamped_length,
            weights=blended.shimmer_weights,
            eligibility=budget.shimmer_eligibility,
            euclidean_ratio=euclidean_ratio * 0.5,  # Less Euclidean for shimmer
            seed=shimmer_seed,
        )
    else:
        output.shimmer_mask = select_hits_gumbel(
            weights=blended.shimmer_weights,
            eligibility_mask=budget.shimmer_eligibility,
            target_count=budget.shimmer_hits,
            seed=shimmer_seed,
            pattern_length=clamped_length,
            min_spacing=min_spacing,
        )

    # 9. Apply voice coupling
    output.shimmer_mask = _apply_voice_coupling(
        output.anchor_mask, output.shimmer_mask, voice_coupling, clamped_length
    )

    # 10. Generate aux hits
    aux_seed = seed ^ 0x67890
    output.aux_mask = select_hits_gumbel(
        weights=blended.aux_weights,
        eligibility_mask=budget.aux_eligibility,
        target_count=budget.aux_hits,
        seed=aux_seed,
        pattern_length=clamped_length,
        min_spacing=0,  # Aux can be dense
    )

    # 11. Count hits
    output.anchor_hits = count_bits(output.anchor_mask)
    output.shimmer_hits = count_bits(output.shimmer_mask)
    output.aux_hits = count_bits(output.aux_mask)

    # 12. Compute velocities
    punch_params = compute_punch_params(punch)
    anchor_accent_mask = get_default_accent_mask(Voice.ANCHOR)
    shimmer_accent_mask = get_default_accent_mask(Voice.SHIMMER)
    aux_accent_mask = get_default_accent_mask(Voice.AUX)

    for step in range(clamped_length):
        # Anchor velocity
        if output.anchor_mask & (1 << step):
            is_accent = should_accent(
                step, anchor_accent_mask, punch_params.accent_probability, build_mods, seed
            )
            output.anchor_velocities[step] = compute_velocity(
                punch_params, build_mods, is_accent, seed, step
            )

        # Shimmer velocity
        if output.shimmer_mask & (1 << step):
            is_accent = should_accent(
                step, shimmer_accent_mask, punch_params.accent_probability, build_mods, shimmer_seed
            )
            output.shimmer_velocities[step] = compute_velocity(
                punch_params, build_mods, is_accent, shimmer_seed, step
            )

        # Aux velocity
        if output.aux_mask & (1 << step):
            is_accent = should_accent(
                step, aux_accent_mask, punch_params.accent_probability, build_mods, aux_seed
            )
            output.aux_velocities[step] = compute_velocity(
                punch_params, build_mods, is_accent, aux_seed, step
            )

    return output


def generate_phrase(
    bars: int = 4,
    **kwargs,
) -> list[PatternOutput]:
    """Generate a complete phrase of patterns.

    Args:
        bars: Number of bars in phrase
        **kwargs: Arguments passed to generate_pattern

    Returns:
        List of PatternOutput, one per bar
    """
    patterns = []
    seed = kwargs.get("seed", 0x12345678)

    for bar in range(bars):
        # Compute phrase progress
        phrase_progress = bar / bars

        # Vary seed per bar based on drift
        drift = kwargs.get("drift", 0.5)
        if drift < 0.01:
            bar_seed = seed  # Same pattern every bar
        else:
            bar_seed = seed ^ (bar * 0x1234567)

        # Generate pattern for this bar
        pattern = generate_pattern(
            **{**kwargs, "phrase_progress": phrase_progress, "seed": bar_seed}
        )
        patterns.append(pattern)

    return patterns
