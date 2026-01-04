"""Output formatting for patterns: ASCII, JSON, WAV."""

import json
from dataclasses import asdict
from typing import Any

from .types import Genre, EnergyZone, VoiceCoupling
from .pipeline import PatternOutput


def format_mask_binary(mask: int, length: int = 32) -> str:
    """Format mask as binary string with markers."""
    chars = []
    for i in range(length):
        if mask & (1 << i):
            chars.append("X")
        else:
            chars.append(".")
    return "".join(chars)


def format_mask_hex(mask: int) -> str:
    """Format mask as hex string."""
    return f"0x{mask:08X}"


def format_velocities(velocities: list[float], mask: int, length: int = 32) -> str:
    """Format velocities with visual representation."""
    chars = []
    for i in range(length):
        if mask & (1 << i):
            vel = velocities[i]
            if vel >= 0.9:
                chars.append("9")
            elif vel >= 0.8:
                chars.append("8")
            elif vel >= 0.7:
                chars.append("7")
            elif vel >= 0.6:
                chars.append("6")
            elif vel >= 0.5:
                chars.append("5")
            elif vel >= 0.4:
                chars.append("4")
            else:
                chars.append("3")
        else:
            chars.append(".")
    return "".join(chars)


def render_ascii(
    pattern: PatternOutput,
    title: str = "",
    show_velocities: bool = True,
    show_info: bool = True,
) -> str:
    """Render pattern as ASCII art.

    Args:
        pattern: Pattern to render
        title: Optional title line
        show_velocities: Show velocity levels
        show_info: Show pattern info

    Returns:
        Multi-line ASCII string
    """
    length = pattern.pattern_length
    lines = []

    # Title
    if title:
        lines.append(title)
        lines.append("=" * len(title))
        lines.append("")

    # Info line
    if show_info:
        zone_name = pattern.energy_zone.name
        lines.append(
            f"Archetype: {pattern.dominant_archetype} | "
            f"Zone: {zone_name} | "
            f"Swing: {pattern.blended_swing:.0%}"
        )
        lines.append(
            f"Hits: Anchor={pattern.anchor_hits}, "
            f"Shimmer={pattern.shimmer_hits}, "
            f"Aux={pattern.aux_hits}"
        )
        lines.append("")

    # Beat markers (for 32 steps)
    if length == 32:
        lines.append("Step:   |1...2...3...4...|5...6...7...8...|")
        lines.append("        |0123456789ABCDEF|0123456789ABCDEF|")
    elif length == 16:
        lines.append("Step:   |1...2...3...4...|")
        lines.append("        |0123456789ABCDEF|")

    # Pattern lines
    anchor_str = format_mask_binary(pattern.anchor_mask, length)
    shimmer_str = format_mask_binary(pattern.shimmer_mask, length)
    aux_str = format_mask_binary(pattern.aux_mask, length)

    if length == 32:
        lines.append(f"Anchor: |{anchor_str[:16]}|{anchor_str[16:]}| ({pattern.anchor_hits})")
        lines.append(f"Shimmer:|{shimmer_str[:16]}|{shimmer_str[16:]}| ({pattern.shimmer_hits})")
        lines.append(f"Aux:    |{aux_str[:16]}|{aux_str[16:]}| ({pattern.aux_hits})")
    else:
        lines.append(f"Anchor: |{anchor_str}| ({pattern.anchor_hits})")
        lines.append(f"Shimmer:|{shimmer_str}| ({pattern.shimmer_hits})")
        lines.append(f"Aux:    |{aux_str}| ({pattern.aux_hits})")

    # Velocity lines
    if show_velocities:
        lines.append("")
        lines.append("Velocities (3-9):")
        anchor_vel = format_velocities(pattern.anchor_velocities, pattern.anchor_mask, length)
        shimmer_vel = format_velocities(pattern.shimmer_velocities, pattern.shimmer_mask, length)
        aux_vel = format_velocities(pattern.aux_velocities, pattern.aux_mask, length)

        if length == 32:
            lines.append(f"Anchor: |{anchor_vel[:16]}|{anchor_vel[16:]}|")
            lines.append(f"Shimmer:|{shimmer_vel[:16]}|{shimmer_vel[16:]}|")
            lines.append(f"Aux:    |{aux_vel[:16]}|{aux_vel[16:]}|")
        else:
            lines.append(f"Anchor: |{anchor_vel}|")
            lines.append(f"Shimmer:|{shimmer_vel}|")
            lines.append(f"Aux:    |{aux_vel}|")

    # Hex masks
    lines.append("")
    lines.append(f"Masks: A={format_mask_hex(pattern.anchor_mask)} "
                 f"S={format_mask_hex(pattern.shimmer_mask)} "
                 f"X={format_mask_hex(pattern.aux_mask)}")

    return "\n".join(lines)


def render_compact(pattern: PatternOutput) -> str:
    """Render pattern in compact single-line format."""
    return (
        f"A:{format_mask_hex(pattern.anchor_mask)} "
        f"S:{format_mask_hex(pattern.shimmer_mask)} "
        f"X:{format_mask_hex(pattern.aux_mask)} "
        f"({pattern.anchor_hits}/{pattern.shimmer_hits}/{pattern.aux_hits})"
    )


def to_dict(pattern: PatternOutput) -> dict[str, Any]:
    """Convert pattern to dictionary for JSON serialization."""
    return {
        "pattern": {
            "anchor_mask": format_mask_hex(pattern.anchor_mask),
            "shimmer_mask": format_mask_hex(pattern.shimmer_mask),
            "aux_mask": format_mask_hex(pattern.aux_mask),
            "anchor_mask_int": pattern.anchor_mask,
            "shimmer_mask_int": pattern.shimmer_mask,
            "aux_mask_int": pattern.aux_mask,
        },
        "hits": {
            "anchor": pattern.anchor_hits,
            "shimmer": pattern.shimmer_hits,
            "aux": pattern.aux_hits,
        },
        "velocities": {
            "anchor": [round(v, 3) for v in pattern.anchor_velocities[:pattern.pattern_length]],
            "shimmer": [round(v, 3) for v in pattern.shimmer_velocities[:pattern.pattern_length]],
            "aux": [round(v, 3) for v in pattern.aux_velocities[:pattern.pattern_length]],
        },
        "info": {
            "pattern_length": pattern.pattern_length,
            "dominant_archetype": pattern.dominant_archetype,
            "energy_zone": pattern.energy_zone.name,
            "blended_swing": round(pattern.blended_swing, 3),
        },
    }


def to_json(pattern: PatternOutput, indent: int = 2) -> str:
    """Convert pattern to JSON string."""
    return json.dumps(to_dict(pattern), indent=indent)


def generate_wav_simple(
    pattern: PatternOutput,
    bpm: float = 120.0,
    sample_rate: int = 44100,
) -> bytes:
    """Generate simple WAV audio with clicks for hits.

    Uses simple sine wave clicks for each voice:
    - Anchor: Low click (200 Hz)
    - Shimmer: Mid click (400 Hz)
    - Aux: High click (800 Hz)

    Args:
        pattern: Pattern to render
        bpm: Beats per minute
        sample_rate: Audio sample rate

    Returns:
        WAV file bytes
    """
    import struct
    import math

    # Calculate timing
    steps_per_beat = 4  # 16th notes
    beats_per_bar = 8 if pattern.pattern_length == 32 else 4
    seconds_per_step = 60.0 / bpm / steps_per_beat

    # Total samples
    total_steps = pattern.pattern_length
    total_samples = int(total_steps * seconds_per_step * sample_rate)

    # Click parameters
    click_samples = int(0.02 * sample_rate)  # 20ms click
    frequencies = {
        "anchor": 200.0,
        "shimmer": 400.0,
        "aux": 800.0,
    }

    # Generate audio buffer (mono float)
    audio = [0.0] * total_samples

    def add_click(start_sample: int, freq: float, velocity: float):
        for i in range(click_samples):
            if start_sample + i >= total_samples:
                break
            t = i / sample_rate
            # Exponential decay envelope
            env = math.exp(-t * 50) * velocity
            sample = math.sin(2 * math.pi * freq * t) * env * 0.5
            audio[start_sample + i] += sample

    # Add clicks for each voice
    for step in range(pattern.pattern_length):
        start_sample = int(step * seconds_per_step * sample_rate)

        if pattern.anchor_mask & (1 << step):
            vel = pattern.anchor_velocities[step]
            add_click(start_sample, frequencies["anchor"], vel)

        if pattern.shimmer_mask & (1 << step):
            vel = pattern.shimmer_velocities[step]
            add_click(start_sample, frequencies["shimmer"], vel)

        if pattern.aux_mask & (1 << step):
            vel = pattern.aux_velocities[step]
            add_click(start_sample, frequencies["aux"], vel * 0.5)  # Quieter aux

    # Normalize and convert to 16-bit PCM
    max_val = max(abs(s) for s in audio) or 1.0
    pcm_data = []
    for sample in audio:
        normalized = sample / max_val * 0.8  # Leave headroom
        pcm = int(normalized * 32767)
        pcm = max(-32768, min(32767, pcm))
        pcm_data.append(pcm)

    # Build WAV file
    num_channels = 1
    bits_per_sample = 16
    byte_rate = sample_rate * num_channels * bits_per_sample // 8
    block_align = num_channels * bits_per_sample // 8
    data_size = len(pcm_data) * 2

    # WAV header
    wav = bytearray()
    wav.extend(b"RIFF")
    wav.extend(struct.pack("<I", 36 + data_size))
    wav.extend(b"WAVE")
    wav.extend(b"fmt ")
    wav.extend(struct.pack("<I", 16))  # fmt chunk size
    wav.extend(struct.pack("<H", 1))   # PCM format
    wav.extend(struct.pack("<H", num_channels))
    wav.extend(struct.pack("<I", sample_rate))
    wav.extend(struct.pack("<I", byte_rate))
    wav.extend(struct.pack("<H", block_align))
    wav.extend(struct.pack("<H", bits_per_sample))
    wav.extend(b"data")
    wav.extend(struct.pack("<I", data_size))

    # PCM data
    for pcm in pcm_data:
        wav.extend(struct.pack("<h", pcm))

    return bytes(wav)


def save_wav(pattern: PatternOutput, filepath: str, bpm: float = 120.0):
    """Save pattern as WAV file."""
    wav_data = generate_wav_simple(pattern, bpm)
    with open(filepath, "wb") as f:
        f.write(wav_data)
