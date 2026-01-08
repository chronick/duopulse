# Codex Consultation: Shimmer Variation Design

**Date**: 2026-01-07

## Context

DuoPulse is a 2-voice algorithmic drum sequencer for Eurorack. Voice 1 (anchor) generates kick-like patterns; Voice 2 (shimmer) fills the gaps. The COMPLEMENT relationship ensures they never collide.

**Current Architecture**:
1. Anchor hits selected via Gumbel Top-K sampling (seed-dependent)
2. Shimmer fills gaps between anchor hits
3. DRIFT parameter (0-1) controls shimmer placement strategy:
   - DRIFT < 0.3: Even spacing (deterministic, ignores seed)
   - DRIFT 0.3-0.7: Weight-based selection (uses SHAPE weights)
   - DRIFT > 0.7: Seed-varied random

**The Problem**:
At moderate energy (GROOVE zone, ~35% energy), seed changes produce NO audible difference:
- Anchor converges to four-on-floor due to guard rails + 4-step minimum spacing
- Shimmer uses even spacing (DRIFT defaults to 0.0) which is purely deterministic
- Result: "Same settings, different seed = identical pattern" - violates user expectations

**Constraints**:
- Must NOT add complexity to the 4-knob control surface
- Must maintain musical coherence (patterns should feel groovy, not random)
- Must be real-time safe (no heap allocation in audio path)
- Must preserve determinism (same seed + settings = reproducible output)

**Current Direction (from design critique)**:
1. Inject seed-based micro-jitter into even-spacing algorithm
2. Raise default DRIFT from 0.0 to 0.25
3. Possibly use Gumbel selection within gaps instead of even spacing

## Request

Propose 2-3 alternative approaches to restoring shimmer variation at moderate energy. For each:
1. Describe the approach
2. Explain the trade-offs
3. When would this be the better choice?

Consider approaches that the existing analysis might have missed. The goal is to move from "One-Trick Pony" (simple but limited) to "Sweet Spot" (simple surface, deep control) on the expressiveness quadrant.
