#pragma once

#include <cstdint>
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * Build phase enumeration for 3-stage phrase arc.
 *
 * BUILD operates in three phases based on phrase progress:
 * - GROOVE (0-60%): Stable pattern, no modification
 * - BUILD (60-87.5%): Ramping density and velocity
 * - FILL (87.5-100%): Maximum energy, forced accents
 *
 * Reference: Task 21 Phase D
 */
enum class BuildPhase
{
    GROOVE,   // 0-60%: stable
    BUILD,    // 60-87.5%: ramping
    FILL      // 87.5-100%: climax
};

/**
 * PunchParams: Velocity dynamics derived from PUNCH parameter
 *
 * PUNCH controls how dynamic the groove feels—the contrast between
 * loud and soft hits.
 *
 * Reference: docs/specs/main.md section 4.3 and 7.2
 */
struct PunchParams
{
    /// How often hits are accented (0.15-0.50)
    float accentProbability;

    /// Minimum velocity for non-accented hits (0.30-0.70)
    float velocityFloor;

    /// How much louder accents are (+0.10 to +0.35)
    float accentBoost;

    /// Random variation range (±0.05 to ±0.20)
    float velocityVariation;

    /**
     * Initialize with default values (moderate punch)
     */
    void Init()
    {
        accentProbability = 0.25f;
        velocityFloor     = 0.55f;
        accentBoost       = 0.20f;
        velocityVariation = 0.10f;
    }

    /**
     * Compute punch parameters from PUNCH knob value (0.0-1.0)
     *
     * @param punch PUNCH parameter value (0.0-1.0)
     */
    void ComputeFromPunch(float punch)
    {
        // Clamp input
        if (punch < 0.0f) punch = 0.0f;
        if (punch > 1.0f) punch = 1.0f;

        // PUNCH = 0%: Flat dynamics (all similar velocity)
        // PUNCH = 100%: Maximum dynamics (huge contrasts)
        accentProbability = 0.15f + punch * 0.35f;       // 15% to 50%
        velocityFloor     = 0.70f - punch * 0.40f;       // 70% down to 30%
        accentBoost       = 0.10f + punch * 0.25f;       // +10% to +35%
        velocityVariation = 0.05f + punch * 0.15f;       // ±5% to ±20%
    }
};

/**
 * BuildModifiers: Phrase arc modifiers derived from BUILD parameter
 *
 * BUILD controls the narrative arc of each phrase—how much tension
 * builds toward the end.
 *
 * Reference: docs/specs/main.md section 4.4
 */
struct BuildModifiers
{
    /// Density multiplier based on phrase position (1.0 = no change)
    float densityMultiplier;

    /// Fill intensity at current position (0.0-1.0)
    float fillIntensity;

    /// Whether we're in a fill zone
    bool inFillZone;

    /// Current phrase progress (0.0-1.0)
    float phraseProgress;

    /// Current build phase (GROOVE/BUILD/FILL)
    BuildPhase phase;

    /// Velocity floor boost (+0.0 to +0.15)
    float velocityBoost;

    /// Force all hits to be accented (FILL phase at high BUILD)
    bool forceAccents;

    /**
     * Initialize with default values (no build)
     */
    void Init()
    {
        densityMultiplier = 1.0f;
        fillIntensity     = 0.0f;
        inFillZone        = false;
        phraseProgress    = 0.0f;
        phase             = BuildPhase::GROOVE;
        velocityBoost     = 0.0f;
        forceAccents      = false;
    }

    /**
     * Compute build modifiers from BUILD value and phrase position
     *
     * @param build BUILD parameter value (0.0-1.0)
     * @param progress Current phrase progress (0.0-1.0)
     */
    void ComputeFromBuild(float build, float progress)
    {
        // Clamp inputs
        if (build < 0.0f) build = 0.0f;
        if (build > 1.0f) build = 1.0f;
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;

        phraseProgress = progress;

        // BUILD = 0%: Flat throughout (no density change)
        // BUILD = 100%: Dramatic arc (density increases toward end)

        // Density ramps up toward phrase end
        float rampAmount = build * progress * 0.5f;  // Up to 50% denser at end
        densityMultiplier = 1.0f + rampAmount;

        // Fill zone is last 12.5% of phrase (last bar of 8-bar phrase)
        inFillZone = (progress > 0.875f);

        // Fill intensity increases with BUILD and proximity to phrase end
        if (inFillZone)
        {
            float fillProgress = (progress - 0.875f) / 0.125f;  // 0-1 within fill zone
            fillIntensity = build * fillProgress;
        }
        else
        {
            fillIntensity = 0.0f;
        }
    }
};

/**
 * FillInputState: State for the fill CV input
 *
 * The fill input is "pressure-sensitive": gate detection for triggering
 * fills, CV level for fill intensity.
 *
 * Reference: docs/specs/main.md section 3.3
 */
struct FillInputState
{
    /// Whether fill gate is currently high (>1V detected)
    bool gateHigh;

    /// Fill intensity from CV level (0.0-1.0, from 0-5V)
    float intensity;

    /// Whether a fill was just triggered (rising edge detected)
    bool triggered;

    /// Whether we're in live fill mode (button held >500ms)
    bool liveFillMode;

    /// Whether a fill is queued for next phrase (button tap)
    bool fillQueued;

    /**
     * Initialize with default values (no fill)
     */
    void Init()
    {
        gateHigh     = false;
        intensity    = 0.0f;
        triggered    = false;
        liveFillMode = false;
        fillQueued   = false;
    }
};

/**
 * ControlState: Complete control parameter state
 *
 * This struct holds all runtime control parameters derived from
 * knobs, CVs, and buttons. It represents the current "intent" of
 * the performer.
 *
 * Reference: docs/specs/main.md section 4
 */
struct ControlState
{
    // =========================================================================
    // Performance Mode Primary (CV-able)
    // =========================================================================

    /// ENERGY: Hit density (0.0-1.0)
    float energy;

    /// BUILD: Phrase arc (0.0-1.0)
    float build;

    /// FIELD X: Syncopation axis (0.0-1.0)
    float fieldX;

    /// FIELD Y: Complexity axis (0.0-1.0)
    float fieldY;

    // =========================================================================
    // Performance Mode Shift
    // =========================================================================

    /// PUNCH: Velocity dynamics (0.0-1.0)
    float punch;

    /// GENRE: Style bank selection
    Genre genre;

    /// DRIFT: Pattern evolution rate (0.0-1.0)
    float drift;

    /// BALANCE: Voice ratio (0.0-1.0, 0=anchor-heavy, 1=shimmer-heavy)
    float balance;

    // =========================================================================
    // Config Mode Primary
    // =========================================================================

    /// Pattern length in steps (16, 24, 32, or 64)
    int patternLength;

    /// Base swing amount (0.0-1.0)
    float swing;

    /// AUX output mode
    AuxMode auxMode;

    /// Reset behavior
    ResetMode resetMode;

    // =========================================================================
    // Config Mode Shift
    // =========================================================================

    /// Phrase length in bars (auto-derived from patternLength, legacy field kept for persistence)
    int phraseLength;

    /// Clock division (1, 2, 4, or 8)
    int clockDivision;

    /// AUX density setting
    AuxDensity auxDensity;

    /// Voice coupling mode
    VoiceCoupling voiceCoupling;

    // =========================================================================
    // Derived Parameters
    // =========================================================================

    /// Current energy zone (derived from energy)
    EnergyZone energyZone;

    /// Computed punch parameters
    PunchParams punchParams;

    /// Computed build modifiers
    BuildModifiers buildModifiers;

    /// Fill input state
    FillInputState fillInput;

    // =========================================================================
    // CV Modulation Values (raw CV inputs before combination)
    // =========================================================================

    /// CV modulation for energy (±0.5)
    float energyCV;

    /// CV modulation for build (±0.5)
    float buildCV;

    /// CV modulation for field X (±0.5)
    float fieldXCV;

    /// CV modulation for field Y (±0.5)
    float fieldYCV;

    /// FLAVOR CV input (0.0-1.0, affects timing/BROKEN)
    float flavorCV;

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * Initialize with default values
     */
    void Init()
    {
        // Performance primary
        energy = 0.5f;
        build  = 0.0f;
        fieldX = 0.0f;
        fieldY = 0.0f;

        // Performance shift
        punch   = 0.5f;
        genre   = Genre::TECHNO;
        drift   = 0.0f;
        balance = 0.5f;

        // Config primary
        patternLength = 32;
        swing         = 0.0f;
        auxMode       = AuxMode::HAT;
        resetMode     = ResetMode::STEP;  // Hardcoded - no longer exposed in UI

        // Config shift
        phraseLength  = 4;
        clockDivision = 1;
        auxDensity    = AuxDensity::NORMAL;
        voiceCoupling = VoiceCoupling::INDEPENDENT;

        // Derived
        energyZone = EnergyZone::GROOVE;
        punchParams.Init();
        buildModifiers.Init();
        fillInput.Init();

        // CV modulation
        energyCV = 0.0f;
        buildCV  = 0.0f;
        fieldXCV = 0.0f;
        fieldYCV = 0.0f;
        flavorCV = 0.0f;
    }

    /**
     * Update derived parameters after control changes
     *
     * @param phraseProgress Current phrase progress (0.0-1.0)
     */
    void UpdateDerived(float phraseProgress)
    {
        energyZone = GetEnergyZone(energy);
        punchParams.ComputeFromPunch(punch);
        buildModifiers.ComputeFromBuild(build, phraseProgress);
    }

    /**
     * Get effective energy (knob + CV modulation, clamped 0-1)
     */
    float GetEffectiveEnergy() const
    {
        float val = energy + energyCV;
        if (val < 0.0f) val = 0.0f;
        if (val > 1.0f) val = 1.0f;
        return val;
    }

    /**
     * Get effective build (knob + CV modulation, clamped 0-1)
     */
    float GetEffectiveBuild() const
    {
        float val = build + buildCV;
        if (val < 0.0f) val = 0.0f;
        if (val > 1.0f) val = 1.0f;
        return val;
    }

    /**
     * Get effective field X (knob + CV modulation, clamped 0-1)
     */
    float GetEffectiveFieldX() const
    {
        float val = fieldX + fieldXCV;
        if (val < 0.0f) val = 0.0f;
        if (val > 1.0f) val = 1.0f;
        return val;
    }

    /**
     * Get effective field Y (knob + CV modulation, clamped 0-1)
     */
    float GetEffectiveFieldY() const
    {
        float val = fieldY + fieldYCV;
        if (val < 0.0f) val = 0.0f;
        if (val > 1.0f) val = 1.0f;
        return val;
    }

    /**
     * Get auto-derived phrase length based on pattern length
     *
     * Derivation keeps total phrase around 128 steps (8 bars at 16th notes)
     * for consistent phrase arc timing.
     *
     * @return Phrase length in bars (1, 2, 4, or 8)
     */
    int GetDerivedPhraseLength() const
    {
        // Target ~128 steps total, minimum 2 bars
        switch (patternLength)
        {
            case 16: return 8;   // 16 × 8 = 128 steps
            case 24: return 5;   // 24 × 5 = 120 steps
            case 32: return 4;   // 32 × 4 = 128 steps
            case 64: return 2;   // 64 × 2 = 128 steps
            default: return 4;   // Fallback to 4 bars (standard 8-bar phrase)
        }
    }
};

} // namespace daisysp_idm_grids
