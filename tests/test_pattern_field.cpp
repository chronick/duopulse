#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/Engine/PatternField.h"
#include "../src/Engine/ArchetypeData.h"
#include "../src/Engine/DuoPulseTypes.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// Softmax Tests
// =============================================================================

TEST_CASE("SoftmaxWithTemperature sharpens weights", "[pattern-field]")
{
    SECTION("Equal weights stay equal regardless of temperature")
    {
        float weights[4] = {0.25f, 0.25f, 0.25f, 0.25f};
        SoftmaxWithTemperature(weights, 0.5f);

        // All should be approximately equal
        for (int i = 0; i < 4; ++i)
        {
            REQUIRE(weights[i] == Approx(0.25f).margin(0.01f));
        }
    }

    SECTION("Low temperature sharpens distribution")
    {
        float weights[4] = {0.5f, 0.3f, 0.15f, 0.05f};
        SoftmaxWithTemperature(weights, 0.1f);

        // Highest should dominate
        REQUIRE(weights[0] > 0.8f);
        REQUIRE(weights[1] < 0.15f);
        REQUIRE(weights[2] < 0.05f);
        REQUIRE(weights[3] < 0.01f);
    }

    SECTION("High temperature flattens distribution")
    {
        float weights[4] = {0.5f, 0.3f, 0.15f, 0.05f};
        SoftmaxWithTemperature(weights, 2.0f);

        // Distribution should be more even than original
        REQUIRE(weights[0] < 0.45f);
        REQUIRE(weights[3] > 0.1f);
    }

    SECTION("Weights sum to 1.0 after softmax")
    {
        float weights[4] = {0.7f, 0.2f, 0.08f, 0.02f};
        SoftmaxWithTemperature(weights, 0.5f);

        float sum = weights[0] + weights[1] + weights[2] + weights[3];
        REQUIRE(sum == Approx(1.0f).margin(0.001f));
    }
}

// =============================================================================
// Grid Weight Computation Tests
// =============================================================================

TEST_CASE("ComputeGridWeights at grid corners", "[pattern-field]")
{
    float weights[4];
    int x0, x1, y0, y1;

    SECTION("Bottom-left corner (0,0)")
    {
        ComputeGridWeights(0.0f, 0.0f, weights, x0, x1, y0, y1);

        REQUIRE(x0 == 0);
        REQUIRE(x1 == 1);
        REQUIRE(y0 == 0);
        REQUIRE(y1 == 1);
        REQUIRE(weights[0] == Approx(1.0f));  // Bottom-left
        REQUIRE(weights[1] == Approx(0.0f));
        REQUIRE(weights[2] == Approx(0.0f));
        REQUIRE(weights[3] == Approx(0.0f));
    }

    SECTION("Bottom-right corner (1,0)")
    {
        ComputeGridWeights(1.0f, 0.0f, weights, x0, x1, y0, y1);

        REQUIRE(x0 == 1);
        REQUIRE(x1 == 2);
        // At right edge: fracX=1.0, fracY=0.0, so weights[1] (bottom-right) is 1.0
        REQUIRE(weights[1] == Approx(1.0f));
    }

    SECTION("Top-left corner (0,1)")
    {
        ComputeGridWeights(0.0f, 1.0f, weights, x0, x1, y0, y1);

        REQUIRE(y0 == 1);
        REQUIRE(y1 == 2);
        // At top edge: fracX=0.0, fracY=1.0, so weights[2] (top-left) is 1.0
        REQUIRE(weights[2] == Approx(1.0f));
    }

    SECTION("Center (0.5, 0.5)")
    {
        ComputeGridWeights(0.5f, 0.5f, weights, x0, x1, y0, y1);

        // Should be in the center cell with equal weights
        float sum = weights[0] + weights[1] + weights[2] + weights[3];
        REQUIRE(sum == Approx(1.0f));
    }
}

TEST_CASE("ComputeGridWeights intermediate positions", "[pattern-field]")
{
    float weights[4];
    int x0, x1, y0, y1;

    SECTION("Quarter position (0.25, 0.25)")
    {
        ComputeGridWeights(0.25f, 0.25f, weights, x0, x1, y0, y1);

        // Should be between grid positions 0 and 1
        REQUIRE(x0 == 0);
        REQUIRE(x1 == 1);
        REQUIRE(y0 == 0);
        REQUIRE(y1 == 1);

        // At (0.25, 0.25) -> gridX=0.5, gridY=0.5 -> center of cell
        // All weights should be equal (0.25 each)
        float sum = weights[0] + weights[1] + weights[2] + weights[3];
        REQUIRE(sum == Approx(1.0f));
    }

    SECTION("Position near origin (0.1, 0.1) favors bottom-left")
    {
        ComputeGridWeights(0.1f, 0.1f, weights, x0, x1, y0, y1);

        // gridX=0.2, gridY=0.2, so fracX=0.2, fracY=0.2
        // weights[0] = (1-0.2)*(1-0.2) = 0.64 (bottom-left)
        // weights[3] = 0.2 * 0.2 = 0.04 (top-right)
        REQUIRE(weights[0] > weights[3]);
    }

    SECTION("Three-quarter position (0.75, 0.75)")
    {
        ComputeGridWeights(0.75f, 0.75f, weights, x0, x1, y0, y1);

        // Should be between grid positions 1 and 2
        REQUIRE(x0 == 1);
        REQUIRE(x1 == 2);
        REQUIRE(y0 == 1);
        REQUIRE(y1 == 2);
    }

    SECTION("Weights always sum to 1.0")
    {
        for (float x = 0.0f; x <= 1.0f; x += 0.1f)
        {
            for (float y = 0.0f; y <= 1.0f; y += 0.1f)
            {
                ComputeGridWeights(x, y, weights, x0, x1, y0, y1);
                float sum = weights[0] + weights[1] + weights[2] + weights[3];
                REQUIRE(sum == Approx(1.0f).margin(0.001f));
            }
        }
    }
}

// =============================================================================
// Archetype Blending Tests
// =============================================================================

TEST_CASE("BlendArchetypes with single dominant weight", "[pattern-field]")
{
    // Create 4 test archetypes with different characteristics
    ArchetypeDNA arch0, arch1, arch2, arch3;
    arch0.Init();
    arch1.Init();
    arch2.Init();
    arch3.Init();

    // Set distinct swing amounts for testing
    arch0.swingAmount = 0.0f;
    arch1.swingAmount = 0.3f;
    arch2.swingAmount = 0.6f;
    arch3.swingAmount = 1.0f;

    // Set distinct weights at step 0
    arch0.anchorWeights[0] = 1.0f;
    arch1.anchorWeights[0] = 0.5f;
    arch2.anchorWeights[0] = 0.3f;
    arch3.anchorWeights[0] = 0.1f;

    const ArchetypeDNA* archetypes[4] = {&arch0, &arch1, &arch2, &arch3};
    ArchetypeDNA result;

    SECTION("100% weight on first archetype")
    {
        float weights[4] = {1.0f, 0.0f, 0.0f, 0.0f};
        BlendArchetypes(archetypes, weights, result);

        REQUIRE(result.swingAmount == Approx(0.0f));
        REQUIRE(result.anchorWeights[0] == Approx(1.0f));
    }

    SECTION("100% weight on last archetype")
    {
        float weights[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        BlendArchetypes(archetypes, weights, result);

        REQUIRE(result.swingAmount == Approx(1.0f));
        REQUIRE(result.anchorWeights[0] == Approx(0.1f));
    }
}

TEST_CASE("BlendArchetypes with equal weights", "[pattern-field]")
{
    ArchetypeDNA arch0, arch1, arch2, arch3;
    arch0.Init();
    arch1.Init();
    arch2.Init();
    arch3.Init();

    // Set different swing amounts
    arch0.swingAmount = 0.2f;
    arch1.swingAmount = 0.4f;
    arch2.swingAmount = 0.6f;
    arch3.swingAmount = 0.8f;

    const ArchetypeDNA* archetypes[4] = {&arch0, &arch1, &arch2, &arch3};
    ArchetypeDNA result;

    float weights[4] = {0.25f, 0.25f, 0.25f, 0.25f};
    BlendArchetypes(archetypes, weights, result);

    // Swing should be average: (0.2 + 0.4 + 0.6 + 0.8) / 4 = 0.5
    REQUIRE(result.swingAmount == Approx(0.5f));
}

TEST_CASE("BlendArchetypes discrete properties come from dominant", "[pattern-field]")
{
    ArchetypeDNA arch0, arch1, arch2, arch3;
    arch0.Init();
    arch1.Init();
    arch2.Init();
    arch3.Init();

    // Set different accent masks
    arch0.anchorAccentMask = 0x11111111;
    arch1.anchorAccentMask = 0x22222222;
    arch2.anchorAccentMask = 0x33333333;
    arch3.anchorAccentMask = 0x44444444;

    const ArchetypeDNA* archetypes[4] = {&arch0, &arch1, &arch2, &arch3};
    ArchetypeDNA result;

    SECTION("Dominant is first")
    {
        float weights[4] = {0.5f, 0.2f, 0.2f, 0.1f};
        BlendArchetypes(archetypes, weights, result);
        REQUIRE(result.anchorAccentMask == 0x11111111);
    }

    SECTION("Dominant is third")
    {
        float weights[4] = {0.1f, 0.2f, 0.5f, 0.2f};
        BlendArchetypes(archetypes, weights, result);
        REQUIRE(result.anchorAccentMask == 0x33333333);
    }
}

// =============================================================================
// Genre Field Tests
// =============================================================================

TEST_CASE("Genre field initialization", "[pattern-field]")
{
    InitializeGenreFields();

    SECTION("All genres are initialized")
    {
        REQUIRE(AreGenreFieldsInitialized() == true);
    }

    SECTION("Can get Techno field")
    {
        const GenreField& field = GetGenreField(Genre::TECHNO);

        // Check that archetypes have correct grid positions
        REQUIRE(field.GetArchetype(0, 0).gridX == 0);
        REQUIRE(field.GetArchetype(0, 0).gridY == 0);
        REQUIRE(field.GetArchetype(2, 2).gridX == 2);
        REQUIRE(field.GetArchetype(2, 2).gridY == 2);
    }

    SECTION("Can get Tribal field")
    {
        const GenreField& field = GetGenreField(Genre::TRIBAL);
        REQUIRE(field.GetArchetype(1, 1).gridX == 1);
        REQUIRE(field.GetArchetype(1, 1).gridY == 1);
    }

    SECTION("Can get IDM field")
    {
        const GenreField& field = GetGenreField(Genre::IDM);
        REQUIRE(field.GetArchetype(0, 2).gridX == 0);
        REQUIRE(field.GetArchetype(0, 2).gridY == 2);
    }
}

TEST_CASE("All 27 archetypes load correctly", "[pattern-field]")
{
    InitializeGenreFields();

    SECTION("Techno archetypes")
    {
        const GenreField& field = GetGenreField(Genre::TECHNO);
        for (int y = 0; y < 3; ++y)
        {
            for (int x = 0; x < 3; ++x)
            {
                const ArchetypeDNA& arch = field.GetArchetype(x, y);
                REQUIRE(arch.gridX == x);
                REQUIRE(arch.gridY == y);
                // Check that downbeat has some weight
                REQUIRE(arch.anchorWeights[0] >= 0.0f);
                REQUIRE(arch.anchorWeights[0] <= 1.0f);
            }
        }
    }

    SECTION("Tribal archetypes")
    {
        const GenreField& field = GetGenreField(Genre::TRIBAL);
        for (int y = 0; y < 3; ++y)
        {
            for (int x = 0; x < 3; ++x)
            {
                const ArchetypeDNA& arch = field.GetArchetype(x, y);
                REQUIRE(arch.gridX == x);
                REQUIRE(arch.gridY == y);
            }
        }
    }

    SECTION("IDM archetypes")
    {
        const GenreField& field = GetGenreField(Genre::IDM);
        for (int y = 0; y < 3; ++y)
        {
            for (int x = 0; x < 3; ++x)
            {
                const ArchetypeDNA& arch = field.GetArchetype(x, y);
                REQUIRE(arch.gridX == x);
                REQUIRE(arch.gridY == y);
            }
        }
    }
}

// =============================================================================
// GetBlendedArchetype Tests
// =============================================================================

TEST_CASE("GetBlendedArchetype at grid corners returns exact archetype", "[pattern-field]")
{
    InitializeGenreFields();
    const GenreField& field = GetGenreField(Genre::TECHNO);
    ArchetypeDNA result;

    SECTION("Corner (0,0) - Minimal")
    {
        GetBlendedArchetype(field, 0.0f, 0.0f, 0.1f, result);

        // With low temperature and corner position, should match exactly
        const ArchetypeDNA& corner = field.GetArchetype(0, 0);
        REQUIRE(result.anchorWeights[0] == Approx(corner.anchorWeights[0]).margin(0.01f));
    }

    SECTION("Corner (1,1) - Chaos with very low temperature")
    {
        // High values with low temperature at corner
        GetBlendedArchetype(field, 1.0f, 1.0f, 0.01f, result);

        // Should be dominated by the corner archetype
        const ArchetypeDNA& corner = field.GetArchetype(2, 2);
        REQUIRE(result.gridX == corner.gridX);
        REQUIRE(result.gridY == corner.gridY);
    }
}

TEST_CASE("GetBlendedArchetype at center produces weighted mix", "[pattern-field]")
{
    InitializeGenreFields();
    const GenreField& field = GetGenreField(Genre::TECHNO);
    ArchetypeDNA result;

    // Get blend at center of grid
    GetBlendedArchetype(field, 0.5f, 0.5f, 1.0f, result);  // High temp for more mixing

    // Should have intermediate values
    // With high temperature blending, swing should be more averaged
    // This is a loose check since exact value depends on all 4 corners
    REQUIRE(result.swingAmount >= 0.0f);
    REQUIRE(result.swingAmount <= 1.0f);
}

TEST_CASE("GetBlendedArchetype temperature affects blending", "[pattern-field]")
{
    InitializeGenreFields();
    const GenreField& field = GetGenreField(Genre::TECHNO);

    // Get blend slightly off-center (favoring one corner slightly)
    float x = 0.2f;  // Slightly left of center
    float y = 0.2f;  // Slightly below center

    ArchetypeDNA lowTemp, highTemp;
    GetBlendedArchetype(field, x, y, 0.1f, lowTemp);   // Low temp = sharper
    GetBlendedArchetype(field, x, y, 2.0f, highTemp);  // High temp = smoother

    // Low temperature should be closer to dominant (bottom-left)
    const ArchetypeDNA& dominant = field.GetArchetype(0, 0);

    // Low temp result should be closer to dominant than high temp
    float lowDiff = std::abs(lowTemp.swingAmount - dominant.swingAmount);
    float highDiff = std::abs(highTemp.swingAmount - dominant.swingAmount);

    // With the bilinear weights favoring bottom-left, low temp should
    // produce result closer to the dominant archetype
    REQUIRE(lowDiff <= highDiff + 0.01f);  // Allow small margin
}

// =============================================================================
// Utility Function Tests
// =============================================================================

TEST_CASE("FindDominantArchetype finds correct index", "[pattern-field]")
{
    SECTION("First is dominant")
    {
        float weights[4] = {0.5f, 0.2f, 0.2f, 0.1f};
        REQUIRE(FindDominantArchetype(weights) == 0);
    }

    SECTION("Last is dominant")
    {
        float weights[4] = {0.1f, 0.2f, 0.2f, 0.5f};
        REQUIRE(FindDominantArchetype(weights) == 3);
    }

    SECTION("Middle is dominant")
    {
        float weights[4] = {0.1f, 0.5f, 0.3f, 0.1f};
        REQUIRE(FindDominantArchetype(weights) == 1);
    }

    SECTION("Equal weights returns first")
    {
        float weights[4] = {0.25f, 0.25f, 0.25f, 0.25f};
        REQUIRE(FindDominantArchetype(weights) == 0);
    }
}

TEST_CASE("InterpolateFloat computes weighted average", "[pattern-field]")
{
    SECTION("Single weight")
    {
        float values[4] = {1.0f, 2.0f, 3.0f, 4.0f};
        float weights[4] = {1.0f, 0.0f, 0.0f, 0.0f};
        REQUIRE(InterpolateFloat(values, weights) == Approx(1.0f));
    }

    SECTION("Equal weights")
    {
        float values[4] = {1.0f, 2.0f, 3.0f, 4.0f};
        float weights[4] = {0.25f, 0.25f, 0.25f, 0.25f};
        REQUIRE(InterpolateFloat(values, weights) == Approx(2.5f));
    }

    SECTION("Unequal weights")
    {
        float values[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        float weights[4] = {0.1f, 0.2f, 0.3f, 0.4f};
        REQUIRE(InterpolateFloat(values, weights) == Approx(0.4f));
    }
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_CASE("Pattern field handles edge cases", "[pattern-field]")
{
    InitializeGenreFields();

    SECTION("Negative field values are clamped to 0")
    {
        const GenreField& field = GetGenreField(Genre::TECHNO);
        ArchetypeDNA result;

        // Should not crash, should clamp to (0,0)
        GetBlendedArchetype(field, -1.0f, -1.0f, 0.5f, result);

        // Result should be valid
        REQUIRE(result.anchorWeights[0] >= 0.0f);
    }

    SECTION("Field values > 1 are clamped")
    {
        const GenreField& field = GetGenreField(Genre::TECHNO);
        ArchetypeDNA result;

        // Should not crash, should clamp to (1,1)
        GetBlendedArchetype(field, 2.0f, 2.0f, 0.5f, result);

        // Result should be valid
        REQUIRE(result.anchorWeights[0] >= 0.0f);
    }

    SECTION("Invalid genre defaults to Techno")
    {
        // Cast an invalid value
        const GenreField& field = GetGenreField(static_cast<Genre>(99));

        // Should return Techno field (index 0)
        REQUIRE(field.GetArchetype(0, 0).gridX == 0);
    }
}

// =============================================================================
// Archetype Data Verification
// =============================================================================

TEST_CASE("Archetype data has valid ranges", "[pattern-field]")
{
    InitializeGenreFields();

    for (int g = 0; g < static_cast<int>(Genre::COUNT); ++g)
    {
        const GenreField& field = GetGenreField(static_cast<Genre>(g));

        for (int y = 0; y < 3; ++y)
        {
            for (int x = 0; x < 3; ++x)
            {
                const ArchetypeDNA& arch = field.GetArchetype(x, y);

                SECTION("Weights in valid range [0,1]")
                {
                    for (int step = 0; step < kMaxSteps; ++step)
                    {
                        REQUIRE(arch.anchorWeights[step] >= 0.0f);
                        REQUIRE(arch.anchorWeights[step] <= 1.0f);
                        REQUIRE(arch.shimmerWeights[step] >= 0.0f);
                        REQUIRE(arch.shimmerWeights[step] <= 1.0f);
                        REQUIRE(arch.auxWeights[step] >= 0.0f);
                        REQUIRE(arch.auxWeights[step] <= 1.0f);
                    }
                }

                SECTION("Timing parameters in valid range")
                {
                    REQUIRE(arch.swingAmount >= 0.0f);
                    REQUIRE(arch.swingAmount <= 1.0f);
                    REQUIRE(arch.defaultCouple >= 0.0f);
                    REQUIRE(arch.defaultCouple <= 1.0f);
                }

                SECTION("Fill multiplier is positive")
                {
                    REQUIRE(arch.fillDensityMultiplier >= 1.0f);
                }
            }
        }
    }
}
