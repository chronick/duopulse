# Algorithm Weight Configuration

This directory contains JSON configuration files for the DuoPulse algorithm weight blending system.

## Files

- `schema.json` - JSON Schema for validation
- `baseline.json` - Default configuration matching v5.2 release
- Additional configs can be created for experimentation

## Usage

### Generate C++ Header (Firmware)

```bash
# Generate header from baseline config
make weights-header

# Generate from specific config
make weights-header CONFIG=config/weights/experimental.json
```

The generated header goes to `inc/generated/weights_config.h` and contains `constexpr` values with zero runtime overhead.

### Runtime Loading (pattern_viz only)

```bash
# Use specific config file
./build/pattern_viz --config config/weights/experimental.json --shape 0.5

# Uses compiled-in defaults if --config not specified
./build/pattern_viz --shape 0.5
```

## Creating New Configurations

1. Copy `baseline.json` to a new file (e.g., `experimental.json`)
2. Modify the values
3. Update `version` and `name` fields
4. Test with pattern_viz: `./build/pattern_viz --config config/weights/experimental.json --debug-weights`

## Configuration Parameters

### Euclidean

| Parameter | Default | Description |
|-----------|---------|-------------|
| `fadeStart` | 0.30 | SHAPE value where euclidean begins fading |
| `fadeEnd` | 0.70 | SHAPE value where euclidean reaches zero |
| `anchor.kMin/kMax` | 4-12 | Hit count range for anchor voice |
| `shimmer.kMin/kMax` | 6-16 | Hit count range for shimmer voice |
| `aux.kMin/kMax` | 2-8 | Hit count range for aux voice |

### Syncopation

| Parameter | Default | Description |
|-----------|---------|-------------|
| `center` | 0.50 | SHAPE value at peak syncopation |
| `width` | 0.30 | Bell curve width (standard deviation) |

### Random

| Parameter | Default | Description |
|-----------|---------|-------------|
| `fadeStart` | 0.50 | SHAPE value where random begins appearing |
| `fadeEnd` | 0.90 | SHAPE value where random reaches full strength |

## Weight Calculation

At any SHAPE value, the three algorithm weights are computed:

```
euclidean_raw = 1.0 - smoothstep(fadeStart, fadeEnd, shape)
syncopation_raw = bell_curve(shape, center, width)
random_raw = smoothstep(fadeStart, fadeEnd, shape)

total = euclidean_raw + syncopation_raw + random_raw
euclidean = euclidean_raw / total
syncopation = syncopation_raw / total
random = random_raw / total
```

All weights are normalized to sum to 1.0.
