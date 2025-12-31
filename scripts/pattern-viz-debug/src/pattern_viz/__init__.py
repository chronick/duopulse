"""DuoPulse v4 Pattern Visualization and Debug Tool."""

from .types import Genre, EnergyZone, VoiceCoupling, BuildPhase, Voice
from .archetypes import get_archetype, ArchetypeDNA
from .pattern_field import blend_archetypes
from .hit_budget import compute_bar_budget, compute_energy_zone, BarBudget
from .gumbel_sampler import select_hits_gumbel
from .euclidean import generate_euclidean, get_genre_euclidean_ratio
from .velocity import compute_velocity, compute_punch_params, compute_build_modifiers
from .pipeline import generate_pattern, PatternOutput

__all__ = [
    "Genre",
    "EnergyZone",
    "VoiceCoupling",
    "BuildPhase",
    "Voice",
    "ArchetypeDNA",
    "get_archetype",
    "blend_archetypes",
    "compute_bar_budget",
    "compute_energy_zone",
    "BarBudget",
    "select_hits_gumbel",
    "generate_euclidean",
    "get_genre_euclidean_ratio",
    "compute_velocity",
    "compute_punch_params",
    "compute_build_modifiers",
    "generate_pattern",
    "PatternOutput",
]
