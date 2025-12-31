"""Tests for archetype weight tables."""

import pytest
from pattern_viz.archetypes import get_archetype, get_archetype_by_index, ArchetypeDNA
from pattern_viz.types import Genre


class TestArchetypeLoading:
    """Test archetype loading and structure."""

    def test_get_archetype_all_positions(self, all_genres):
        """Verify all 27 archetypes can be loaded."""
        for genre in all_genres:
            for x in range(3):
                for y in range(3):
                    arch = get_archetype(genre, x, y)
                    assert isinstance(arch, ArchetypeDNA)
                    assert arch.grid_x == x
                    assert arch.grid_y == y

    def test_archetype_weights_length(self, all_genres):
        """Verify all weight arrays have 32 elements."""
        for genre in all_genres:
            for x in range(3):
                for y in range(3):
                    arch = get_archetype(genre, x, y)
                    assert len(arch.anchor_weights) == 32
                    assert len(arch.shimmer_weights) == 32
                    assert len(arch.aux_weights) == 32

    def test_archetype_weights_range(self, all_genres):
        """Verify weight values are in [0, 1] range."""
        for genre in all_genres:
            for x in range(3):
                for y in range(3):
                    arch = get_archetype(genre, x, y)
                    for w in arch.anchor_weights:
                        assert 0.0 <= w <= 1.0
                    for w in arch.shimmer_weights:
                        assert 0.0 <= w <= 1.0
                    for w in arch.aux_weights:
                        assert 0.0 <= w <= 1.0

    def test_archetype_by_index(self, all_genres):
        """Test linear index lookup."""
        for genre in all_genres:
            for idx in range(9):
                arch = get_archetype_by_index(genre, idx)
                expected_x = idx % 3
                expected_y = idx // 3
                assert arch.grid_x == expected_x
                assert arch.grid_y == expected_y

    def test_techno_minimal_has_four_on_floor(self):
        """Techno Minimal should have strong weights on quarter notes."""
        arch = get_archetype(Genre.TECHNO, 0, 0)
        # Steps 0, 4, 8, 12, etc. should have high weights
        quarter_notes = [0, 4, 8, 12, 16, 20, 24, 28]
        for step in quarter_notes:
            assert arch.anchor_weights[step] >= 0.9, f"Step {step} should be >= 0.9"

    def test_each_archetype_has_name(self, all_genres):
        """Each archetype should have a non-empty name."""
        for genre in all_genres:
            for x in range(3):
                for y in range(3):
                    arch = get_archetype(genre, x, y)
                    assert arch.name, f"Archetype at [{x},{y}] for {genre} has no name"

    def test_archetype_clamps_out_of_range(self):
        """Out of range grid positions should clamp."""
        # Should not raise, should clamp to valid range
        arch = get_archetype(Genre.TECHNO, -1, 5)
        assert 0 <= arch.grid_x <= 2
        assert 0 <= arch.grid_y <= 2
