# Double Down Model: DuoPulse

## Intent
DuoPulse is a two-lane percussion sequencer optimized for the Patch.Init constraint of **two trigger outputs + two CV outputs**. Instead of simulating a multi-voice drum kit, DuoPulse treats the pair of lanes as a single intertwined instrument with complementary roles:

* **Anchor Lane** (Trigger 1 + CV 1): weighted toward low or body percussion, responsible for groove, pulse, and momentum.
* **Shimmer Lane** (Trigger 2 + CV 2): weighted toward mid/high transients, responsible for contrast, sparkle, and fills.

The core objective is to extract maximal expressive range from these lanes through macro-level controls that reshape groove, dynamics, micro-timing, and CV gesture without breaking the two-voice limitation.

## Design Pillars
1. **Interlocked Lanes** – Patterns emerge from interactions between Anchor and Shimmer, not independent tracks. Every control influences both lanes in musically useful ways.
2. **Macro Expressivity** – Performance knobs are large levers that shift feel, articulation, or structure in real time. Shifted functions provide deeper biasing without overwhelming the player.
3. **Dynamic CV Story** – CV outs deliver meaningful envelopes and modulation signals (not just velocity mirrors) so downstream voices feel alive.
4. **Composable Systems** – Users can blend pulse families, polymetric relationships, bursts, and articulation treatments to sculpt unique grooves quickly.

## Engine Architecture
### Rhythm Fabric
* Base time grid: 32 steps (eight 16th-note bars) internally addressed as four 8-step "cells".
* Pattern generation produces three layered states per lane:
  * **Skeleton**: primary beats per lane (immutable anchors).
  * **Ghost**: probabilistic off-beats and pickups.
  * **Burst**: micro-rolls and ratchets generated on demand.
* A **Lane Coupler** enforces complementary behavior:
  * Ghost events for Shimmer are biased away from steps selected by Anchor Skeleton, creating call/response.
  * Burst events prefer alternating lanes unless `Burst Bias` overrides.

### Articulation & CV Layer
* Each trigger event produces a **Gesture Packet** containing velocity, articulation type (accent, ghost, burst), and CV shape selection.
* CV shapes for each lane are drawn from small lookup banks (Hit, Ramp, Decay, Random Walk) scaled by current `Contour` parameter.
* Swing and micro-offsets are applied per-lane using `Slip` and `Orbit` controls.

### Memory & Scenes
* DuoPulse maintains **four scene slots** in runtime memory. Scenes capture all parameters across both knob banks and can be strided in performance (handled via Config Shift 4).
* Scene recall is non-destructive: soft takeover ensures knobs glide toward stored values.

## Control Mapping
Controls follow the existing hardware convention: **Switch DOWN = Performance**, **Switch UP = Config**. Each knob has a primary function and a shifted function (holding the button).

### Performance Mode (Switch DOWN)
| Knob | Primary Function | Shifted Function | Notes |
| :--- | :--- | :--- | :--- |
| K1 | **Pulse** – Sets Anchor Skeleton density from half-time pulses to relentless drive. | **Pulse Spread** – Nudges Anchor step spacing between straight, gallop, and dotted feels. | Spread uses subtle micro-timing to maintain groove. |
| K2 | **Glint** – Sets Shimmer Skeleton density (sparseness to frenetic). | **Glint Scatter** – Controls probability of Shimmer Ghost events (interlocking syncopation). | Scatter respects Anchor collisions unless intentionally overridden by Shift+K3 Burst Bias. |
| K3 | **Fuse** – Controls cross-lane energy exchange: CCW favors Anchor, CW favors Shimmer. | **Burst Bias** – Determines which lane gets priority for Burst rolls (Anchor, Alternating, Shimmer). | Fuse also tilts velocity scaling between the lanes. |
| K4 | **Flux** – Global mutation sending the engine through variation tables (fills, stutters, swing accents). | **Slip** – Applies lane-specific swing and micro-offset: Anchor swing CCW, Shimmer swing CW. | Flux includes quantized fill triggers on high settings. |

### Config Mode (Switch UP)
| Knob | Primary Function | Shifted Function | Notes |
| :--- | :--- | :--- | :--- |
| K1 | **Grid Loom** – Selects base cell pattern family (4/4, 3:2 clave, 5-step IDM, Euclid variants). | **Subdivision** – Scales internal step resolution (straight 16ths, 32nds, triplet grids). | Changing Subdivision morphs existing patterns instead of re-randomizing. |
| K2 | **Orbit** – Sets lane relationship: shared loop, 3:2 polymeter, 5:4 stagger, etc. | **Reset Mode** – Chooses how Reset input behaves (hard reset, phrase reset, scene advance). | Orbit defines how Anchor and Shimmer cycle against each other. |
| K3 | **Contour** – Picks CV shape palette for both lanes (punchy decay, bowed ramp, random walk, steady gate). | **Contour Depth** – Scales CV amplitude and post-trigger slew amount per lane. | Depth also sets velocity-to-CV modulation index. |
| K4 | **Scene Select** – Scrolls through four scene slots (A-D). | **Scene Write** – Press+turn to arm write, release to commit current state into slot. | Scene recall happens on knob release to avoid abrupt jumps. |

### Additional Interactions
* **Button Tap (no knob)**: Toggles Burst latch. When engaged, Burst events persist at current bias until toggled off.
* **Hold Button + Reset Input pulse**: Quick-save current state to Scene slot currently selected (identical to Config Shift K4 write, provided as a performance shortcut).

## Output Behavior
* **Trigger Outputs**: 0-5 V pulses with articulation-dependent widths (accent = 12 ms, ghost = 6 ms, burst = stacked 4 ms). Bursts manifest as ratcheted pulses up to 4 sub-steps.
* **CV Output 1 (Anchor)**:
  * Mirrors Anchor Gesture Packet with shape from `Contour`. Examples: punchy exponential for kicks, long decay for drones.
  * Responds to `Fuse` by expanding amplitude when Anchor is favored.
* **CV Output 2 (Shimmer)**:
  * Provides complementary modulation (e.g., ramp up for hats, random walk for percussion).
  * `Glint Scatter` increases CV jitter, `Slip` adds micro-LFO when swing is high.
* **Velocity Scaling**: Both CV outs and trigger pulse heights scale with internal velocity. Minimum floor keeps ghosts audible (~2.5 V min).

## Engine Behaviors & Algorithms
### Skeleton Generation
* Each pattern family defines Anchor and Shimmer skeleton seeds.
* `Pulse` and `Glint` remap seeds by selecting different rotation tables per density region.
* When `Orbit` introduces polymeter, skeletons rewrap independently while still referencing shared downbeat markers.

### Ghost Layer
* Probability maps keyed by `Glint Scatter`, modulated by `Flux`.
* Anti-collision logic ensures ghost hits do not overwhelm skeleton except at high `Flux`.
* Ghost velocity is attenuated unless `Fuse` favors that lane.

### Burst Layer
* Triggered when `Flux` crosses thresholds, on manual Burst latch, or when external reset hits on upbeat.
* `Burst Bias` decides lane targeting; alternating mode staggers bursts between lanes.
* Ratchet resolution depends on `Subdivision`.

### CV Articulation
* CV shapes stored as four 32-sample tables per palette.
* On each event, selected shape is pumped through simple per-lane VCA controlled by velocity and `Contour Depth`.
* Optional randomization (from `Flux`) picks alternate shape variants for evolving motion.

## Scene System
* Four in-memory scene slots (A–D).
* Scene recall is queued until next bar boundary to keep performance smooth.
* Shifted Config K4 handles write/recall; LED flash (existing front LED) blinks scene letter count for confirmation (A=1 blink ... D=4).

## Integration Notes
* Respects existing Soft Takeover infrastructure.
* Works with current clock/reset wiring.
* Requires new parameter mappings within engine:
  * `SetPulse`, `SetPulseSpread`, `SetGlint`, `SetGlintScatter`, `SetFuse`, `SetBurstBias`, `SetFlux`, `SetSlip`.
  * `SetGridLoom`, `SetSubdivision`, `SetOrbit`, `SetResetMode`, `SetContour`, `SetContourDepth`, `SetSceneIndex`, `WriteScene`.
* Pattern resources can leverage `GridsData` as a source for initial seeds but require additional tables for ghost/burst logic and polymetric orbit definitions.

## Future Extension Hooks
* Add CV modulation inputs per control by mapping CV ins to the same soft takeover pipeline.
* Consider saving scenes to flash for persistence across power cycles.
* Explore tying CV Outs into feedback routing (e.g., CV2 controlling Flux via internal link) as optional advanced mode.


