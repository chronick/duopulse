#pragma once

#include <cstdint>
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * Shape phase enumeration for 3-stage phrase arc.
 *
 * SHAPE operates in three phases based on phrase progress:
 * - GROOVE (0-60%): Stable pattern, no modification
 * - BUILD (60-87.5%): Ramping density and velocity
 * - FILL (87.5-100%): Maximum energy, forced accents
 *
 * V5 Change: Renamed from BuildPhase to ShapePhase (Task 27)
 * Reference: Task 21 Phase D
 */
enum class ShapePhase
{
    GROOVE,   // 0-60%: stable
    BUILD,    // 60-87.5%: ramping
    FILL      // 87.5-100%: climax
};

/**
 * AccentParams: Velocity dynamics derived from ACCENT parameter
 *
 * ACCENT controls how dynamic the groove feels-the contrast between
 * loud and soft hits.
 *
 * V5 Change: Renamed from PunchParams to AccentParams (Task 27)
 * The internal algorithm remains unchanged for now (Task 35 will update it).
 *
 * Reference: docs/specs/main.md section 4.3 and 7.2
 */
struct AccentParams
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
     * Compute accent parameters from ACCENT knob value (0.0-1.0)
     *
     * V5 Change: Renamed from ComputeFromPunch to ComputeFromAccent (Task 27)
     * The algorithm is unchanged for now (Task 35 will update it).
     *
     * @param accent ACCENT parameter value (0.0-1.0)
     */
    void ComputeFromAccent(float accent)
    {
        // Clamp input
        if (accent < 0.0f) accent = 0.0f;
        if (accent > 1.0f) accent = 1.0f;

        // ACCENT = 0%: Flat dynamics (all similar velocity)
        // ACCENT = 100%: Maximum dynamics (huge contrasts)
        accentProbability = 0.15f + accent * 0.35f;       // 15% to 50%
        velocityFloor     = 0.70f - accent * 0.40f;       // 70% down to 30%
        accentBoost       = 0.10f + accent * 0.25f;       // +10% to +35%
        velocityVariation = 0.05f + accent * 0.15f;       // ±5% to ±20%
    }

    /// Legacy alias for ComputeFromAccent (V5: renamed from ComputeFromPunch)
    void ComputeFromPunch(float punch)
    {
        ComputeFromAccent(punch);
    }
};

/**
 * ShapeModifiers: Phrase arc modifiers derived from SHAPE parameter
 *
 * SHAPE controls the narrative arc of each phrase-how much tension
 * builds toward the end.
 *
 * V5 Change: Renamed from BuildModifiers to ShapeModifiers (Task 27)
 * The internal algorithm remains unchanged for now (Task 28 will update it).
 *
 * Reference: docs/specs/main.md section 4.4
 */
struct ShapeModifiers
{
    /// Density multiplier based on phrase position (1.0 = no change)
    float densityMultiplier;

    /// Fill intensity at current position (0.0-1.0)
    float fillIntensity;

    /// Whether we're in a fill zone
    bool inFillZone;

    /// Current phrase progress (0.0-1.0)
    float phraseProgress;

    /// Current shape phase (GROOVE/BUILD/FILL)
    ShapePhase phase;

    /// Velocity floor boost (+0.0 to +0.15)
    float velocityBoost;

    /// Force all hits to be accented (FILL phase at high SHAPE)
    bool forceAccents;

    /**
     * Initialize with default values (no shape modulation)
     */
    void Init()
    {
        densityMultiplier = 1.0f;
        fillIntensity     = 0.0f;
        inFillZone        = false;
        phraseProgress    = 0.0f;
        phase             = ShapePhase::GROOVE;
        velocityBoost     = 0.0f;
        forceAccents      = false;
    }

    /**
     * Compute shape modifiers from SHAPE value and phrase position
     *
     * V5 Change: Renamed from ComputeFromBuild to ComputeFromShape (Task 27)
     * The algorithm is unchanged for now (Task 28 will update it).
     *
     * @param shape SHAPE parameter value (0.0-1.0)
     * @param progress Current phrase progress (0.0-1.0)
     */
    void ComputeFromShape(float shape, float progress)
    {
        // Clamp inputs
        if (shape < 0.0f) shape = 0.0f;
        if (shape > 1.0f) shape = 1.0f;
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;

        phraseProgress = progress;

        // SHAPE = 0%: Flat throughout (no density change)
        // SHAPE = 100%: Dramatic arc (density increases toward end)

        // Density ramps up toward phrase end
        float rampAmount = shape * progress * 0.5f;  // Up to 50% denser at end
        densityMultiplier = 1.0f + rampAmount;

        // Fill zone is last 12.5% of phrase (last bar of 8-bar phrase)
        inFillZone = (progress > 0.875f);

        // Fill intensity increases with SHAPE and proximity to phrase end
        if (inFillZone)
        {
            float fillProgress = (progress - 0.875f) / 0.125f;  // 0-1 within fill zone
            fillIntensity = shape * fillProgress;
        }
        else
        {
            fillIntensity = 0.0f;
        }
    }

    /// Legacy alias for ComputeFromShape (V5: renamed from ComputeFromBuild)
    void ComputeFromBuild(float build, float progress)
    {
        ComputeFromShape(build, progress);
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
 * V5 Changes (Task 27):
 * - Renamed fieldX/fieldY to axisX/axisY
 * - Renamed build to shape
 * - Renamed punch to accent (moved to config K4)
 * - Removed genre, balance, auxDensity, voiceCoupling (internal only)
 * - Added clockDiv to config mode
 * - Moved drift to config K3
 * - Eliminated shift layers (all parameters now direct-access)
 *
 * Reference: docs/specs/main.md section 4
 */
struct ControlState
{
    // =========================================================================
    // V5 Performance Mode (4 direct knobs, CV-modulatable)
    // =========================================================================

    /// ENERGY: Hit density (0.0-1.0)
    float energy;

    /// SHAPE: Phrase arc / algorithm blending (0.0-1.0)
    /// V5 Change: Renamed from BUILD (Task 27)
    float shape;

    /// AXIS X: Beat/syncopation position (0.0-1.0)
    /// V5 Change: Renamed from FIELD X (Task 27)
    float axisX;

    /// AXIS Y: Intricacy/complexity (0.0-1.0)
    /// V5 Change: Renamed from FIELD Y (Task 27)
    float axisY;

    // =========================================================================
    // V5 Config Mode (4 direct knobs, no shift layer)
    // =========================================================================

    /// CLOCK DIV: Tempo division/multiplication
    /// V5 Change: Was shift+K2, now config K1 (Task 27)
    /// Values: -4 (÷4), -2 (÷2), 1 (×1), 4 (×4)
    int clockDiv;

    /// SWING: Base groove amount (0.0-1.0)
    /// V5: Unchanged, config K2
    float swing;

    /// DRIFT: Pattern evolution rate (0.0-1.0)
    /// V5 Change: Was perf shift+K3, now config K3 (Task 27)
    float drift;

    /// ACCENT: Velocity dynamics (0.0-1.0)
    /// V5 Change: Renamed from PUNCH, was perf shift+K1, now config K4 (Task 27)
    float accent;

    // =========================================================================
    // Internal Parameters (not exposed in V5 UI)
    // =========================================================================

    /// Pattern length in steps (16, 24, 32, or 64)
    /// V5: Auto-derived or preset, not knob-controlled
    int patternLength;

    /// AUX output mode
    /// V5: Auto-derived or preset, not knob-controlled
    AuxMode auxMode;

    /// Reset behavior (always STEP in V5)
    ResetMode resetMode;

    /// Phrase length in bars (auto-derived from patternLength)
    int phraseLength;

    /// Clock division (legacy alias for clockDiv)
    /// V5: Kept for backward compatibility
    int clockDivision;

    /// AUX density setting (always NORMAL in V5)
    AuxDensity auxDensity;

    /// Voice coupling mode (always INDEPENDENT in V5, COMPLEMENT in Task 30)
    VoiceCoupling voiceCoupling;

    /// Genre (always TECHNO in V5)
    Genre genre;

    /// Balance (0.5 = equal voices, internal only in V5)
    float balance;

    // =========================================================================
    // Derived Parameters
    // =========================================================================

    /// Current energy zone (derived from energy)
    EnergyZone energyZone;

    /// Computed accent parameters
    /// V5 Change: Renamed from punchParams (Task 27)
    AccentParams accentParams;

    /// Computed shape modifiers
    /// V5 Change: Renamed from buildModifiers (Task 27)
    ShapeModifiers shapeModifiers;

    /// Fill input state
    FillInputState fillInput;

    // =========================================================================
    // CV Modulation Values (raw CV inputs before combination)
    // =========================================================================

    /// CV modulation for energy (±0.5)
    float energyCV;

    /// CV modulation for shape (±0.5)
    /// V5 Change: Renamed from buildCV (Task 27)
    float shapeCV;

    /// CV modulation for axis X (±0.5)
    /// V5 Change: Renamed from fieldXCV (Task 27)
    float axisXCV;

    /// CV modulation for axis Y (±0.5)
    /// V5 Change: Renamed from fieldYCV (Task 27)
    float axisYCV;

    /// FLAVOR CV input (0.0-1.0, affects timing/BROKEN)
    float flavorCV;

    // =========================================================================
    // Initialization
    // =========================================================================

    /**
     * Initialize with V5 boot defaults
     *
     * V5 Boot Defaults (Task 27):
     * - energy = 0.50f (50% neutral density)
     * - shape = 0.30f (30% humanized euclidean zone)
     * - axisX = 0.50f (50% neutral beat position)
     * - axisY = 0.50f (50% moderate intricacy)
     * - clockDiv = 1 (x1 no division)
     * - swing = 0.50f (50% neutral groove)
     * - drift = 0.0f (0% locked pattern)
     * - accent = 0.50f (50% moderate dynamics)
     */
    void Init()
    {
        // V5 Performance Mode (4 direct knobs)
        energy = 0.50f;  // V5: 50% neutral density
        shape  = 0.30f;  // V5: 30% humanized euclidean zone
        axisX  = 0.50f;  // V5: 50% neutral beat position
        axisY  = 0.50f;  // V5: 50% moderate intricacy

        // V5 Config Mode (4 direct knobs)
        clockDiv = 1;     // V5: x1 no division
        swing    = 0.50f; // V5: 50% neutral groove
        drift    = 0.0f;  // V5: 0% locked pattern
        accent   = 0.50f; // V5: 50% moderate dynamics

        // Internal parameters (not exposed in V5 UI)
        patternLength = 32;
        auxMode       = AuxMode::HAT;
        resetMode     = ResetMode::STEP;
        phraseLength  = 4;
        clockDivision = 1;  // Legacy alias for clockDiv
        auxDensity    = AuxDensity::NORMAL;
        voiceCoupling = VoiceCoupling::INDEPENDENT;
        genre         = Genre::TECHNO;
        balance       = 0.5f;

        // Derived
        energyZone = EnergyZone::GROOVE;
        accentParams.Init();
        shapeModifiers.Init();
        fillInput.Init();

        // CV modulation
        energyCV = 0.0f;
        shapeCV  = 0.0f;
        axisXCV  = 0.0f;
        axisYCV  = 0.0f;
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
        accentParams.ComputeFromAccent(accent);
        shapeModifiers.ComputeFromShape(shape, phraseProgress);
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
     * Get effective shape (knob + CV modulation, clamped 0-1)
     * V5 Change: Renamed from GetEffectiveBuild (Task 27)
     */
    float GetEffectiveShape() const
    {
        float val = shape + shapeCV;
        if (val < 0.0f) val = 0.0f;
        if (val > 1.0f) val = 1.0f;
        return val;
    }

    /**
     * Get effective axis X (knob + CV modulation, clamped 0-1)
     * V5 Change: Renamed from GetEffectiveFieldX (Task 27)
     */
    float GetEffectiveAxisX() const
    {
        float val = axisX + axisXCV;
        if (val < 0.0f) val = 0.0f;
        if (val > 1.0f) val = 1.0f;
        return val;
    }

    /**
     * Get effective axis Y (knob + CV modulation, clamped 0-1)
     * V5 Change: Renamed from GetEffectiveFieldY (Task 27)
     */
    float GetEffectiveAxisY() const
    {
        float val = axisY + axisYCV;
        if (val < 0.0f) val = 0.0f;
        if (val > 1.0f) val = 1.0f;
        return val;
    }

    // =========================================================================
    // Legacy Accessors (for backward compatibility)
    // =========================================================================

    /// Legacy alias for shape
    float GetEffectiveBuild() const { return GetEffectiveShape(); }

    /// Legacy alias for axisX
    float GetEffectiveFieldX() const { return GetEffectiveAxisX(); }

    /// Legacy alias for axisY
    float GetEffectiveFieldY() const { return GetEffectiveAxisY(); }

    // =========================================================================
    // Legacy Field References (for backward compatibility)
    // =========================================================================

    /// Legacy reference: build is alias for shape
    float& build = shape;

    /// Legacy reference: fieldX is alias for axisX
    float& fieldX = axisX;

    /// Legacy reference: fieldY is alias for axisY
    float& fieldY = axisY;

    /// Legacy reference: punch is alias for accent
    float& punch = accent;

    /// Legacy reference: buildCV is alias for shapeCV
    float& buildCV = shapeCV;

    /// Legacy reference: fieldXCV is alias for axisXCV
    float& fieldXCV = axisXCV;

    /// Legacy reference: fieldYCV is alias for axisYCV
    float& fieldYCV = axisYCV;

    /// Legacy reference: punchParams is alias for accentParams
    AccentParams& punchParams = accentParams;

    /// Legacy reference: buildModifiers is alias for shapeModifiers
    ShapeModifiers& buildModifiers = shapeModifiers;

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

// =============================================================================
// Legacy Type Aliases (for backward compatibility)
// =============================================================================

/// Legacy alias for ShapePhase (V5: renamed from BuildPhase)
using BuildPhase = ShapePhase;

/// Legacy alias for AccentParams (V5: renamed from PunchParams)
using PunchParams = AccentParams;

/// Legacy alias for ShapeModifiers (V5: renamed from BuildModifiers)
using BuildModifiers = ShapeModifiers;

} // namespace daisysp_idm_grids
