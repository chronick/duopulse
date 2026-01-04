# Feedback Log

## Feedback Round 1 - 2026-01-03

Source: `initial-feedback.md`

### Raw Feedback

> Controls are unwieldy. Config mode + shift for both modes increases complexity.
> We should strive to include just one set of 4 performance controls + 4 config mode controls.

> Performance:
> - 1 For energy
> - 1 morphs between 'regular' vs 'irregular' (euclidean vs musically-random-ish with hit budgets)
> - 2 for navigation within current algorithm paradigm

> Config:
> - Pattern length (1-bar techno vs song-oriented structure)
> - Aux mode

> Aux modes: Can be simplified to just hat + fill bar for now

> Button: Freeing up button lets us do things like manual fills, or some kind of performance toggle.

> Voice coupling doesn't feel important.

> The important thing is we can maintain simplicity with fewer controls, challenge is maintaining expressiveness with these new limitations.

> Lets also not limit ourselves to specific genres of music and act on first-principles.

### Key Themes

1. **Complexity reduction** - Shift layers in both modes is too much cognitive load
2. **Performance-first** - Performance mode should be self-sufficient
3. **Algorithm paradigm shift** - User wants euclidean ↔ irregular morphing, not genre selection
4. **Button liberation** - Button should be freed for performance use
5. **Voice coupling is low priority** - Can be removed or deeply buried
6. **Genre-agnostic** - First-principles approach, not genre-specific patterns
7. **Aux simplification** - Just hat + fill bar, not 4 modes

### Implied Pain Points from v4

- **GENRE control** (Shift+K2) - Forces genre thinking, user wants algorithm focus
- **DRIFT control** (Shift+K3) - May not be essential for performance
- **BALANCE control** (Shift+K4) - Voice ratio, but coupling not important
- **PUNCH control** (Shift+K1) - Velocity dynamics, unclear if needed in shift
- **Field X/Y navigation** - User mentions "2 for navigation" but unclear if current mapping works

---

## Feedback Round 2 - 2026-01-03

Source: User response to iteration-1

### Raw Feedback

> I can configure punch with VCAs so its less important that anything that influences actual trigs.

> I feel like expressive possibilities are much better with two navigation directions (can work with euclidean and with other modes).

> CV should always map to the 4 performance knobs as a law.

> Aux mode simplification good, but should remain clock if clock unpatched.

### Key Decisions

1. **PUNCH deprioritized** - External VCAs handle velocity dynamics; trigger-influencing params more valuable
2. **2D navigation preserved** - Keep two axes for pattern navigation (not 1D MORPH)
3. **CV Law established** - CV 1-4 MUST map to K1-K4 primary functions, non-negotiable
4. **Aux behavior confirmed** - HAT + FILL BAR modes, clock output when unpatched

### Design Implications

This confirms the 4 performance controls should be:
1. ENERGY (density)
2. SHAPE (euclidean ↔ irregular)
3. Navigation axis 1
4. Navigation axis 2

No shift layer in performance mode. PUNCH, BUILD, DRIFT, BALANCE all move to config or removed.

---

## Feedback Round 3 - 2026-01-03

Source: User response to iteration-2

### Raw Feedback

> aux mode and swing can go behind shift as I think they're less likely to change.

> Shape 0 should have slight humanization.

> Sparse/busy should be handled by energy.

> Stable/unstable and on/off grid should be handled by SHAPE.

> Grounded/floating and simple/complex better.

> button is not velocity sensitive so is limited to on/off switch. Good for fills but thats all I can think of (maybe fine to just keep it as fills for now)

### Parameter Domain Clarification

| Parameter | Controls | NOT this |
|-----------|----------|----------|
| ENERGY | Sparse ↔ Busy (hit count) | - |
| SHAPE | Stable/on-grid ↔ Unstable/off-grid | Sparse/busy (that's ENERGY) |
| SEEK X | Grounded ↔ Floating | - |
| SEEK Y | Simple ↔ Complex | Sparse/busy (that's ENERGY) |

### Config Mode Changes

- SWING → shift function (less frequently changed)
- AUX MODE → shift function (less frequently changed)
- This frees up primary slots on K2/K3

### Algorithm Notes

- SHAPE=0% should NOT be pure robotic euclidean
- Add slight humanization even at minimum SHAPE
- "Slight" = enough to sound musical, not enough to sound random

### Button Simplification

- On/off only (not velocity sensitive)
- Keep as fill trigger for now
- Reseed can be double-tap or moved elsewhere

---

## Feedback Round 4 - 2026-01-03

Source: User response to iteration-3 + critic response

### Raw Feedback

> AXIS better. I like X/Y navigation as it is in line with the idea of 'duopulse', but lets make sure the X and Y axes are intuitive and work well with energy/shape.

> Removal of BUILD also good idea, we can automate with CV to performance controls. That also means pattern length not as important.

> SHAPE algorithm looks good, lets specify the logic.

> Hold-to-reseed good, lets do 3 seconds that is accompanied by LED feedback (details of LED feedback left for later, just mark it as a need for when we formalize all LED feedback behaviors)

### Key Decisions

1. **AXIS X/Y naming** - confirmed, aligns with "duopulse" 2D concept
2. **BUILD removed from module** - users can automate with CV to performance controls externally
3. **Pattern LENGTH less important** - still present but deprioritized
4. **SHAPE algorithm** - crossfade logic to be specified
5. **Hold-to-reseed = 3 seconds** - with LED feedback (details TBD)

### Implications

Config mode now has even fewer controls:
- BUILD removed entirely
- LENGTH still present but less critical
- DRIFT, SWING, CLOCK DIV, AUX MODE remain

This may allow further config simplification (3 primary controls + shifts, or consolidation).

---

## Feedback Round 5 - 2026-01-04

Source: User response to dual critic reviews (Opus + alternative analysis)

### Raw Feedback

> 1. Deterministic direction good, we want musicality, variety, expressiveness. Random not as important.

> 2. Additive good

> 3. Shape (performance) with Swing (config), both on K2 (sidenote, this makes ENERGY + CLOCK DIV good pair for performance/config). (sidenote, lets make sure knobs are related between perf and config as much as possible)

> 4. I want fill to temporarily make stuff go nuts by intelligently modifying other attributes. That direction makes sense, it should also always trigger fill gate or a hat burst / trill (based on aux config + shape). It should also respect energy control as a modifier (fill during low energy does less, fill during high energy does more)

### Key Decisions

1. **Deterministic SHAPE blending** - Replace coin-flip with score interpolation for consistent groove
2. **Additive AXIS biasing** - Replace multiplicative to prevent dead zones at low ENERGY
3. **Knob pairing principle** - Same knob position should have related concepts across perf/config modes:
   - K1: ENERGY (perf) / CLOCK DIV (config) - both about "rate/density"
   - K2: SHAPE (perf) / SWING (config) - both about "timing feel"
   - K3: AXIS X / ??? - TBD
   - K4: AXIS Y / ??? - TBD
4. **Fill algorithm** - Should:
   - Intelligently modify multiple attributes (not just density)
   - Always trigger fill gate OR hat burst/trill (based on AUX MODE + SHAPE)
   - Scale with ENERGY (low energy = subtle fill, high energy = intense fill)

### Algorithm Implications

- SHAPE crossfade now uses deterministic score interpolation
- AXIS X/Y use additive offsets instead of multiplicative
- Fill behavior becomes a cohesive "performance burst" that respects current settings

---

## Feedback Round 6 - 2026-01-04

Source: User response to iteration-5

### Raw Feedback

> 1. Suggest a good set of LED feedback behaviors based on different actions (when switching modes, fill, hits, param changes, resets). Remember we can specify brightness only, so it would have to be a combination of brightness display + flashing patterns. Can make them a bit fun too if we want like for fills/reset.

> 2. We want voice 2 relationship to generally act as the "response" to call/response patterns. I didn't mind v4 but it could have used some variety. Suggest option for redesign and let the critic evaluate. One important thing to note however: We want to be able to extend this firmware to include multiple variations: instead of just percussive we'd like to have variations for basslines/ambient that may use CV as pitch/mod. During implementation, lets make sure we can specify different relationship patterns + sequences easily to support these types of variations. The abstract nature of the controls should hopefully map well to these alternative variations.

> 3. For Aux hat burst: should generally follow ENERGY + SHAPE (low shape more regular, high shape more irregular, just like the rest of the patterns). Aux hats should always be sensitive to the rest of the pattern (aux fill gate can just go high during fill time)

### Key Decisions

1. **LED feedback specification needed** - Brightness + patterns only (no color). Make fills/reset fun.
2. **Voice 2 = Response** - Call/response paradigm with more variety than v4
3. **Extensibility requirement** - Design for multiple firmware variations:
   - Percussive (current) - triggers
   - Basslines - CV as pitch
   - Ambient - CV as modulation
4. **Aux hat burst** - Follow ENERGY + SHAPE, sensitive to main pattern

### Architectural Implications

- Voice relationship should be abstracted/pluggable for different firmware modes
- Abstract controls (ENERGY, SHAPE, AXIS X/Y) should map meaningfully across variations
- Consider "variation profiles" that change how the engine interprets controls

---

## Feedback Round 7 - 2026-01-04

Source: User decisions on critic recommendations

### Decisions Made

| Topic | Decision | Rationale |
|-------|----------|-----------|
| Voice Relationship | **Option B**: COMPLEMENT only + DRIFT variation | Simple, predictable, no hidden magic |
| LED Specification | **Option A**: Keep detailed spec | Implementation guidance valuable |
| Extensibility | **Option B**: Document intent only (YAGNI) | Keep as implementation notes, don't build framework |
| Hat Burst | **Option A**: Pattern-aware version | Sophistication where it matters musically |

### Implications

1. **Voice 2 simplified**: Remove auto-selection, use COMPLEMENT as default, DRIFT adds subtle phrase-to-phrase variation
2. **LED spec preserved**: Keep pseudo-code as implementation guidance, fix layering semantics
3. **VariationProfile removed**: Replace with extensibility notes, don't implement struct
4. **Hat burst kept**: Fix heap allocation, keep pattern-awareness

### Critical Fixes Required (from Opus critic)

1. Pre-allocate HatBurst arrays (no `new` in real-time path)
2. Define LED layering semantics (add/max/blend)
3. Layer trigger feedback on top of fills (don't suppress)

---

## Feedback Round 8 - 2026-01-04

Source: User response to iteration-7 (final specification)

### Raw Feedback

> I'm a bit bugged by the fact that we still have a config + shift but with just one setting (aux mode). I want to be able to switch AUX mode but I'd love the opportunity to remove config + shift page.

### Pain Point

The current design has:
- Performance Mode: 4 controls, **zero shift** ✓
- Config Mode: 4 controls + 1 shift (K3+Shift = AUX MODE)

Having a shift layer for a single setting feels like vestigial complexity. The original goal was to eliminate shift layers entirely where possible.

### Constraints

- AUX MODE must still be accessible (HAT vs FILL BAR)
- Config mode has: CLOCK DIV, SWING, DRIFT, LENGTH
- Want to maintain knob pairing principle

### Question

How can we provide AUX MODE selection without a dedicated shift layer?

---

## Feedback Round 8b - 2026-01-04

Source: User response to iteration-8 options

### Raw Feedback

> If we need to keep knobs as-is, I kind of like the idea of a "duopulse secret mode" that becomes "2.5 pulse". Since the extra hat moves us away from the idea of duopulse, I think its good to advertise fill gate as default + hat secret mode. It should be a deliberate action and doesn't have to be done very often. What if we held down button while switching up/down? hold + switch up is hat mode, hold + switch down always resets to fill gate. Fun flash pattern when we do it.

> Second, pattern length brings up a good point. Given the other changes and simplifications, does pattern length matter much here? Might it matter during irregular shaped patterns? If there's a use for it lets keep it, if not lets consider it for removal. I wouldn't mind the ability to have a set-and-forget set of regular patterns, but if we're moving so many controls to be CV-able it might not make sense.

### Key Decisions

1. **AUX MODE via Hold+Switch gesture**
   - Hold button + Switch UP = HAT mode ("2.5 pulse" secret)
   - Hold button + Switch DOWN = FILL GATE mode (default)
   - Fun flash pattern confirms the change
   - Deliberate action, won't happen accidentally

2. **"2.5 Pulse" branding** - HAT mode as a secret unlock, FILL GATE as advertised default

3. **LENGTH questionable** - With CV-able performance controls, pattern length may be redundant
   - CV modulation provides phrase variation
   - DRIFT adds evolution
   - 16-step is the techno standard
   - Could be compile-time for power users

### Implications

- Config+Shift layer completely eliminated
- Config mode potentially reduced to 3 knobs (CLOCK DIV, SWING, DRIFT)
- K4 in Config mode either unused or available for future feature

---

## Feedback Round 8c - 2026-01-04

Source: User response to K4 ideation options

### Decision

**K4 Config = ACCENT** (velocity depth)

- 0% = Flat dynamics (all hits equal velocity)
- 100% = Full dynamics (ghost notes to accents)

### Rationale

- Directly affects musicality
- VCAs can't add ghost notes - this is internal character
- Natural pairing with AXIS Y (both about "intricacy/depth")
- "Set and forget" calibration

### Final Config Layout

```
K1: CLOCK DIV   K2: SWING   K3: DRIFT   K4: ACCENT
```

### Knob Pairing Complete

| Knob | Performance | Config | Link |
|------|-------------|--------|------|
| K1 | ENERGY | CLOCK DIV | Rate/Density |
| K2 | SHAPE | SWING | Timing Feel |
| K3 | AXIS X | DRIFT | Variation |
| K4 | AXIS Y | ACCENT | Intricacy/Depth |

---

## Critic Review - 2026-01-04

Source: design-critic agent review of iteration-8

### Critical Issues Found & Resolved

| Issue | Problem | Resolution |
|-------|---------|------------|
| Hold+Switch conflicts | Gesture would trigger shift mode and live fill | Added explicit cancel of shift/fill, consume switch event |
| ACCENT weight vs velocity | Sketch conflated hit probability with velocity | Rewrote to use musical weight for velocity mapping |

### Concerns Addressed

| Concern | Decision |
|---------|----------|
| HAT mode discoverability | OK as "secret mode" - document prominently |
| AUX MODE persistence | **Volatile** - defaults to FILL GATE on boot (consistent with "nothing persists") |
| ACCENT + SHAPE interaction | Fixed - uses musical weight, not step position |
| LENGTH compile-time only | Added **boot-time override** option (hold button at boot) |
| CV in Config mode | Clarified: CV always modulates perf params regardless of mode |

### Validated as Good

- Zero-shift control layout
- Knob pairing concept
- ACCENT as config parameter placement
- LED feedback for AUX mode changes
- Pre-allocated HatBurst (real-time safe)
- COMPLEMENT voice relationship
