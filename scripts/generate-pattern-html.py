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


def generate_html(patterns: list[tuple[str, list[Pattern]]], title: str = "Pattern Visualization") -> str:
    """Generate complete HTML page with multiple pattern groups."""
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
    for section_name, _ in patterns:
        section_id = section_name.lower().replace(" ", "-")
        html_parts.append(f'            <a href="#{section_id}" class="toc-item">{section_name}</a>\n')

    html_parts.append('''        </div>
    </div>
''')

    # Pattern sections
    for section_name, section_patterns in patterns:
        section_id = section_name.lower().replace(" ", "-")
        html_parts.append(f'''
    <section class="sweep-section" id="{section_id}">
        <h2 class="sweep-title">{section_name}</h2>
        <div class="pattern-grid">
''')

        for pattern in section_patterns:
            svg = generate_pattern_svg(pattern)
            html_parts.append(f'''            <div class="pattern-row">
                <div class="pattern-label">
                    <span class="param-value">
                        SHAPE={pattern.shape:.2f} ENERGY={pattern.energy:.2f}
                    </span>
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
    all_patterns.append(("SHAPE Sweep (stable to wild)", shape_patterns))

    # ENERGY sweep
    print("  - ENERGY sweep...")
    energy_patterns = []
    for energy in [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]:
        pattern = run_pattern_viz(args.pattern_viz, energy=energy, shape=0.3, seed=args.seed)
        energy_patterns.append(pattern)
    all_patterns.append(("ENERGY Sweep (sparse to dense)", energy_patterns))

    # AXIS X sweep
    print("  - AXIS X sweep...")
    axis_x_patterns = []
    for axis_x in [0.0, 0.25, 0.5, 0.75, 1.0]:
        pattern = run_pattern_viz(args.pattern_viz, axis_x=axis_x, shape=0.3, energy=0.6, seed=args.seed)
        axis_x_patterns.append(pattern)
    all_patterns.append(("AXIS X Sweep (downbeat to offbeat bias)", axis_x_patterns))

    # AXIS Y sweep
    print("  - AXIS Y sweep...")
    axis_y_patterns = []
    for axis_y in [0.0, 0.25, 0.5, 0.75, 1.0]:
        pattern = run_pattern_viz(args.pattern_viz, axis_y=axis_y, shape=0.3, energy=0.6, seed=args.seed)
        axis_y_patterns.append(pattern)
    all_patterns.append(("AXIS Y Sweep (bar start to bar end bias)", axis_y_patterns))

    # Interesting combinations: SHAPE x ENERGY matrix (3x3)
    print("  - SHAPE x ENERGY matrix...")
    matrix_patterns = []
    for energy in [0.3, 0.6, 0.9]:
        for shape in [0.0, 0.5, 1.0]:
            pattern = run_pattern_viz(args.pattern_viz, shape=shape, energy=energy, seed=args.seed)
            matrix_patterns.append(pattern)
    all_patterns.append(("SHAPE x ENERGY Matrix", matrix_patterns))

    # Different seeds (showing variation)
    print("  - Seed variation...")
    seed_patterns = []
    for seed in [0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCD1234]:
        pattern = run_pattern_viz(args.pattern_viz, shape=0.5, energy=0.6, seed=seed)
        seed_patterns.append(pattern)
    all_patterns.append(("Seed Variation (same params, different patterns)", seed_patterns))

    # Generate HTML
    print("Generating HTML...")
    html = generate_html(all_patterns, "DaisySP IDM Grids - Pattern Visualization")

    # Ensure output directory exists
    args.output.parent.mkdir(parents=True, exist_ok=True)

    # Write output
    args.output.write_text(html)
    print(f"Output written to: {args.output}")

    return 0


if __name__ == "__main__":
    exit(main())
