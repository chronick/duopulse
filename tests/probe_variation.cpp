#include <catch2/catch_test_macros.hpp>
#include "../src/Engine/Sequencer.h"
#include "../src/Engine/HitBudget.h"

using namespace daisysp_idm_grids;

TEST_CASE("Sequencer generates non-empty patterns at high energy", "[probe][generation]")
{
    Sequencer seq;
    seq.Init(32000.0f);  // match firmware runtime

    // Set a hot, complex position similar to the hardware repro log
    seq.SetEnergy(0.99f);
    seq.SetFieldX(0.12f);
    seq.SetFieldY(0.92f);
    seq.SetBalance(0.5f);
    seq.SetPatternLength(32);
    seq.SetPhraseLength(1);

    // Force regeneration with current controls
    seq.TriggerReset();

    float w0 = seq.GetBlendedAnchorWeight(0);
    float w4 = seq.GetBlendedAnchorWeight(4);
    float w8 = seq.GetBlendedAnchorWeight(8);
    REQUIRE(w0 > 0.1f);
    REQUIRE(w4 > 0.1f);
    REQUIRE(w8 > 0.1f);

    // Anchor mask should have multiple hits at peak energy
    uint64_t anchorMask = seq.GetAnchorMask();
    int anchorHits = CountBits(static_cast<uint32_t>(anchorMask & 0xFFFFFFFF));
    REQUIRE(anchorHits >= 3);
}
