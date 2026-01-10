#!/usr/bin/env python3
"""
Generate HTML/SVG pattern visualization from pattern_viz C++ tool.

Creates an Ableton Live-style grid visualization with:
- Note blocks for hits/gates
- Velocity bars showing intensity
- Multiple parameter sweeps side-by-side

Usage:
    python scripts/generate-pattern-html.py [--output docs/patterns.html]
    make pattern-html
"""

import subprocess
import csv
import io
import argparse
import math
from pathlib import Path
from dataclasses import dataclass
from typing import Optional


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
    name: Optional[str] = None  # Optional display name for presets
    description: Optional[str] = None  # Optional description

    @property
    def v1_hits(self) -> int:
        return sum(1 for s in self.steps if s.v1)

    @property
    def v2_hits(self) -> int:
        return sum(1 for s in self.steps if s.v2)

    @property
    def aux_hits(self) -> int:
        return sum(1 for s in self.steps if s.aux)


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
    name: Optional[str] = None,
    description: Optional[str] = None,
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
        name=name,
        description=description,
    )


# Color scheme (Ableton-inspired dark theme)
COLORS = {
    "bg": "#1e1e1e",
    "grid_bg": "#2d2d2d",
    "grid_line": "#3d3d3d",
    "grid_line_strong": "#505050",
    "text": "#cccccc",
    "text_dim": "#888888",
    "v1": "#ff6b35",      # Orange for Anchor/V1
    "v1_dim": "#994020",
    "v2": "#4ecdc4",      # Teal for Shimmer/V2
    "v2_dim": "#2e7a75",
    "aux": "#95e1d3",     # Light teal for Aux
    "aux_dim": "#5a8a82",
    "velocity_bg": "#1a1a1a",
    "downbeat": "#ffffff22",
}

# Layout constants
STEP_WIDTH = 24
STEP_HEIGHT = 32
VOICE_GAP = 4
LABEL_WIDTH = 80
PADDING = 16
VELOCITY_HEIGHT = 40

# Knob constants
KNOB_RADIUS = 20
KNOB_TRACK_WIDTH = 3
KNOB_INDICATOR_WIDTH = 2

# =============================================================================
# Pentagon of Musicality Metrics
# =============================================================================

# LHL Metric weights for 32-step pattern (2 bars of 4/4 at 16th notes)
METRIC_WEIGHTS_32 = [
    1.00, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,  # Steps 0-7
    0.90, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,  # Steps 8-15
    0.95, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,  # Steps 16-23
    0.90, 0.10, 0.40, 0.10, 0.80, 0.10, 0.40, 0.10,  # Steps 24-31
]


@dataclass
class PentagonMetrics:
    """Pentagon of Musicality - 5 orthogonal metrics."""
    raw_syncopation: float = 0.0
    raw_density: float = 0.0
    raw_velocity_range: float = 0.0
    raw_voice_separation: float = 0.0
    raw_regularity: float = 0.0
    score_syncopation: float = 0.0
    score_density: float = 0.0
    score_velocity_range: float = 0.0
    score_voice_separation: float = 0.0
    score_regularity: float = 0.0
    composite: float = 0.0
    shape_zone: str = "stable"


def compute_pentagon_syncopation(hits: list[bool], pattern_length: int = 32) -> float:
    """Compute LHL syncopation score."""
    weights = METRIC_WEIGHTS_32[:pattern_length]
    syncopation_tension = 0.0
    max_possible_tension = 0.0

    for i in range(pattern_length):
        if hits[i]:
            next_pos = (i + 1) % pattern_length
            weight_diff = weights[next_pos] - weights[i]
            if weight_diff > 0 and not hits[next_pos]:
                syncopation_tension += weight_diff
            max_possible_tension += max(0, weights[(i + 1) % pattern_length] - weights[i])

    return min(1.0, syncopation_tension / max_possible_tension) if max_possible_tension > 0 else 0.0


def compute_pentagon_density(v1_hits: list[bool], v2_hits: list[bool], aux_hits: list[bool], pattern_length: int = 32) -> float:
    """Compute pattern density as fraction of active steps."""
    active_steps = sum(1 for i in range(pattern_length) if v1_hits[i] or v2_hits[i] or aux_hits[i])
    return active_steps / pattern_length


def compute_pentagon_velocity_range(v1_hits: list[bool], v2_hits: list[bool], aux_hits: list[bool],
                                    v1_vels: list[float], v2_vels: list[float], aux_vels: list[float]) -> float:
    """Compute velocity range across all voices."""
    all_velocities = []
    for i in range(len(v1_hits)):
        if v1_hits[i] and v1_vels[i] > 0:
            all_velocities.append(v1_vels[i])
        if v2_hits[i] and v2_vels[i] > 0:
            all_velocities.append(v2_vels[i])
        if aux_hits[i] and aux_vels[i] > 0:
            all_velocities.append(aux_vels[i])
    return max(all_velocities) - min(all_velocities) if len(all_velocities) >= 2 else 0.0


def compute_pentagon_voice_separation(v1_hits: list[bool], v2_hits: list[bool], aux_hits: list[bool], pattern_length: int = 32) -> float:
    """Compute voice separation as inverse of overlap."""
    overlap_count = 0
    total_active = 0
    for i in range(pattern_length):
        voices_active = v1_hits[i] + v2_hits[i] + aux_hits[i]
        if voices_active > 0:
            total_active += 1
        if voices_active >= 2:
            overlap_count += 1
    return 1.0 - (overlap_count / total_active) if total_active > 0 else 0.5


def compute_pentagon_regularity(hits: list[bool], pattern_length: int = 32) -> float:
    """Compute regularity as inverse of gap variance (CV)."""
    hit_positions = [i for i, h in enumerate(hits) if h]
    if len(hit_positions) < 2:
        return 0.5

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
    cv = (variance ** 0.5) / mean_gap
    return max(0.0, 1.0 - min(1.0, cv))


def score_pentagon_metric(raw: float, target_center: float, width: float) -> float:
    """Score metric using parabolic function around target center."""
    distance = abs(raw - target_center)
    return max(0.0, 1.0 - (distance / width) ** 2)


def compute_pentagon_metrics(pattern: "Pattern") -> PentagonMetrics:
    """Compute full Pentagon of Musicality analysis."""
    v1_hits = [s.v1 for s in pattern.steps]
    v2_hits = [s.v2 for s in pattern.steps]
    aux_hits = [s.aux for s in pattern.steps]
    v1_vels = [s.v1_vel for s in pattern.steps]
    v2_vels = [s.v2_vel for s in pattern.steps]
    aux_vels = [s.aux_vel for s in pattern.steps]
    pattern_length = pattern.length
    shape = pattern.shape
    energy = pattern.energy
    accent = pattern.accent

    # Raw metrics
    raw_sync = compute_pentagon_syncopation(v1_hits, pattern_length)
    raw_dens = compute_pentagon_density(v1_hits, v2_hits, aux_hits, pattern_length)
    raw_vel = compute_pentagon_velocity_range(v1_hits, v2_hits, aux_hits, v1_vels, v2_vels, aux_vels)
    raw_sep = compute_pentagon_voice_separation(v1_hits, v2_hits, aux_hits, pattern_length)
    raw_reg = compute_pentagon_regularity(v1_hits, pattern_length)

    # SHAPE-zone scoring targets (TIGHTENED v2)
    # Syncopation: Stable: 0.00-0.22, Syncopated: 0.22-0.48, Wild: 0.42-0.75
    # VoiceSep: Stable: 0.62-0.88, Syncopated: 0.52-0.78, Wild: 0.32-0.68
    # Regularity: Stable: 0.72-1.00, Syncopated: 0.42-0.68, Wild: 0.12-0.48
    if shape < 0.3:
        sync_target, sync_width = 0.11, 0.14
        sep_target, sep_width = 0.75, 0.16
        reg_target, reg_width = 0.86, 0.18
        zone = "stable"
    elif shape < 0.7:
        sync_target, sync_width = 0.35, 0.16
        sep_target, sep_width = 0.65, 0.16
        reg_target, reg_width = 0.55, 0.16
        zone = "syncopated"
    else:
        sync_target, sync_width = 0.58, 0.20
        sep_target, sep_width = 0.50, 0.22
        reg_target, reg_width = 0.30, 0.22
        zone = "wild"

    # ENERGY-based density target (TIGHTENED v2)
    # Stable: 0.15-0.32, Syncopated: 0.25-0.48, Wild: 0.32-0.65
    if shape < 0.3:
        base_min, base_max = 0.15, 0.32
    elif shape < 0.7:
        base_min, base_max = 0.25, 0.48
    else:
        base_min, base_max = 0.32, 0.65
    zone_range = base_max - base_min
    dens_target = base_min + energy * zone_range
    dens_width = zone_range / 2.2

    # ACCENT-based velocity target (TIGHTENED v2)
    # Low: 0.12-0.38, Med: 0.32-0.58, High: 0.25-0.72
    if accent < 0.3:
        vel_target, vel_width = 0.25, 0.16
    elif accent < 0.7:
        vel_target, vel_width = 0.45, 0.16
    else:
        vel_target, vel_width = 0.48, 0.28

    # Compute scores
    s_sync = score_pentagon_metric(raw_sync, sync_target, sync_width)
    s_dens = score_pentagon_metric(raw_dens, dens_target, dens_width)
    s_vel = score_pentagon_metric(raw_vel, vel_target, vel_width)
    s_sep = score_pentagon_metric(raw_sep, sep_target, sep_width)
    s_reg = score_pentagon_metric(raw_reg, reg_target, reg_width)

    # Composite (weighted sum)
    composite = 0.25 * s_sync + 0.15 * s_dens + 0.20 * s_vel + 0.20 * s_sep + 0.20 * s_reg

    return PentagonMetrics(
        raw_syncopation=raw_sync, raw_density=raw_dens, raw_velocity_range=raw_vel,
        raw_voice_separation=raw_sep, raw_regularity=raw_reg,
        score_syncopation=s_sync, score_density=s_dens, score_velocity_range=s_vel,
        score_voice_separation=s_sep, score_regularity=s_reg,
        composite=composite, shape_zone=zone,
    )


# Pentagon metric definitions with full descriptions and context-sensitive guidance
# TIGHTENED RANGES (v2) - more discriminating for better hill-climbing feedback
PENTAGON_METRICS = {
    "syncopation": {
        "short": "Sync",
        "name": "Syncopation",
        "description": "Tension from metric displacement. Measures hits on weak beats that create anticipation toward stronger beats. Based on the Longuet-Higgins & Lee (LHL) model.",
        "low_meaning": "Low (0-0.2): Straight, on-the-beat patterns. Good for: four-on-floor techno, marching rhythms, stable grooves.",
        "high_meaning": "High (0.6-1.0): Heavy off-beat emphasis. Good for: IDM, broken beats, jungle, complex polyrhythms.",
        "target_by_zone": {"stable": "0.00-0.22", "syncopated": "0.22-0.48", "wild": "0.42-0.75"},
        "why_matters": "Syncopation creates groove and forward motion. Too little feels mechanical; too much loses the pulse.",
    },
    "density": {
        "short": "Dens",
        "name": "Density",
        "description": "Overall activity level as fraction of steps with hits. Higher density = busier pattern with more notes per bar.",
        "low_meaning": "Low (0.1-0.3): Sparse, breathing space between hits. Good for: dub, ambient, minimal techno.",
        "high_meaning": "High (0.6-0.9): Busy, relentless activity. Good for: gabber, breakcore, drum & bass fills.",
        "target_by_zone": {"stable": "0.15-0.32", "syncopated": "0.25-0.48", "wild": "0.32-0.65"},
        "why_matters": "Density sets the energy level. Match it to ENERGY parameter for zone compliance.",
    },
    "velocity_range": {
        "short": "VelRng",
        "name": "Velocity Range",
        "description": "Dynamic contrast between loudest and quietest hits. Wide range enables ghost notes and accents for expressive dynamics.",
        "low_meaning": "Low (0.0-0.2): Flat, machine-like dynamics. Good for: industrial, hard techno, aggressive styles.",
        "high_meaning": "High (0.5-1.0): Expressive dynamics with ghost notes. Good for: jazz-influenced, hip-hop, nuanced grooves.",
        "target_by_zone": {"stable": "0.12-0.38", "syncopated": "0.32-0.58", "wild": "0.25-0.72"},
        "why_matters": "Velocity variation humanizes patterns. Ghost notes add texture without changing rhythm.",
    },
    "voice_separation": {
        "short": "VoiceSep",
        "name": "Voice Separation",
        "description": "Independence between voices (1 - overlap ratio). High separation means voices rarely hit simultaneously, creating clearer polyrhythms.",
        "low_meaning": "Low (0.3-0.5): Voices often hit together. Good for: powerful unison accents, wall-of-sound.",
        "high_meaning": "High (0.8-1.0): Voices rarely overlap. Good for: call-response, polyrhythms, complex textures.",
        "target_by_zone": {"stable": "0.62-0.88", "syncopated": "0.52-0.78", "wild": "0.32-0.68"},
        "why_matters": "Separation affects clarity vs. power. High DRIFT should increase separation.",
    },
    "regularity": {
        "short": "Reg",
        "name": "Regularity",
        "description": "Temporal predictability based on inter-onset interval (IOI) consistency. Regular patterns have even spacing; irregular patterns have varied gaps.",
        "low_meaning": "Low (0.0-0.3): Chaotic, unpredictable gaps. Good for: IDM, glitch, experimental music.",
        "high_meaning": "High (0.8-1.0): Even, metronomic spacing. Good for: techno, house, dance music.",
        "target_by_zone": {"stable": "0.72-1.00", "syncopated": "0.42-0.68", "wild": "0.12-0.48"},
        "why_matters": "Regularity = danceability. Stable patterns need high regularity; wild patterns break it.",
    },
}


def generate_pentagon_svg(metrics: PentagonMetrics, size: int = 120, show_values: bool = True) -> str:
    """Generate SVG radar chart for Pentagon of Musicality."""
    cx, cy = size / 2, size / 2
    r = size / 2 - 20  # Radius for max score

    # 5 axes at 72-degree intervals, starting from top
    angles = [math.radians(-90 + i * 72) for i in range(5)]

    # Metric keys in display order
    metric_keys = ["syncopation", "density", "velocity_range", "voice_separation", "regularity"]
    labels = [PENTAGON_METRICS[k]["short"] for k in metric_keys]
    # Rich tooltips with context
    tooltips = []
    for k in metric_keys:
        m = PENTAGON_METRICS[k]
        tooltip = f'{m["name"]}: {m["why_matters"]} Target for {metrics.shape_zone}: {m["target_by_zone"].get(metrics.shape_zone, "N/A")}'
        tooltips.append(tooltip)
    raw_values = [
        metrics.raw_syncopation, metrics.raw_density, metrics.raw_velocity_range,
        metrics.raw_voice_separation, metrics.raw_regularity
    ]
    score_values = [
        metrics.score_syncopation, metrics.score_density, metrics.score_velocity_range,
        metrics.score_voice_separation, metrics.score_regularity
    ]

    svg_parts = [
        f'<svg width="{size}" height="{size + 24}" viewBox="0 0 {size} {size + 24}" xmlns="http://www.w3.org/2000/svg">',
        f'<rect width="100%" height="100%" fill="#1a1a1a" rx="4"/>',
    ]

    # Draw grid circles (at 25%, 50%, 75%, 100%)
    for pct in [0.25, 0.5, 0.75, 1.0]:
        svg_parts.append(
            f'<circle cx="{cx}" cy="{cy}" r="{r * pct}" fill="none" stroke="#333333" stroke-width="1" opacity="0.5"/>'
        )

    # Draw axis lines and labels with color-coded range status
    for i, (angle, label, raw_val, score_val) in enumerate(zip(angles, labels, raw_values, score_values)):
        key = metric_keys[i]
        meta = PENTAGON_METRICS[key]
        target = meta["target_by_zone"].get(metrics.shape_zone, "0.0-1.0")

        # Check if in range
        in_range, distance, status_text = check_in_range(raw_val, target)
        alignment = compute_alignment_score(raw_val, target)

        x_end = cx + r * math.cos(angle)
        y_end = cy + r * math.sin(angle)
        svg_parts.append(
            f'<line x1="{cx}" y1="{cy}" x2="{x_end}" y2="{y_end}" stroke="#444444" stroke-width="1"/>'
        )

        # Label position (slightly beyond the axis)
        lx = cx + (r + 14) * math.cos(angle)
        ly = cy + (r + 14) * math.sin(angle)

        # Color based on range status: green=in, orange=close, red=far
        if in_range:
            label_color = "#44ff44"  # Green - in range
            status_icon = "✓"
        elif alignment > 0.5:
            label_color = "#ffaa44"  # Orange - close
            status_icon = "~"
        else:
            label_color = "#ff6666"  # Red - out of range
            status_icon = "✗"

        # Rich tooltip with status
        full_tooltip = (
            f'{meta["name"]} ({metrics.shape_zone.upper()} zone)\n'
            f'{status_text}\n'
            f'Value: {raw_val:.2f} | Target: {target}\n'
            f'Alignment: {alignment:.0%}'
        )

        svg_parts.append(
            f'<text x="{lx}" y="{ly + 3}" text-anchor="middle" fill="{label_color}" font-size="8" '
            f'font-weight="600" style="cursor: help;">'
            f'<title>{full_tooltip}</title>{label}</text>'
        )

    # Draw raw values polygon (dim)
    raw_points = []
    for i, (angle, val) in enumerate(zip(angles, raw_values)):
        px = cx + r * val * math.cos(angle)
        py = cy + r * val * math.sin(angle)
        raw_points.append(f"{px},{py}")
    svg_parts.append(
        f'<polygon points="{" ".join(raw_points)}" fill="#4ecdc4" fill-opacity="0.15" '
        f'stroke="#4ecdc4" stroke-width="1" stroke-opacity="0.4"/>'
    )

    # Draw scored values polygon (bright)
    score_points = []
    for i, (angle, val) in enumerate(zip(angles, score_values)):
        px = cx + r * val * math.cos(angle)
        py = cy + r * val * math.sin(angle)
        score_points.append(f"{px},{py}")
    svg_parts.append(
        f'<polygon points="{" ".join(score_points)}" fill="#ff6b35" fill-opacity="0.3" '
        f'stroke="#ff6b35" stroke-width="2"/>'
    )

    # Draw dots at scored vertices
    for i, (angle, val) in enumerate(zip(angles, score_values)):
        px = cx + r * val * math.cos(angle)
        py = cy + r * val * math.sin(angle)
        svg_parts.append(f'<circle cx="{px}" cy="{py}" r="3" fill="#ff6b35"/>')

    # Composite score in center
    svg_parts.append(
        f'<text x="{cx}" y="{cy + 4}" text-anchor="middle" fill="#ffffff" font-size="14" font-weight="600">'
        f'{metrics.composite:.0%}</text>'
    )

    # Zone label at bottom
    zone_colors = {"stable": "#44aa44", "syncopated": "#aaaa44", "wild": "#aa4444"}
    svg_parts.append(
        f'<text x="{cx}" y="{size + 16}" text-anchor="middle" fill="{zone_colors.get(metrics.shape_zone, "#888888")}" '
        f'font-size="10" font-weight="500">{metrics.shape_zone.upper()}</text>'
    )

    svg_parts.append('</svg>')
    return '\n'.join(svg_parts)


def generate_knob_svg(
    value: float,
    label: str,
    color: str = "#888888",
    highlight_color: str = "#ffffff",
    size: int = 48,
) -> str:
    """Generate SVG for a single rotary knob."""
    cx, cy = size / 2, size / 2
    r = size / 2 - 6  # Leave room for label

    # Arc angles: start at 135° (bottom-left), end at 45° (bottom-right)
    # Total sweep is 270° (from 135° to 405° = 45°)
    start_angle = 135
    end_angle = 405  # 45 + 360
    sweep = end_angle - start_angle

    # Calculate current angle based on value
    current_angle = start_angle + (value * sweep)

    # Convert to radians for SVG
    import math
    def polar_to_cart(cx, cy, r, angle_deg):
        angle_rad = math.radians(angle_deg)
        return cx + r * math.cos(angle_rad), cy + r * math.sin(angle_rad)

    # Track arc (full range, dim)
    track_start = polar_to_cart(cx, cy, r - 4, start_angle)
    track_end = polar_to_cart(cx, cy, r - 4, end_angle)

    # Value arc (filled portion)
    value_end = polar_to_cart(cx, cy, r - 4, current_angle)

    # Indicator line (pointer)
    indicator_inner = polar_to_cart(cx, cy, r * 0.3, current_angle)
    indicator_outer = polar_to_cart(cx, cy, r - 2, current_angle)

    # Large arc flag (1 if sweep > 180°)
    large_arc_track = 1 if sweep > 180 else 0
    value_sweep = current_angle - start_angle
    large_arc_value = 1 if value_sweep > 180 else 0

    # Only draw value arc if value is meaningful (> 0.01)
    value_arc_svg = ""
    if value > 0.01:
        value_arc_svg = f'''
  <!-- Value arc (filled) -->
  <path d="M {track_start[0]:.1f} {track_start[1]:.1f} A {r - 4} {r - 4} 0 {large_arc_value} 1 {value_end[0]:.1f} {value_end[1]:.1f}"
        fill="none" stroke="{color}" stroke-width="{KNOB_TRACK_WIDTH}" stroke-linecap="round"/>'''

    svg = f'''<svg width="{size}" height="{size + 16}" viewBox="0 0 {size} {size + 16}" xmlns="http://www.w3.org/2000/svg">
  <!-- Knob body -->
  <circle cx="{cx}" cy="{cy}" r="{r}" fill="#1e1e1e" stroke="#3d3d3d" stroke-width="1"/>
  <circle cx="{cx}" cy="{cy}" r="{r - 2}" fill="#2a2a2a"/>

  <!-- Track arc (background) -->
  <path d="M {track_start[0]:.1f} {track_start[1]:.1f} A {r - 4} {r - 4} 0 {large_arc_track} 1 {track_end[0]:.1f} {track_end[1]:.1f}"
        fill="none" stroke="#3d3d3d" stroke-width="{KNOB_TRACK_WIDTH}" stroke-linecap="round"/>{value_arc_svg}

  <!-- Indicator line -->
  <line x1="{indicator_inner[0]:.1f}" y1="{indicator_inner[1]:.1f}"
        x2="{indicator_outer[0]:.1f}" y2="{indicator_outer[1]:.1f}"
        stroke="{highlight_color}" stroke-width="{KNOB_INDICATOR_WIDTH}" stroke-linecap="round"/>

  <!-- Center dot -->
  <circle cx="{cx}" cy="{cy}" r="3" fill="#4d4d4d"/>

  <!-- Label -->
  <text x="{cx}" y="{size + 12}" text-anchor="middle" fill="#888888" font-size="9" font-family="sans-serif">{label}</text>
</svg>'''
    return svg


def generate_knob_panel_svg(
    shape: float,
    energy: float,
    axis_x: float,
    axis_y: float,
    drift: float = 0.0,
    accent: float = 0.5,
    seed: Optional[int] = None,
    highlight_param: Optional[str] = None,
) -> str:
    """Generate a panel with two 2x2 grids of knobs matching hardware layout."""
    knob_size = 44
    knob_spacing = 4
    grid_spacing = 24  # Space between the two 2x2 grids
    label_height = 16

    # 2x2 grid dimensions
    grid_width = 2 * knob_size + knob_spacing
    grid_height = 2 * (knob_size + 16) + knob_spacing  # +16 for knob labels

    # Total panel dimensions
    panel_width = 2 * grid_width + grid_spacing + 32  # Extra for section labels
    panel_height = grid_height + label_height + 8

    # Performance knobs (K1-K4): SHAPE, ENERGY, AXIS X, AXIS Y
    performance_knobs = [
        ("K1", "SHAPE", shape, COLORS["v1"]),
        ("K2", "ENERGY", energy, COLORS["v2"]),
        ("K3", "AXIS X", axis_x, COLORS["aux"]),
        ("K4", "AXIS Y", axis_y, COLORS["aux"]),
    ]

    # Config knobs: DRIFT, ACCENT (and placeholders for CV attenuators)
    config_knobs = [
        ("K5", "DRIFT", drift, "#a0a0a0"),
        ("K6", "ACCENT", accent, "#ffd700"),
        ("", "", 0.0, "#404040"),  # Placeholder
        ("", "", 0.0, "#404040"),  # Placeholder
    ]

    svg_parts = [
        f'<svg width="{panel_width}" height="{panel_height}" xmlns="http://www.w3.org/2000/svg" '
        f'style="font-family: -apple-system, BlinkMacSystemFont, sans-serif;">',
        f'<rect width="100%" height="100%" fill="#1a1a1a" rx="6"/>',
    ]

    # Section labels
    perf_x = 8
    config_x = perf_x + grid_width + grid_spacing

    svg_parts.append(
        f'<text x="{perf_x + grid_width / 2}" y="14" text-anchor="middle" '
        f'fill="#666666" font-size="10" font-weight="500">PERFORMANCE</text>'
    )
    svg_parts.append(
        f'<text x="{config_x + grid_width / 2}" y="14" text-anchor="middle" '
        f'fill="#666666" font-size="10" font-weight="500">CONFIG</text>'
    )

    def draw_2x2_grid(knobs, start_x, start_y):
        """Draw a 2x2 grid of knobs."""
        positions = [(0, 0), (1, 0), (0, 1), (1, 1)]  # col, row
        for (col, row), (k_id, name, value, color) in zip(positions, knobs):
            if not name:  # Skip placeholders
                continue
            x = start_x + col * (knob_size + knob_spacing)
            y = start_y + row * (knob_size + 16 + knob_spacing)

            # Highlight the parameter being swept
            is_highlighted = highlight_param and name.replace(" ", "_").lower() == highlight_param.replace("-", "_").lower()
            knob_color = "#ffffff" if is_highlighted else color
            highlight = "#ffffff" if is_highlighted else "#cccccc"

            svg_parts.append(f'<g transform="translate({x}, {y})">')
            svg_parts.append(generate_knob_svg(value, name, knob_color, highlight, knob_size))
            svg_parts.append('</g>')

    # Draw performance grid
    draw_2x2_grid(performance_knobs, perf_x, label_height + 4)

    # Draw config grid
    draw_2x2_grid(config_knobs, config_x, label_height + 4)

    # Divider line between sections
    div_x = perf_x + grid_width + grid_spacing / 2 - 1
    svg_parts.append(
        f'<line x1="{div_x}" y1="8" x2="{div_x}" y2="{panel_height - 8}" '
        f'stroke="#333333" stroke-width="1"/>'
    )

    # Seed display
    if seed is not None:
        svg_parts.append(
            f'<text x="{panel_width - 8}" y="{panel_height - 4}" text-anchor="end" '
            f'fill="#404040" font-size="8" font-family="monospace">SEED: {seed:08X}</text>'
        )

    svg_parts.append('</svg>')
    return '\n'.join(svg_parts)


def generate_pattern_svg(pattern: Pattern, show_velocity: bool = True) -> str:
    """Generate SVG for a single pattern."""
    num_steps = pattern.length
    num_voices = 3

    # Calculate dimensions
    grid_width = num_steps * STEP_WIDTH
    grid_height = num_voices * STEP_HEIGHT + (num_voices - 1) * VOICE_GAP

    if show_velocity:
        total_height = grid_height + VELOCITY_HEIGHT + 8
    else:
        total_height = grid_height

    total_width = LABEL_WIDTH + grid_width

    svg_parts = [
        f'<svg width="{total_width}" height="{total_height}" '
        f'xmlns="http://www.w3.org/2000/svg" style="font-family: -apple-system, BlinkMacSystemFont, sans-serif; font-size: 11px;">',
        f'<rect width="100%" height="100%" fill="{COLORS["grid_bg"]}"/>',
    ]

    # Draw grid background with beat markers
    for step in range(num_steps):
        x = LABEL_WIDTH + step * STEP_WIDTH
        # Highlight downbeats (every 4 steps)
        if step % 4 == 0:
            svg_parts.append(
                f'<rect x="{x}" y="0" width="{STEP_WIDTH}" height="{grid_height}" '
                f'fill="{COLORS["downbeat"]}"/>'
            )
        # Draw vertical grid lines
        line_color = COLORS["grid_line_strong"] if step % 4 == 0 else COLORS["grid_line"]
        svg_parts.append(
            f'<line x1="{x}" y1="0" x2="{x}" y2="{grid_height}" '
            f'stroke="{line_color}" stroke-width="1"/>'
        )

    # Voice labels and rows
    voices = [
        ("V1", "Anchor", COLORS["v1"], COLORS["v1_dim"], lambda s: s.v1, lambda s: s.v1_vel),
        ("V2", "Shimmer", COLORS["v2"], COLORS["v2_dim"], lambda s: s.v2, lambda s: s.v2_vel),
        ("AUX", "Aux", COLORS["aux"], COLORS["aux_dim"], lambda s: s.aux, lambda s: s.aux_vel),
    ]

    for voice_idx, (short_name, full_name, color, dim_color, hit_fn, vel_fn) in enumerate(voices):
        y = voice_idx * (STEP_HEIGHT + VOICE_GAP)

        # Voice label
        svg_parts.append(
            f'<text x="{LABEL_WIDTH - 8}" y="{y + STEP_HEIGHT / 2 + 4}" '
            f'fill="{color}" text-anchor="end" font-weight="500">{short_name}</text>'
        )

        # Draw hits for this voice
        for step in pattern.steps:
            x = LABEL_WIDTH + step.step * STEP_WIDTH
            if hit_fn(step):
                vel = vel_fn(step)
                # Note block with velocity-based opacity
                opacity = 0.4 + vel * 0.6
                svg_parts.append(
                    f'<rect x="{x + 2}" y="{y + 2}" width="{STEP_WIDTH - 4}" height="{STEP_HEIGHT - 4}" '
                    f'rx="3" fill="{color}" opacity="{opacity:.2f}"/>'
                )
            else:
                # Empty step indicator
                svg_parts.append(
                    f'<rect x="{x + 6}" y="{y + STEP_HEIGHT // 2 - 2}" width="{STEP_WIDTH - 12}" height="4" '
                    f'rx="2" fill="{dim_color}" opacity="0.3"/>'
                )

        # Horizontal divider line
        svg_parts.append(
            f'<line x1="{LABEL_WIDTH}" y1="{y + STEP_HEIGHT + VOICE_GAP // 2}" '
            f'x2="{total_width}" y2="{y + STEP_HEIGHT + VOICE_GAP // 2}" '
            f'stroke="{COLORS["grid_line"]}" stroke-width="1"/>'
        )

    # Velocity section
    if show_velocity:
        vel_y = grid_height + 8

        # Background
        svg_parts.append(
            f'<rect x="{LABEL_WIDTH}" y="{vel_y}" width="{grid_width}" height="{VELOCITY_HEIGHT}" '
            f'fill="{COLORS["velocity_bg"]}" rx="2"/>'
        )

        # Label
        svg_parts.append(
            f'<text x="{LABEL_WIDTH - 8}" y="{vel_y + VELOCITY_HEIGHT / 2 + 4}" '
            f'fill="{COLORS["text_dim"]}" text-anchor="end" font-size="10">VEL</text>'
        )

        # Draw velocity bars for all voices
        bar_width = (STEP_WIDTH - 6) / 3
        for step in pattern.steps:
            x = LABEL_WIDTH + step.step * STEP_WIDTH + 2

            for i, (_, _, color, _, hit_fn, vel_fn) in enumerate(voices):
                if hit_fn(step):
                    vel = vel_fn(step)
                    bar_height = vel * (VELOCITY_HEIGHT - 4)
                    bar_x = x + i * (bar_width + 1)
                    svg_parts.append(
                        f'<rect x="{bar_x}" y="{vel_y + VELOCITY_HEIGHT - 2 - bar_height}" '
                        f'width="{bar_width}" height="{bar_height}" fill="{color}" opacity="0.8"/>'
                    )

    svg_parts.append('</svg>')
    return '\n'.join(svg_parts)


def compute_pattern_metrics(pattern: Pattern) -> dict:
    """Compute detailed metrics for a single pattern (from expressiveness evaluation)."""
    v1_positions = [s.step for s in pattern.steps if s.v1]
    v2_positions = [s.step for s in pattern.steps if s.v2]

    metrics = {
        "v1_hits": pattern.v1_hits,
        "v2_hits": pattern.v2_hits,
        "aux_hits": pattern.aux_hits,
        "v1_mask": sum(1 << s.step for s in pattern.steps if s.v1),
        "v2_mask": sum(1 << s.step for s in pattern.steps if s.v2),
        "aux_mask": sum(1 << s.step for s in pattern.steps if s.aux),
    }

    # Rhythmic analysis
    quarter_notes = {0, 4, 8, 12, 16, 20, 24, 28}
    metrics["quarter_note_hits"] = len([p for p in v1_positions if p in quarter_notes])
    metrics["offbeat_hits"] = len([p for p in v1_positions if p % 2 == 1])
    metrics["syncopation_ratio"] = metrics["offbeat_hits"] / len(v1_positions) if v1_positions else 0.0

    # Gap analysis (V1)
    if len(v1_positions) > 1:
        gaps = [v1_positions[i+1] - v1_positions[i] for i in range(len(v1_positions)-1)]
        metrics["gaps"] = gaps
        metrics["max_gap"] = max(gaps)
        metrics["min_gap"] = min(gaps)
        metrics["avg_gap"] = sum(gaps) / len(gaps)
        gap_variance = sum((g - metrics["avg_gap"]) ** 2 for g in gaps) / len(gaps)
        std_dev = gap_variance ** 0.5
        metrics["regularity_score"] = max(0.0, 1.0 - std_dev / 8.0)
    else:
        metrics["gaps"] = []
        metrics["max_gap"] = 0
        metrics["min_gap"] = 0
        metrics["avg_gap"] = 0
        metrics["regularity_score"] = 0

    # Velocity analysis
    v1_vels = [s.v1_vel for s in pattern.steps if s.v1 and s.v1_vel > 0]
    v2_vels = [s.v2_vel for s in pattern.steps if s.v2 and s.v2_vel > 0]
    metrics["v1_avg_velocity"] = sum(v1_vels) / len(v1_vels) if v1_vels else 0
    metrics["v1_velocity_range"] = max(v1_vels) - min(v1_vels) if len(v1_vels) > 1 else 0
    metrics["v2_avg_velocity"] = sum(v2_vels) / len(v2_vels) if v2_vels else 0
    metrics["v2_velocity_range"] = max(v2_vels) - min(v2_vels) if len(v2_vels) > 1 else 0

    return metrics


def compute_seed_variation(patterns: list[Pattern]) -> dict:
    """Compute seed variation scores across a set of patterns."""
    if not patterns:
        return {"v1_score": 0, "v2_score": 0, "aux_score": 0, "unique_v1": 0, "unique_v2": 0, "unique_aux": 0}

    v1_masks = set()
    v2_masks = set()
    aux_masks = set()

    for p in patterns:
        v1_masks.add(sum(1 << s.step for s in p.steps if s.v1))
        v2_masks.add(sum(1 << s.step for s in p.steps if s.v2))
        aux_masks.add(sum(1 << s.step for s in p.steps if s.aux))

    def variation_score(unique_count: int, total: int) -> float:
        if total <= 1:
            return 0.0
        return (unique_count - 1) / (total - 1)

    n = len(patterns)
    return {
        "unique_v1": len(v1_masks),
        "unique_v2": len(v2_masks),
        "unique_aux": len(aux_masks),
        "total_patterns": n,
        "v1_score": variation_score(len(v1_masks), n),
        "v2_score": variation_score(len(v2_masks), n),
        "aux_score": variation_score(len(aux_masks), n),
    }


def generate_expressiveness_svg(stats: dict, title: str, width: int = 600) -> str:
    """Generate SVG showing expressiveness metrics."""
    height = 120

    svg_parts = [
        f'<svg width="{width}" height="{height}" xmlns="http://www.w3.org/2000/svg" '
        f'style="font-family: -apple-system, BlinkMacSystemFont, sans-serif; font-size: 11px;">',
        f'<rect width="100%" height="100%" fill="#1e1e1e" rx="4"/>',
    ]

    # Title
    svg_parts.append(
        f'<text x="12" y="20" fill="#888888" font-size="12" font-weight="500">{title}</text>'
    )

    # Metrics bars
    bar_width = 120
    bar_height = 16
    bar_gap = 8

    metrics = [
        ("V1 Variation", stats.get("v1_score", 0), COLORS["v1"]),
        ("V2 Variation", stats.get("v2_score", 0), COLORS["v2"]),
        ("AUX Variation", stats.get("aux_score", 0), COLORS["aux"]),
    ]

    for i, (label, value, color) in enumerate(metrics):
        x = 12 + i * (bar_width + bar_gap + 60)
        y = 40

        # Label
        svg_parts.append(
            f'<text x="{x}" y="{y + 10}" fill="#888888" font-size="10">{label}</text>'
        )

        # Background bar
        svg_parts.append(
            f'<rect x="{x}" y="{y + 16}" width="{bar_width}" height="{bar_height}" '
            f'rx="3" fill="#333333"/>'
        )

        # Value bar
        fill_width = value * bar_width
        bar_color = color if value >= 0.5 else "#ff4444"
        svg_parts.append(
            f'<rect x="{x}" y="{y + 16}" width="{fill_width:.1f}" height="{bar_height}" '
            f'rx="3" fill="{bar_color}"/>'
        )

        # Percentage
        svg_parts.append(
            f'<text x="{x + bar_width + 8}" y="{y + 28}" fill="{bar_color}" font-size="11" font-weight="500">'
            f'{value:.0%}</text>'
        )

    # Unique counts
    y = 85
    svg_parts.append(
        f'<text x="12" y="{y}" fill="#666666" font-size="10">'
        f'Unique patterns: V1={stats.get("unique_v1", 0)}/{stats.get("total_patterns", 0)} | '
        f'V2={stats.get("unique_v2", 0)}/{stats.get("total_patterns", 0)} | '
        f'AUX={stats.get("unique_aux", 0)}/{stats.get("total_patterns", 0)}</text>'
    )

    # Pass/fail indicator
    overall = (stats.get("v1_score", 0) + stats.get("v2_score", 0) + stats.get("aux_score", 0)) / 3
    status = "PASS" if stats.get("v2_score", 0) >= 0.5 else "FAIL"
    status_color = "#44ff44" if status == "PASS" else "#ff4444"
    svg_parts.append(
        f'<text x="{width - 12}" y="{y}" text-anchor="end" fill="{status_color}" '
        f'font-size="11" font-weight="600">[{status}] Overall: {overall:.0%}</text>'
    )

    svg_parts.append('</svg>')
    return '\n'.join(svg_parts)


def compute_step_statistics(patterns: list[Pattern], num_steps: int = 32) -> dict:
    """Compute per-step hit statistics across a collection of patterns."""
    # Initialize counters
    v1_counts = [0] * num_steps
    v2_counts = [0] * num_steps
    aux_counts = [0] * num_steps
    v1_velocities = [[] for _ in range(num_steps)]
    v2_velocities = [[] for _ in range(num_steps)]
    aux_velocities = [[] for _ in range(num_steps)]

    num_patterns = len(patterns)

    for pattern in patterns:
        for step in pattern.steps:
            if step.step >= num_steps:
                continue
            if step.v1:
                v1_counts[step.step] += 1
                v1_velocities[step.step].append(step.v1_vel)
            if step.v2:
                v2_counts[step.step] += 1
                v2_velocities[step.step].append(step.v2_vel)
            if step.aux:
                aux_counts[step.step] += 1
                aux_velocities[step.step].append(step.aux_vel)

    def calc_stats(counts, velocities):
        import statistics
        frequencies = [c / num_patterns if num_patterns > 0 else 0 for c in counts]
        avg_vels = []
        for vels in velocities:
            avg_vels.append(statistics.mean(vels) if vels else 0)
        return {
            "counts": counts,
            "frequencies": frequencies,
            "avg_velocities": avg_vels,
            "total_hits": sum(counts),
            "mean_hits_per_pattern": sum(counts) / num_patterns if num_patterns > 0 else 0,
        }

    return {
        "num_patterns": num_patterns,
        "num_steps": num_steps,
        "v1": calc_stats(v1_counts, v1_velocities),
        "v2": calc_stats(v2_counts, v2_velocities),
        "aux": calc_stats(aux_counts, aux_velocities),
    }


def compute_pentagon_statistics(patterns: list[Pattern]) -> dict:
    """Compute Pentagon metric averages across a collection of patterns."""
    if not patterns:
        return {}

    # Compute Pentagon metrics for each pattern
    all_metrics = [compute_pentagon_metrics(p) for p in patterns]

    # Aggregate by zone
    by_zone = {"stable": [], "syncopated": [], "wild": []}
    for m in all_metrics:
        by_zone[m.shape_zone].append(m)

    def avg_metrics(metrics_list):
        if not metrics_list:
            return None
        n = len(metrics_list)
        return {
            "count": n,
            "raw_syncopation": sum(m.raw_syncopation for m in metrics_list) / n,
            "raw_density": sum(m.raw_density for m in metrics_list) / n,
            "raw_velocity_range": sum(m.raw_velocity_range for m in metrics_list) / n,
            "raw_voice_separation": sum(m.raw_voice_separation for m in metrics_list) / n,
            "raw_regularity": sum(m.raw_regularity for m in metrics_list) / n,
            "composite": sum(m.composite for m in metrics_list) / n,
        }

    return {
        "total": avg_metrics(all_metrics),
        "stable": avg_metrics(by_zone["stable"]),
        "syncopated": avg_metrics(by_zone["syncopated"]),
        "wild": avg_metrics(by_zone["wild"]),
    }


def parse_target_range(target_str: str) -> tuple[float, float]:
    """Parse target range string like '0.0-0.25' into (min, max) tuple."""
    try:
        parts = target_str.split("-")
        if len(parts) == 2:
            return float(parts[0]), float(parts[1])
    except (ValueError, IndexError):
        pass
    return 0.0, 1.0  # Default to full range


def check_in_range(value: float, target_str: str) -> tuple[bool, float, str]:
    """
    Check if value is in target range.
    Returns: (is_in_range, distance_from_range, status_text)
    """
    min_val, max_val = parse_target_range(target_str)

    if min_val <= value <= max_val:
        # In range - calculate how centered it is (0 = edge, 1 = center)
        range_center = (min_val + max_val) / 2
        range_width = (max_val - min_val) / 2
        if range_width > 0:
            centeredness = 1.0 - abs(value - range_center) / range_width
        else:
            centeredness = 1.0
        return True, 0.0, f"✓ In range ({min_val:.2f}-{max_val:.2f})"
    else:
        # Out of range - calculate distance
        if value < min_val:
            distance = min_val - value
            return False, distance, f"✗ Below target (need +{distance:.2f})"
        else:
            distance = value - max_val
            return False, distance, f"✗ Above target (need -{distance:.2f})"


def compute_alignment_score(value: float, target_str: str) -> float:
    """
    Compute 0-1 alignment score for a single metric.
    1.0 = perfectly centered in target range
    0.0 = far outside target range
    """
    min_val, max_val = parse_target_range(target_str)
    range_center = (min_val + max_val) / 2
    range_width = (max_val - min_val) / 2

    distance_from_center = abs(value - range_center)

    # Score is 1.0 at center, decreasing as we move away
    # We use a tolerance of 2x the range width before hitting 0
    tolerance = max(range_width * 2, 0.2)  # At least 0.2 tolerance

    score = max(0.0, 1.0 - distance_from_center / tolerance)
    return score


def generate_pentagon_radar_svg(pentagon_stats: dict, size: int = 400) -> str:
    """Generate just the radar chart SVG for Pentagon of Musicality."""
    metric_keys = ["syncopation", "density", "velocity_range", "voice_separation", "regularity"]

    chart_cx = size // 2
    chart_cy = size // 2
    chart_r = size // 2 - 50

    svg_parts = [
        f'<svg width="{size}" height="{size}" xmlns="http://www.w3.org/2000/svg" '
        f'style="font-family: -apple-system, BlinkMacSystemFont, sans-serif;">',
    ]

    angles = [math.radians(-90 + i * 72) for i in range(5)]
    labels = [PENTAGON_METRICS[k]["short"] for k in metric_keys]

    total_data = pentagon_stats.get("total", {})
    raw_attrs = ["raw_syncopation", "raw_density", "raw_velocity_range", "raw_voice_separation", "raw_regularity"]
    total_values = [total_data.get(attr, 0.5) for attr in raw_attrs]

    # Draw grid circles
    for pct in [0.25, 0.5, 0.75, 1.0]:
        svg_parts.append(
            f'<circle cx="{chart_cx}" cy="{chart_cy}" r="{chart_r * pct}" fill="none" stroke="#333333" stroke-width="1" opacity="0.5"/>'
        )

    # Draw target zone bands
    zone_colors_map = {"stable": "#44aa44", "syncopated": "#aaaa44", "wild": "#aa4444"}
    for zone, zone_color in zone_colors_map.items():
        zone_max_points = []
        for i, (angle, key) in enumerate(zip(angles, metric_keys)):
            target_str = PENTAGON_METRICS[key]["target_by_zone"].get(zone, "0.0-1.0")
            _, t_max = parse_target_range(target_str)
            px_max = chart_cx + chart_r * t_max * math.cos(angle)
            py_max = chart_cy + chart_r * t_max * math.sin(angle)
            zone_max_points.append(f"{px_max},{py_max}")
        svg_parts.append(
            f'<polygon points="{" ".join(zone_max_points)}" fill="none" '
            f'stroke="{zone_color}" stroke-width="1" stroke-dasharray="4,4" opacity="0.4"/>'
        )

    # Draw axis lines and labels
    for i, (angle, label) in enumerate(zip(angles, labels)):
        key = metric_keys[i]
        meta = PENTAGON_METRICS[key]
        x_end = chart_cx + chart_r * math.cos(angle)
        y_end = chart_cy + chart_r * math.sin(angle)
        svg_parts.append(
            f'<line x1="{chart_cx}" y1="{chart_cy}" x2="{x_end}" y2="{y_end}" stroke="#444444" stroke-width="1"/>'
        )

        lx = chart_cx + (chart_r + 30) * math.cos(angle)
        ly = chart_cy + (chart_r + 30) * math.sin(angle)
        tooltip = f'{meta["name"]}: {meta["why_matters"]}'
        svg_parts.append(
            f'<text x="{lx}" y="{ly + 4}" text-anchor="middle" fill="#ffffff" font-size="12" '
            f'font-weight="600" style="cursor: help;"><title>{tooltip}</title>{label}</text>'
        )

    # Draw actual values polygon
    points = []
    for angle, val in zip(angles, total_values):
        px = chart_cx + chart_r * val * math.cos(angle)
        py = chart_cy + chart_r * val * math.sin(angle)
        points.append(f"{px},{py}")
    svg_parts.append(
        f'<polygon points="{" ".join(points)}" fill="#4ecdc4" fill-opacity="0.3" stroke="#4ecdc4" stroke-width="2"/>'
    )

    # Draw dots at vertices
    for angle, val in zip(angles, total_values):
        px = chart_cx + chart_r * val * math.cos(angle)
        py = chart_cy + chart_r * val * math.sin(angle)
        svg_parts.append(f'<circle cx="{px}" cy="{py}" r="5" fill="#4ecdc4"/>')

    # Composite in center
    total_composite = total_data.get("composite", 0.5)
    svg_parts.append(
        f'<text x="{chart_cx}" y="{chart_cy + 8}" text-anchor="middle" fill="#ffffff" font-size="32" font-weight="700">'
        f'{total_composite:.0%}</text>'
    )
    svg_parts.append(
        f'<text x="{chart_cx}" y="{chart_cy + 28}" text-anchor="middle" fill="#666666" font-size="11">composite</text>'
    )

    svg_parts.append('</svg>')
    return '\n'.join(svg_parts)


def generate_pentagon_summary_html(pentagon_stats: dict) -> str:
    """Generate HTML section for Pentagon of Musicality with radar chart and table."""
    metric_keys = ["syncopation", "density", "velocity_range", "voice_separation", "regularity"]
    raw_attrs = ["raw_syncopation", "raw_density", "raw_velocity_range", "raw_voice_separation", "raw_regularity"]
    zones = [("stable", "#44aa44", "STABLE", "0-30%"), ("syncopated", "#aaaa44", "SYNCOPATED", "30-70%"), ("wild", "#aa4444", "WILD", "70-100%")]

    # Track alignment scores
    all_alignment_scores = []

    # Generate radar chart SVG
    radar_svg = generate_pentagon_radar_svg(pentagon_stats, size=380)

    # Build HTML
    html_parts = [
        '<div style="background: #1e1e1e; border-radius: 8px; padding: 20px; margin-bottom: 20px;">',
        '<h3 style="color: #ffffff; margin: 0 0 16px 0; font-size: 16px;">Pentagon of Musicality — Overview</h3>',
        '<div style="display: flex; gap: 30px; align-items: flex-start;">',
        # Left: Radar chart
        '<div style="flex-shrink: 0;">',
        '<div style="color: #888888; font-size: 11px; text-align: center; margin-bottom: 8px;">ALL PATTERNS AVERAGE</div>',
        radar_svg,
        '</div>',
        # Right: Table
        '<div style="flex-grow: 1;">',
        '<table style="width: 100%; border-collapse: collapse; font-size: 13px;">',
        '<thead>',
        '<tr style="border-bottom: 1px solid #333;">',
        '<th style="text-align: left; padding: 8px 12px; color: #888;">Metric</th>',
    ]

    # Zone headers
    for zone, color, label, shape_range in zones:
        html_parts.append(
            f'<th style="text-align: center; padding: 8px 12px;">'
            f'<span style="color: {color}; font-weight: 600;">{label}</span><br>'
            f'<span style="color: #555; font-size: 10px;">SHAPE {shape_range}</span></th>'
        )
    html_parts.append('</tr></thead><tbody>')

    # Metric rows
    for key, attr in zip(metric_keys, raw_attrs):
        meta = PENTAGON_METRICS[key]
        html_parts.append('<tr style="border-bottom: 1px solid #2a2a2a;">')
        html_parts.append(
            f'<td style="padding: 10px 12px; max-width: 300px;">'
            f'<span style="color: {COLORS["v1"]}; font-weight: 500;">{meta["name"]}</span><br>'
            f'<span style="color: #666; font-size: 10px;">{meta["why_matters"]}</span></td>'
        )

        for zone, zone_color, _, _ in zones:
            zone_data = pentagon_stats.get(zone)
            target = meta["target_by_zone"].get(zone, "0.0-1.0")

            if zone_data:
                val = zone_data.get(attr, 0)
                in_range, distance, status_text = check_in_range(val, target)
                alignment = compute_alignment_score(val, target)
                all_alignment_scores.append(alignment)

                if in_range:
                    val_color = "#44ff44"
                elif alignment > 0.5:
                    val_color = "#ffaa44"
                else:
                    val_color = "#ff4444"

                html_parts.append(
                    f'<td style="text-align: center; padding: 10px 12px;" title="{status_text} | Alignment: {alignment:.0%}">'
                    f'<span style="color: {val_color}; font-weight: 600; font-size: 14px;">{val:.2f}</span><br>'
                    f'<span style="color: #555; font-size: 10px;">target: {target}</span></td>'
                )
            else:
                html_parts.append('<td style="text-align: center; color: #444;">—</td>')
        html_parts.append('</tr>')

    # Compliance row
    html_parts.append('<tr style="border-top: 2px solid #333;">')
    html_parts.append('<td style="padding: 10px 12px; color: #fff; font-weight: 600;">Zone Compliance</td>')
    for zone, color, _, _ in zones:
        zone_data = pentagon_stats.get(zone)
        if zone_data:
            comp = zone_data.get("composite", 0)
            count = zone_data.get("count", 0)
            html_parts.append(
                f'<td style="text-align: center; padding: 10px 12px;">'
                f'<span style="color: {color}; font-weight: 600; font-size: 14px;">{comp:.0%}</span><br>'
                f'<span style="color: #555; font-size: 10px;">n={count}</span></td>'
            )
        else:
            html_parts.append('<td style="text-align: center; color: #444;">—</td>')
    html_parts.append('</tr>')

    html_parts.append('</tbody></table>')
    html_parts.append('</div></div>')  # Close table div and flex container

    # Alignment score section
    overall_alignment = sum(all_alignment_scores) / len(all_alignment_scores) if all_alignment_scores else 0.0
    pentagon_stats["overall_alignment"] = overall_alignment

    if overall_alignment >= 0.7:
        align_color = "#44ff44"
        align_status = "GOOD"
    elif overall_alignment >= 0.5:
        align_color = "#ffaa44"
        align_status = "FAIR"
    else:
        align_color = "#ff4444"
        align_status = "POOR"

    total_count = pentagon_stats.get("total", {}).get("count", 0)

    html_parts.append(
        f'<div style="border-top: 2px solid #444; margin-top: 16px; padding-top: 20px; display: flex; align-items: center;">'
        f'<div style="flex: 1;">'
        f'<div style="color: #fff; font-weight: 700; font-size: 13px;">OVERALL ALIGNMENT SCORE</div>'
        f'<div style="color: #666; font-size: 11px;">Hill-climbing metric (1.0 = all metrics in target ranges)</div>'
        f'</div>'
        f'<div style="flex: 2; text-align: center;">'
        f'<span style="color: {align_color}; font-size: 48px; font-weight: 700;">{overall_alignment:.1%}</span>'
        f'<span style="color: {align_color}; font-size: 18px; margin-left: 12px; vertical-align: middle;">[{align_status}]</span>'
        f'</div>'
        f'<div style="flex: 1; color: #666; font-size: 11px; text-align: right;">{total_count} patterns analyzed</div>'
        f'</div>'
    )

    html_parts.append('</div>')  # Close main container
    return '\n'.join(html_parts)


def generate_heatmap_svg(stats: dict, title: str, width: int = 800) -> str:
    """Generate SVG heatmap showing per-step hit frequency."""
    num_steps = stats["num_steps"]
    step_width = (width - 100) / num_steps
    row_height = 28
    header_height = 24
    footer_height = 40
    total_height = header_height + 3 * row_height + footer_height + 8

    svg_parts = [
        f'<svg width="{width}" height="{total_height}" xmlns="http://www.w3.org/2000/svg" '
        f'style="font-family: -apple-system, BlinkMacSystemFont, sans-serif; font-size: 10px;">',
        f'<rect width="100%" height="100%" fill="#1e1e1e" rx="4"/>',
    ]

    # Title
    svg_parts.append(
        f'<text x="50" y="16" fill="#888888" font-size="11" font-weight="500">{title}</text>'
    )
    svg_parts.append(
        f'<text x="{width - 8}" y="16" text-anchor="end" fill="#505050" font-size="9">'
        f'{stats["num_patterns"]} patterns</text>'
    )

    # Voice rows
    voices = [
        ("V1", stats["v1"], COLORS["v1"]),
        ("V2", stats["v2"], COLORS["v2"]),
        ("AUX", stats["aux"], COLORS["aux"]),
    ]

    for row_idx, (voice_name, voice_stats, color) in enumerate(voices):
        y = header_height + row_idx * row_height

        # Voice label
        svg_parts.append(
            f'<text x="42" y="{y + row_height / 2 + 4}" text-anchor="end" '
            f'fill="{color}" font-weight="500">{voice_name}</text>'
        )

        # Heat cells
        for step, freq in enumerate(voice_stats["frequencies"]):
            x = 50 + step * step_width

            # Background for downbeats
            if step % 4 == 0:
                svg_parts.append(
                    f'<rect x="{x}" y="{y + 2}" width="{step_width}" height="{row_height - 4}" '
                    f'fill="#ffffff08"/>'
                )

            # Heat cell - opacity based on frequency
            if freq > 0:
                # Color intensity based on frequency
                opacity = 0.2 + freq * 0.8
                svg_parts.append(
                    f'<rect x="{x + 1}" y="{y + 3}" width="{step_width - 2}" height="{row_height - 6}" '
                    f'rx="2" fill="{color}" opacity="{opacity:.2f}"/>'
                )

                # Show percentage if significant
                if freq >= 0.1 and step_width >= 20:
                    svg_parts.append(
                        f'<text x="{x + step_width / 2}" y="{y + row_height / 2 + 3}" '
                        f'text-anchor="middle" fill="#000000" font-size="8" opacity="0.7">'
                        f'{int(freq * 100)}</text>'
                    )

        # Stats on the right
        mean_hits = voice_stats["mean_hits_per_pattern"]
        svg_parts.append(
            f'<text x="{width - 8}" y="{y + row_height / 2 + 4}" text-anchor="end" '
            f'fill="#666666" font-size="9">μ={mean_hits:.1f}</text>'
        )

    # Step numbers at bottom
    footer_y = header_height + 3 * row_height + 4
    for step in range(num_steps):
        if step % 4 == 0:
            x = 50 + step * step_width + step_width / 2
            svg_parts.append(
                f'<text x="{x}" y="{footer_y + 12}" text-anchor="middle" '
                f'fill="#505050" font-size="9">{step + 1}</text>'
            )

    # Legend
    svg_parts.append(
        f'<text x="50" y="{footer_y + 32}" fill="#505050" font-size="9">'
        f'Heat: hit frequency (0-100%) across patterns | μ = mean hits per pattern</text>'
    )

    svg_parts.append('</svg>')
    return '\n'.join(svg_parts)


def generate_stats_summary_svg(all_stats: list[tuple[str, dict]], width: int = 800) -> str:
    """Generate a summary table comparing statistics across seed sets."""
    row_height = 24
    header_height = 30
    col_widths = [180, 80, 80, 80, 80, 80, 80, 80]  # name, patterns, v1μ, v2μ, auxμ, total, density
    total_height = header_height + len(all_stats) * row_height + 16

    svg_parts = [
        f'<svg width="{width}" height="{total_height}" xmlns="http://www.w3.org/2000/svg" '
        f'style="font-family: -apple-system, BlinkMacSystemFont, sans-serif; font-size: 11px;">',
        f'<rect width="100%" height="100%" fill="#1e1e1e" rx="4"/>',
    ]

    # Header
    headers = ["Seed Set", "Patterns", "V1 μ", "V2 μ", "AUX μ", "Total μ", "Density"]
    x = 8
    for i, (header, col_w) in enumerate(zip(headers, col_widths)):
        svg_parts.append(
            f'<text x="{x + 4}" y="20" fill="#888888" font-weight="500">{header}</text>'
        )
        x += col_w

    # Divider
    svg_parts.append(
        f'<line x1="8" y1="{header_height}" x2="{width - 8}" y2="{header_height}" '
        f'stroke="#3d3d3d" stroke-width="1"/>'
    )

    # Data rows
    for row_idx, (name, stats) in enumerate(all_stats):
        y = header_height + row_idx * row_height + 18

        v1_mean = stats["v1"]["mean_hits_per_pattern"]
        v2_mean = stats["v2"]["mean_hits_per_pattern"]
        aux_mean = stats["aux"]["mean_hits_per_pattern"]
        total_mean = v1_mean + v2_mean + aux_mean
        density = total_mean / stats["num_steps"] if stats["num_steps"] > 0 else 0

        values = [
            (name[:22], "#cccccc"),
            (str(stats["num_patterns"]), "#888888"),
            (f"{v1_mean:.1f}", COLORS["v1"]),
            (f"{v2_mean:.1f}", COLORS["v2"]),
            (f"{aux_mean:.1f}", COLORS["aux"]),
            (f"{total_mean:.1f}", "#cccccc"),
            (f"{density:.0%}", "#888888"),
        ]

        x = 8
        for (val, color), col_w in zip(values, col_widths):
            svg_parts.append(
                f'<text x="{x + 4}" y="{y}" fill="{color}">{val}</text>'
            )
            x += col_w

    svg_parts.append('</svg>')
    return '\n'.join(svg_parts)


def generate_summary_text(
    patterns: list[tuple[str, list[Pattern], Optional[str]]],
    statistics: list[tuple[str, dict]],
    title: str = "Pattern Visualization Summary",
    pentagon_stats: Optional[dict] = None,
) -> str:
    """Generate AI and human-readable text summary of pattern statistics."""
    lines = []

    # Header
    lines.append("=" * 72)
    lines.append(title.upper())
    lines.append("=" * 72)
    lines.append("")
    lines.append(f"Generated by: DaisySP IDM Grids pattern visualization tool")
    lines.append(f"Total pattern sections: {len(patterns)}")
    total_patterns = sum(len(pats) for _, pats, *_ in patterns)
    lines.append(f"Total patterns generated: {total_patterns}")
    lines.append("")

    # Pentagon of Musicality Summary (with status indicators)
    if pentagon_stats:
        lines.append("-" * 72)
        lines.append("PENTAGON OF MUSICALITY — ZONE COMPLIANCE")
        lines.append("-" * 72)
        lines.append("")
        lines.append("Status: ✓ = in range (green), ~ = close (yellow), ✗ = out of range (red)")
        lines.append("")

        metric_keys = ["syncopation", "density", "velocity_range", "voice_separation", "regularity"]
        raw_attrs = ["raw_syncopation", "raw_density", "raw_velocity_range", "raw_voice_separation", "raw_regularity"]
        zones = [("stable", "STABLE"), ("syncopated", "SYNCOPATED"), ("wild", "WILD")]

        for zone_key, zone_label in zones:
            zone_data = pentagon_stats.get(zone_key)
            if not zone_data:
                continue

            lines.append(f"## {zone_label} ZONE (n={zone_data.get('count', 0)})")
            lines.append(f"   Composite Score: {zone_data.get('composite', 0):.0%}")
            lines.append("")

            for key, attr in zip(metric_keys, raw_attrs):
                meta = PENTAGON_METRICS[key]
                target = meta["target_by_zone"].get(zone_key, "0.0-1.0")
                val = zone_data.get(attr, 0)
                in_range, distance, _ = check_in_range(val, target)
                alignment = compute_alignment_score(val, target)

                # Status indicator
                if in_range:
                    status = "✓"  # Green - in range
                elif alignment > 0.5:
                    status = "~"  # Yellow - close
                else:
                    status = "✗"  # Red - out of range

                lines.append(f"   {status} {meta['name']:<18}: {val:.2f}  (target: {target})")

            lines.append("")

        # Overall alignment
        overall_alignment = pentagon_stats.get("overall_alignment", 0.0)
        if overall_alignment >= 0.7:
            align_status = "✓ GOOD"
        elif overall_alignment >= 0.5:
            align_status = "~ FAIR"
        else:
            align_status = "✗ POOR"

        lines.append(f"   OVERALL ALIGNMENT: {overall_alignment:.0%} [{align_status}]")
        lines.append("")

    # Overall Expressiveness Summary
    lines.append("-" * 72)
    lines.append("EXPRESSIVENESS SUMMARY")
    lines.append("-" * 72)
    lines.append("")

    for stat_name, stat_data in statistics:
        sv = stat_data["seed_variation"]
        ss = stat_data["step_stats"]

        v1_score = sv.get("v1_score", 0)
        v2_score = sv.get("v2_score", 0)
        aux_score = sv.get("aux_score", 0)
        overall = (v1_score + v2_score + aux_score) / 3
        status = "PASS" if v2_score >= 0.5 else "FAIL"

        lines.append(f"## {stat_name}")
        lines.append(f"   Patterns: {sv.get('total_patterns', 0)}")
        lines.append(f"   V1 (Anchor) variation:  {v1_score:6.1%} ({sv.get('unique_v1', 0)}/{sv.get('total_patterns', 0)} unique)")
        lines.append(f"   V2 (Shimmer) variation: {v2_score:6.1%} ({sv.get('unique_v2', 0)}/{sv.get('total_patterns', 0)} unique)")
        lines.append(f"   AUX variation:          {aux_score:6.1%} ({sv.get('unique_aux', 0)}/{sv.get('total_patterns', 0)} unique)")
        lines.append(f"   Overall score: {overall:.1%} [{status}]")
        lines.append("")
        lines.append(f"   Mean hits per pattern:")
        lines.append(f"     V1:  {ss['v1']['mean_hits_per_pattern']:.1f}")
        lines.append(f"     V2:  {ss['v2']['mean_hits_per_pattern']:.1f}")
        lines.append(f"     AUX: {ss['aux']['mean_hits_per_pattern']:.1f}")
        total_mean = ss['v1']['mean_hits_per_pattern'] + ss['v2']['mean_hits_per_pattern'] + ss['aux']['mean_hits_per_pattern']
        density = total_mean / ss['num_steps'] if ss['num_steps'] > 0 else 0
        lines.append(f"     Total: {total_mean:.1f} ({density:.0%} density)")
        lines.append("")

    # Per-Section Pattern Counts
    lines.append("-" * 72)
    lines.append("PATTERN SECTIONS")
    lines.append("-" * 72)
    lines.append("")
    lines.append(f"{'Section':<45} {'Patterns':>8}  {'Sweep Param':<12}")
    lines.append("-" * 72)

    for entry in patterns:
        section_name = entry[0]
        section_patterns = entry[1]
        highlight = entry[2] if len(entry) > 2 else None
        sweep_param = highlight.upper() if highlight else "-"
        lines.append(f"{section_name[:44]:<45} {len(section_patterns):>8}  {sweep_param:<12}")

    lines.append("-" * 72)
    lines.append(f"{'TOTAL':<45} {total_patterns:>8}")
    lines.append("")

    # Detailed per-step hit frequency (condensed)
    lines.append("-" * 72)
    lines.append("PER-STEP HIT FREQUENCY (across all patterns with default seed)")
    lines.append("-" * 72)
    lines.append("")

    # Find the "All Patterns" statistics
    all_patterns_stats = None
    for stat_name, stat_data in statistics:
        if "All Patterns" in stat_name:
            all_patterns_stats = stat_data
            break

    if all_patterns_stats:
        ss = all_patterns_stats["step_stats"]
        lines.append("Step  |  V1 freq  |  V2 freq  |  AUX freq")
        lines.append("------|-----------|-----------|----------")

        for step in range(ss["num_steps"]):
            v1_freq = ss["v1"]["frequencies"][step]
            v2_freq = ss["v2"]["frequencies"][step]
            aux_freq = ss["aux"]["frequencies"][step]
            # Only show steps with meaningful frequency
            if v1_freq > 0.05 or v2_freq > 0.05 or aux_freq > 0.05:
                marker = "*" if step % 4 == 0 else " "
                lines.append(f"{step:4}{marker} |   {v1_freq:5.0%}   |   {v2_freq:5.0%}   |   {aux_freq:5.0%}")

        lines.append("")
        lines.append("(* = downbeat, only steps with >5% frequency shown)")
        lines.append("")

    # Named Presets Summary
    lines.append("-" * 72)
    lines.append("NAMED PRESETS")
    lines.append("-" * 72)
    lines.append("")

    for entry in patterns:
        section_name = entry[0]
        if "Preset" not in section_name:
            continue

        section_patterns = entry[1]
        for p in section_patterns:
            if p.name:
                lines.append(f"## {p.name}")
                if p.description:
                    lines.append(f"   {p.description}")
                lines.append(f"   SHAPE={p.shape:.2f} ENERGY={p.energy:.2f} AXIS_X={p.axis_x:.2f} AXIS_Y={p.axis_y:.2f}")
                lines.append(f"   DRIFT={p.drift:.2f} ACCENT={p.accent:.2f}")
                lines.append(f"   Hits: V1={p.v1_hits}, V2={p.v2_hits}, AUX={p.aux_hits}")
                lines.append("")

    # Footer
    lines.append("-" * 72)
    lines.append("END OF SUMMARY")
    lines.append("-" * 72)
    lines.append("")

    return "\n".join(lines)


def generate_html(
    patterns: list[tuple[str, list[Pattern], Optional[str]]],
    title: str = "Pattern Visualization",
    statistics: Optional[list[tuple[str, dict]]] = None,
    pentagon_stats: Optional[dict] = None,
) -> str:
    """Generate complete HTML page with multiple pattern groups and statistics."""
    html_parts = [f'''<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{title}</title>
    <style>
        * {{
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }}
        body {{
            background: {COLORS["bg"]};
            color: {COLORS["text"]};
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            padding: 24px;
            line-height: 1.5;
        }}
        h1 {{
            font-size: 24px;
            font-weight: 600;
            margin-bottom: 8px;
            color: #ffffff;
        }}
        .subtitle {{
            color: {COLORS["text_dim"]};
            margin-bottom: 32px;
        }}
        .sweep-section {{
            margin-bottom: 48px;
        }}
        .sweep-title {{
            font-size: 18px;
            font-weight: 500;
            margin-bottom: 16px;
            padding-bottom: 8px;
            border-bottom: 1px solid {COLORS["grid_line"]};
        }}
        .pattern-grid {{
            display: flex;
            flex-direction: column;
            gap: 24px;
        }}
        .pattern-row {{
            display: flex;
            flex-direction: column;
            gap: 8px;
        }}
        .pattern-label {{
            display: flex;
            align-items: center;
            gap: 16px;
        }}
        .param-value {{
            font-size: 14px;
            font-weight: 500;
            min-width: 120px;
        }}
        .param-name {{
            color: {COLORS["text_dim"]};
            font-size: 12px;
        }}
        .stats {{
            display: flex;
            gap: 16px;
            font-size: 11px;
            color: {COLORS["text_dim"]};
        }}
        .stat {{
            display: flex;
            align-items: center;
            gap: 4px;
        }}
        .stat-dot {{
            width: 8px;
            height: 8px;
            border-radius: 50%;
        }}
        .pattern-svg {{
            border-radius: 4px;
            overflow: hidden;
        }}
        .toc {{
            background: {COLORS["grid_bg"]};
            padding: 16px;
            border-radius: 8px;
            margin-bottom: 32px;
        }}
        .toc h2 {{
            font-size: 14px;
            margin-bottom: 12px;
            color: {COLORS["text_dim"]};
        }}
        .toc-list {{
            display: flex;
            flex-wrap: wrap;
            gap: 8px;
        }}
        .toc-item {{
            background: {COLORS["bg"]};
            padding: 6px 12px;
            border-radius: 4px;
            color: {COLORS["text"]};
            text-decoration: none;
            font-size: 13px;
        }}
        .toc-item:hover {{
            background: #3d3d3d;
        }}
        .legend {{
            background: {COLORS["grid_bg"]};
            padding: 16px;
            border-radius: 8px;
            margin-bottom: 32px;
            display: flex;
            gap: 24px;
            align-items: center;
        }}
        .legend-item {{
            display: flex;
            align-items: center;
            gap: 8px;
        }}
        .legend-box {{
            width: 16px;
            height: 16px;
            border-radius: 3px;
        }}
        .knob-panel {{
            margin-bottom: 8px;
        }}
        .pattern-container {{
            display: flex;
            flex-direction: column;
            gap: 4px;
        }}
        .preset-header {{
            margin-bottom: 4px;
        }}
        .preset-name {{
            font-size: 14px;
            font-weight: 600;
            color: #ffffff;
        }}
        .preset-description {{
            font-size: 12px;
            color: {COLORS["text_dim"]};
            margin-left: 12px;
        }}
        .stats-section {{
            background: {COLORS["grid_bg"]};
            padding: 20px;
            border-radius: 8px;
            margin-bottom: 32px;
        }}
        .stats-title {{
            font-size: 18px;
            font-weight: 500;
            margin-bottom: 16px;
            padding-bottom: 8px;
            border-bottom: 1px solid {COLORS["grid_line"]};
        }}
        .stats-grid {{
            display: flex;
            flex-direction: column;
            gap: 16px;
        }}
        .stats-row {{
            display: flex;
            flex-direction: column;
            gap: 8px;
        }}
        .stats-label {{
            font-size: 13px;
            color: {COLORS["text"]};
            font-weight: 500;
        }}
    </style>
</head>
<body>
    <h1>{title}</h1>
    <p class="subtitle">Generated from DaisySP IDM Grids firmware pattern algorithms</p>

    <div class="legend">
        <div class="legend-item">
            <div class="legend-box" style="background: {COLORS['v1']}"></div>
            <span>V1 (Anchor/Kick)</span>
        </div>
        <div class="legend-item">
            <div class="legend-box" style="background: {COLORS['v2']}"></div>
            <span>V2 (Shimmer/Snare)</span>
        </div>
        <div class="legend-item">
            <div class="legend-box" style="background: {COLORS['aux']}"></div>
            <span>AUX (Hat/Perc)</span>
        </div>
    </div>
''']

    # Statistics section at TOP (if provided)
    if statistics or pentagon_stats:
        html_parts.append('''
    <section class="stats-section" id="statistics">
        <h2 class="stats-title">📊 Pattern Statistics & Pentagon of Musicality</h2>
        <div class="stats-grid">
''')

        # Pentagon summary first (using HTML table layout)
        if pentagon_stats:
            pentagon_summary = generate_pentagon_summary_html(pentagon_stats)
            html_parts.append(f'''            <div class="stats-row">
                {pentagon_summary}
            </div>
''')

        # Existing statistics
        if statistics:
            for stat_name, stat_data in statistics:
                # Generate heatmap
                heatmap = generate_heatmap_svg(stat_data["step_stats"], stat_name)
                # Generate expressiveness metrics
                expr_svg = generate_expressiveness_svg(stat_data["seed_variation"], f"Seed Variation: {stat_name}")

                html_parts.append(f'''            <div class="stats-row">
                <div class="stats-label">{stat_name}</div>
                {heatmap}
                {expr_svg}
            </div>
''')

            # Summary table
            summary_stats = [(name, data["step_stats"]) for name, data in statistics]
            summary_svg = generate_stats_summary_svg(summary_stats)
            html_parts.append(f'''            <div class="stats-row">
                <div class="stats-label">Summary Comparison</div>
                {summary_svg}
            </div>
''')

        html_parts.append('''        </div>
    </section>
''')

    html_parts.append('''
    <div class="toc">
        <h2>Jump to Section</h2>
        <div class="toc-list">
''')

    # Table of contents
    for entry in patterns:
        section_name = entry[0]
        section_id = section_name.lower().replace(" ", "-")
        html_parts.append(f'            <a href="#{section_id}" class="toc-item">{section_name}</a>\n')

    # Add statistics link if provided
    if statistics:
        html_parts.append('            <a href="#statistics" class="toc-item" style="background: #2a3a2a; color: #88cc88;">📊 Statistics & Metrics</a>\n')

    html_parts.append('''        </div>
    </div>
''')

    # Pattern sections
    for entry in patterns:
        section_name = entry[0]
        section_patterns = entry[1]
        highlight_param = entry[2] if len(entry) > 2 else None

        section_id = section_name.lower().replace(" ", "-")
        html_parts.append(f'''
    <section class="sweep-section" id="{section_id}">
        <h2 class="sweep-title">{section_name}</h2>
        <div class="pattern-grid">
''')

        for pattern in section_patterns:
            svg = generate_pattern_svg(pattern)
            knob_panel = generate_knob_panel_svg(
                shape=pattern.shape,
                energy=pattern.energy,
                axis_x=pattern.axis_x,
                axis_y=pattern.axis_y,
                drift=pattern.drift,
                accent=pattern.accent,
                seed=pattern.seed,
                highlight_param=highlight_param,
            )

            # Compute Pentagon metrics and generate radar chart
            pentagon_metrics = compute_pentagon_metrics(pattern)
            pentagon_svg = generate_pentagon_svg(pentagon_metrics, size=140)

            # Preset header (name and description if available)
            preset_header = ""
            if pattern.name:
                desc_html = f'<span class="preset-description">— {pattern.description}</span>' if pattern.description else ""
                preset_header = f'''
                    <div class="preset-header">
                        <span class="preset-name">{pattern.name}</span>{desc_html}
                    </div>'''

            html_parts.append(f'''            <div class="pattern-row">
                <div class="pattern-container">{preset_header}
                    <div class="controls-row" style="display: flex; gap: 12px; align-items: flex-start;">
                        <div class="knob-panel">
                            {knob_panel}
                        </div>
                        <div class="pentagon-chart" title="Pentagon of Musicality: Syncopation, Density, Velocity Range, Voice Separation, Regularity">
                            {pentagon_svg}
                        </div>
                    </div>
                    <div class="pattern-label">
                        <div class="stats">
                            <span class="stat">
                                <span class="stat-dot" style="background: {COLORS['v1']}"></span>
                                V1: {pattern.v1_hits}
                            </span>
                            <span class="stat">
                                <span class="stat-dot" style="background: {COLORS['v2']}"></span>
                                V2: {pattern.v2_hits}
                            </span>
                            <span class="stat">
                                <span class="stat-dot" style="background: {COLORS['aux']}"></span>
                                AUX: {pattern.aux_hits}
                            </span>
                        </div>
                    </div>
                    <div class="pattern-svg">
                        {svg}
                    </div>
                </div>
            </div>
''')

        html_parts.append('''        </div>
    </section>
''')

    html_parts.append('''</body>
</html>
''')

    return ''.join(html_parts)


def main():
    parser = argparse.ArgumentParser(description="Generate HTML pattern visualization")
    parser.add_argument(
        "--output", "-o",
        type=Path,
        default=Path("docs/patterns.html"),
        help="Output HTML file path (default: docs/patterns.html)"
    )
    parser.add_argument(
        "--summary",
        type=Path,
        default=None,
        help="Output text summary file path (default: <output>.summary.txt)"
    )
    parser.add_argument(
        "--pattern-viz",
        type=Path,
        default=Path("build/pattern_viz"),
        help="Path to pattern_viz binary (default: build/pattern_viz)"
    )
    parser.add_argument(
        "--seed",
        type=lambda x: int(x, 0),
        default=0xDEADBEEF,
        help="Pattern seed (default: 0xDEADBEEF)"
    )
    args = parser.parse_args()

    # Default summary path based on output path
    if args.summary is None:
        args.summary = args.output.with_suffix(".summary.txt")

    # Check if pattern_viz exists
    if not args.pattern_viz.exists():
        print(f"Error: pattern_viz not found at {args.pattern_viz}")
        print("Run 'make pattern-viz' first to build the tool.")
        return 1

    print(f"Generating patterns using {args.pattern_viz}...")

    all_patterns = []

    # SHAPE sweep (primary interest)
    print("  - SHAPE sweep...")
    shape_patterns = []
    for shape in [0.0, 0.15, 0.30, 0.50, 0.70, 0.85, 1.0]:
        pattern = run_pattern_viz(args.pattern_viz, shape=shape, energy=0.6, seed=args.seed)
        shape_patterns.append(pattern)
    all_patterns.append(("SHAPE Sweep (stable to wild)", shape_patterns, "shape"))

    # ENERGY sweep
    print("  - ENERGY sweep...")
    energy_patterns = []
    for energy in [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]:
        pattern = run_pattern_viz(args.pattern_viz, energy=energy, shape=0.3, seed=args.seed)
        energy_patterns.append(pattern)
    all_patterns.append(("ENERGY Sweep (sparse to dense)", energy_patterns, "energy"))

    # AXIS X sweep
    print("  - AXIS X sweep...")
    axis_x_patterns = []
    for axis_x in [0.0, 0.25, 0.5, 0.75, 1.0]:
        pattern = run_pattern_viz(args.pattern_viz, axis_x=axis_x, shape=0.3, energy=0.6, seed=args.seed)
        axis_x_patterns.append(pattern)
    all_patterns.append(("AXIS X Sweep (downbeat to offbeat bias)", axis_x_patterns, "axis_x"))

    # AXIS Y sweep
    print("  - AXIS Y sweep...")
    axis_y_patterns = []
    for axis_y in [0.0, 0.25, 0.5, 0.75, 1.0]:
        pattern = run_pattern_viz(args.pattern_viz, axis_y=axis_y, shape=0.3, energy=0.6, seed=args.seed)
        axis_y_patterns.append(pattern)
    all_patterns.append(("AXIS Y Sweep (bar start to bar end bias)", axis_y_patterns, "axis_y"))

    # DRIFT sweep (voice independence)
    print("  - DRIFT sweep...")
    drift_patterns = []
    for drift in [0.0, 0.25, 0.5, 0.75, 1.0]:
        pattern = run_pattern_viz(args.pattern_viz, drift=drift, shape=0.4, energy=0.6, seed=args.seed)
        drift_patterns.append(pattern)
    all_patterns.append(("DRIFT Sweep (locked to independent)", drift_patterns, "drift"))

    # ACCENT sweep (velocity dynamics)
    print("  - ACCENT sweep...")
    accent_patterns = []
    for accent in [0.0, 0.25, 0.5, 0.75, 1.0]:
        pattern = run_pattern_viz(args.pattern_viz, accent=accent, shape=0.3, energy=0.6, seed=args.seed)
        accent_patterns.append(pattern)
    all_patterns.append(("ACCENT Sweep (flat to punchy)", accent_patterns, "accent"))

    # Interesting combinations: SHAPE x ENERGY matrix (3x3)
    print("  - SHAPE x ENERGY matrix...")
    matrix_patterns = []
    for energy in [0.3, 0.6, 0.9]:
        for shape in [0.0, 0.5, 1.0]:
            pattern = run_pattern_viz(args.pattern_viz, shape=shape, energy=energy, seed=args.seed)
            matrix_patterns.append(pattern)
    all_patterns.append(("SHAPE x ENERGY Matrix", matrix_patterns, None))

    # Different seeds (showing variation) - with more seeds
    print("  - Seed variation...")
    seed_patterns = []
    for seed in [0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCD1234, 0xBEEFCAFE, 0x87654321]:
        pattern = run_pattern_viz(args.pattern_viz, shape=0.5, energy=0.6, seed=seed)
        seed_patterns.append(pattern)
    all_patterns.append(("Seed Variation (same params, different patterns)", seed_patterns, None))

    # Wild variations (high SHAPE with different seeds)
    print("  - Wild variations...")
    wild_patterns = []
    for seed in [0x11111111, 0x22222222, 0x33333333, 0x44444444]:
        pattern = run_pattern_viz(args.pattern_viz, shape=0.9, energy=0.7, seed=seed)
        wild_patterns.append(pattern)
    all_patterns.append(("Wild Patterns (SHAPE=0.9)", wild_patterns, "shape"))

    # Minimal/sparse patterns
    print("  - Minimal patterns...")
    minimal_patterns = []
    for energy in [0.1, 0.2, 0.3]:
        for seed in [0xAAAAAAAA, 0xBBBBBBBB]:
            pattern = run_pattern_viz(args.pattern_viz, shape=0.2, energy=energy, seed=seed)
            minimal_patterns.append(pattern)
    all_patterns.append(("Minimal Patterns (low energy)", minimal_patterns, "energy"))

    # 2D Sweep: SHAPE x DRIFT
    print("  - SHAPE x DRIFT matrix...")
    shape_drift_patterns = []
    for shape in [0.0, 0.3, 0.6, 1.0]:
        for drift in [0.0, 0.5, 1.0]:
            pattern = run_pattern_viz(args.pattern_viz, shape=shape, drift=drift, energy=0.6, seed=args.seed)
            shape_drift_patterns.append(pattern)
    all_patterns.append(("SHAPE x DRIFT Matrix", shape_drift_patterns, None))

    # 2D Sweep: SHAPE x AXIS X
    print("  - SHAPE x AXIS X matrix...")
    shape_axisx_patterns = []
    for shape in [0.0, 0.3, 0.6, 1.0]:
        for axis_x in [0.0, 0.5, 1.0]:
            pattern = run_pattern_viz(args.pattern_viz, shape=shape, axis_x=axis_x, energy=0.6, seed=args.seed)
            shape_axisx_patterns.append(pattern)
    all_patterns.append(("SHAPE x AXIS X Matrix", shape_axisx_patterns, None))

    # 2D Sweep: SHAPE x AXIS Y
    print("  - SHAPE x AXIS Y matrix...")
    shape_axisy_patterns = []
    for shape in [0.0, 0.3, 0.6, 1.0]:
        for axis_y in [0.0, 0.5, 1.0]:
            pattern = run_pattern_viz(args.pattern_viz, shape=shape, axis_y=axis_y, energy=0.6, seed=args.seed)
            shape_axisy_patterns.append(pattern)
    all_patterns.append(("SHAPE x AXIS Y Matrix", shape_axisy_patterns, None))

    # 2D Sweep: SHAPE x ACCENT
    print("  - SHAPE x ACCENT matrix...")
    shape_accent_patterns = []
    for shape in [0.0, 0.3, 0.6, 1.0]:
        for accent in [0.0, 0.5, 1.0]:
            pattern = run_pattern_viz(args.pattern_viz, shape=shape, accent=accent, energy=0.6, seed=args.seed)
            shape_accent_patterns.append(pattern)
    all_patterns.append(("SHAPE x ACCENT Matrix", shape_accent_patterns, None))

    # Named Preset Patterns with musical descriptions
    print("  - Named presets...")

    # Define musical presets: (name, description, params)
    presets = [
        # Classic Four-on-Floor Techno
        ("Four on Floor", "Classic techno kick pattern, minimal complexity", {
            "shape": 0.0, "energy": 0.5, "axis_x": 0.0, "axis_y": 0.5, "drift": 0.0, "accent": 0.6
        }),
        # House with Offbeat Hats
        ("House Groove", "Driving house with offbeat emphasis", {
            "shape": 0.15, "energy": 0.6, "axis_x": 0.6, "axis_y": 0.5, "drift": 0.3, "accent": 0.5
        }),
        # Tribal / African-influenced
        ("Tribal Syncopation", "Syncopated patterns, call-and-response feel", {
            "shape": 0.4, "energy": 0.65, "axis_x": 0.4, "axis_y": 0.6, "drift": 0.5, "accent": 0.7
        }),
        # Breakbeat
        ("Breakbeat", "Broken rhythms, snare emphasis", {
            "shape": 0.35, "energy": 0.55, "axis_x": 0.5, "axis_y": 0.3, "drift": 0.6, "accent": 0.65
        }),
        # Industrial
        ("Industrial Stomp", "Heavy, aggressive, distorted", {
            "shape": 0.2, "energy": 0.8, "axis_x": 0.2, "axis_y": 0.4, "drift": 0.2, "accent": 0.9
        }),
        # Minimal Techno
        ("Minimal Pulse", "Sparse, hypnotic, space between notes", {
            "shape": 0.1, "energy": 0.3, "axis_x": 0.3, "axis_y": 0.5, "drift": 0.1, "accent": 0.4
        }),
        # Dub Techno
        ("Dub Techno", "Laid back, dubby, chord hits", {
            "shape": 0.25, "energy": 0.45, "axis_x": 0.4, "axis_y": 0.6, "drift": 0.4, "accent": 0.5
        }),
        # Gabber / Hardcore
        ("Gabber Assault", "Fast, aggressive, constant pressure", {
            "shape": 0.3, "energy": 0.95, "axis_x": 0.3, "axis_y": 0.5, "drift": 0.1, "accent": 0.95
        }),
        # IDM Complex
        ("IDM Complexity", "Irregular patterns, algorithmic feel", {
            "shape": 0.7, "energy": 0.6, "axis_x": 0.6, "axis_y": 0.4, "drift": 0.7, "accent": 0.6
        }),
        # IDM Chaos (Aphex-style)
        ("IDM Chaos", "Maximum unpredictability, glitchy", {
            "shape": 1.0, "energy": 0.7, "axis_x": 0.5, "axis_y": 0.5, "drift": 0.9, "accent": 0.7
        }),
        # Ambient Percussion
        ("Ambient Scatter", "Sparse, random, textural", {
            "shape": 0.8, "energy": 0.25, "axis_x": 0.7, "axis_y": 0.7, "drift": 0.8, "accent": 0.3
        }),
        # Half-time
        ("Half-Time Feel", "Emphasis on beats 1 and 3", {
            "shape": 0.15, "energy": 0.4, "axis_x": 0.0, "axis_y": 0.3, "drift": 0.2, "accent": 0.6
        }),
        # Polyrhythmic
        ("Polyrhythmic", "Multiple simultaneous rhythmic layers", {
            "shape": 0.5, "energy": 0.6, "axis_x": 0.5, "axis_y": 0.5, "drift": 0.6, "accent": 0.55
        }),
        # Acid Techno
        ("Acid Drive", "Relentless, 303-era inspired", {
            "shape": 0.25, "energy": 0.7, "axis_x": 0.35, "axis_y": 0.5, "drift": 0.3, "accent": 0.75
        }),
        # Detroit Techno
        ("Detroit Soul", "Funky, machine-groove, swing", {
            "shape": 0.2, "energy": 0.55, "axis_x": 0.45, "axis_y": 0.55, "drift": 0.35, "accent": 0.6
        }),
        # Berlin Minimal
        ("Berlin Loop", "Hypnotic, repetitive, subtle variation", {
            "shape": 0.1, "energy": 0.5, "axis_x": 0.5, "axis_y": 0.5, "drift": 0.15, "accent": 0.5
        }),
    ]

    preset_patterns = []
    for preset_name, desc, params in presets:
        pattern = run_pattern_viz(
            args.pattern_viz,
            shape=params["shape"],
            energy=params["energy"],
            axis_x=params["axis_x"],
            axis_y=params["axis_y"],
            drift=params["drift"],
            accent=params["accent"],
            seed=args.seed,
            name=preset_name,
            description=desc,
        )
        preset_patterns.append(pattern)
    all_patterns.insert(0, ("Named Presets (Musical Styles)", preset_patterns, None))

    # Compute statistics for each pattern group
    print("Computing statistics...")
    statistics = []

    # Group patterns by seed type for statistics
    seed_groups = [
        ("All Patterns (Default Seed)", [p for name, pats, _ in all_patterns if "Seed" not in name for p in pats]),
        ("Seed Variation Set", seed_patterns),
        ("Wild Patterns (High SHAPE)", wild_patterns),
        ("Named Presets", preset_patterns),
    ]

    for group_name, group_patterns in seed_groups:
        if not group_patterns:
            continue

        # Compute step-level statistics
        step_stats = compute_step_statistics(group_patterns)

        # Compute seed variation metrics
        seed_variation = compute_seed_variation(group_patterns)

        statistics.append((group_name, {
            "step_stats": step_stats,
            "seed_variation": seed_variation,
        }))

        print(f"  - {group_name}: {len(group_patterns)} patterns, "
              f"V1={seed_variation['v1_score']:.0%} V2={seed_variation['v2_score']:.0%}")

    # Compute Pentagon of Musicality statistics across all patterns
    print("Computing Pentagon metrics...")
    all_flat_patterns = [p for _, pats, _ in all_patterns for p in pats]
    pentagon_stats = compute_pentagon_statistics(all_flat_patterns)

    # Generate summary to calculate alignment score (stored in pentagon_stats)
    _ = generate_pentagon_summary_html(pentagon_stats)
    alignment = pentagon_stats.get("overall_alignment", 0.0)

    print(f"  - Pentagon: {pentagon_stats['total']['count']} patterns, "
          f"Composite={pentagon_stats['total']['composite']:.0%}")
    print(f"  - ALIGNMENT SCORE: {alignment:.1%} (hill-climbing metric)")

    # Generate HTML
    print("Generating HTML...")
    html = generate_html(all_patterns, "DaisySP IDM Grids - Pattern Visualization", statistics, pentagon_stats)

    # Generate text summary
    print("Generating text summary...")
    summary = generate_summary_text(all_patterns, statistics, "DaisySP IDM Grids - Pattern Statistics", pentagon_stats)

    # Ensure output directory exists
    args.output.parent.mkdir(parents=True, exist_ok=True)

    # Write HTML output
    args.output.write_text(html)
    print(f"HTML output written to: {args.output}")

    # Write text summary
    args.summary.write_text(summary)
    print(f"Text summary written to: {args.summary}")

    return 0


if __name__ == "__main__":
    exit(main())
