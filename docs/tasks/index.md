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
| 08 | bulletproof-clock | Bulletproof Clock & External Clock Behavior (updated for v4) | pending | 2025-11-25 | 2025-12-19 | backlog/08-bulletproof-clock.md |
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
| 22 | control-simplification | Control Simplification — remove reset mode, auto-derive phrase, extend balance range, simplify coupling | ready | 2025-12-29 | 2025-12-30 | active/22-control-simplification.md |
| 23 | immediate-field-updates | Immediate Field Updates — Field X/Y changes should update pattern immediately without waiting for reset | pending | 2025-12-29 | 2025-12-29 | active/23-immediate-field-updates.md |
| 24 | power-on-behavior | Power-On Behavior — define boot defaults, reset to musical defaults on power cycle, read performance knobs on startup | pending | 2025-12-29 | 2025-12-30 | active/24-power-on-behavior.md |
| 25 | voice-control-redesign | VOICE Control Redesign — merge balance + coupling into single 0-100% control (anchor solo → shimmer solo) | backlog | 2025-12-30 | 2025-12-30 | backlog/25-voice-control-redesign.md |
