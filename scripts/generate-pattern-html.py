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

    <div class="toc">
        <h2>Jump to Section</h2>
        <div class="toc-list">
''']

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
                    <div class="knob-panel">
                        {knob_panel}
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

    # Statistics section (if provided)
    if statistics:
        html_parts.append('''
    <section class="stats-section" id="statistics">
        <h2 class="stats-title">Pattern Statistics & Expressiveness Metrics</h2>
        <div class="stats-grid">
''')

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

    # Generate HTML
    print("Generating HTML...")
    html = generate_html(all_patterns, "DaisySP IDM Grids - Pattern Visualization", statistics)

    # Generate text summary
    print("Generating text summary...")
    summary = generate_summary_text(all_patterns, statistics, "DaisySP IDM Grids - Pattern Statistics")

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
