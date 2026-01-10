# Pattern Visualization Tools

Tools for visualizing and evaluating DuoPulse pattern generation.

## Prerequisites

- Python 3.9+
- C++ pattern_viz tool built (`make test` builds it)

## Tools

### generate-pattern-html.py

Generates HTML/SVG pattern visualization from the C++ `pattern_viz` tool output.

```bash
# Generate default visualization
python scripts/pattern-viz/generate-pattern-html.py

# Specify output file
python scripts/pattern-viz/generate-pattern-html.py --output docs/patterns.html

# Or use make target
make pattern-html
```

Creates an Ableton Live-style grid visualization with:
- Note blocks for hits/gates
- Velocity bars showing intensity
- Multiple parameter sweeps side-by-side

### evaluate-expressiveness.py

Evaluates pattern expressiveness across parameter space. Measures seed variation and pattern diversity.

```bash
python scripts/pattern-viz/evaluate-expressiveness.py
```

Outputs:
- Seed variation scores per voice
- Zone analysis (stable/syncopated/wild)
- Convergence pattern detection

### test_shape_budget.sh

Shell script for testing SHAPE budget behavior across zones.

```bash
./scripts/pattern-viz/test_shape_budget.sh
```

## C++ Backend

These scripts use the `pattern_viz` C++ tool located at `tools/pattern_viz.cpp`. Build it with:

```bash
make test  # Builds test runner including pattern_viz
```

Run directly:
```bash
./build/test_runner --pattern-viz [options]
```
