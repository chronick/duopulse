# LED Feedback System

[← Back to Index](00-index.md) | [← AUX Output](11-aux.md)

---

## 1. 5-Layer Priority System

| Layer | Priority | State | Behavior |
|-------|----------|-------|----------|
| 1 | Base | Idle (perf) | Gentle breath synced to clock |
| 1 | Base | Idle (config) | Slower breath |
| 2 | Additive | Clock sync | Subtle pulse on beats |
| 3 | Maximum | Trigger activity | Pulse on hits, envelope decay |
| 4 | Maximum | Fill active | Accelerating strobe + trigger overlay |
| 5 | Replace | Reseed progress | Building pulse (1-5Hz over 3s) |
| 5 | Replace | Reseed confirm | POP POP POP (3 flashes) |
| 5 | Replace | Mode switch | Quick signature |
| 5 | Replace | AUX mode unlock | Triple rising flash |
| 5 | Replace | AUX mode reset | Single fade |

---

## 2. Layer Composition

```
finalBrightness = max(
  baseBrightness,
  clockPulse,      // Layer 2 additive
  triggerPulse,    // Layer 3 maximum
  fillStrobe       // Layer 4 maximum
)

IF layer5Active:
  finalBrightness = layer5Value  // Layer 5 replaces all
```

---

[Next: Boot Behavior →](13-boot.md)
