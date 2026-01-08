# Expressiveness Evaluation Report

This report evaluates pattern expressiveness across parameter sweeps.

## Executive Summary

- **Overall Seed Variation Score**: 42.4%
- **V1 (Anchor) Expressiveness**: 32.7%
- **V2 (Shimmer) Expressiveness**: 94.6%
- **AUX Expressiveness**: 0.0%

**V2 Variation Test**: PASS

## Seed Variation Tests

| Energy | Shape | Drift | V1 Unique | V2 Unique | AUX Unique | V2 Score |
|--------|-------|-------|-----------|-----------|------------|----------|
| 0.20 | 0.00 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.00 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.00 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.00 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.15 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.15 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.15 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.15 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.30 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.30 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.30 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.30 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.50 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.50 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.50 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.50 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.70 | 0.00 | 1/8 | 7/8 | 1/8 | 86% OK |
| 0.20 | 0.70 | 0.25 | 1/8 | 7/8 | 1/8 | 86% OK |
| 0.20 | 0.70 | 0.50 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.70 | 0.75 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.85 | 0.00 | 1/8 | 4/8 | 1/8 | 43% LOW |
| 0.20 | 0.85 | 0.25 | 1/8 | 4/8 | 1/8 | 43% LOW |
| 0.20 | 0.85 | 0.50 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 0.85 | 0.75 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 1.00 | 0.00 | 1/8 | 7/8 | 1/8 | 86% OK |
| 0.20 | 1.00 | 0.25 | 1/8 | 7/8 | 1/8 | 86% OK |
| 0.20 | 1.00 | 0.50 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.20 | 1.00 | 0.75 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.00 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.00 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.00 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.00 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.15 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.15 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.15 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.15 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.30 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.30 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.30 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.30 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.50 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.50 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.50 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.50 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.70 | 0.00 | 1/8 | 4/8 | 1/8 | 43% LOW |
| 0.40 | 0.70 | 0.25 | 1/8 | 4/8 | 1/8 | 43% LOW |
| 0.40 | 0.70 | 0.50 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.70 | 0.75 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.85 | 0.00 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.85 | 0.25 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.85 | 0.50 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 0.85 | 0.75 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 1.00 | 0.00 | 1/8 | 4/8 | 1/8 | 43% LOW |
| 0.40 | 1.00 | 0.25 | 1/8 | 4/8 | 1/8 | 43% LOW |
| 0.40 | 1.00 | 0.50 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.40 | 1.00 | 0.75 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.00 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.00 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.00 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.00 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.15 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.15 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.15 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.15 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.30 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.30 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.30 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.30 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.50 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.50 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.50 | 0.50 | 5/8 | 7/8 | 1/8 | 86% OK |
| 0.60 | 0.50 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.70 | 0.00 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.70 | 0.25 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.70 | 0.50 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.70 | 0.75 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.85 | 0.00 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.85 | 0.25 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.85 | 0.50 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 0.85 | 0.75 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 1.00 | 0.00 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 1.00 | 0.25 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 1.00 | 0.50 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.60 | 1.00 | 0.75 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.00 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.00 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.00 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.00 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.15 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.15 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.15 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.15 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.30 | 0.00 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.30 | 0.25 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.30 | 0.50 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.30 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.50 | 0.00 | 5/8 | 7/8 | 1/8 | 86% OK |
| 0.80 | 0.50 | 0.25 | 5/8 | 7/8 | 1/8 | 86% OK |
| 0.80 | 0.50 | 0.50 | 5/8 | 7/8 | 1/8 | 86% OK |
| 0.80 | 0.50 | 0.75 | 5/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.70 | 0.00 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.70 | 0.25 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.70 | 0.50 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.70 | 0.75 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.85 | 0.00 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.85 | 0.25 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.85 | 0.50 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 0.85 | 0.75 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 1.00 | 0.00 | 1/8 | 3/8 | 1/8 | 29% LOW |
| 0.80 | 1.00 | 0.25 | 1/8 | 3/8 | 1/8 | 29% LOW |
| 0.80 | 1.00 | 0.50 | 1/8 | 8/8 | 1/8 | 100% OK |
| 0.80 | 1.00 | 0.75 | 1/8 | 8/8 | 1/8 | 100% OK |

## Low Variation Zones (Problem Areas)

These parameter regions have insufficient seed variation:

### WARNING: E=0.20 S=0.85 D=0.00

- V1 Score: 0%
- V2 Score: 43%
- AUX Score: 0%
- Issue: V2 only has 4 unique patterns from 8 seeds

### WARNING: E=0.20 S=0.85 D=0.25

- V1 Score: 0%
- V2 Score: 43%
- AUX Score: 0%
- Issue: V2 only has 4 unique patterns from 8 seeds

### WARNING: E=0.40 S=0.70 D=0.00

- V1 Score: 0%
- V2 Score: 43%
- AUX Score: 0%
- Issue: V2 only has 4 unique patterns from 8 seeds

### WARNING: E=0.40 S=0.70 D=0.25

- V1 Score: 0%
- V2 Score: 43%
- AUX Score: 0%
- Issue: V2 only has 4 unique patterns from 8 seeds

### WARNING: E=0.40 S=1.00 D=0.00

- V1 Score: 0%
- V2 Score: 43%
- AUX Score: 0%
- Issue: V2 only has 4 unique patterns from 8 seeds

### WARNING: E=0.40 S=1.00 D=0.25

- V1 Score: 0%
- V2 Score: 43%
- AUX Score: 0%
- Issue: V2 only has 4 unique patterns from 8 seeds

### WARNING: E=0.80 S=1.00 D=0.00

- V1 Score: 0%
- V2 Score: 29%
- AUX Score: 0%
- Issue: V2 only has 3 unique patterns from 8 seeds

### WARNING: E=0.80 S=1.00 D=0.25

- V1 Score: 0%
- V2 Score: 29%
- AUX Score: 0%
- Issue: V2 only has 3 unique patterns from 8 seeds

## Convergence Patterns (Identical Outputs)

These specific patterns are produced by multiple seeds:

- **0x40400040** produced by 2 seeds at E=0.20 S=0.70 D=0.00
  - Seeds: 0xABCD1234, 0xBEEFCAFE
- **0x40400040** produced by 2 seeds at E=0.20 S=0.70 D=0.25
  - Seeds: 0xABCD1234, 0xBEEFCAFE
- **0x02020202** produced by 2 seeds at E=0.20 S=0.85 D=0.00
  - Seeds: 0xDEADBEEF, 0xCAFEBABE
- **0x04040404** produced by 2 seeds at E=0.20 S=0.85 D=0.00
  - Seeds: 0x12345678, 0xFEEDFACE
- **0x40404040** produced by 3 seeds at E=0.20 S=0.85 D=0.00
  - Seeds: 0xABCD1234, 0x87654321, 0xBEEFCAFE
- **0x02020202** produced by 2 seeds at E=0.20 S=0.85 D=0.25
  - Seeds: 0xDEADBEEF, 0xCAFEBABE
- **0x04040404** produced by 2 seeds at E=0.20 S=0.85 D=0.25
  - Seeds: 0x12345678, 0xFEEDFACE
- **0x40404040** produced by 3 seeds at E=0.20 S=0.85 D=0.25
  - Seeds: 0xABCD1234, 0x87654321, 0xBEEFCAFE
- **0x40400040** produced by 2 seeds at E=0.20 S=1.00 D=0.00
  - Seeds: 0xABCD1234, 0xBEEFCAFE
- **0x40400040** produced by 2 seeds at E=0.20 S=1.00 D=0.25
  - Seeds: 0xABCD1234, 0xBEEFCAFE

## SHAPE Parameter Sweep

| SHAPE | V1 Hits | V2 Hits | Regularity | Syncopation |
|-------|---------|---------|------------|-------------|
| 0.00 | 8 | 3 | 0.79 | 0% |
| 0.15 | 8 | 3 | 0.79 | 0% |
| 0.30 | 8 | 3 | 0.79 | 0% |
| 0.50 | 8 | 4 | 0.79 | 0% |
| 0.70 | 8 | 4 | 1.00 | 0% |
| 0.85 | 8 | 4 | 1.00 | 0% |
| 1.00 | 8 | 4 | 1.00 | 0% |

## ENERGY Parameter Sweep

| ENERGY | V1 Hits | V2 Hits | V1 Mask | V2 Mask |
|--------|---------|---------|---------|---------|
| 0.00 | 0 | 1 | 0x00000000 | 0x00000400 |
| 0.20 | 4 | 2 | 0x40404001 | 0x00000804 |
| 0.40 | 4 | 3 | 0x40404001 | 0x00080804 |
| 0.60 | 8 | 5 | 0x44444405 | 0x81001110 |
| 0.80 | 8 | 6 | 0x44444405 | 0x81011110 |
| 1.00 | 8 | 8 | 0x44444405 | 0x88888890 |

## Future Work: Improving V1 Variation to 50%+

Current V1 (Anchor) variation is **33%**, meeting the 25% minimum target but below the ideal 50% threshold. Here are approaches to consider for future improvement:

### High-Impact Approaches

1. **Increase Rotation Range**
   - Current: `maxRotation = halfLength / 4` (8 positions for 32-step)
   - Proposed: `maxRotation = halfLength / 2` (16 positions)
   - Risk: May feel less stable at very low SHAPE; consider SHAPE-scaled range

2. **Seed-Based Hit Budget Variation**
   - Add ±1 to anchor hit count based on seed
   - More hits = more possible arrangements
   - Example: `anchorHits += (HashToInt(seed, 3000) % 3) - 1`

3. **Micro-Displacement**
   - Probabilistically shift individual hits by ±1 step
   - Preserve step 0, only displace others
   - Would create subtle timing variations within the same "feel"

### Medium-Impact Approaches

4. **Zone-Specific Strategies**
   - SHAPE 0.0-0.3: Increase rotation range (stable patterns need more help)
   - SHAPE 0.3-0.7: Use current approach (natural syncopation)
   - SHAPE 0.7+: Rely on weight randomness (wild patterns)

5. **Metrically-Aware Swapping**
   - Swap hits between metrically equivalent positions
   - Example: Swap step 4 ↔ step 12 (both quarter notes)
   - Preserves musical hierarchy while adding variation

6. **Weight Injection Points**
   - Add high-weight "spice" at random seed-based positions
   - Occasionally pulls a hit to an unexpected place
   - More aggressive than current additive noise

### Low-Risk Improvements

7. **Per-Half Different Rotation**
   - Currently both halves of 64-step pattern use same rotation
   - Use independent rotation for second half
   - Doubles effective variation for long patterns

8. **DRIFT-Influenced Anchor Variation**
   - Currently DRIFT only affects shimmer placement
   - Could add DRIFT-scaled anchor variation
   - Gives performer control over expressiveness

### Analysis Notes

Looking at the test data:
- V1 achieves 5/8 unique patterns at low SHAPE (good!)
- V1 drops to 1/8 unique at high SHAPE (>0.7)
- High SHAPE disables rotation (`shape < 0.7f` guard)

**Quick Win**: Remove or raise the SHAPE threshold for rotation. High SHAPE patterns are already chaotic, so rotation would add minimal musical value but would increase the metric.

## Recommendations

No critical issues found. Pattern expressiveness is adequate.

Consider monitoring these metrics after future changes:
- V2 variation score should stay above 50%
- V1 variation score target: 50% (currently 33%)
- Syncopation should increase with SHAPE
- Hit counts should scale with ENERGY

---
*Generated by evaluate-expressiveness.py*