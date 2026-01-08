# Tasks Index

| id | feature | description | status | created_at | updated_at | file |
|----|---------|-------------|--------|------------|------------|------|
| 00 | hardware-setup | Phase 1: Hardware Setup & Hello World | completed | 2025-11-25 | 2025-11-25 | completed/00-hardware-setup.md |
| 01 | clock-sequencer | Phase 2: The Clock & Simple Sequencer | completed | 2025-11-25 | 2025-11-25 | completed/01-clock-sequencer.md |
| 02 | core-cv-chaos | Phase 4: Core CV & Chaos | completed | 2025-11-25 | 2025-11-26 | completed/02-core-cv-chaos.md |
| 03 | grids-core | Phase 3: The Grids Core (Algorithm Port) | completed | 2025-11-25 | 2025-11-25 | completed/03-grids-core.md |
| 04 | control-mapping | Phase 4: Full Control Mapping & Chaos | completed | 2025-11-25 | 2025-11-25 | completed/04-control-mapping.md |
| 05 | attenuation-fix | Phase 4.1: Attenuation Bugfix | completed | 2025-11-25 | 2025-11-26 | completed/05-attenuation-fix.md |
| 06 | config-mode | Phase 5: Configuration Mode | completed | 2025-11-25 | 2025-11-26 | completed/06-config-mode.md |
| 07 | opinionated-sequencer | Opinionated Drum Sequencer Implementation (superseded by DuoPulse v2) | completed | 2025-12-01 | 2025-12-01 | completed/07-opinionated-sequencer.md |
| 08 | bulletproof-clock | Bulletproof Clock & External Clock Behavior (superseded by Task 19) | archived | 2025-11-25 | 2026-01-03 | archived/08-bulletproof-clock.md |
| 09 | soft-takeover-ux | Improve Soft Takeover and Visual Feedback | completed | 2025-11-25 | 2025-11-25 | completed/09-soft-takeover-ux.md |
| 10 | duopulse-v2 | DuoPulse v2: 2-Voice Percussive Sequencer with genre-aware swing, phrase structure, voice relationships | completed | 2025-12-01 | 2025-12-02 | completed/10-duopulse-v2.md |
| 11 | control-layout-fixes | Control Layout Fixes: mode persistence bug, FLUX/FUSE investigation, control reorganization | completed | 2025-12-03 | 2025-12-16 | completed/11-control-layout-fixes.md |
| 12 | aux-output-modes | Auxiliary Output Modes (CV Out 1) — superseded by task 15 (v4) | archived | 2025-12-16 | 2025-12-19 | archived/12-aux-output-modes.md |
| 13 | pulse-field-v3 | DuoPulse v3: Algorithmic Pulse Field — BROKEN/DRIFT controls, weighted pulse field, COUPLE interlock | done | 2025-12-16 | 2025-12-17T22:00:00Z | completed/13-pulse-field-v3.md |
| 14 | v3-ratchet-fixes | v3 Fixes: RATCHET parameter, DENSITY=0/DRIFT=0 bug fixes, header/cpp code quality | completed | 2025-12-17 | 2025-12-19 | completed/14-v3-ratchet-fixes.md |
| 15 | duopulse-v4 | DuoPulse v4: Complete architecture overhaul — 2D pattern field, hit budgets, Gumbel sampling, PUNCH/BUILD/BALANCE controls | completed | 2025-12-19 | 2025-12-29 | completed/15-duopulse-v4.md |
| 16 | v4-hardware-validation | DuoPulse v4 Hardware Validation & Debug — progressive feature testing, debug flags, hardware validation guide | completed | 2025-12-26 | 2025-12-29 | completed/16-v4-hardware-validation.md |
| 17 | runtime-logging | Runtime Logging System — compile-time + runtime configurable logging via USB serial (TRACE/DEBUG/INFO/WARN/ERROR) | completed | 2025-12-26 | 2025-12-26T18:00:00Z | completed/17-runtime-logging.md |
| 18 | level0-debug-fixes | Level 0 Debug Fixes — correct 4-on-floor pattern masks, add gate logging, fix aux bypass, event latch system, 10ms triggers, non-blocking ring buffer logger | completed | 2025-12-26 | 2025-12-27 | completed/18-level0-debug-fixes.md |
| 19 | exclusive-external-clock | Exclusive External Clock Mode — simplify clock behavior: external clock disables internal Metro, steps only on rising edges, no timeout fallback | completed | 2025-12-27 | 2025-12-27 | completed/19-exclusive-external-clock.md |
| 20 | level1-archetype-debug | Level 1 Archetype Pattern Debug — diagnose why Field X/Y knobs don't affect pattern at DEBUG_FEATURE_LEVEL 1 | completed | 2025-12-28 | 2025-12-28 | completed/20-level1-archetype-debug.md |
| 21 | musicality-improvements | Musicality Improvements — archetype weight retuning, velocity contrast, BUILD phases, Euclidean blend | completed | 2025-12-28 | 2025-12-31 | completed/21-musicality/21-04-implementation.md |
| 22 | control-simplification | Control Simplification — remove reset mode, auto-derive phrase, extend balance range, simplify coupling | completed | 2025-12-29 | 2026-01-01 | completed/22-control-simplification.md |
| 23 | immediate-field-updates | Immediate Field Updates — Field X/Y changes should update pattern immediately without waiting for reset | done | 2025-12-29 | 2026-01-01 | completed/23-immediate-field-updates.md |
| 24 | power-on-behavior | Power-On Behavior — define boot defaults, reset to musical defaults on power cycle, read performance knobs on startup | completed | 2025-12-29 | 2026-01-03 | completed/24-power-on-behavior.md |
| 25 | voice-control-redesign | VOICE Control Redesign — superseded by V5 COMPLEMENT design (Task 30) | archived | 2025-12-30 | 2026-01-04 | archived/25-voice-control-redesign.md |
| 26 | fix-tasks-22-24-bugs | Fix Tasks 22-24 Implementation Bugs — phrase length auto-derivation, freed controls still in main loop, inconsistent defaults | completed | 2026-01-03 | 2026-01-03 | completed/26-fix-tasks-22-24-bugs.md |
| 27 | v5-control-renaming | V5 Control Renaming and Zero Shift Layers — rename parameters, eliminate shift layers, establish V5 vocabulary | completed | 2026-01-04 | 2026-01-04 | completed/27-v5-control-renaming.md |
| 28 | v5-shape-algorithm | V5 SHAPE Parameter: 3-Way Blending — implement 3-zone blending with crossfade transitions | completed | 2026-01-04 | 2026-01-04 | completed/28-v5-shape-algorithm.md |
| 29 | v5-axis-biasing | V5 AXIS X/Y: Bidirectional Biasing — implement bidirectional biasing with broken mode | completed | 2026-01-04 | 2026-01-04 | completed/29-v5-axis-biasing.md |
| 30 | v5-voice-complement | V5 Voice COMPLEMENT Relationship — implement gap-filling shimmer with DRIFT placement | completed | 2026-01-04 | 2026-01-04 | completed/30-v5-voice-complement.md |
| 31 | v5-hat-burst | V5 Hat Burst: Pattern-Aware Fill Triggers — implement hat burst during fills with velocity ducking | completed | 2026-01-04 | 2026-01-04 | completed/31-v5-hat-burst.md |
| 32 | v5-aux-gesture | V5 Hold+Switch Gesture for AUX Mode — implement secret gesture for HAT/FILL GATE selection (persistent) | completed | 2026-01-04 | 2026-01-04 | completed/32-v5-aux-gesture.md |
| 33 | v5-boot-aux-mode | ~~V5 Boot-Time Pattern Length~~ → V5 Boot-Time AUX Mode — boot Hold+Switch sets persistent AUX mode | completed | 2026-01-04 | 2026-01-04 | completed/33-v5-boot-aux-mode.md |
| 34 | v5-led-feedback | V5 LED Feedback System Update — implement 5-layer priority LED feedback | completed | 2026-01-04 | 2026-01-04 | completed/34-v5-led-feedback.md |
| 35 | v5-accent-velocity | V5 ACCENT Parameter: Musical Weight Velocity — replace PUNCH with position-aware dynamics | completed | 2026-01-04 | 2026-01-04 | completed/35-v5-accent-velocity.md |
| 37 | v5-code-review-fixes | V5 Code Review Critical Fixes — buffer bounds, null checks, invariant violations from pre-merge review | completed | 2026-01-04 | 2026-01-04 | completed/37-v5-code-review-fixes.md |
| 38 | v5-pattern-viz-cpp | V5 C++ Pattern Visualization Test Tool — deterministic pattern output for test validation | completed | 2026-01-06 | 2026-01-06 | completed/38-v5-pattern-viz-cpp.md |
| 39 | shape-hit-budget | SHAPE-Modulated Hit Budget — vary anchor/shimmer density based on SHAPE zone for pattern variation | completed | 2026-01-06 | 2026-01-06 | completed/39-shape-hit-budget.md |
| 40 | v5-pattern-generator | V5 Pattern Generator Extraction & Spec Alignment — firmware now uses V5 SHAPE-based generation, archetype system deprecated | completed | 2026-01-06 | 2026-01-06 | completed/40-v5-pattern-generator.md |
| 41 | v5-shape-budget-fix | V5 SHAPE Budget Fix: Zone Boundaries and Anchor/Shimmer Ratio — fix 0.33/0.66→0.30/0.70, separate multipliers | completed | 2026-01-06 | 2026-01-06 | completed/41-v5-shape-budget-fix.md |
| 42 | v4-dead-code-cleanup | V4 Dead Code Cleanup: Remove Deprecated Archetype System — delete ~2,630 lines of unused code | completed | 2026-01-06 | 2026-01-07 | completed/42-v4-dead-code-cleanup.md |
| 43 | v5-shimmer-seed-variation | V5 Shimmer Seed Variation: Fix pattern convergence bug | completed | 2026-01-07 | 2026-01-07 | completed/43-v5-shimmer-seed-variation.md |
| 44 | v5-anchor-seed-variation | V5 Anchor Seed Variation: Fix zero pattern diversity across seeds | pending | 2026-01-07 | 2026-01-07 | active/44-v5-anchor-seed-variation.md |
