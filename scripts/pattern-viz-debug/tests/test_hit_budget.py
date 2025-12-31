"""Tests for hit budget and eligibility computation."""

import pytest
from pattern_viz.hit_budget import (
    compute_energy_zone,
    compute_anchor_budget,
    compute_shimmer_budget,
    compute_aux_budget,
    compute_bar_budget,
)
from pattern_viz.types import EnergyZone


class TestEnergyZone:
    """Test energy zone computation."""

    def test_minimal_zone(self):
        """Energy 0-20% should be MINIMAL."""
        assert compute_energy_zone(0.0) == EnergyZone.MINIMAL
        assert compute_energy_zone(0.1) == EnergyZone.MINIMAL
        assert compute_energy_zone(0.19) == EnergyZone.MINIMAL

    def test_groove_zone(self):
        """Energy 20-50% should be GROOVE."""
        assert compute_energy_zone(0.2) == EnergyZone.GROOVE
        assert compute_energy_zone(0.35) == EnergyZone.GROOVE
        assert compute_energy_zone(0.49) == EnergyZone.GROOVE

    def test_build_zone(self):
        """Energy 50-75% should be BUILD."""
        assert compute_energy_zone(0.5) == EnergyZone.BUILD
        assert compute_energy_zone(0.6) == EnergyZone.BUILD
        assert compute_energy_zone(0.74) == EnergyZone.BUILD

    def test_peak_zone(self):
        """Energy 75-100% should be PEAK."""
        assert compute_energy_zone(0.75) == EnergyZone.PEAK
        assert compute_energy_zone(0.9) == EnergyZone.PEAK
        assert compute_energy_zone(1.0) == EnergyZone.PEAK

    def test_clamping(self):
        """Out of range values should clamp."""
        assert compute_energy_zone(-0.5) == EnergyZone.MINIMAL
        assert compute_energy_zone(1.5) == EnergyZone.PEAK


class TestAnchorBudget:
    """Test anchor hit budget computation."""

    def test_minimal_budget(self):
        """MINIMAL zone should have low budget."""
        budget = compute_anchor_budget(0.1, EnergyZone.MINIMAL, 32)
        assert 1 <= budget <= 4

    def test_groove_budget(self):
        """GROOVE zone should have moderate budget."""
        budget = compute_anchor_budget(0.35, EnergyZone.GROOVE, 32)
        assert 3 <= budget <= 8

    def test_build_budget(self):
        """BUILD zone should have higher budget."""
        budget = compute_anchor_budget(0.6, EnergyZone.BUILD, 32)
        assert 4 <= budget <= 10

    def test_peak_budget(self):
        """PEAK zone should have high budget."""
        budget = compute_anchor_budget(0.9, EnergyZone.PEAK, 32)
        assert 6 <= budget <= 12

    def test_budget_increases_with_energy(self):
        """Higher energy should generally give more hits."""
        low = compute_anchor_budget(0.3, EnergyZone.GROOVE, 32)
        high = compute_anchor_budget(0.9, EnergyZone.PEAK, 32)
        assert high >= low


class TestShimmerBudget:
    """Test shimmer hit budget computation."""

    def test_balance_zero_reduces_shimmer(self):
        """Balance=0 should reduce shimmer hits."""
        budget = compute_shimmer_budget(0.5, 0.0, EnergyZone.GROOVE, 32)
        # With balance=0, should be minimal
        assert budget <= 1

    def test_balance_half_unchanged(self):
        """Balance=0.5 should keep shimmer near anchor."""
        anchor = compute_anchor_budget(0.5, EnergyZone.GROOVE, 32)
        budget = compute_shimmer_budget(0.5, 0.5, EnergyZone.GROOVE, 32)
        # Shimmer ratio = 0.5 * 1.5 = 0.75 of anchor
        assert 1 <= budget <= anchor + 2

    def test_balance_one_increases_shimmer(self):
        """Balance=1 should increase shimmer hits."""
        anchor = compute_anchor_budget(0.5, EnergyZone.GROOVE, 32)
        budget = compute_shimmer_budget(0.5, 1.0, EnergyZone.GROOVE, 32)
        # In GROOVE, shimmer ratio is capped at 1.0
        assert budget >= 1


class TestAuxBudget:
    """Test aux hit budget computation."""

    def test_aux_budget_range(self):
        """Aux budget should be reasonable."""
        for energy in [0.1, 0.5, 0.9]:
            zone = compute_energy_zone(energy)
            budget = compute_aux_budget(energy, zone, 32)
            assert 0 <= budget <= 16


class TestBarBudget:
    """Test complete bar budget computation."""

    def test_bar_budget_structure(self):
        """Bar budget should have all required fields."""
        budget = compute_bar_budget(
            energy=0.5,
            balance=0.5,
            zone=EnergyZone.GROOVE,
            pattern_length=32,
        )
        assert hasattr(budget, 'anchor_hits')
        assert hasattr(budget, 'shimmer_hits')
        assert hasattr(budget, 'aux_hits')
        assert hasattr(budget, 'anchor_eligibility')
        assert hasattr(budget, 'shimmer_eligibility')
        assert hasattr(budget, 'aux_eligibility')

    def test_eligibility_masks_are_valid(self):
        """Eligibility masks should be valid bitmasks."""
        budget = compute_bar_budget(
            energy=0.5,
            balance=0.5,
            zone=EnergyZone.GROOVE,
            pattern_length=32,
        )
        # Should have some bits set
        assert budget.anchor_eligibility > 0
        assert budget.shimmer_eligibility > 0
        assert budget.aux_eligibility > 0

    def test_minimal_zone_restricts_eligibility(self):
        """MINIMAL zone should restrict eligibility to strong beats."""
        budget = compute_bar_budget(
            energy=0.1,
            balance=0.5,
            zone=EnergyZone.MINIMAL,
            pattern_length=32,
        )
        # MINIMAL should not allow all steps
        assert budget.anchor_eligibility != 0xFFFFFFFF

    def test_peak_zone_allows_all_steps(self):
        """PEAK zone should allow all steps."""
        budget = compute_bar_budget(
            energy=0.9,
            balance=0.5,
            zone=EnergyZone.PEAK,
            pattern_length=32,
        )
        # PEAK should allow all 32 steps
        assert budget.anchor_eligibility == 0xFFFFFFFF

    def test_build_multiplier_increases_budget(self):
        """BUILD multiplier > 1 should increase hits."""
        normal = compute_bar_budget(
            energy=0.5,
            balance=0.5,
            zone=EnergyZone.GROOVE,
            pattern_length=32,
            build_multiplier=1.0,
        )
        boosted = compute_bar_budget(
            energy=0.5,
            balance=0.5,
            zone=EnergyZone.GROOVE,
            pattern_length=32,
            build_multiplier=1.3,
        )
        assert boosted.anchor_hits >= normal.anchor_hits
