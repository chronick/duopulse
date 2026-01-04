"""Pytest configuration and fixtures."""

import pytest
from pattern_viz.types import Genre, EnergyZone, VoiceCoupling


@pytest.fixture
def default_seed():
    """Default seed for deterministic tests."""
    return 0x12345678


@pytest.fixture
def all_genres():
    """All genre values."""
    return [Genre.TECHNO, Genre.TRIBAL, Genre.IDM]


@pytest.fixture
def all_zones():
    """All energy zone values."""
    return [EnergyZone.MINIMAL, EnergyZone.GROOVE, EnergyZone.BUILD, EnergyZone.PEAK]
