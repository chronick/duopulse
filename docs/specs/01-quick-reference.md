# Quick Reference

[← Back to Index](00-index.md)

---

## I/O Summary

### Outputs
| Output | Hardware | Signal |
|--------|----------|--------|
| **Voice 1 Trig** | Gate Out 1 | 5V trigger on anchor hits |
| **Voice 2 Trig** | Gate Out 2 | 5V trigger on shimmer hits |
| **Voice 1 Velocity** | Audio Out L | 0-5V sample & hold |
| **Voice 2 Velocity** | Audio Out R | 0-5V sample & hold |
| **AUX** | CV Out 1 | Fill Gate (default) OR Hat Burst |
| **LED** | CV Out 2 | Visual feedback (brightness) |

### Inputs
| Input | Hardware | Function |
|-------|----------|----------|
| **Clock** | Gate In 1 | External clock (disables internal when patched) |
| **Reset** | Gate In 2 | Reset to step 0 |
| **Fill CV** | Audio In L | Fill trigger (>1V gate) |

---

## Control Layouts

### Performance Mode (Switch UP)
| Knob | Parameter | 0% | 100% |
|------|-----------|-----|------|
| K1 | **ENERGY** | Sparse | Busy |
| K2 | **SHAPE** | Stable (humanized euclidean) | Wild (weighted) |
| K3 | **AXIS X** | Grounded (downbeats) | Floating (offbeats) |
| K4 | **AXIS Y** | Simple | Complex |

### Config Mode (Switch DOWN)
| Knob | Parameter | 0% | 100% |
|------|-----------|-----|------|
| K1 | **CLOCK DIV** | ÷4 (slow) | ×4 (fast) |
| K2 | **SWING** | Straight | Heavy swing |
| K3 | **DRIFT** | Locked (same each phrase) | Evolving |
| K4 | **ACCENT** | Flat (all hits equal) | Dynamic (ghosts to accents) |

### CV Inputs (always modulate Performance params)
| CV In | Modulates | Range |
|-------|-----------|-------|
| CV 1 | ENERGY | ±50% |
| CV 2 | SHAPE | ±50% |
| CV 3 | AXIS X | ±50% |
| CV 4 | AXIS Y | ±50% |

---

[Next: Core Concepts →](02-core-concepts.md)
