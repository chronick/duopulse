---
id: 12
slug: aux-output-modes
title: "Auxiliary Output Modes (CV Out 1)"
status: archived
created_date: 2025-12-16
updated_date: 2025-12-19
archived_date: 2025-12-19
superseded_by: 15
branch: feature/aux-output-modes
spec_refs:
  - "aux-output-modes"
---

# Feature: Auxiliary Output Modes [aux-output-modes]

## Status: SUPERSEDED

**This task has been superseded by task 15 (DuoPulse v4).**

The v4 implementation includes AUX output modes as part of **Phase 6: Output System** in `docs/tasks/active/15-duopulse-v4.md`.

### Key Differences in v4

| v2/v3 Spec | v4 Spec |
|------------|---------|
| 6 modes (Clock, HiHat, Fill Gate, Phrase CV, Accent, Downbeat) | 4 modes (HAT, FILL_GATE, PHRASE_CV, EVENT) |
| HiHat uses `ChaosModulator.ghostTrigger` | HAT uses aux hit mask from generation pipeline |
| Accent/Downbeat as separate modes | Combined into EVENT mode |
| K4+Shift controls AUX MODE (Config, external clock) | K3 controls AUX MODE (Config, always) |

### v4 AUX Mode Summary

| Mode | Output | Description |
|------|--------|-------------|
| **HAT** | Trigger | Third voice from aux hit mask |
| **FILL_GATE** | Gate | High during fill zones |
| **PHRASE_CV** | 0-5V | Ramp over phrase |
| **EVENT** | Trigger | Fires on accents, fills, section boundaries |

See `docs/specs/main.md` section 8.3 for full specification.

---

## Original Context (v2/v3)

When external clock is patched, outputting clock on CV_OUT_1 is redundant. Instead, CV_OUT_1 can serve as a multi-purpose auxiliary output with several useful modes: a third trigger (hi-hat/ghost), fill zone indicator, phrase position CV, accent trigger, or downbeat marker.

*This context is still valid in v4, but the implementation approach has changed.*
