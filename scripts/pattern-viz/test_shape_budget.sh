#!/bin/bash
# Test SHAPE budget modulation (Task 39)
# Uses pattern_viz to compare hit counts at different SHAPE values

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PATTERN_VIZ="$PROJECT_DIR/build/pattern_viz"

# Check if pattern_viz exists
if [ ! -f "$PATTERN_VIZ" ]; then
    echo "Building pattern_viz..."
    (cd "$PROJECT_DIR" && make pattern-viz)
fi

echo "=== SHAPE Budget Modulation Test (Task 39) ==="
echo ""
echo "Testing hit counts at different SHAPE zones with same seed"
echo "Common: ENERGY=0.5, SEED=0xDEADBEEF, LENGTH=32"
echo ""

# Run each test case and capture V1 hit count
count_v1_hits() {
    local shape=$1
    "$PATTERN_VIZ" --energy=0.5 --shape="$shape" --seed=0xDEADBEEF 2>/dev/null | \
        awk '/^ *[0-9]+ +X/ {count++} END {print count+0}'
}

# Extract patterns as masks
get_v1_mask() {
    local shape=$1
    "$PATTERN_VIZ" --energy=0.5 --shape="$shape" --seed=0xDEADBEEF 2>/dev/null | \
        awk '/^ *[0-9]+ +X/ {printf "1"} /^ *[0-9]+ +\./ {printf "0"}'
}

stable_hits=$(count_v1_hits 0.15)
sync_hits=$(count_v1_hits 0.50)
wild_hits=$(count_v1_hits 0.85)

stable_mask=$(get_v1_mask 0.15)
sync_mask=$(get_v1_mask 0.50)
wild_mask=$(get_v1_mask 0.85)

echo "| Zone        | SHAPE | V1 Hits | Expected Mult |"
echo "|-------------|-------|---------|---------------|"
echo "| Stable      | 0.15  | $stable_hits       | 0.795x        |"
echo "| Syncopated  | 0.50  | $sync_hits       | 1.000x        |"
echo "| Wild        | 0.85  | $wild_hits       | 1.206x        |"
echo ""

# Validate results
echo "Validation:"
if [ "$stable_hits" -lt "$sync_hits" ]; then
    echo "  [PASS] Stable zone has fewer hits than Syncopated"
else
    echo "  [FAIL] Stable zone should have fewer hits than Syncopated ($stable_hits >= $sync_hits)"
fi

if [ "$wild_hits" -gt "$sync_hits" ]; then
    echo "  [PASS] Wild zone has more hits than Syncopated"
else
    echo "  [FAIL] Wild zone should have more hits than Syncopated ($wild_hits <= $sync_hits)"
fi

# Check if masks differ (breaks convergence)
if [ "$stable_mask" != "$sync_mask" ] || [ "$sync_mask" != "$wild_mask" ]; then
    echo "  [PASS] Different SHAPE values produce different patterns"
else
    echo "  [FAIL] All patterns are identical (convergence not broken)"
    echo "         Stable:     $stable_mask"
    echo "         Syncopated: $sync_mask"
    echo "         Wild:       $wild_mask"
fi

echo ""
echo "Raw V1 patterns (first 16 steps):"
echo "  Stable:     ${stable_mask:0:16}..."
echo "  Syncopated: ${sync_mask:0:16}..."
echo "  Wild:       ${wild_mask:0:16}..."
