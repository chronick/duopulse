"""Hit budget and eligibility mask computation."""

from dataclasses import dataclass

from .types import (
    EnergyZone,
    DOWNBEAT_MASK,
    QUARTER_NOTE_MASK,
    BACKBEAT_MASK,
    EIGHTH_NOTE_MASK,
    SIXTEENTH_NOTE_MASK,
    OFFBEAT_MASK,
    SYNCOPATION_MASK,
)


@dataclass
class BarBudget:
    """Budget for a single bar."""
    anchor_hits: int = 4
    shimmer_hits: int = 2
    aux_hits: int = 4
    anchor_eligibility: int = SIXTEENTH_NOTE_MASK
    shimmer_eligibility: int = SIXTEENTH_NOTE_MASK
    aux_eligibility: int = SIXTEENTH_NOTE_MASK


def compute_energy_zone(energy: float) -> EnergyZone:
    """Compute energy zone from energy value.

    Args:
        energy: Energy level (0.0-1.0)

    Returns:
        EnergyZone enum value
    """
    energy = max(0.0, min(1.0, energy))

    if energy < 0.20:
        return EnergyZone.MINIMAL
    elif energy < 0.50:
        return EnergyZone.GROOVE
    elif energy < 0.75:
        return EnergyZone.BUILD
    else:
        return EnergyZone.PEAK


def compute_anchor_budget(
    energy: float, zone: EnergyZone, pattern_length: int
) -> int:
    """Compute anchor hit budget.

    Args:
        energy: Energy level (0.0-1.0)
        zone: Energy zone
        pattern_length: Steps per bar (16/24/32/64)

    Returns:
        Number of anchor hits for this bar
    """
    energy = max(0.0, min(1.0, energy))
    clamped_length = min(pattern_length, 32)

    # Max = 8th note density (pattern_length / 3)
    max_hits = clamped_length // 3

    # Zone-specific minimums and typical values
    if zone == EnergyZone.MINIMAL:
        min_hits = 1
        typical_hits = max(1, clamped_length // 16)
    elif zone == EnergyZone.GROOVE:
        min_hits = 3
        typical_hits = clamped_length // 6
    elif zone == EnergyZone.BUILD:
        min_hits = 4
        typical_hits = clamped_length // 4
    else:  # PEAK
        min_hits = 6
        typical_hits = clamped_length // 3

    # Compute zone progress for interpolation
    if zone == EnergyZone.MINIMAL:
        zone_progress = energy / 0.20
    elif zone == EnergyZone.GROOVE:
        zone_progress = (energy - 0.20) / 0.30
    elif zone == EnergyZone.BUILD:
        zone_progress = (energy - 0.50) / 0.25
    else:  # PEAK
        zone_progress = (energy - 0.75) / 0.25

    zone_progress = max(0.0, min(1.0, zone_progress))

    # Interpolate within zone range
    hits = min_hits + int((typical_hits - min_hits) * zone_progress + 0.5)
    return max(1, min(hits, max_hits))


def compute_shimmer_budget(
    energy: float, balance: float, zone: EnergyZone, pattern_length: int
) -> int:
    """Compute shimmer hit budget.

    Args:
        energy: Energy level (0.0-1.0)
        balance: Balance parameter (0.0-1.0)
        zone: Energy zone
        pattern_length: Steps per bar

    Returns:
        Number of shimmer hits for this bar
    """
    balance = max(0.0, min(1.0, balance))

    # Base shimmer budget from anchor
    anchor_budget = compute_anchor_budget(energy, zone, pattern_length)

    # Balance range: 0-150% of anchor
    shimmer_ratio = balance * 1.5

    # Zone-aware cap to prevent over-density in low-energy zones
    if zone in (EnergyZone.GROOVE, EnergyZone.MINIMAL):
        shimmer_ratio = min(shimmer_ratio, 1.0)

    hits = int(anchor_budget * shimmer_ratio + 0.5)

    # Minimum of 1 hit except in MINIMAL zone
    clamped_length = min(pattern_length, 32)
    if zone == EnergyZone.MINIMAL:
        return max(0, min(hits, clamped_length // 8))

    return max(1, min(hits, clamped_length // 4))


def compute_aux_budget(
    energy: float, zone: EnergyZone, pattern_length: int, aux_density: float = 1.0
) -> int:
    """Compute aux hit budget.

    Args:
        energy: Energy level (0.0-1.0)
        zone: Energy zone
        pattern_length: Steps per bar
        aux_density: Density multiplier (0.5-2.0)

    Returns:
        Number of aux hits for this bar
    """
    clamped_length = min(pattern_length, 32)

    # Base aux budget based on zone
    if zone == EnergyZone.MINIMAL:
        base_budget = 0
    elif zone == EnergyZone.GROOVE:
        base_budget = clamped_length // 8
    elif zone == EnergyZone.BUILD:
        base_budget = clamped_length // 4
    else:  # PEAK
        base_budget = clamped_length // 2

    # Apply density multiplier
    hits = int(base_budget * aux_density + 0.5)
    return max(0, min(hits, clamped_length))


def compute_anchor_eligibility(
    energy: float, flavor: float, zone: EnergyZone, pattern_length: int
) -> int:
    """Compute anchor eligibility mask.

    Determines which metric positions CAN fire based on energy zone.
    """
    energy = max(0.0, min(1.0, energy))
    flavor = max(0.0, min(1.0, flavor))
    clamped_length = min(pattern_length, 32)

    # Create length mask
    if clamped_length >= 32:
        length_mask = 0xFFFFFFFF
    else:
        length_mask = (1 << clamped_length) - 1

    # Base eligibility based on zone
    if zone == EnergyZone.MINIMAL:
        eligibility = DOWNBEAT_MASK | QUARTER_NOTE_MASK
    elif zone == EnergyZone.GROOVE:
        eligibility = QUARTER_NOTE_MASK
        if energy > 0.35:
            eligibility |= EIGHTH_NOTE_MASK
    elif zone == EnergyZone.BUILD:
        eligibility = EIGHTH_NOTE_MASK
        if energy > 0.60:
            eligibility |= SIXTEENTH_NOTE_MASK
    else:  # PEAK
        eligibility = SIXTEENTH_NOTE_MASK

    # Flavor adds syncopation/offbeat positions
    if flavor > 0.3:
        eligibility |= OFFBEAT_MASK
    if flavor > 0.6:
        eligibility |= SYNCOPATION_MASK

    return eligibility & length_mask


def compute_shimmer_eligibility(
    energy: float, flavor: float, zone: EnergyZone, pattern_length: int
) -> int:
    """Compute shimmer eligibility mask."""
    energy = max(0.0, min(1.0, energy))
    flavor = max(0.0, min(1.0, flavor))
    clamped_length = min(pattern_length, 32)

    if clamped_length >= 32:
        length_mask = 0xFFFFFFFF
    else:
        length_mask = (1 << clamped_length) - 1

    # Base eligibility based on zone
    if zone == EnergyZone.MINIMAL:
        eligibility = BACKBEAT_MASK
    elif zone == EnergyZone.GROOVE:
        eligibility = BACKBEAT_MASK
        if energy > 0.30:
            eligibility |= OFFBEAT_MASK
    elif zone == EnergyZone.BUILD:
        eligibility = EIGHTH_NOTE_MASK
    else:  # PEAK
        eligibility = SIXTEENTH_NOTE_MASK

    # Flavor allows more syncopation
    if flavor > 0.4:
        eligibility |= SYNCOPATION_MASK

    return eligibility & length_mask


def compute_aux_eligibility(
    energy: float, flavor: float, zone: EnergyZone, pattern_length: int
) -> int:
    """Compute aux eligibility mask."""
    energy = max(0.0, min(1.0, energy))
    clamped_length = min(pattern_length, 32)

    if clamped_length >= 32:
        length_mask = 0xFFFFFFFF
    else:
        length_mask = (1 << clamped_length) - 1

    # Aux (hi-hat) is more permissive
    if zone == EnergyZone.MINIMAL:
        eligibility = 0
    elif zone == EnergyZone.GROOVE:
        eligibility = EIGHTH_NOTE_MASK
    elif zone == EnergyZone.BUILD:
        eligibility = EIGHTH_NOTE_MASK
        if energy > 0.60:
            eligibility |= SIXTEENTH_NOTE_MASK
    else:  # PEAK
        eligibility = SIXTEENTH_NOTE_MASK

    return eligibility & length_mask


def compute_bar_budget(
    energy: float,
    balance: float,
    zone: EnergyZone,
    pattern_length: int,
    build_multiplier: float = 1.0,
    flavor: float = 0.5,
) -> BarBudget:
    """Compute complete bar budget with eligibility masks.

    Args:
        energy: Energy level (0.0-1.0)
        balance: Voice balance (0.0-1.0)
        zone: Energy zone
        pattern_length: Steps per bar
        build_multiplier: BUILD phase density multiplier
        flavor: Flavor for eligibility (Field X)

    Returns:
        BarBudget with hit counts and eligibility masks
    """
    clamped_length = min(pattern_length, 32)

    budget = BarBudget()

    # Compute base budgets
    budget.anchor_hits = compute_anchor_budget(energy, zone, clamped_length)
    budget.shimmer_hits = compute_shimmer_budget(energy, balance, zone, clamped_length)
    budget.aux_hits = compute_aux_budget(energy, zone, clamped_length)

    # Apply build multiplier
    if build_multiplier > 1.0:
        budget.anchor_hits = int(budget.anchor_hits * build_multiplier + 0.5)
        budget.shimmer_hits = int(budget.shimmer_hits * build_multiplier + 0.5)
        budget.aux_hits = int(budget.aux_hits * build_multiplier + 0.5)

    # Clamp to valid ranges (up to 2/3 of steps)
    max_hits = clamped_length * 2 // 3
    budget.anchor_hits = min(budget.anchor_hits, max_hits)
    budget.shimmer_hits = min(budget.shimmer_hits, max_hits)
    budget.aux_hits = min(budget.aux_hits, clamped_length)

    # Compute eligibility masks
    budget.anchor_eligibility = compute_anchor_eligibility(
        energy, flavor, zone, clamped_length
    )
    budget.shimmer_eligibility = compute_shimmer_eligibility(
        energy, flavor, zone, clamped_length
    )
    budget.aux_eligibility = compute_aux_eligibility(
        energy, flavor, zone, clamped_length
    )

    return budget
