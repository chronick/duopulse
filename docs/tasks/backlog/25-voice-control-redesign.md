# Task 25: VOICE Control Redesign

**Status**: BACKLOG
**Branch**: TBD
**Parent Task**: Task 22 (Control Simplification)
**Depends On**: Task 22 Phase B (Balance Range Extension) should ship first

---

## Problem Statement

Task 22 originally proposed merging Balance and Voice Coupling into a single "VOICE" control that provides full control over the anchor/shimmer relationship:

- **0%** = Anchor Solo (shimmer silent)
- **25%** = Anchor Lead (shimmer sparse)
- **50%** = Balanced
- **75%** = Call-Response (interlock behavior)
- **100%** = Shimmer Solo (anchor sparse)

This was deferred because it requires architectural changes to `HitBudget.cpp` that go beyond parameter tweaking.

---

## Why This Was Deferred

### Current Architecture Limitation

`HitBudget.cpp:101-125` computes shimmer budget as a ratio of anchor:

```cpp
int ComputeShimmerBudget(float energy, float balance, EnergyZone zone, int patternLength)
{
    int anchorBudget = ComputeAnchorBudget(energy, zone, patternLength);
    float shimmerRatio = 0.3f + balance * 0.7f;  // 30% to 100%
    int hits = static_cast<int>(anchorBudget * shimmerRatio + 0.5f);
    // ...
}
```

**Problems**:
1. Shimmer is always derived from anchor (can't be independent)
2. Anchor budget doesn't respond to balance at all
3. No way to make anchor sparse when shimmer is dominant

### Required Architectural Changes

To implement VOICE 0-100% = Anchor Solo → Shimmer Solo:

1. **New budget computation model**:
   - Total hit budget based on ENERGY
   - VOICE distributes budget between voices
   - At extremes, one voice gets near-zero

2. **Modify `ComputeAnchorBudget()`**:
   - Accept `voice` parameter
   - Reduce anchor when `voice > 0.9`

3. **Modify `ComputeShimmerBudget()`**:
   - Independent of anchor budget
   - Scale from 0% at `voice = 0` to 150%+ at `voice = 1.0`

4. **Guard rails for extremes**:
   - When VOICE > 90%: shimmer can take downbeat
   - When VOICE < 10%: shimmer truly silent
   - Ensure AUX voice behavior at extremes

---

## Proposed Implementation

### New Budget Model

```cpp
void ComputeVoiceBudgets(float energy, float voice, EnergyZone zone,
                         int patternLength, int& anchorBudget, int& shimmerBudget)
{
    // Total budget based on energy and zone
    int totalBudget = ComputeTotalBudget(energy, zone, patternLength);

    // VOICE distributes budget between voices
    // voice = 0.0: anchor gets 100%, shimmer gets 0%
    // voice = 0.5: anchor gets 60%, shimmer gets 40%
    // voice = 1.0: anchor gets 20%, shimmer gets 80%

    float anchorShare = 1.0f - (voice * 0.8f);  // 100% down to 20%
    float shimmerShare = voice * 0.8f;           // 0% up to 80%

    anchorBudget = static_cast<int>(totalBudget * anchorShare + 0.5f);
    shimmerBudget = static_cast<int>(totalBudget * shimmerShare + 0.5f);

    // Minimum 1 hit for non-silent voice
    if (voice > 0.1f && shimmerBudget < 1) shimmerBudget = 1;
    if (voice < 0.9f && anchorBudget < 1) anchorBudget = 1;
}
```

### VOICE Curve Behavior

| VOICE | Anchor Share | Shimmer Share | Musical Feel |
|-------|--------------|---------------|--------------|
| 0% | 100% | 0% | Kick only |
| 25% | 80% | 20% | Kick-heavy with sparse snare |
| 50% | 60% | 40% | Balanced groove |
| 75% | 40% | 60% | Snare-forward |
| 100% | 20% | 80% | Snare dominant, sparse kick |

### Coupling Integration

Instead of separate coupling modes, VOICE position implies relationship:

- **0-40%**: Independent (both fire freely)
- **40-60%**: Balanced with soft interlock
- **60-100%**: Shimmer fills anchor gaps (call-response)

---

## Files to Modify

- `src/Engine/HitBudget.h` - New `ComputeVoiceBudgets()` function
- `src/Engine/HitBudget.cpp` - Implement new budget model
- `src/Engine/ControlState.h` - Replace `balance` + `voiceCoupling` with `voice`
- `src/Engine/ControlProcessor.cpp` - New knob mapping
- `src/Engine/GuardRails.cpp` - Extreme VOICE guard rails
- `src/Engine/VoiceRelation.cpp` - Derive coupling from VOICE position

---

## Success Criteria

- [ ] VOICE 0% produces anchor-only patterns (shimmer truly silent)
- [ ] VOICE 100% produces shimmer-dominant patterns (anchor sparse)
- [ ] VOICE 50% produces balanced groove
- [ ] Smooth musical transition across VOICE range
- [ ] No separate coupling control needed
- [ ] AUX voice behaves sensibly at VOICE extremes

---

## Risks

1. **Behavioral change**: Users familiar with current balance may be surprised
2. **Complexity**: More parameters flowing through generation pipeline
3. **Testing**: Need extensive hardware validation for musical feel

---

## Prerequisites

1. Task 22 Phase B ships (extended balance range validates the direction)
2. User feedback on Phase B confirms desire for fuller control
3. Task 21 findings inform VOICE interaction with timing

---

## Notes

This is a "nice to have" enhancement. Task 22 Phase B (balance 0-150%) provides 80% of the benefit with minimal risk. Only pursue this if users specifically request anchor-solo / shimmer-solo capability.
