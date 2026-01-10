# Hardware Interface

[← Back to Index](00-index.md) | [← Architecture](03-architecture.md)

---

## 1. Patch.init() Hardware Map

| Hardware | Label | Function |
|----------|-------|----------|
| Knob 1 | CTRL_1 | K1: ENERGY (perf) / CLOCK DIV (config) |
| Knob 2 | CTRL_2 | K2: SHAPE (perf) / SWING (config) |
| Knob 3 | CTRL_3 | K3: AXIS X (perf) / DRIFT (config) |
| Knob 4 | CTRL_4 | K4: AXIS Y (perf) / ACCENT (config) |
| CV In 1 | CV_1 | ENERGY modulation |
| CV In 2 | CV_2 | SHAPE modulation |
| CV In 3 | CV_3 | AXIS X modulation |
| CV In 4 | CV_4 | AXIS Y modulation |
| Audio In L | IN_L | FILL CV input (>1V triggers fill) |
| Gate In 1 | GATE_IN_1 | Clock input |
| Gate In 2 | GATE_IN_2 | Reset input |
| Gate Out 1 | GATE_OUT_1 | Voice 1 trigger |
| Gate Out 2 | GATE_OUT_2 | Voice 2 trigger |
| CV Out 1 | CV_OUT_1 | AUX output (Fill Gate or Hat Burst) |
| CV Out 2 | CV_OUT_2 | LED output |
| Audio Out L | OUT_L | Voice 1 velocity (0-5V S&H) |
| Audio Out R | OUT_R | Voice 2 velocity (0-5V S&H) |
| Button | SW_1 | Fill (tap) / Reseed (hold 3s) / AUX gesture |
| Switch | SW_2 | Performance (UP) / Config (DOWN) |

---

## 2. CV Input Behavior

CV inputs **always modulate Performance mode parameters**, regardless of current mode:

| CV Input | Modulates | Behavior |
|----------|-----------|----------|
| CV_1 | ENERGY | Bipolar: 0V = no mod, ±5V = ±50% |
| CV_2 | SHAPE | Bipolar: 0V = no mod, ±5V = ±50% |
| CV_3 | AXIS X | Bipolar: 0V = no mod, ±5V = ±50% |
| CV_4 | AXIS Y | Bipolar: 0V = no mod, ±5V = ±50% |

This allows CV sequences to keep running while adjusting config settings.

---

[Next: Control System →](05-controls.md)
