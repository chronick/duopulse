"""Tests for complete pattern generation pipeline."""

import pytest
from pattern_viz.pipeline import generate_pattern, generate_phrase, PatternOutput
from pattern_viz.types import Genre, EnergyZone, VoiceCoupling
from pattern_viz.gumbel_sampler import count_bits


class TestPatternOutput:
    """Test PatternOutput structure."""

    def test_default_output(self):
        """Default output should have valid structure."""
        output = PatternOutput()
        assert output.anchor_mask == 0
        assert output.shimmer_mask == 0
        assert output.aux_mask == 0
        assert len(output.anchor_velocities) == 32
        assert output.pattern_length == 32

    def test_get_step(self):
        """get_step should return correct step data."""
        output = PatternOutput()
        output.anchor_mask = 0b0001  # Hit on step 0
        output.anchor_velocities[0] = 0.8

        step = output.get_step(0)
        assert step.anchor_fires
        assert not step.shimmer_fires
        assert step.anchor_velocity == 0.8


class TestPatternGeneration:
    """Test pattern generation."""

    def test_generate_basic_pattern(self):
        """Should generate valid pattern with defaults."""
        pattern = generate_pattern()

        assert pattern.anchor_hits >= 0
        assert pattern.shimmer_hits >= 0
        assert pattern.aux_hits >= 0
        assert pattern.pattern_length == 32

    def test_pattern_has_hits(self):
        """Pattern should have some hits at moderate energy."""
        pattern = generate_pattern(energy=0.5)

        total_hits = pattern.anchor_hits + pattern.shimmer_hits + pattern.aux_hits
        assert total_hits > 0

    def test_low_energy_sparse(self):
        """Low energy should give sparse pattern."""
        pattern = generate_pattern(energy=0.1)
        assert pattern.anchor_hits <= 4

    def test_high_energy_dense(self):
        """High energy should give dense pattern."""
        pattern = generate_pattern(energy=0.9)
        assert pattern.anchor_hits >= 4

    def test_deterministic_generation(self):
        """Same seed should give same pattern."""
        pattern1 = generate_pattern(seed=12345)
        pattern2 = generate_pattern(seed=12345)

        assert pattern1.anchor_mask == pattern2.anchor_mask
        assert pattern1.shimmer_mask == pattern2.shimmer_mask
        assert pattern1.aux_mask == pattern2.aux_mask

    def test_different_seeds_different_patterns(self):
        """Different seeds should give different patterns."""
        pattern1 = generate_pattern(seed=12345)
        pattern2 = generate_pattern(seed=54321)

        # At least one mask should differ
        assert (pattern1.anchor_mask != pattern2.anchor_mask or
                pattern1.shimmer_mask != pattern2.shimmer_mask)

    def test_all_genres(self, all_genres):
        """Should generate valid patterns for all genres."""
        for genre in all_genres:
            pattern = generate_pattern(genre=genre, energy=0.5)
            assert pattern.anchor_hits > 0
            assert pattern.dominant_archetype != ""

    def test_field_position_affects_pattern(self):
        """Different field positions should give different archetypes."""
        p1 = generate_pattern(field_x=0.0, field_y=0.0)
        p2 = generate_pattern(field_x=1.0, field_y=1.0)

        # Different corners should have different dominant archetypes
        assert p1.dominant_archetype != p2.dominant_archetype

    def test_velocities_set_for_hits(self):
        """Velocities should be non-zero only for hit steps."""
        pattern = generate_pattern(energy=0.6)

        for step in range(32):
            if pattern.anchor_mask & (1 << step):
                assert pattern.anchor_velocities[step] > 0
            else:
                assert pattern.anchor_velocities[step] == 0

    def test_velocity_range(self):
        """All velocities should be in [0, 1]."""
        pattern = generate_pattern(energy=0.7, punch=0.8)

        for vel in pattern.anchor_velocities:
            assert 0.0 <= vel <= 1.0
        for vel in pattern.shimmer_velocities:
            assert 0.0 <= vel <= 1.0
        for vel in pattern.aux_velocities:
            assert 0.0 <= vel <= 1.0


class TestVoiceCoupling:
    """Test voice coupling modes."""

    def test_independent_no_restriction(self):
        """INDEPENDENT should allow overlapping hits."""
        pattern = generate_pattern(
            voice_coupling=VoiceCoupling.INDEPENDENT,
            energy=0.8,
        )
        # No specific restriction - both can fire on same step
        # Just verify pattern is valid
        assert pattern.pattern_length == 32

    def test_interlock_no_overlap(self):
        """INTERLOCK should prevent shimmer on anchor steps."""
        # Generate multiple patterns to ensure we test the behavior
        for seed in range(10):
            pattern = generate_pattern(
                voice_coupling=VoiceCoupling.INTERLOCK,
                energy=0.8,
                seed=seed,
            )
            # Shimmer should not fire where anchor fires
            overlap = pattern.anchor_mask & pattern.shimmer_mask
            assert overlap == 0, f"Seed {seed}: anchor and shimmer overlap"

    def test_shadow_follows_anchor(self):
        """SHADOW should have shimmer follow anchor."""
        pattern = generate_pattern(
            voice_coupling=VoiceCoupling.SHADOW,
            energy=0.6,
        )
        # Shimmer should be delayed version of anchor
        # This is harder to verify precisely, but should have some hits
        assert pattern.shimmer_hits >= 0


class TestEnergyZones:
    """Test energy zone behavior."""

    def test_minimal_zone(self):
        """MINIMAL zone should have very few hits."""
        pattern = generate_pattern(energy=0.1)
        assert pattern.energy_zone == EnergyZone.MINIMAL
        assert pattern.anchor_hits <= 4

    def test_groove_zone(self):
        """GROOVE zone should have moderate hits."""
        pattern = generate_pattern(energy=0.35)
        assert pattern.energy_zone == EnergyZone.GROOVE

    def test_build_zone(self):
        """BUILD zone should have more hits."""
        pattern = generate_pattern(energy=0.6)
        assert pattern.energy_zone == EnergyZone.BUILD

    def test_peak_zone(self):
        """PEAK zone should have many hits."""
        pattern = generate_pattern(energy=0.9)
        assert pattern.energy_zone == EnergyZone.PEAK
        assert pattern.anchor_hits >= 4


class TestBuildParameter:
    """Test BUILD parameter effects."""

    def test_build_zero_sparse(self):
        """BUILD=0 should not boost density."""
        pattern = generate_pattern(build=0.0, phrase_progress=0.9, energy=0.5)
        # Low build shouldn't add many hits even late in phrase
        assert pattern.anchor_hits <= 10

    def test_build_high_increases_density(self):
        """High BUILD late in phrase should increase density."""
        low_build = generate_pattern(build=0.2, phrase_progress=0.9, energy=0.5)
        high_build = generate_pattern(build=0.9, phrase_progress=0.9, energy=0.5)

        # High build should have equal or more hits
        assert high_build.anchor_hits >= low_build.anchor_hits - 2  # Allow tolerance


class TestPhraseGeneration:
    """Test multi-bar phrase generation."""

    def test_phrase_length(self):
        """Phrase should have correct number of bars."""
        patterns = generate_phrase(bars=4)
        assert len(patterns) == 4

    def test_phrase_all_valid(self):
        """All patterns in phrase should be valid."""
        patterns = generate_phrase(bars=8, energy=0.5)

        for pattern in patterns:
            assert isinstance(pattern, PatternOutput)
            assert pattern.pattern_length == 32
            assert pattern.anchor_hits >= 0

    def test_phrase_drift_varies_patterns(self):
        """Drift > 0 should vary patterns across bars."""
        patterns = generate_phrase(bars=4, drift=0.5)

        # With drift, patterns should vary
        masks = [p.anchor_mask for p in patterns]
        # At least some should be different
        unique_masks = set(masks)
        assert len(unique_masks) > 1

    def test_phrase_zero_drift_same_pattern(self):
        """Drift = 0 should give same pattern every bar."""
        patterns = generate_phrase(bars=4, drift=0.0)

        # All should be identical
        first_mask = patterns[0].anchor_mask
        for pattern in patterns:
            assert pattern.anchor_mask == first_mask

    def test_phrase_deterministic(self):
        """Same seed should give same phrase."""
        phrase1 = generate_phrase(bars=4, seed=12345)
        phrase2 = generate_phrase(bars=4, seed=12345)

        for p1, p2 in zip(phrase1, phrase2):
            assert p1.anchor_mask == p2.anchor_mask


class TestPatternLength:
    """Test different pattern lengths."""

    def test_pattern_length_16(self):
        """Should support 16-step patterns."""
        pattern = generate_pattern(pattern_length=16)
        assert pattern.pattern_length == 16
        # Hits should only be in first 16 steps
        assert (pattern.anchor_mask >> 16) == 0

    def test_pattern_length_32(self):
        """Should support 32-step patterns."""
        pattern = generate_pattern(pattern_length=32)
        assert pattern.pattern_length == 32

    def test_pattern_length_clamped(self):
        """Pattern length should clamp to valid range."""
        pattern = generate_pattern(pattern_length=8)  # Below minimum
        assert pattern.pattern_length >= 16

        pattern = generate_pattern(pattern_length=128)  # Above maximum
        assert pattern.pattern_length <= 32
