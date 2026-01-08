/**
 * Pattern Visualization CLI Tool
 *
 * Standalone tool to output deterministic drum patterns for given parameters.
 * Uses the REAL firmware algorithms for accurate pattern preview.
 *
 * Usage:
 *   ./build/pattern_viz [options]
 *
 * Options:
 *   --energy=0.50    ENERGY parameter (0.0-1.0)
 *   --shape=0.30     SHAPE parameter (0.0-1.0)
 *   --axis-x=0.50    AXIS X parameter (0.0-1.0)
 *   --axis-y=0.50    AXIS Y parameter (0.0-1.0)
 *   --drift=0.00     DRIFT parameter (0.0-1.0)
 *   --accent=0.50    ACCENT parameter (0.0-1.0)
 *   --seed=0xDEADBEEF  Pattern seed (hex or decimal)
 *   --length=32      Pattern length (16 or 32)
 *   --sweep=shape    Sweep a parameter (shape, energy, axis-x, axis-y)
 *   --output=file    Output to file (default: stdout)
 *   --format=grid    Output format: grid, csv, mask
 *
 * Examples:
 *   ./build/pattern_viz --energy=0.7 --shape=0.5
 *   ./build/pattern_viz --sweep=shape --output=patterns.txt
 *   ./build/pattern_viz --format=csv > patterns.csv
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

#include "../src/Engine/DuoPulseTypes.h"
#include "../src/Engine/PatternField.h"
#include "../src/Engine/VoiceRelation.h"
#include "../src/Engine/VelocityCompute.h"
#include "../src/Engine/HashUtils.h"
#include "../src/Engine/GumbelSampler.h"
#include "../src/Engine/HitBudget.h"
#include "../src/Engine/GuardRails.h"

using namespace daisysp_idm_grids;

// =============================================================================
// Pattern Generation (matches firmware exactly)
// =============================================================================

struct PatternParams
{
    float energy = 0.50f;
    float shape = 0.30f;
    float axisX = 0.50f;
    float axisY = 0.50f;
    float drift = 0.00f;
    float accent = 0.50f;
    uint32_t seed = 0xDEADBEEF;
    int patternLength = 32;
};

struct PatternData
{
    uint32_t v1Mask = 0;
    uint32_t v2Mask = 0;
    uint32_t auxMask = 0;
    float v1Velocity[kMaxSteps] = {0};
    float v2Velocity[kMaxSteps] = {0};
    float auxVelocity[kMaxSteps] = {0};
    int patternLength = 32;
};

static int ComputeTargetHits(float energy, int patternLength, Voice voice, float shape = 0.5f)
{
    EnergyZone zone = GetEnergyZone(energy);
    BarBudget budget;
    ComputeBarBudget(energy, 0.5f, zone, AuxDensity::NORMAL, patternLength, 1.0f, shape, budget);

    switch (voice)
    {
        case Voice::ANCHOR:  return budget.anchorHits;
        case Voice::SHIMMER: return budget.shimmerHits;
        case Voice::AUX:     return budget.auxHits;
        default:             return budget.anchorHits;
    }
}

// V5 Task 44: Helper to rotate bitmask while preserving a specific step's state
// Used for anchor variation without disrupting beat 1 (Techno kick stability)
static uint32_t RotateWithPreserve(uint32_t mask, int rotation, int length, int preserveStep)
{
    if (rotation == 0 || length <= 1) return mask;

    // Check if preserve step is set
    bool preserveWasSet = (mask & (1U << preserveStep)) != 0;

    // Clear the preserve step before rotation
    mask &= ~(1U << preserveStep);

    // Rotate the remaining bits
    rotation = rotation % length;
    uint32_t lengthMask = (length >= 32) ? 0xFFFFFFFF : ((1U << length) - 1);
    mask = ((mask << rotation) | (mask >> (length - rotation))) & lengthMask;

    // Restore preserve step to its original position
    if (preserveWasSet) {
        mask |= (1U << preserveStep);
    }

    return mask;
}

static void GeneratePattern(const PatternParams& params, PatternData& out)
{
    out.patternLength = params.patternLength;
    out.v1Mask = 0;
    out.v2Mask = 0;
    out.auxMask = 0;

    EnergyZone zone = GetEnergyZone(params.energy);
    int minSpacing = GetMinSpacingForZone(zone);

    // Generate anchor weights
    float anchorWeights[kMaxSteps];
    ComputeShapeBlendedWeights(params.shape, params.energy, params.seed,
                               params.patternLength, anchorWeights);
    ApplyAxisBias(anchorWeights, params.axisX, params.axisY,
                  params.shape, params.seed, params.patternLength);

    // V5 Task 44: Add seed-based weight perturbation for anchor variation
    // Uses additive noise scaled to actually affect hit selection
    {
        // At low SHAPE, add more noise to break up the predictable pattern
        // At high SHAPE, weights already vary naturally, so less noise needed
        const float noiseScale = 0.4f * (1.0f - params.shape);
        for (int step = 0; step < params.patternLength; ++step) {
            // Protect beat 1 at low SHAPE (Techno kick stability)
            if (step == 0 && params.shape < 0.3f) continue;

            // Additive perturbation that can actually affect ranking
            float noise = (HashToFloat(params.seed, step + 1000) - 0.5f) * noiseScale;
            anchorWeights[step] = ClampWeight(anchorWeights[step] + noise);
        }
    }

    // Select anchor hits (SHAPE modulates budget - Task 39)
    int v1TargetHits = ComputeTargetHits(params.energy, params.patternLength, Voice::ANCHOR, params.shape);
    uint32_t eligibility = (1U << params.patternLength) - 1;
    out.v1Mask = SelectHitsGumbelTopK(anchorWeights, eligibility, v1TargetHits,
                                       params.seed, params.patternLength, minSpacing);

    // Apply guard rails
    uint32_t dummyShimmer = 0;
    ApplyHardGuardRails(out.v1Mask, dummyShimmer, zone, Genre::TECHNO, params.patternLength);

    // V5 Task 44: Apply seed-based rotation for anchor variation (AFTER guard rails)
    // Rotate non-beat-1 positions while preserving step 0 for Techno kick stability
    if (params.shape < 0.7f) {
        int maxRotation = std::max(1, params.patternLength / 4);
        int rotation = static_cast<int>(HashToFloat(params.seed, 2000) * maxRotation);
        out.v1Mask = RotateWithPreserve(out.v1Mask, rotation, params.patternLength, 0);
    }

    // Generate shimmer with COMPLEMENT (SHAPE modulates budget - Task 39)
    float shimmerWeights[kMaxSteps];
    ComputeShapeBlendedWeights(params.shape, params.energy, params.seed + 1,
                               params.patternLength, shimmerWeights);
    int v2TargetHits = ComputeTargetHits(params.energy, params.patternLength, Voice::SHIMMER, params.shape);
    out.v2Mask = ApplyComplementRelationship(out.v1Mask, shimmerWeights,
                                              params.drift, params.seed + 2,
                                              params.patternLength, v2TargetHits);

    // Generate aux
    int auxTargetHits = ComputeTargetHits(params.energy, params.patternLength, Voice::AUX);
    float auxWeights[kMaxSteps];
    uint32_t combinedMask = out.v1Mask | out.v2Mask;
    for (int i = 0; i < params.patternLength; ++i)
    {
        float metricW = GetMetricWeight(i, params.patternLength);
        auxWeights[i] = 1.0f - metricW * 0.5f;
        if ((combinedMask & (1U << i)) != 0)
            auxWeights[i] *= 0.3f;
    }
    out.auxMask = SelectHitsGumbelTopK(auxWeights, eligibility, auxTargetHits,
                                        params.seed + 3, params.patternLength, 0);

    // Compute velocities
    for (int step = 0; step < params.patternLength; ++step)
    {
        if ((out.v1Mask & (1U << step)) != 0)
            out.v1Velocity[step] = ComputeAccentVelocity(params.accent, step, params.patternLength, params.seed);
        if ((out.v2Mask & (1U << step)) != 0)
            out.v2Velocity[step] = ComputeAccentVelocity(params.accent * 0.7f, step, params.patternLength, params.seed + 1);
        if ((out.auxMask & (1U << step)) != 0)
        {
            float baseVel = 0.5f + params.energy * 0.3f;
            float variation = (HashToFloat(params.seed + 4, step) - 0.5f) * 0.15f;
            out.auxVelocity[step] = std::max(0.3f, std::min(1.0f, baseVel + variation));
        }
    }
}

// =============================================================================
// Output Formatters
// =============================================================================

static int CountHits(uint32_t mask, int length)
{
    int count = 0;
    for (int i = 0; i < length; ++i)
        if ((mask & (1U << i)) != 0) count++;
    return count;
}

static void PrintPatternGrid(std::ostream& out, const PatternParams& params, const PatternData& pattern)
{
    out << "\n=== Pattern Visualization ===\n";
    out << "Params: ENERGY=" << std::fixed << std::setprecision(2) << params.energy
        << " SHAPE=" << params.shape
        << " AXIS_X=" << params.axisX
        << " AXIS_Y=" << params.axisY << "\n";
    out << "Config: DRIFT=" << params.drift
        << " ACCENT=" << params.accent << "\n";
    out << "Seed: 0x" << std::hex << std::uppercase << params.seed << std::dec << "\n";
    out << "Pattern Length: " << params.patternLength << " steps\n\n";

    out << "Step  V1  V2  Aux  V1_Vel  V2_Vel  Aux_Vel  Metric\n";
    out << std::string(56, '-') << "\n";

    for (int step = 0; step < pattern.patternLength; ++step)
    {
        bool v1Hit = (pattern.v1Mask & (1U << step)) != 0;
        bool v2Hit = (pattern.v2Mask & (1U << step)) != 0;
        bool auxHit = (pattern.auxMask & (1U << step)) != 0;
        float metric = GetMetricWeight(step, pattern.patternLength);

        out << std::setw(2) << step << "    "
            << (v1Hit ? "X" : ".") << "   "
            << (v2Hit ? "X" : ".") << "   "
            << (auxHit ? "X" : ".") << "    ";

        if (v1Hit)
            out << std::fixed << std::setprecision(2) << pattern.v1Velocity[step] << "    ";
        else
            out << "----    ";

        if (v2Hit)
            out << std::fixed << std::setprecision(2) << pattern.v2Velocity[step] << "    ";
        else
            out << "----    ";

        if (auxHit)
            out << std::fixed << std::setprecision(2) << pattern.auxVelocity[step] << "     ";
        else
            out << "----     ";

        out << std::fixed << std::setprecision(2) << metric << "\n";
    }

    int v1Hits = CountHits(pattern.v1Mask, pattern.patternLength);
    int v2Hits = CountHits(pattern.v2Mask, pattern.patternLength);
    int auxHits = CountHits(pattern.auxMask, pattern.patternLength);

    out << "\nSummary:\n";
    out << "  V1 hits: " << v1Hits << "/" << pattern.patternLength
        << " (" << (v1Hits * 100 / pattern.patternLength) << "%)\n";
    out << "  V2 hits: " << v2Hits << "/" << pattern.patternLength
        << " (" << (v2Hits * 100 / pattern.patternLength) << "%)\n";
    out << "  Aux hits: " << auxHits << "/" << pattern.patternLength
        << " (" << (auxHits * 100 / pattern.patternLength) << "%)\n";
    out << "  V1 mask: 0x" << std::hex << pattern.v1Mask << std::dec << "\n";
    out << "  V2 mask: 0x" << std::hex << pattern.v2Mask << std::dec << "\n";
    out << "  Aux mask: 0x" << std::hex << pattern.auxMask << std::dec << "\n";
}

static void PrintPatternCSV(std::ostream& out, const PatternParams& params, const PatternData& pattern, bool header = true)
{
    if (header)
    {
        out << "energy,shape,axis_x,axis_y,drift,accent,seed,length,step,v1,v2,aux,v1_vel,v2_vel,aux_vel,metric\n";
    }

    for (int step = 0; step < pattern.patternLength; ++step)
    {
        bool v1Hit = (pattern.v1Mask & (1U << step)) != 0;
        bool v2Hit = (pattern.v2Mask & (1U << step)) != 0;
        bool auxHit = (pattern.auxMask & (1U << step)) != 0;

        out << std::fixed << std::setprecision(2)
            << params.energy << ","
            << params.shape << ","
            << params.axisX << ","
            << params.axisY << ","
            << params.drift << ","
            << params.accent << ","
            << params.seed << ","
            << params.patternLength << ","
            << step << ","
            << (v1Hit ? 1 : 0) << ","
            << (v2Hit ? 1 : 0) << ","
            << (auxHit ? 1 : 0) << ","
            << (v1Hit ? pattern.v1Velocity[step] : 0.0f) << ","
            << (v2Hit ? pattern.v2Velocity[step] : 0.0f) << ","
            << (auxHit ? pattern.auxVelocity[step] : 0.0f) << ","
            << GetMetricWeight(step, pattern.patternLength) << "\n";
    }
}

static void PrintPatternMask(std::ostream& out, const PatternParams& params, const PatternData& pattern)
{
    out << "ENERGY=" << std::fixed << std::setprecision(2) << params.energy
        << " SHAPE=" << params.shape
        << " SEED=0x" << std::hex << params.seed << std::dec << "\n";
    out << "V1:  0x" << std::hex << std::setw(8) << std::setfill('0') << pattern.v1Mask << std::dec << "\n";
    out << "V2:  0x" << std::hex << std::setw(8) << std::setfill('0') << pattern.v2Mask << std::dec << "\n";
    out << "AUX: 0x" << std::hex << std::setw(8) << std::setfill('0') << pattern.auxMask << std::dec << "\n\n";
}

// =============================================================================
// Argument Parsing
// =============================================================================

static float ParseFloat(const char* arg)
{
    const char* eq = strchr(arg, '=');
    if (!eq) return 0.0f;
    return std::strtof(eq + 1, nullptr);
}

static uint32_t ParseSeed(const char* arg)
{
    const char* eq = strchr(arg, '=');
    if (!eq) return 0;
    const char* val = eq + 1;
    if (val[0] == '0' && (val[1] == 'x' || val[1] == 'X'))
        return static_cast<uint32_t>(std::strtoul(val, nullptr, 16));
    return static_cast<uint32_t>(std::strtoul(val, nullptr, 10));
}

static int ParseInt(const char* arg)
{
    const char* eq = strchr(arg, '=');
    if (!eq) return 0;
    return std::atoi(eq + 1);
}

static const char* ParseString(const char* arg)
{
    const char* eq = strchr(arg, '=');
    if (!eq) return "";
    return eq + 1;
}

static void PrintUsage()
{
    std::cout << R"(
Pattern Visualization CLI Tool

Usage: ./build/pattern_viz [options]

Options:
  --energy=0.50    ENERGY parameter (0.0-1.0)
  --shape=0.30     SHAPE parameter (0.0-1.0)
  --axis-x=0.50    AXIS X parameter (0.0-1.0)
  --axis-y=0.50    AXIS Y parameter (0.0-1.0)
  --drift=0.00     DRIFT parameter (0.0-1.0)
  --accent=0.50    ACCENT parameter (0.0-1.0)
  --seed=0xDEADBEEF  Pattern seed (hex or decimal)
  --length=32      Pattern length (16 or 32)
  --sweep=param    Sweep parameter: shape, energy, axis-x, axis-y
  --output=file    Output to file (default: stdout)
  --format=grid    Output format: grid, csv, mask
  --help           Show this help

Examples:
  ./build/pattern_viz --energy=0.7 --shape=0.5
  ./build/pattern_viz --sweep=shape --output=shape_sweep.txt
  ./build/pattern_viz --format=csv > patterns.csv
  ./build/pattern_viz --sweep=energy --format=mask
)";
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char* argv[])
{
    PatternParams params;
    std::string outputFile;
    std::string format = "grid";
    std::string sweep;

    // Parse arguments
    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];

        if (strncmp(arg, "--energy=", 9) == 0)
            params.energy = ParseFloat(arg);
        else if (strncmp(arg, "--shape=", 8) == 0)
            params.shape = ParseFloat(arg);
        else if (strncmp(arg, "--axis-x=", 9) == 0)
            params.axisX = ParseFloat(arg);
        else if (strncmp(arg, "--axis-y=", 9) == 0)
            params.axisY = ParseFloat(arg);
        else if (strncmp(arg, "--drift=", 8) == 0)
            params.drift = ParseFloat(arg);
        else if (strncmp(arg, "--accent=", 9) == 0)
            params.accent = ParseFloat(arg);
        else if (strncmp(arg, "--seed=", 7) == 0)
            params.seed = ParseSeed(arg);
        else if (strncmp(arg, "--length=", 9) == 0)
            params.patternLength = ParseInt(arg);
        else if (strncmp(arg, "--output=", 9) == 0)
            outputFile = ParseString(arg);
        else if (strncmp(arg, "--format=", 9) == 0)
            format = ParseString(arg);
        else if (strncmp(arg, "--sweep=", 8) == 0)
            sweep = ParseString(arg);
        else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
        {
            PrintUsage();
            return 0;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << "\n";
            PrintUsage();
            return 1;
        }
    }

    // Setup output stream
    std::ostream* out = &std::cout;
    std::ofstream fileOut;
    if (!outputFile.empty())
    {
        fileOut.open(outputFile);
        if (!fileOut.is_open())
        {
            std::cerr << "Error: Cannot open output file: " << outputFile << "\n";
            return 1;
        }
        out = &fileOut;
    }

    // Generate pattern(s)
    if (!sweep.empty())
    {
        // Parameter sweep
        *out << "=== Parameter Sweep: " << sweep << " ===\n\n";

        bool csvHeader = true;
        for (float val = 0.0f; val <= 1.01f; val += 0.1f)
        {
            PatternParams sweepParams = params;
            if (sweep == "shape")
                sweepParams.shape = val;
            else if (sweep == "energy")
                sweepParams.energy = val;
            else if (sweep == "axis-x")
                sweepParams.axisX = val;
            else if (sweep == "axis-y")
                sweepParams.axisY = val;
            else
            {
                std::cerr << "Unknown sweep parameter: " << sweep << "\n";
                return 1;
            }

            PatternData pattern;
            GeneratePattern(sweepParams, pattern);

            if (format == "grid")
                PrintPatternGrid(*out, sweepParams, pattern);
            else if (format == "csv")
            {
                PrintPatternCSV(*out, sweepParams, pattern, csvHeader);
                csvHeader = false;
            }
            else if (format == "mask")
                PrintPatternMask(*out, sweepParams, pattern);
        }
    }
    else
    {
        // Single pattern
        PatternData pattern;
        GeneratePattern(params, pattern);

        if (format == "grid")
            PrintPatternGrid(*out, params, pattern);
        else if (format == "csv")
            PrintPatternCSV(*out, params, pattern);
        else if (format == "mask")
            PrintPatternMask(*out, params, pattern);
    }

    return 0;
}
