"""Command-line interface for pattern visualization."""

import click
import sys

from .types import Genre, VoiceCoupling
from .pipeline import generate_pattern, generate_phrase
from .output import render_ascii, render_compact, to_json, save_wav


def parse_genre(value: str) -> Genre:
    """Parse genre string to enum."""
    value = value.lower()
    if value in ("techno", "t"):
        return Genre.TECHNO
    elif value in ("tribal", "tr"):
        return Genre.TRIBAL
    elif value in ("idm", "i"):
        return Genre.IDM
    else:
        raise click.BadParameter(f"Unknown genre: {value}")


def parse_coupling(value: str) -> VoiceCoupling:
    """Parse coupling string to enum."""
    value = value.lower()
    if value in ("independent", "ind", "i"):
        return VoiceCoupling.INDEPENDENT
    elif value in ("interlock", "int", "l"):
        return VoiceCoupling.INTERLOCK
    elif value in ("shadow", "sh", "s"):
        return VoiceCoupling.SHADOW
    else:
        raise click.BadParameter(f"Unknown coupling: {value}")


@click.group()
@click.version_option(version="0.1.0")
def main():
    """DuoPulse v4 Pattern Visualization and Debug Tool.

    Generate and visualize drum patterns using the same algorithms as the firmware.
    """
    pass


@main.command()
@click.option("--energy", "-e", type=float, default=0.5, help="Energy level (0.0-1.0)")
@click.option("--build", "-b", type=float, default=0.5, help="BUILD parameter (0.0-1.0)")
@click.option("--field-x", "-x", type=float, default=0.5, help="Field X position (0.0-1.0)")
@click.option("--field-y", "-y", type=float, default=0.5, help="Field Y position (0.0-1.0)")
@click.option("--punch", "-p", type=float, default=0.5, help="PUNCH parameter (0.0-1.0)")
@click.option("--genre", "-g", type=str, default="techno", help="Genre (techno/tribal/idm)")
@click.option("--balance", type=float, default=0.5, help="Voice balance (0.0-1.0)")
@click.option("--length", "-l", type=int, default=32, help="Pattern length (16/24/32)")
@click.option("--phrase", type=float, default=0.0, help="Phrase progress (0.0-1.0)")
@click.option("--swing", "-s", type=float, default=0.5, help="Swing amount (0.0-1.0)")
@click.option("--coupling", "-c", type=str, default="independent",
              help="Voice coupling (independent/interlock/shadow)")
@click.option("--seed", type=int, default=0x12345678, help="Random seed")
@click.option("--format", "-f", "output_format", type=click.Choice(["ascii", "json", "compact"]),
              default="ascii", help="Output format")
@click.option("--output", "-o", type=str, default=None, help="Output file (WAV for audio)")
@click.option("--bpm", type=float, default=120.0, help="BPM for audio output")
def generate(
    energy, build, field_x, field_y, punch, genre, balance, length,
    phrase, swing, coupling, seed, output_format, output, bpm
):
    """Generate a single drum pattern.

    Example:
        pattern-viz generate --energy 0.5 --genre techno --field-x 0.5 --field-y 0.5
    """
    try:
        genre_enum = parse_genre(genre)
        coupling_enum = parse_coupling(coupling)
    except click.BadParameter as e:
        click.echo(f"Error: {e}", err=True)
        sys.exit(1)

    pattern = generate_pattern(
        energy=energy,
        build=build,
        field_x=field_x,
        field_y=field_y,
        punch=punch,
        genre=genre_enum,
        balance=balance,
        pattern_length=length,
        phrase_progress=phrase,
        swing=swing,
        voice_coupling=coupling_enum,
        seed=seed,
    )

    # Output
    if output and output.endswith(".wav"):
        save_wav(pattern, output, bpm)
        click.echo(f"Saved audio to {output}")
    elif output:
        # Write to file
        if output_format == "json":
            content = to_json(pattern)
        elif output_format == "compact":
            content = render_compact(pattern)
        else:
            content = render_ascii(pattern)

        with open(output, "w") as f:
            f.write(content)
        click.echo(f"Saved to {output}")
    else:
        # Print to stdout
        if output_format == "json":
            click.echo(to_json(pattern))
        elif output_format == "compact":
            click.echo(render_compact(pattern))
        else:
            title = f"Pattern: {genre.upper()} [{field_x:.1f},{field_y:.1f}] | Energy={energy:.0%} | Build={build:.0%}"
            click.echo(render_ascii(pattern, title=title))


@main.command()
@click.option("--param", "-p", type=str, required=True,
              help="Parameter to sweep (energy/build/field_x/field_y/punch)")
@click.option("--range", "sweep_range", type=str, default="0.0:1.0:0.1",
              help="Sweep range (start:end:step)")
@click.option("--genre", "-g", type=str, default="techno", help="Genre")
@click.option("--seed", type=int, default=0x12345678, help="Random seed")
@click.option("--format", "-f", "output_format", type=click.Choice(["compact", "full"]),
              default="compact", help="Output format")
def sweep(param, sweep_range, genre, seed, output_format):
    """Sweep a parameter and show patterns.

    Example:
        pattern-viz sweep --param energy --range 0.0:1.0:0.2
    """
    try:
        genre_enum = parse_genre(genre)
    except click.BadParameter as e:
        click.echo(f"Error: {e}", err=True)
        sys.exit(1)

    # Parse range
    parts = sweep_range.split(":")
    if len(parts) != 3:
        click.echo("Error: Range must be start:end:step", err=True)
        sys.exit(1)

    try:
        start, end, step = float(parts[0]), float(parts[1]), float(parts[2])
    except ValueError:
        click.echo("Error: Range values must be numbers", err=True)
        sys.exit(1)

    # Generate patterns
    click.echo(f"Sweeping {param} from {start} to {end} by {step}")
    click.echo("=" * 60)

    value = start
    while value <= end + 0.001:
        kwargs = {
            "energy": 0.5,
            "build": 0.5,
            "field_x": 0.5,
            "field_y": 0.5,
            "punch": 0.5,
            "genre": genre_enum,
            "seed": seed,
        }
        kwargs[param] = value

        pattern = generate_pattern(**kwargs)

        if output_format == "full":
            title = f"{param}={value:.2f}"
            click.echo(render_ascii(pattern, title=title))
            click.echo("")
        else:
            click.echo(f"{param}={value:.2f}: {render_compact(pattern)}")

        value += step


@main.command()
@click.option("--genre", "-g", type=str, default="techno", help="Genre")
@click.option("--seed", type=int, default=0x12345678, help="Random seed")
def grid(genre, seed):
    """Show patterns at all 9 grid positions.

    Example:
        pattern-viz grid --genre techno
    """
    try:
        genre_enum = parse_genre(genre)
    except click.BadParameter as e:
        click.echo(f"Error: {e}", err=True)
        sys.exit(1)

    click.echo(f"Grid for {genre.upper()}")
    click.echo("=" * 60)

    for y in [2, 1, 0]:  # Top to bottom
        click.echo(f"\nY={y}:")
        for x in [0, 1, 2]:
            field_x = x / 2.0
            field_y = y / 2.0

            pattern = generate_pattern(
                genre=genre_enum,
                field_x=field_x,
                field_y=field_y,
                seed=seed,
            )

            click.echo(f"  [{x},{y}] {pattern.dominant_archetype:12s}: {render_compact(pattern)}")


@main.command()
@click.option("--bars", "-n", type=int, default=4, help="Number of bars")
@click.option("--energy", "-e", type=float, default=0.5, help="Energy level")
@click.option("--build", "-b", type=float, default=0.5, help="BUILD parameter")
@click.option("--genre", "-g", type=str, default="techno", help="Genre")
@click.option("--seed", type=int, default=0x12345678, help="Random seed")
@click.option("--output", "-o", type=str, default=None, help="Output WAV file")
@click.option("--bpm", type=float, default=120.0, help="BPM for audio")
def phrase(bars, energy, build, genre, seed, output, bpm):
    """Generate a phrase (multiple bars) showing BUILD arc.

    Example:
        pattern-viz phrase --bars 4 --build 0.8 --output phrase.wav
    """
    try:
        genre_enum = parse_genre(genre)
    except click.BadParameter as e:
        click.echo(f"Error: {e}", err=True)
        sys.exit(1)

    patterns = generate_phrase(
        bars=bars,
        energy=energy,
        build=build,
        genre=genre_enum,
        seed=seed,
    )

    click.echo(f"Phrase: {bars} bars, BUILD={build:.0%}")
    click.echo("=" * 60)

    for i, pattern in enumerate(patterns):
        progress = i / bars
        phase = "GROOVE" if progress < 0.60 else "BUILD" if progress < 0.875 else "FILL"
        click.echo(f"Bar {i+1} ({progress:.0%} - {phase}): {render_compact(pattern)}")

    if output and output.endswith(".wav"):
        # Concatenate all patterns into one
        # For now, just save first pattern (full phrase requires more work)
        click.echo(f"\nNote: Phrase audio not yet implemented. Saving first bar to {output}")
        save_wav(patterns[0], output, bpm)


@main.command()
def info():
    """Show information about archetypes and parameters."""
    from .archetypes import get_archetype

    click.echo("DuoPulse v4 Archetype Grid")
    click.echo("=" * 60)
    click.echo()

    for genre in [Genre.TECHNO, Genre.TRIBAL, Genre.IDM]:
        click.echo(f"{genre.name}:")
        for y in [2, 1, 0]:
            row = []
            for x in [0, 1, 2]:
                arch = get_archetype(genre, x, y)
                row.append(f"{arch.name:12s}")
            click.echo(f"  Y={y}: " + " | ".join(row))
        click.echo()

    click.echo("Energy Zones:")
    click.echo("  MINIMAL  (0-20%):  Sparse, skeleton only")
    click.echo("  GROOVE   (20-50%): Stable, danceable")
    click.echo("  BUILD    (50-75%): Increasing ghosts, fills")
    click.echo("  PEAK     (75-100%): Maximum activity")


if __name__ == "__main__":
    main()
