# Core Concepts

[← Back to Index](00-index.md) | [← Quick Reference](01-quick-reference.md)

---

## 1. Design Philosophy

DuoPulse v5 is an **opinionated drum sequencer** with:

1. **Zero shift layers**: Every parameter is directly accessible
2. **CV law**: CV1-4 always modulate performance parameters, regardless of mode
3. **Knob pairing**: Related functions across Performance/Config modes
4. **Secret mode**: Hat burst as discoverable easter egg ("2.5 pulse")
5. **Deterministic variation**: Same settings + seed = identical output

---

## 2. Key Changes from v4

| Aspect | v4 | v5 |
|--------|----|----|
| Shift layers | 4 shift parameters | None |
| Parameters | 12+ with enums | 8 direct knobs |
| GENRE control | 3 selectable genres | Algorithm-driven (removed) |
| BALANCE | Voice ratio knob | Removed (SHAPE handles) |
| BUILD | Phrase arc | Replaced by SHAPE |
| PUNCH | Velocity dynamics | Replaced by ACCENT |
| Voice coupling | Independent/Shadow/Interlock | COMPLEMENT only |
| AUX modes | 4 config options | 2 via gesture (HAT/FILL GATE) |

---

## 3. Knob Pairing Concept

Each knob position has related functions across modes:

| Knob | Performance | Config | Conceptual Link |
|------|-------------|--------|-----------------|
| K1 | ENERGY | CLOCK DIV | Rate/Density |
| K2 | SHAPE | SWING | Timing Feel |
| K3 | AXIS X | DRIFT | Variation/Movement |
| K4 | AXIS Y | ACCENT | Intricacy/Depth |

---

[Next: Architecture →](03-architecture.md)
