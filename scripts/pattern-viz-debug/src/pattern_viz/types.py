"""Type definitions for DuoPulse v4 pattern generation."""

from dataclasses import dataclass, field
from enum import Enum, auto


class Genre(Enum):
    """Genre bank selection."""
    TECHNO = auto()
    TRIBAL = auto()
    IDM = auto()


class EnergyZone(Enum):
    """Energy zones determine behavioral rules."""
    MINIMAL = auto()  # 0-20%: Sparse, skeleton only
    GROOVE = auto()   # 20-50%: Stable, danceable
    BUILD = auto()    # 50-75%: Increasing ghosts, fills
    PEAK = auto()     # 75-100%: Maximum activity


class VoiceCoupling(Enum):
    """Voice relationship modes."""
    INDEPENDENT = auto()  # Both voices fire freely
    INTERLOCK = auto()    # Suppress simultaneous hits
    SHADOW = auto()       # Shimmer echoes anchor with delay


class BuildPhase(Enum):
    """BUILD parameter phases within a phrase."""
    GROOVE = auto()  # 0-60%: Stable
    BUILD = auto()   # 60-87.5%: Ramping
    FILL = auto()    # 87.5-100%: Maximum energy


class Voice(Enum):
    """Voice types."""
    ANCHOR = auto()   # Primary voice (kick-like)
    SHIMMER = auto()  # Secondary voice (snare/clap-like)
    AUX = auto()      # Third voice (hi-hat-like)


# Standard step masks for metric positions
DOWNBEAT_MASK = 0x00010001       # Steps 0, 16
QUARTER_NOTE_MASK = 0x01010101   # Steps 0, 8, 16, 24
BACKBEAT_MASK = 0x01000100       # Steps 8, 24
EIGHTH_NOTE_MASK = 0x55555555    # All even steps
SIXTEENTH_NOTE_MASK = 0xFFFFFFFF # All steps
OFFBEAT_MASK = 0xAAAAAAAA        # All odd steps
SYNCOPATION_MASK = 0x22222222    # e positions (steps 1, 5, 9, etc.)


@dataclass
class ControlState:
    """All control parameters for pattern generation."""
    # Performance mode - primary
    energy: float = 0.5
    build: float = 0.5
    field_x: float = 0.5
    field_y: float = 0.5

    # Performance mode - shift
    punch: float = 0.5
    genre: Genre = Genre.TECHNO
    drift: float = 0.5
    balance: float = 0.5

    # Config mode
    pattern_length: int = 32
    phrase_length: int = 4
    swing: float = 0.5
    voice_coupling: VoiceCoupling = VoiceCoupling.INDEPENDENT

    # Seed for determinism
    seed: int = 0x12345678

    # Phrase position (0.0 to 1.0)
    phrase_progress: float = 0.0


@dataclass
class PunchParams:
    """PUNCH parameter derived values."""
    accent_probability: float = 0.35
    velocity_floor: float = 0.475
    accent_boost: float = 0.30
    velocity_variation: float = 0.09


@dataclass
class BuildModifiers:
    """BUILD parameter derived modifiers."""
    phase: BuildPhase = BuildPhase.GROOVE
    density_multiplier: float = 1.0
    velocity_boost: float = 0.0
    force_accents: bool = False
    in_fill_zone: bool = False
    fill_intensity: float = 0.0
    phrase_progress: float = 0.0


@dataclass
class BlendedArchetype:
    """Result of blending archetypes at a field position."""
    anchor_weights: list[float] = field(default_factory=lambda: [0.0] * 32)
    shimmer_weights: list[float] = field(default_factory=lambda: [0.0] * 32)
    aux_weights: list[float] = field(default_factory=lambda: [0.0] * 32)
    swing_amount: float = 0.0
    accent_mask: int = QUARTER_NOTE_MASK
    fill_multiplier: float = 1.3


@dataclass
class StepTiming:
    """Timing information for a single step."""
    swing_offset_samples: float = 0.0
    jitter_offset_samples: float = 0.0
    displaced_to: int | None = None


@dataclass
class StepOutput:
    """Output for a single step."""
    step: int = 0
    anchor_fires: bool = False
    shimmer_fires: bool = False
    aux_fires: bool = False
    anchor_velocity: float = 0.0
    shimmer_velocity: float = 0.0
    aux_velocity: float = 0.0
    timing: StepTiming = field(default_factory=StepTiming)
