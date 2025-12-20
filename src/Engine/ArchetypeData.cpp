#include "ArchetypeData.h"
#include <cstring>

namespace daisysp_idm_grids
{

// Helper to copy weight arrays
static void CopyWeights(float* dest, const float* src)
{
    std::memcpy(dest, src, kMaxSteps * sizeof(float));
}

// Helper to load techno archetype
static void LoadTechnoArchetype(int index, ArchetypeDNA& out)
{
    using namespace techno;

    switch (index)
    {
        case 0:  // [0,0] Minimal
            CopyWeights(out.anchorWeights, kMinimal_Anchor);
            CopyWeights(out.shimmerWeights, kMinimal_Shimmer);
            CopyWeights(out.auxWeights, kMinimal_Aux);
            break;
        case 1:  // [1,0] Steady
            CopyWeights(out.anchorWeights, kSteady_Anchor);
            CopyWeights(out.shimmerWeights, kSteady_Shimmer);
            CopyWeights(out.auxWeights, kSteady_Aux);
            break;
        case 2:  // [2,0] Displaced
            CopyWeights(out.anchorWeights, kDisplaced_Anchor);
            CopyWeights(out.shimmerWeights, kDisplaced_Shimmer);
            CopyWeights(out.auxWeights, kDisplaced_Aux);
            break;
        case 3:  // [0,1] Driving
            CopyWeights(out.anchorWeights, kDriving_Anchor);
            CopyWeights(out.shimmerWeights, kDriving_Shimmer);
            CopyWeights(out.auxWeights, kDriving_Aux);
            break;
        case 4:  // [1,1] Groovy
            CopyWeights(out.anchorWeights, kGroovy_Anchor);
            CopyWeights(out.shimmerWeights, kGroovy_Shimmer);
            CopyWeights(out.auxWeights, kGroovy_Aux);
            break;
        case 5:  // [2,1] Broken
            CopyWeights(out.anchorWeights, kBroken_Anchor);
            CopyWeights(out.shimmerWeights, kBroken_Shimmer);
            CopyWeights(out.auxWeights, kBroken_Aux);
            break;
        case 6:  // [0,2] Busy
            CopyWeights(out.anchorWeights, kBusy_Anchor);
            CopyWeights(out.shimmerWeights, kBusy_Shimmer);
            CopyWeights(out.auxWeights, kBusy_Aux);
            break;
        case 7:  // [1,2] Polyrhythm
            CopyWeights(out.anchorWeights, kPolyrhythm_Anchor);
            CopyWeights(out.shimmerWeights, kPolyrhythm_Shimmer);
            CopyWeights(out.auxWeights, kPolyrhythm_Aux);
            break;
        case 8:  // [2,2] Chaos
            CopyWeights(out.anchorWeights, kChaos_Anchor);
            CopyWeights(out.shimmerWeights, kChaos_Shimmer);
            CopyWeights(out.auxWeights, kChaos_Aux);
            break;
        default:
            // Default: initialize with zeros
            out.Init();
            return;
    }

    // Load metadata
    out.swingAmount = kSwingAmounts[index];
    out.swingPattern = kSwingPatterns[index];
    out.defaultCouple = kDefaultCouples[index];
    out.fillDensityMultiplier = kFillMultipliers[index];
    out.anchorAccentMask = kAccentMasks[index];
    out.shimmerAccentMask = kAccentMasks[index];  // Use same mask for both
    out.ratchetEligibleMask = kRatchetMasks[index];
}

// Helper to load tribal archetype
static void LoadTribalArchetype(int index, ArchetypeDNA& out)
{
    using namespace tribal;

    switch (index)
    {
        case 0:
            CopyWeights(out.anchorWeights, kMinimal_Anchor);
            CopyWeights(out.shimmerWeights, kMinimal_Shimmer);
            CopyWeights(out.auxWeights, kMinimal_Aux);
            break;
        case 1:
            CopyWeights(out.anchorWeights, kSteady_Anchor);
            CopyWeights(out.shimmerWeights, kSteady_Shimmer);
            CopyWeights(out.auxWeights, kSteady_Aux);
            break;
        case 2:
            CopyWeights(out.anchorWeights, kDisplaced_Anchor);
            CopyWeights(out.shimmerWeights, kDisplaced_Shimmer);
            CopyWeights(out.auxWeights, kDisplaced_Aux);
            break;
        case 3:
            CopyWeights(out.anchorWeights, kDriving_Anchor);
            CopyWeights(out.shimmerWeights, kDriving_Shimmer);
            CopyWeights(out.auxWeights, kDriving_Aux);
            break;
        case 4:
            CopyWeights(out.anchorWeights, kGroovy_Anchor);
            CopyWeights(out.shimmerWeights, kGroovy_Shimmer);
            CopyWeights(out.auxWeights, kGroovy_Aux);
            break;
        case 5:
            CopyWeights(out.anchorWeights, kBroken_Anchor);
            CopyWeights(out.shimmerWeights, kBroken_Shimmer);
            CopyWeights(out.auxWeights, kBroken_Aux);
            break;
        case 6:
            CopyWeights(out.anchorWeights, kBusy_Anchor);
            CopyWeights(out.shimmerWeights, kBusy_Shimmer);
            CopyWeights(out.auxWeights, kBusy_Aux);
            break;
        case 7:
            CopyWeights(out.anchorWeights, kPolyrhythm_Anchor);
            CopyWeights(out.shimmerWeights, kPolyrhythm_Shimmer);
            CopyWeights(out.auxWeights, kPolyrhythm_Aux);
            break;
        case 8:
            CopyWeights(out.anchorWeights, kChaos_Anchor);
            CopyWeights(out.shimmerWeights, kChaos_Shimmer);
            CopyWeights(out.auxWeights, kChaos_Aux);
            break;
        default:
            out.Init();
            return;
    }

    out.swingAmount = kSwingAmounts[index];
    out.swingPattern = kSwingPatterns[index];
    out.defaultCouple = kDefaultCouples[index];
    out.fillDensityMultiplier = kFillMultipliers[index];
    out.anchorAccentMask = kAccentMasks[index];
    out.shimmerAccentMask = kAccentMasks[index];
    out.ratchetEligibleMask = kRatchetMasks[index];
}

// Helper to load IDM archetype
static void LoadIdmArchetype(int index, ArchetypeDNA& out)
{
    using namespace idm;

    switch (index)
    {
        case 0:
            CopyWeights(out.anchorWeights, kMinimal_Anchor);
            CopyWeights(out.shimmerWeights, kMinimal_Shimmer);
            CopyWeights(out.auxWeights, kMinimal_Aux);
            break;
        case 1:
            CopyWeights(out.anchorWeights, kSteady_Anchor);
            CopyWeights(out.shimmerWeights, kSteady_Shimmer);
            CopyWeights(out.auxWeights, kSteady_Aux);
            break;
        case 2:
            CopyWeights(out.anchorWeights, kDisplaced_Anchor);
            CopyWeights(out.shimmerWeights, kDisplaced_Shimmer);
            CopyWeights(out.auxWeights, kDisplaced_Aux);
            break;
        case 3:
            CopyWeights(out.anchorWeights, kDriving_Anchor);
            CopyWeights(out.shimmerWeights, kDriving_Shimmer);
            CopyWeights(out.auxWeights, kDriving_Aux);
            break;
        case 4:
            CopyWeights(out.anchorWeights, kGroovy_Anchor);
            CopyWeights(out.shimmerWeights, kGroovy_Shimmer);
            CopyWeights(out.auxWeights, kGroovy_Aux);
            break;
        case 5:
            CopyWeights(out.anchorWeights, kBroken_Anchor);
            CopyWeights(out.shimmerWeights, kBroken_Shimmer);
            CopyWeights(out.auxWeights, kBroken_Aux);
            break;
        case 6:
            CopyWeights(out.anchorWeights, kBusy_Anchor);
            CopyWeights(out.shimmerWeights, kBusy_Shimmer);
            CopyWeights(out.auxWeights, kBusy_Aux);
            break;
        case 7:
            CopyWeights(out.anchorWeights, kPolyrhythm_Anchor);
            CopyWeights(out.shimmerWeights, kPolyrhythm_Shimmer);
            CopyWeights(out.auxWeights, kPolyrhythm_Aux);
            break;
        case 8:
            CopyWeights(out.anchorWeights, kChaos_Anchor);
            CopyWeights(out.shimmerWeights, kChaos_Shimmer);
            CopyWeights(out.auxWeights, kChaos_Aux);
            break;
        default:
            out.Init();
            return;
    }

    out.swingAmount = kSwingAmounts[index];
    out.swingPattern = kSwingPatterns[index];
    out.defaultCouple = kDefaultCouples[index];
    out.fillDensityMultiplier = kFillMultipliers[index];
    out.anchorAccentMask = kAccentMasks[index];
    out.shimmerAccentMask = kAccentMasks[index];
    out.ratchetEligibleMask = kRatchetMasks[index];
}

void LoadArchetypeData(Genre genre, int archetypeIndex, ArchetypeDNA& outArchetype)
{
    // Validate index
    if (archetypeIndex < 0 || archetypeIndex >= kArchetypesPerGenre)
    {
        outArchetype.Init();
        return;
    }

    switch (genre)
    {
        case Genre::TECHNO:
            LoadTechnoArchetype(archetypeIndex, outArchetype);
            break;
        case Genre::TRIBAL:
            LoadTribalArchetype(archetypeIndex, outArchetype);
            break;
        case Genre::IDM:
            LoadIdmArchetype(archetypeIndex, outArchetype);
            break;
        default:
            outArchetype.Init();
            break;
    }

    // Set grid position from index
    outArchetype.gridX = static_cast<uint8_t>(archetypeIndex % 3);
    outArchetype.gridY = static_cast<uint8_t>(archetypeIndex / 3);
}

} // namespace daisysp_idm_grids
