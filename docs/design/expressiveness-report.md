# Expressiveness Evaluation Report

This report evaluates pattern expressiveness across parameter sweeps.

## Executive Summary

- **Overall Seed Variation Score**: 8.3%
- **V1 (Anchor) Expressiveness**: 0.0%
- **V2 (Shimmer) Expressiveness**: 25.0%
- **AUX Expressiveness**: 0.0%

**V2 Variation Test**: FAIL

> CRITICAL: Shimmer (V2) patterns do not vary sufficiently across seeds.
> This means the reseed gesture feels 'broken' to users.

## Seed Variation Tests

| Energy | Shape | Drift | V1 Unique | V2 Unique | AUX Unique | V2 Score |
|--------|-------|-------|-----------|-----------|------------|----------|
| 0.30 | 0.15 | 0.00 | 1/4 | 1/4 | 1/4 | 0% LOW |
| 0.30 | 0.15 | 0.50 | 1/4 | 1/4 | 1/4 | 0% LOW |
| 0.30 | 0.50 | 0.00 | 1/4 | 1/4 | 1/4 | 0% LOW |
| 0.30 | 0.50 | 0.50 | 1/4 | 4/4 | 1/4 | 100% OK |
| 0.30 | 0.85 | 0.00 | 1/4 | 1/4 | 1/4 | 0% LOW |
| 0.30 | 0.85 | 0.50 | 1/4 | 4/4 | 1/4 | 100% OK |
| 0.60 | 0.15 | 0.00 | 1/4 | 1/4 | 1/4 | 0% LOW |
| 0.60 | 0.15 | 0.50 | 1/4 | 1/4 | 1/4 | 0% LOW |
| 0.60 | 0.50 | 0.00 | 1/4 | 1/4 | 1/4 | 0% LOW |
| 0.60 | 0.50 | 0.50 | 1/4 | 1/4 | 1/4 | 0% LOW |
| 0.60 | 0.85 | 0.00 | 1/4 | 1/4 | 1/4 | 0% LOW |
| 0.60 | 0.85 | 0.50 | 1/4 | 4/4 | 1/4 | 100% OK |

## Low Variation Zones (Problem Areas)

These parameter regions have insufficient seed variation:

### CRITICAL: E=0.30 S=0.15 D=0.00

- V1 Score: 0%
- V2 Score: 0%
- AUX Score: 0%
- Issue: V2 only has 1 unique patterns from 4 seeds

### CRITICAL: E=0.30 S=0.15 D=0.50

- V1 Score: 0%
- V2 Score: 0%
- AUX Score: 0%
- Issue: V2 only has 1 unique patterns from 4 seeds

### CRITICAL: E=0.30 S=0.50 D=0.00

- V1 Score: 0%
- V2 Score: 0%
- AUX Score: 0%
- Issue: V2 only has 1 unique patterns from 4 seeds

### CRITICAL: E=0.30 S=0.85 D=0.00

- V1 Score: 0%
- V2 Score: 0%
- AUX Score: 0%
- Issue: V2 only has 1 unique patterns from 4 seeds

### CRITICAL: E=0.60 S=0.15 D=0.00

- V1 Score: 0%
- V2 Score: 0%
- AUX Score: 0%
- Issue: V2 only has 1 unique patterns from 4 seeds

### CRITICAL: E=0.60 S=0.15 D=0.50

- V1 Score: 0%
- V2 Score: 0%
- AUX Score: 0%
- Issue: V2 only has 1 unique patterns from 4 seeds

### CRITICAL: E=0.60 S=0.50 D=0.00

- V1 Score: 0%
- V2 Score: 0%
- AUX Score: 0%
- Issue: V2 only has 1 unique patterns from 4 seeds

### CRITICAL: E=0.60 S=0.50 D=0.50

- V1 Score: 0%
- V2 Score: 0%
- AUX Score: 0%
- Issue: V2 only has 1 unique patterns from 4 seeds

### CRITICAL: E=0.60 S=0.85 D=0.00

- V1 Score: 0%
- V2 Score: 0%
- AUX Score: 0%
- Issue: V2 only has 1 unique patterns from 4 seeds

## Convergence Patterns (Identical Outputs)

These specific patterns are produced by multiple seeds:

- **0x00020202** produced by 4 seeds at E=0.30 S=0.15 D=0.00
  - Seeds: 0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCD1234
- **0x00101010** produced by 4 seeds at E=0.30 S=0.15 D=0.50
  - Seeds: 0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCD1234
- **0x02020202** produced by 4 seeds at E=0.30 S=0.50 D=0.00
  - Seeds: 0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCD1234
- **0x02020202** produced by 4 seeds at E=0.30 S=0.85 D=0.00
  - Seeds: 0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCD1234
- **0x00022222** produced by 4 seeds at E=0.60 S=0.15 D=0.00
  - Seeds: 0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCD1234
- **0x00044444** produced by 4 seeds at E=0.60 S=0.15 D=0.50
  - Seeds: 0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCD1234
- **0x00222222** produced by 4 seeds at E=0.60 S=0.50 D=0.00
  - Seeds: 0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCD1234
- **0x00888888** produced by 4 seeds at E=0.60 S=0.50 D=0.50
  - Seeds: 0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCD1234
- **0x00222222** produced by 4 seeds at E=0.60 S=0.85 D=0.00
  - Seeds: 0xDEADBEEF, 0xCAFEBABE, 0x12345678, 0xABCD1234

## SHAPE Parameter Sweep

| SHAPE | V1 Hits | V2 Hits | Regularity | Syncopation |
|-------|---------|---------|------------|-------------|
| 0.00 | 8 | 3 | 1.00 | 0% |
| 0.15 | 8 | 3 | 1.00 | 0% |
| 0.30 | 8 | 3 | 1.00 | 0% |
| 0.50 | 8 | 4 | 1.00 | 0% |
| 0.70 | 8 | 4 | 1.00 | 0% |
| 0.85 | 8 | 4 | 1.00 | 0% |
| 1.00 | 8 | 4 | 1.00 | 0% |

## ENERGY Parameter Sweep

| ENERGY | V1 Hits | V2 Hits | V1 Mask | V2 Mask |
|--------|---------|---------|---------|---------|
| 0.00 | 0 | 1 | 0x00000000 | 0x00000001 |
| 0.20 | 4 | 2 | 0x01010101 | 0x00000202 |
| 0.40 | 4 | 3 | 0x01010101 | 0x00020202 |
| 0.60 | 8 | 5 | 0x11111111 | 0x00022222 |
| 0.80 | 8 | 6 | 0x11111111 | 0x00222222 |
| 1.00 | 8 | 8 | 0x11111111 | 0x22222222 |

## Recommendations

### Priority 1: Fix V2 (Shimmer) Variation

The shimmer voice does not vary enough with different seeds. Recommended fixes:

1. **Inject seed into even-spacing placement** (Low risk)
   - Add seed-based micro-jitter to `PlaceEvenlySpaced()`
   - Preserves overall structure while adding variation

2. **Raise default DRIFT from 0.0 to 0.25** (Medium risk)
   - Enables seed-sensitive weighted placement
   - Matches user expectation that reseed changes pattern

3. **Use Gumbel selection within gaps** (Structural fix)
   - Apply same selection mechanism to shimmer as anchor
   - Most comprehensive solution


---
*Generated by evaluate-expressiveness.py*