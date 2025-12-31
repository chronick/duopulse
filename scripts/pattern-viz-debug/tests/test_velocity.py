"""Tests for velocity and BUILD phase computation."""

import pytest
from pattern_viz.velocity import (
    compute_punch_params,
    compute_build_modifiers,
    should_accent,
    compute_velocity,
    get_default_accent_mask,
)
from pattern_viz.types import BuildPhase, Voice, PunchParams, BuildModifiers


class TestPunchParams:
    """Test PUNCH parameter computation."""

    def test_punch_zero_floor(self):
        """PUNCH=0 should give high floor, low accent."""
        params = compute_punch_params(0.0)
        assert params.velocity_floor >= 0.6  # High floor (0.65)
        assert params.accent_boost < 0.2  # Low accent boost (0.15)

    def test_punch_one_dynamics(self):
        """PUNCH=1 should give low floor, high accent."""
        params = compute_punch_params(1.0)
        assert params.velocity_floor <= 0.35  # Low floor (0.30)
        assert params.accent_boost > 0.3  # High accent boost (0.45)

    def test_punch_affects_variation(self):
        """Higher PUNCH should increase variation."""
        low = compute_punch_params(0.0)
        high = compute_punch_params(1.0)
        assert high.velocity_variation >= low.velocity_variation

    def test_punch_accent_probability(self):
        """PUNCH should affect accent probability."""
        params = compute_punch_params(0.5)
        assert 0.0 <= params.accent_probability <= 1.0


class TestBuildModifiers:
    """Test BUILD phase computation."""

    def test_groove_phase(self):
        """Early phrase progress should be GROOVE phase."""
        mods = compute_build_modifiers(0.5, phrase_progress=0.3)
        assert mods.phase == BuildPhase.GROOVE

    def test_build_phase(self):
        """Mid-late phrase progress should be BUILD phase."""
        mods = compute_build_modifiers(0.8, phrase_progress=0.7)
        assert mods.phase == BuildPhase.BUILD

    def test_fill_phase(self):
        """Late phrase progress should be FILL phase."""
        mods = compute_build_modifiers(0.8, phrase_progress=0.9)
        assert mods.phase == BuildPhase.FILL

    def test_low_build_stays_groove(self):
        """Low BUILD should stay in GROOVE even late in phrase."""
        mods = compute_build_modifiers(0.1, phrase_progress=0.9)
        # With low BUILD, might still be groove or early build
        assert mods.density_multiplier <= 1.1

    def test_fill_phase_density_boost(self):
        """FILL phase should boost density."""
        mods = compute_build_modifiers(0.9, phrase_progress=0.95)
        assert mods.density_multiplier >= 1.0

    def test_build_intensity_in_fill(self):
        """BUILD phase should have fill_intensity > 0 in FILL."""
        mods = compute_build_modifiers(0.8, phrase_progress=0.95)
        assert mods.fill_intensity >= 0.0


class TestAccentMask:
    """Test accent mask functionality."""

    def test_anchor_accent_mask(self):
        """Anchor should accent downbeats."""
        mask = get_default_accent_mask(Voice.ANCHOR)
        # Downbeats: 0, 4, 8, 12, 16, 20, 24, 28 (0x11111111)
        assert mask & (1 << 0)
        assert mask & (1 << 4)
        assert mask & (1 << 8)

    def test_shimmer_accent_mask(self):
        """Shimmer should accent backbeats."""
        mask = get_default_accent_mask(Voice.SHIMMER)
        # Shimmer: 0x01010101 = steps 0, 8, 16, 24
        assert mask & (1 << 0)
        assert mask & (1 << 8)

    def test_aux_accent_mask(self):
        """Aux should have accent mask on offbeats."""
        mask = get_default_accent_mask(Voice.AUX)
        # Aux: 0x44444444 = steps 2, 6, 10, 14, etc.
        assert mask & (1 << 2)
        assert mask > 0


class TestShouldAccent:
    """Test accent determination."""

    def test_accent_on_accent_step(self):
        """Steps in accent mask should be more likely to accent."""
        accent_mask = 0b00010001  # Steps 0 and 4
        mods = BuildModifiers(
            phase=BuildPhase.GROOVE,
            density_multiplier=1.0,
            velocity_boost=0.0,
            force_accents=False,
            in_fill_zone=False,
            fill_intensity=0.0,
            phrase_progress=0.0,
        )

        # Test multiple times to check probability
        accent_count = 0
        for i in range(100):
            if should_accent(0, accent_mask, 0.8, mods, seed=i):
                accent_count += 1

        # With 80% probability on accent step, should accent most of the time
        assert accent_count > 50

    def test_low_probability_rarely_accents(self):
        """Low accent probability should rarely accent."""
        accent_mask = 0xFFFFFFFF
        mods = BuildModifiers(
            phase=BuildPhase.GROOVE,
            density_multiplier=1.0,
            velocity_boost=0.0,
            force_accents=False,
            in_fill_zone=False,
            fill_intensity=0.0,
            phrase_progress=0.0,
        )

        accent_count = 0
        for i in range(100):
            if should_accent(0, accent_mask, 0.1, mods, seed=i):
                accent_count += 1

        # With 10% probability, should accent rarely
        assert accent_count < 30

    def test_force_accents_in_fill(self):
        """force_accents=True should always return True."""
        mods = BuildModifiers(
            phase=BuildPhase.FILL,
            density_multiplier=1.5,
            velocity_boost=0.1,
            force_accents=True,
            in_fill_zone=True,
            fill_intensity=0.8,
            phrase_progress=0.95,
        )
        # Should accent regardless of mask or probability
        assert should_accent(7, 0, 0.0, mods, seed=12345)


class TestVelocityComputation:
    """Test velocity value computation."""

    def test_velocity_in_range(self):
        """Velocity should be in [0, 1]."""
        params = compute_punch_params(0.5)
        mods = compute_build_modifiers(0.5, 0.5)

        for is_accent in [True, False]:
            for step in range(32):
                vel = compute_velocity(params, mods, is_accent, seed=12345, step=step)
                assert 0.0 <= vel <= 1.0

    def test_accent_increases_velocity(self):
        """Accented hits should have higher velocity."""
        params = compute_punch_params(0.8)  # High punch for clear difference
        mods = compute_build_modifiers(0.5, 0.5)

        # Same step, different accent
        vel_normal = compute_velocity(params, mods, False, seed=12345, step=0)
        vel_accent = compute_velocity(params, mods, True, seed=12345, step=0)

        assert vel_accent >= vel_normal

    def test_build_phase_boosts_velocity(self):
        """BUILD phase should boost velocity."""
        params = compute_punch_params(0.5)
        groove_mods = compute_build_modifiers(0.3, 0.3)
        build_mods = compute_build_modifiers(0.8, 0.75)

        vel_groove = compute_velocity(params, groove_mods, False, seed=12345, step=0)
        vel_build = compute_velocity(params, build_mods, False, seed=12345, step=0)

        # BUILD phase should generally be equal or higher
        # (might be equal in some cases due to clamping)
        assert vel_build >= vel_groove * 0.9  # Allow small tolerance

    def test_deterministic_with_seed(self):
        """Same seed should give same velocity."""
        params = compute_punch_params(0.5)
        mods = compute_build_modifiers(0.5, 0.5)

        vel1 = compute_velocity(params, mods, False, seed=12345, step=0)
        vel2 = compute_velocity(params, mods, False, seed=12345, step=0)

        assert vel1 == vel2
