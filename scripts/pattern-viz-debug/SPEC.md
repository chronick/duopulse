# Pattern Visualization & Debug Tool

## Mini-Spec v1.0

A Python-based debug/visualization tool that replicates DuoPulse v4's pattern generation algorithms to help understand and tune the drum sequencer.

---

## Purpose

1. **Algorithm Verification**: Ensure Python implementation matches C++ behavior
2. **Pattern Visualization**: ASCII/visual output of generated patterns
3. **Audio Preview**: Generate WAV files with drum sounds for audition
4. **Parameter Exploration**: Sweep parameters to understand behavior

---

## Supported Parameters

### Performance Mode Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| `energy` | 0.0-1.0 | Hit density (MINIMAL/GROOVE/BUILD/PEAK zones) |
| `build` | 0.0-1.0 | Phrase arc (0=flat, 1=dramatic fills) |
| `field_x` | 0.0-1.0 | Syncopation axis (X in 3x3 grid) |
| `field_y` | 0.0-1.0 | Complexity axis (Y in 3x3 grid) |
| `punch` | 0.0-1.0 | Velocity dynamics (0=flat, 1=punchy) |
| `genre` | techno/tribal/idm | Genre bank selection |
| `balance` | 0.0-1.0 | Shimmer hit ratio (0-150% of anchor) |
| `drift` | 0.0-1.0 | Pattern evolution (seed variation) |

### Config Mode Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| `pattern_length` | 16/24/32/64 | Steps per bar |
| `phrase_length` | 1/2/4/8 | Bars per phrase |
| `swing` | 0.0-1.0 | Swing multiplier (0x-2x of archetype base) |
| `voice_coupling` | independent/interlock/shadow | Voice relationship |

### Internal Parameters (derived)

| Parameter | Description |
|-----------|-------------|
| `energy_zone` | MINIMAL/GROOVE/BUILD/PEAK (from energy) |
| `phrase_progress` | 0.0-1.0 position in phrase |
| `build_phase` | GROOVE/BUILD/FILL (from build + phrase_progress) |

---

## Algorithms Implemented

### 1. Archetype Weight Tables

27 archetypes (9 per genre) with:
- `anchor_weights[32]` - Kick/bass weights per step
- `shimmer_weights[32]` - Snare/clap weights per step
- `aux_weights[32]` - Hi-hat weights per step
- `swing_amount` - Base swing for archetype
- `accent_mask` - Which steps can accent
- `fill_multiplier` - Fill density boost

### 2. Pattern Field Blending

Winner-take-more softmax blending:
```python
def blend_archetypes(x, y, temperature=0.5):
    # Get 4 corner archetypes
    # Compute distances
    # Apply softmax with temperature
    # Blend weights
```

### 3. Energy Zone Computation

| Zone | Energy Range | Characteristics |
|------|--------------|-----------------|
| MINIMAL | 0-20% | Sparse, skeleton |
| GROOVE | 20-50% | Stable, danceable |
| BUILD | 50-75% | Increasing ghosts |
| PEAK | 75-100% | Maximum activity |

### 4. Hit Budget Calculation

```python
anchor_budget = compute_anchor_budget(energy, zone, pattern_length)
shimmer_budget = anchor_budget * (balance * 1.5)  # 0-150%
```

Zone-specific minimums and maximums apply.

### 5. Eligibility Mask Computation

Which metric positions can fire based on zone:
- MINIMAL: Downbeats + quarter notes
- GROOVE: Quarter notes + some 8ths
- BUILD: 8th notes + some 16ths
- PEAK: All positions

### 6. Gumbel Top-K Selection

```python
def select_hits_gumbel(weights, eligibility, budget, seed):
    # For each step: score = log(weight) + gumbel_noise(seed, step)
    # Select top K by score, respecting spacing rules
```

### 7. Euclidean Foundation (Genre-Dependent)

```python
def get_euclidean_ratio(genre, field_x, zone):
    # Techno: 70% at X=0, tapers with X
    # Tribal: 40% at X=0, tapers with X
    # IDM: 0% (disabled)
```

### 8. Voice Relationship

- **Independent**: Both patterns fire as generated
- **Interlock**: `shimmer_mask &= ~anchor_mask`
- **Shadow**: `shimmer_mask = shift_left(anchor_mask, 1)`

### 9. Velocity Computation

PUNCH parameter controls:
- `velocity_floor`: 65% → 30% (as punch increases)
- `accent_boost`: +15% → +45%
- `velocity_variation`: ±3% → ±15%

BUILD modifiers:
- GROOVE phase (0-60%): No modification
- BUILD phase (60-87.5%): Density +0-35%, velocity +0-8%
- FILL phase (87.5-100%): Density +35-50%, velocity +8-12%

### 10. Swing/Timing

```python
effective_swing = archetype_swing * (1.0 + config_swing)
swing = clamp(effective_swing, 0.50, zone_max)
```

Zone maxes: MINIMAL=0.60, GROOVE=0.65, BUILD=0.68, PEAK=0.70

---

## Output Formats

### 1. ASCII Pattern Display

```
Pattern: Techno [1,1] Groovy | Energy=50% | Build=50%
Step:  |1...|2...|3...|4...|5...|6...|7...|8...|
       |0123456789ABCDEF0123456789ABCDEF|
Anchor:|X...X...X...X...|X...X...X...X...|  (8 hits)
Shimmer|....X.......X...|....X.......X...|  (4 hits)
Aux:   |X.X.X.X.X.X.X.X.|X.X.X.X.X.X.X.X.|  (16 hits)
       |^ accent        |                 |
```

### 2. JSON Output

```json
{
  "params": {...},
  "pattern": {
    "anchor_mask": "0x11111111",
    "shimmer_mask": "0x01000100",
    "aux_mask": "0x55555555"
  },
  "velocities": {
    "anchor": [0.85, 0.0, 0.0, 0.0, ...],
    "shimmer": [0.0, 0.0, 0.0, 0.0, ...]
  },
  "timing": {
    "swing_offsets": [0, 0.15, 0, 0.15, ...]
  }
}
```

### 3. WAV Audio Output

- Uses bundled click samples or sine tones
- Applies velocity to amplitude
- Applies swing timing offsets
- Configurable BPM

### 4. MIDI Output (optional)

- Standard MIDI file with drum map
- Anchor → Kick (C1)
- Shimmer → Snare (D1)
- Aux → Hi-hat (F#1)

---

## CLI Interface

```bash
# Basic pattern generation
uv run pattern-viz --energy 0.5 --field-x 0.5 --field-y 0.5 --genre techno

# With audio output
uv run pattern-viz --energy 0.5 --genre techno --output pattern.wav --bpm 128

# Parameter sweep
uv run pattern-viz sweep --param energy --range 0.0:1.0:0.1 --output sweep/

# Interactive mode
uv run pattern-viz interactive

# JSON output for analysis
uv run pattern-viz --energy 0.5 --genre techno --format json > pattern.json
```

---

## Testing Strategy

### Unit Tests

1. **Archetype Loading**: Verify weight tables match C++ constants
2. **Energy Zone**: Test zone boundaries (0.2, 0.5, 0.75)
3. **Hit Budget**: Test budget calculation for all zones/lengths
4. **Gumbel Selection**: Test determinism with fixed seed
5. **Euclidean**: Test E(4,16), E(3,8), E(5,16) known patterns
6. **Velocity**: Test PUNCH=0 (flat) and PUNCH=1 (dynamic)
7. **BUILD Phases**: Test phase transitions at 60%, 87.5%
8. **Voice Coupling**: Test all three modes

### Integration Tests

1. **Full Pipeline**: Generate 100 patterns, verify constraints
2. **Seed Determinism**: Same params + seed = identical output
3. **Genre Differences**: Techno vs Tribal vs IDM produce different patterns
4. **Field Navigation**: All 9 grid positions produce distinct patterns

### Comparison Tests (Optional)

1. Compare Python output to C++ firmware output via serial logging
2. Generate patterns on both, diff the masks

---

## Dependencies

```toml
[project]
dependencies = [
    "numpy",      # Numerical operations
    "click",      # CLI framework
    "scipy",      # Audio output (wavfile)
]

[project.optional-dependencies]
midi = ["midiutil"]
audio = ["soundfile", "pydub"]
```

---

## File Structure

```
scripts/pattern-viz-debug/
├── SPEC.md                    # This file
├── pyproject.toml             # uv project config
├── src/
│   └── pattern_viz/
│       ├── __init__.py
│       ├── cli.py             # CLI entry point
│       ├── archetypes.py      # Weight tables (all 27)
│       ├── pattern_field.py   # Blending, interpolation
│       ├── hit_budget.py      # Budget calculation
│       ├── gumbel_sampler.py  # Gumbel Top-K selection
│       ├── euclidean.py       # Euclidean generation
│       ├── velocity.py        # PUNCH, BUILD computation
│       ├── timing.py          # Swing, timing effects
│       ├── voice_relation.py  # Voice coupling
│       ├── output.py          # ASCII, JSON, WAV output
│       └── types.py           # Enums, dataclasses
└── tests/
    ├── test_archetypes.py
    ├── test_hit_budget.py
    ├── test_gumbel.py
    ├── test_euclidean.py
    ├── test_velocity.py
    ├── test_pipeline.py
    └── conftest.py            # Pytest fixtures
```

---

## Success Criteria

- [ ] Generates patterns matching C++ implementation behavior
- [ ] All 27 archetypes produce distinct patterns
- [ ] Energy sweep shows clear density progression
- [ ] BUILD sweep shows audible phrase arc
- [ ] Field X/Y navigation produces 9 distinct characters
- [ ] Seed determinism: identical params = identical output
- [ ] Audio output is playable and demonstrates timing/velocity
