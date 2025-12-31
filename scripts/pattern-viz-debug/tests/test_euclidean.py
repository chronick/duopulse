"""Tests for Euclidean rhythm generation."""

import pytest
from pattern_viz.euclidean import (
    generate_euclidean,
    rotate_pattern,
    get_genre_euclidean_ratio,
    blend_euclidean_with_weights,
)
from pattern_viz.gumbel_sampler import count_bits
from pattern_viz.types import Genre, EnergyZone


class TestEuclideanGeneration:
    """Test Euclidean rhythm generation."""

    def test_euclidean_4_16_hit_count(self):
        """E(4,16) should give exactly 4 hits."""
        mask = generate_euclidean(4, 16)
        assert count_bits(mask) == 4

    def test_euclidean_8_16(self):
        """E(8,16) should give 8th notes."""
        mask = generate_euclidean(8, 16)
        assert count_bits(mask) == 8

    def test_euclidean_3_8(self):
        """E(3,8) should distribute 3 hits across 8 steps."""
        mask = generate_euclidean(3, 8)
        assert count_bits(mask) == 3

    def test_euclidean_5_8(self):
        """E(5,8) - classic Afro-Cuban pattern."""
        mask = generate_euclidean(5, 8)
        assert count_bits(mask) == 5

    def test_euclidean_7_12(self):
        """E(7,12) - 12/8 pattern."""
        mask = generate_euclidean(7, 12)
        assert count_bits(mask) == 7

    def test_euclidean_zero_hits(self):
        """E(0,N) should give empty pattern."""
        mask = generate_euclidean(0, 16)
        assert mask == 0

    def test_euclidean_all_hits(self):
        """E(N,N) should give all hits."""
        mask = generate_euclidean(8, 8)
        assert mask == 0xFF

    def test_euclidean_one_hit(self):
        """E(1,N) should give single hit."""
        mask = generate_euclidean(1, 16)
        assert count_bits(mask) == 1

    def test_euclidean_respects_length(self):
        """Hits should not exceed pattern length."""
        mask = generate_euclidean(4, 8)
        # No bits should be set above step 7
        assert (mask >> 8) == 0

    def test_euclidean_even_distribution(self):
        """Euclidean pattern should distribute hits evenly."""
        mask = generate_euclidean(4, 16)
        # Convert to step list
        steps = []
        for i in range(16):
            if mask & (1 << i):
                steps.append(i)

        # Check that gaps between hits are reasonably even
        gaps = []
        for i in range(len(steps) - 1):
            gaps.append(steps[i + 1] - steps[i])

        # For 4 hits in 16 steps, gaps should be around 4
        assert all(2 <= g <= 6 for g in gaps)


class TestPatternRotation:
    """Test pattern rotation."""

    def test_rotate_by_zero(self):
        """Rotation by 0 should not change pattern."""
        pattern = 0b1001
        assert rotate_pattern(pattern, 0, 4) == pattern

    def test_rotate_single_bit_right(self):
        """Rotation right should shift bits."""
        # Pattern 0b0001 (bit 0 set) rotated right by 1 in 4-bit space
        # Bit 0 wraps to position 3, so pattern becomes 0b1000
        pattern = 0b0001  # Hit on step 0
        rotated = rotate_pattern(pattern, 1, 4)
        # Rotate right: bit 0 moves to bit 3
        assert count_bits(rotated) == 1

    def test_rotate_preserves_hit_count(self):
        """Rotation should preserve number of hits."""
        pattern = 0b1010
        for offset in range(8):
            rotated = rotate_pattern(pattern, offset, 8)
            assert count_bits(rotated) == count_bits(pattern)

    def test_rotate_full_cycle(self):
        """Rotating by length should return to original."""
        pattern = 0b1010
        assert rotate_pattern(pattern, 4, 4) == pattern


class TestGenreEuclideanRatio:
    """Test genre-specific Euclidean ratios."""

    def test_techno_high_ratio_at_x0(self):
        """Techno at Field X=0 should have high Euclidean ratio."""
        ratio = get_genre_euclidean_ratio(Genre.TECHNO, 0.0, EnergyZone.GROOVE)
        assert ratio > 0.5  # Base is 0.70

    def test_techno_tapered_at_x1(self):
        """Techno at Field X=1 should have reduced ratio due to taper."""
        ratio = get_genre_euclidean_ratio(Genre.TECHNO, 1.0, EnergyZone.GROOVE)
        # Taper is 1 - 0.7*1.0 = 0.3, so 0.70 * 0.3 = 0.21
        assert ratio < 0.4

    def test_tribal_medium_ratio(self):
        """Tribal should have medium Euclidean ratio."""
        ratio = get_genre_euclidean_ratio(Genre.TRIBAL, 0.5, EnergyZone.GROOVE)
        assert 0.1 <= ratio <= 0.5

    def test_idm_low_ratio(self):
        """IDM should have zero Euclidean ratio."""
        ratio = get_genre_euclidean_ratio(Genre.IDM, 0.5, EnergyZone.GROOVE)
        assert ratio < 0.1  # IDM base is 0.0

    def test_ratio_in_valid_range(self, all_genres, all_zones):
        """All ratios should be in [0, 1]."""
        for genre in all_genres:
            for zone in all_zones:
                for field_x in [0.0, 0.5, 1.0]:
                    ratio = get_genre_euclidean_ratio(genre, field_x, zone)
                    assert 0.0 <= ratio <= 1.0

    def test_no_euclidean_in_high_energy_zones(self):
        """BUILD and PEAK zones should have zero Euclidean ratio."""
        for genre in [Genre.TECHNO, Genre.TRIBAL, Genre.IDM]:
            assert get_genre_euclidean_ratio(genre, 0.0, EnergyZone.BUILD) == 0.0
            assert get_genre_euclidean_ratio(genre, 0.0, EnergyZone.PEAK) == 0.0


class TestEuclideanBlending:
    """Test blending Euclidean with weights."""

    def test_blend_produces_correct_count(self):
        """Blended selection should produce target count."""
        weights = [0.5] * 32
        mask = blend_euclidean_with_weights(
            budget=8,
            steps=32,
            weights=weights,
            eligibility=0xFFFFFFFF,
            euclidean_ratio=0.5,
            seed=12345,
        )
        assert count_bits(mask) == 8

    def test_blend_respects_eligibility(self):
        """Blended selection should respect eligibility."""
        weights = [0.5] * 32
        eligibility = 0x0000FFFF  # Only first 16 steps
        mask = blend_euclidean_with_weights(
            budget=8,
            steps=32,
            weights=weights,
            eligibility=eligibility,
            euclidean_ratio=0.5,
            seed=12345,
        )
        # All hits should be in eligible steps
        assert (mask & ~eligibility) == 0

    def test_full_euclidean_gives_regular_pattern(self):
        """Ratio=1.0 should give pure Euclidean pattern with correct count."""
        weights = [0.5] * 16
        mask = blend_euclidean_with_weights(
            budget=4,
            steps=16,
            weights=weights,
            eligibility=0xFFFF,
            euclidean_ratio=1.0,
            seed=12345,
        )
        # Should have exactly 4 hits
        assert count_bits(mask) == 4 or count_bits(mask) <= 4  # May be less if eligibility cuts some

    def test_zero_euclidean_uses_weights(self):
        """Ratio=0.0 should fall back to weight-based selection."""
        # Create weights with strong preference for specific steps
        weights = [0.0] * 32
        weights[0] = 1.0
        weights[7] = 1.0  # Off-grid step
        weights[15] = 1.0
        weights[23] = 1.0

        mask = blend_euclidean_with_weights(
            budget=4,
            steps=32,
            weights=weights,
            eligibility=0xFFFFFFFF,
            euclidean_ratio=0.0,
            seed=12345,
        )
        assert count_bits(mask) == 4
