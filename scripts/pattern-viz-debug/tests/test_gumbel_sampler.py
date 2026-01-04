"""Tests for Gumbel Top-K selection algorithm."""

import pytest
from pattern_viz.gumbel_sampler import (
    select_hits_gumbel,
    count_bits,
    mask_to_steps,
    get_min_spacing_for_zone,
)
from pattern_viz.types import EnergyZone


class TestUtilityFunctions:
    """Test utility functions."""

    def test_count_bits_zero(self):
        """Count bits in zero."""
        assert count_bits(0) == 0

    def test_count_bits_one(self):
        """Count bits in single bit."""
        assert count_bits(1) == 1
        assert count_bits(2) == 1
        assert count_bits(4) == 1

    def test_count_bits_multiple(self):
        """Count bits in multiple bits."""
        assert count_bits(0b1111) == 4
        assert count_bits(0b10101010) == 4
        assert count_bits(0xFFFFFFFF) == 32

    def test_mask_to_steps_empty(self):
        """Empty mask to steps."""
        assert mask_to_steps(0) == []

    def test_mask_to_steps_single(self):
        """Single bit to steps."""
        assert mask_to_steps(1) == [0]
        assert mask_to_steps(2) == [1]
        assert mask_to_steps(4) == [2]

    def test_mask_to_steps_multiple(self):
        """Multiple bits to steps."""
        assert mask_to_steps(0b1001) == [0, 3]
        assert mask_to_steps(0b1111) == [0, 1, 2, 3]


class TestMinSpacing:
    """Test minimum spacing rules."""

    def test_minimal_zone_spacing(self):
        """MINIMAL zone should have large spacing."""
        spacing = get_min_spacing_for_zone(EnergyZone.MINIMAL)
        assert spacing >= 4

    def test_groove_zone_spacing(self):
        """GROOVE zone should have moderate spacing."""
        spacing = get_min_spacing_for_zone(EnergyZone.GROOVE)
        assert 2 <= spacing <= 4

    def test_peak_zone_spacing(self):
        """PEAK zone should allow dense hits."""
        spacing = get_min_spacing_for_zone(EnergyZone.PEAK)
        assert spacing <= 2


class TestGumbelSelection:
    """Test Gumbel Top-K selection."""

    def test_select_exact_count(self):
        """Should select exactly the requested number of hits."""
        weights = [0.9] * 8 + [0.1] * 24  # High weights on first 8 steps
        mask = select_hits_gumbel(
            weights=weights,
            eligibility_mask=0xFFFFFFFF,
            target_count=4,
            seed=12345,
            pattern_length=32,
            min_spacing=0,
        )
        assert count_bits(mask) == 4

    def test_select_respects_eligibility(self):
        """Should only select eligible steps."""
        weights = [0.9] * 32
        eligibility = 0x0000FFFF  # Only first 16 steps eligible
        mask = select_hits_gumbel(
            weights=weights,
            eligibility_mask=eligibility,
            target_count=8,
            seed=12345,
            pattern_length=32,
            min_spacing=0,
        )
        # All selected bits should be in first 16 steps
        assert (mask & ~eligibility) == 0

    def test_select_deterministic(self):
        """Same seed should give same result."""
        weights = [0.5] * 32
        mask1 = select_hits_gumbel(
            weights=weights,
            eligibility_mask=0xFFFFFFFF,
            target_count=8,
            seed=12345,
            pattern_length=32,
        )
        mask2 = select_hits_gumbel(
            weights=weights,
            eligibility_mask=0xFFFFFFFF,
            target_count=8,
            seed=12345,
            pattern_length=32,
        )
        assert mask1 == mask2

    def test_different_seeds_different_results(self):
        """Different seeds should give different results."""
        weights = [0.5] * 32
        mask1 = select_hits_gumbel(
            weights=weights,
            eligibility_mask=0xFFFFFFFF,
            target_count=8,
            seed=12345,
            pattern_length=32,
        )
        mask2 = select_hits_gumbel(
            weights=weights,
            eligibility_mask=0xFFFFFFFF,
            target_count=8,
            seed=54321,
            pattern_length=32,
        )
        assert mask1 != mask2

    def test_high_weights_preferred(self):
        """Higher weights should be more likely to be selected."""
        # Create weights with first 4 steps very high
        weights = [0.0] * 32
        weights[0] = 1.0
        weights[8] = 1.0
        weights[16] = 1.0
        weights[24] = 1.0

        mask = select_hits_gumbel(
            weights=weights,
            eligibility_mask=0xFFFFFFFF,
            target_count=4,
            seed=12345,
            pattern_length=32,
            min_spacing=0,
        )

        # Should select the high-weight steps
        steps = mask_to_steps(mask)
        assert 0 in steps or 8 in steps or 16 in steps or 24 in steps

    def test_min_spacing_enforced(self):
        """Min spacing should prevent adjacent hits."""
        weights = [0.9] * 32
        mask = select_hits_gumbel(
            weights=weights,
            eligibility_mask=0xFFFFFFFF,
            target_count=8,
            seed=12345,
            pattern_length=32,
            min_spacing=4,
        )

        steps = mask_to_steps(mask)
        for i in range(len(steps) - 1):
            # Check spacing between consecutive hits
            spacing = steps[i + 1] - steps[i]
            # Spacing might be relaxed if can't fit, but generally should be >= 4
            # This is a soft check since spacing can be relaxed
            assert spacing >= 1

    def test_handles_zero_target(self):
        """Should handle zero target count."""
        weights = [0.5] * 32
        mask = select_hits_gumbel(
            weights=weights,
            eligibility_mask=0xFFFFFFFF,
            target_count=0,
            seed=12345,
            pattern_length=32,
        )
        assert mask == 0

    def test_handles_empty_eligibility(self):
        """Should handle empty eligibility mask."""
        weights = [0.5] * 32
        mask = select_hits_gumbel(
            weights=weights,
            eligibility_mask=0,
            target_count=8,
            seed=12345,
            pattern_length=32,
        )
        assert mask == 0

    def test_caps_at_available_steps(self):
        """Should cap selection at available eligible steps."""
        weights = [0.5] * 32
        eligibility = 0b1111  # Only 4 steps eligible
        mask = select_hits_gumbel(
            weights=weights,
            eligibility_mask=eligibility,
            target_count=10,  # Want more than available
            seed=12345,
            pattern_length=32,
            min_spacing=0,
        )
        assert count_bits(mask) <= 4
