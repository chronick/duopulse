# Feedback Log: Shimmer Variation Design

## Round 1: Clarification Needed

**Date**: 2026-01-07
**Status**: Awaiting response

### Question 1: DRIFT Semantics

The existing critique and my analysis have surfaced a fundamental semantic question:

**What is the intended relationship between DRIFT and seed?**

| Model | Description | DRIFT=0 Behavior | DRIFT=1 Behavior |
|-------|-------------|------------------|------------------|
| **A** | DRIFT controls "seed importance" | Seed ignored | Seed fully determines pattern |
| **B** | DRIFT controls "evolution rate" | Seed sets locked pattern | Pattern varies each phrase |

**Evidence for Model A** (current implementation):
- At DRIFT=0, `PlaceEvenlySpaced()` is called without seed
- Comment in code: "0% locked pattern"

**Evidence for Model B** (spec language):
- Section 1.1: "Same settings + seed = identical output" implies different seed = different output
- Section 4.3: "DRIFT: Evolution: locked -> varies each phrase"

If Model B is correct, the current behavior is a **bug**: seed should affect shimmer even at DRIFT=0.

**Please clarify which model is intended.**

---

### Question 2: Default DRIFT Change

The proposed fix includes raising default DRIFT from 0.0 to 0.25.

**Trade-off**:
- Pro: Immediate seed variation out of the box
- Con: Breaking change for existing users/patches

**Is changing the default DRIFT acceptable?**

Options:
1. Yes, raise to 0.25 and document the change
2. No, keep 0.0 but fix even-spacing to use seed anyway
3. Other (please specify)

---

### Question 3: Variation Degree

How much variation is "enough"?

- **Subtle**: Phase offset (hits shift by 1-2 steps within gaps)
- **Moderate**: Some hits use weighted placement instead of even spacing
- **Dramatic**: Full Gumbel selection within gaps (like anchor)

The proposed Phase 1 fix provides **subtle** variation. Is this sufficient, or should we aim for moderate/dramatic from the start?

---

## User Response

(To be filled in after user responds)

**Q1 Answer**:
**Q2 Answer**:
**Q3 Answer**:

---

## Round 2: (Reserved)

(Will be added if needed based on Round 1 responses)
