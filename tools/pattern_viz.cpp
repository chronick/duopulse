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

#include "../src/Engine/PatternGenerator.h"  // Shared pattern generation
#include "../src/Engine/PatternField.h"       // GetMetricWeight for output formatting
#include "../src/Engine/EuclideanGen.h"       // GetGenreEuclideanRatio
#include "../src/Engine/HitBudget.h"          // GetEnergyZone
#include "../src/Engine/AlgorithmWeights.h"   // ComputeAlgorithmWeightsDebug
#include "WeightConfigLoader.h"               // Runtime JSON config loading

using namespace daisysp_idm_grids;

// =============================================================================
// Runtime Weight Configuration Overrides
// =============================================================================

// When set, these override the compile-time AlgorithmConfig values
struct WeightOverrides {
    bool enabled = false;
    float euclideanFadeStart = 0.30f;
    float euclideanFadeEnd = 0.70f;
    float syncopationCenter = 0.50f;
    float syncopationWidth = 0.30f;
    float randomFadeStart = 0.50f;
    float randomFadeEnd = 0.90f;
};

// Global weight overrides (set via CLI args)
static WeightOverrides g_weightOverrides;

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

static void PrintPatternGrid(std::ostream& out, const PatternParams& params, const PatternResult& pattern)
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
        bool v1Hit = (pattern.anchorMask & (1U << step)) != 0;
        bool v2Hit = (pattern.shimmerMask & (1U << step)) != 0;
        bool auxHit = (pattern.auxMask & (1U << step)) != 0;
        float metric = GetMetricWeight(step, pattern.patternLength);

        out << std::setw(2) << step << "    "
            << (v1Hit ? "X" : ".") << "   "
            << (v2Hit ? "X" : ".") << "   "
            << (auxHit ? "X" : ".") << "    ";

        if (v1Hit)
            out << std::fixed << std::setprecision(2) << pattern.anchorVelocity[step] << "    ";
        else
            out << "----    ";

        if (v2Hit)
            out << std::fixed << std::setprecision(2) << pattern.shimmerVelocity[step] << "    ";
        else
            out << "----    ";

        if (auxHit)
            out << std::fixed << std::setprecision(2) << pattern.auxVelocity[step] << "     ";
        else
            out << "----     ";

        out << std::fixed << std::setprecision(2) << metric << "\n";
    }

    int v1Hits = CountHits(pattern.anchorMask, pattern.patternLength);
    int v2Hits = CountHits(pattern.shimmerMask, pattern.patternLength);
    int auxHits = CountHits(pattern.auxMask, pattern.patternLength);

    out << "\nSummary:\n";
    out << "  V1 hits: " << v1Hits << "/" << pattern.patternLength
        << " (" << (v1Hits * 100 / pattern.patternLength) << "%)\n";
    out << "  V2 hits: " << v2Hits << "/" << pattern.patternLength
        << " (" << (v2Hits * 100 / pattern.patternLength) << "%)\n";
    out << "  Aux hits: " << auxHits << "/" << pattern.patternLength
        << " (" << (auxHits * 100 / pattern.patternLength) << "%)\n";
    out << "  V1 mask: 0x" << std::hex << pattern.anchorMask << std::dec << "\n";
    out << "  V2 mask: 0x" << std::hex << pattern.shimmerMask << std::dec << "\n";
    out << "  Aux mask: 0x" << std::hex << pattern.auxMask << std::dec << "\n";
}

static void PrintPatternCSV(std::ostream& out, const PatternParams& params, const PatternResult& pattern, bool header = true)
{
    if (header)
    {
        out << "energy,shape,axis_x,axis_y,drift,accent,seed,length,step,v1,v2,aux,v1_vel,v2_vel,aux_vel,metric\n";
    }

    for (int step = 0; step < pattern.patternLength; ++step)
    {
        bool v1Hit = (pattern.anchorMask & (1U << step)) != 0;
        bool v2Hit = (pattern.shimmerMask & (1U << step)) != 0;
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
            << (v1Hit ? pattern.anchorVelocity[step] : 0.0f) << ","
            << (v2Hit ? pattern.shimmerVelocity[step] : 0.0f) << ","
            << (auxHit ? pattern.auxVelocity[step] : 0.0f) << ","
            << GetMetricWeight(step, pattern.patternLength) << "\n";
    }
}

static void PrintPatternMask(std::ostream& out, const PatternParams& params, const PatternResult& pattern)
{
    out << "ENERGY=" << std::fixed << std::setprecision(2) << params.energy
        << " SHAPE=" << params.shape
        << " SEED=0x" << std::hex << params.seed << std::dec << "\n";
    out << "V1:  0x" << std::hex << std::setw(8) << std::setfill('0') << pattern.anchorMask << std::dec << "\n";
    out << "V2:  0x" << std::hex << std::setw(8) << std::setfill('0') << pattern.shimmerMask << std::dec << "\n";
    out << "AUX: 0x" << std::hex << std::setw(8) << std::setfill('0') << pattern.auxMask << std::dec << "\n\n";
}

static void PrintDebugWeights(std::ostream& out, const PatternParams& params)
{
    AlgorithmWeightsDebug debug;
    debug.shape = params.shape;
    debug.energy = params.energy;

    if (g_weightOverrides.enabled) {
        // Use override values for configuration
        debug.euclideanFadeStart = g_weightOverrides.euclideanFadeStart;
        debug.euclideanFadeEnd = g_weightOverrides.euclideanFadeEnd;
        debug.syncopationCenter = g_weightOverrides.syncopationCenter;
        debug.syncopationWidth = g_weightOverrides.syncopationWidth;
        debug.randomFadeStart = g_weightOverrides.randomFadeStart;
        debug.randomFadeEnd = g_weightOverrides.randomFadeEnd;

        // Compute raw weights with overrides
        debug.rawEuclidean = 1.0f - Smoothstep(g_weightOverrides.euclideanFadeStart,
                                                g_weightOverrides.euclideanFadeEnd, params.shape);
        debug.rawSyncopation = BellCurve(params.shape, g_weightOverrides.syncopationCenter,
                                          g_weightOverrides.syncopationWidth);
        debug.rawRandom = Smoothstep(g_weightOverrides.randomFadeStart,
                                      g_weightOverrides.randomFadeEnd, params.shape);

        // Normalize
        float total = debug.rawEuclidean + debug.rawSyncopation + debug.rawRandom;
        if (total > 0.001f) {
            debug.weights.euclidean = debug.rawEuclidean / total;
            debug.weights.syncopation = debug.rawSyncopation / total;
            debug.weights.random = debug.rawRandom / total;
        } else {
            debug.weights.euclidean = 0.0f;
            debug.weights.syncopation = 1.0f;
            debug.weights.random = 0.0f;
        }

        // Compute channel params (these don't have overrides yet)
        debug.channelParams = ComputeChannelEuclidean(params.energy, params.seed, params.patternLength);
    } else {
        // Use compiled-in config values
        debug = ComputeAlgorithmWeightsDebug(
            params.shape, params.energy, params.seed, params.patternLength);
    }

    out << "\n=== Algorithm Weights Debug ===\n";
    if (g_weightOverrides.enabled) {
        out << "(Using command-line weight overrides)\n";
    }
    out << "Input Parameters:\n";
    out << "  SHAPE:  " << std::fixed << std::setprecision(2) << debug.shape << "\n";
    out << "  ENERGY: " << std::fixed << std::setprecision(2) << debug.energy << "\n\n";

    out << "Configuration Values:\n";
    out << "  Euclidean fade:    [" << debug.euclideanFadeStart
        << ", " << debug.euclideanFadeEnd << "]\n";
    out << "  Syncopation curve: center=" << debug.syncopationCenter
        << ", width=" << debug.syncopationWidth << "\n";
    out << "  Random fade:       [" << debug.randomFadeStart
        << ", " << debug.randomFadeEnd << "]\n\n";

    out << "Raw (Unnormalized) Weights:\n";
    out << "  Euclidean:    " << std::fixed << std::setprecision(3) << debug.rawEuclidean << "\n";
    out << "  Syncopation:  " << std::fixed << std::setprecision(3) << debug.rawSyncopation << "\n";
    out << "  Random:       " << std::fixed << std::setprecision(3) << debug.rawRandom << "\n";
    out << "  Total:        " << std::fixed << std::setprecision(3)
        << (debug.rawEuclidean + debug.rawSyncopation + debug.rawRandom) << "\n\n";

    out << "Normalized Weights (sum=1.0):\n";
    float total = debug.weights.euclidean + debug.weights.syncopation + debug.weights.random;
    out << "  Euclidean:    " << std::fixed << std::setprecision(1)
        << (debug.weights.euclidean * 100) << "%\n";
    out << "  Syncopation:  " << std::fixed << std::setprecision(1)
        << (debug.weights.syncopation * 100) << "%\n";
    out << "  Random:       " << std::fixed << std::setprecision(1)
        << (debug.weights.random * 100) << "%\n";
    out << "  Verify total: " << std::fixed << std::setprecision(3) << total << "\n\n";

    out << "Per-Channel Euclidean Parameters:\n";
    out << "  Anchor k:   " << debug.channelParams.anchorK << " hits\n";
    out << "  Shimmer k:  " << debug.channelParams.shimmerK << " hits\n";
    out << "  Aux k:      " << debug.channelParams.auxK << " hits\n";
    out << "  Rotation:   " << debug.channelParams.rotation << " steps\n\n";

    // Visual weight bar
    out << "Weight Distribution:\n";
    int eucBar = static_cast<int>(debug.weights.euclidean * 40);
    int syncBar = static_cast<int>(debug.weights.syncopation * 40);
    int randBar = static_cast<int>(debug.weights.random * 40);

    out << "  Euclidean   |" << std::string(eucBar, '#')
        << std::string(40 - eucBar, '.') << "|\n";
    out << "  Syncopation |" << std::string(syncBar, '#')
        << std::string(40 - syncBar, '.') << "|\n";
    out << "  Random      |" << std::string(randBar, '#')
        << std::string(40 - randBar, '.') << "|\n";
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

static Genre ParseGenre(const char* arg)
{
    const char* val = ParseString(arg);
    if (strcmp(val, "techno") == 0) return Genre::TECHNO;
    if (strcmp(val, "tribal") == 0) return Genre::TRIBAL;
    if (strcmp(val, "idm") == 0) return Genre::IDM;
    return Genre::TECHNO;  // default
}

static AuxDensity ParseAuxDensity(const char* arg)
{
    const char* val = ParseString(arg);
    if (strcmp(val, "sparse") == 0) return AuxDensity::SPARSE;
    if (strcmp(val, "normal") == 0) return AuxDensity::NORMAL;
    if (strcmp(val, "dense") == 0) return AuxDensity::DENSE;
    if (strcmp(val, "busy") == 0) return AuxDensity::BUSY;
    return AuxDensity::NORMAL;  // default
}

static VoiceCoupling ParseVoiceCoupling(const char* arg)
{
    const char* val = ParseString(arg);
    if (strcmp(val, "independent") == 0) return VoiceCoupling::INDEPENDENT;
    if (strcmp(val, "interlock") == 0) return VoiceCoupling::INTERLOCK;
    if (strcmp(val, "shadow") == 0) return VoiceCoupling::SHADOW;
    return VoiceCoupling::INDEPENDENT;  // default
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

Firmware-matching options:
  --firmware       Use all firmware defaults (recommended)
  --balance=0.50   Balance parameter (0.0-1.0)
  --euclidean=0.00 Euclidean blend ratio (0.0-1.0, or 'auto')
  --soft-repair    Enable soft repair pass
  --fill           Enable fill zone
  --fill-intensity=0.50  Fill intensity (0.0-1.0)
  --genre=techno   Genre: techno, tribal, idm
  --aux-density=normal   Aux density: sparse, normal, dense, busy
  --voice-coupling=independent  Coupling: independent, interlock, shadow
  --density-mult=1.0  Density multiplier (SHAPE-derived in firmware)

Debug options:
  --debug-weights  Show algorithm blend weight breakdown
  --debug-euclidean Show per-channel euclidean parameters

Configuration:
  --config=file    Load weight config from JSON (validates config, shows values)

  --help           Show this help

Weight Override Options (for sensitivity analysis):
  --euclidean-fade-start=0.30  Override euclidean fade start
  --euclidean-fade-end=0.70    Override euclidean fade end
  --syncopation-center=0.50    Override syncopation bell center
  --syncopation-width=0.30     Override syncopation bell width
  --random-fade-start=0.50     Override random fade start
  --random-fade-end=0.90       Override random fade end

Examples:
  ./build/pattern_viz --energy=0.7 --shape=0.5
  ./build/pattern_viz --firmware --energy=0.6 --shape=0.4
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
    std::string configFile;      // JSON config file path
    bool autoEuclidean = false;  // Compute euclidean like firmware
    bool debugWeights = false;   // Show algorithm weight breakdown
    bool debugEuclidean = false; // Show per-channel euclidean params

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
        // Firmware-matching options
        else if (strcmp(arg, "--firmware") == 0)
        {
            // Apply all firmware defaults
            params.applySoftRepair = true;
            autoEuclidean = true;
        }
        else if (strncmp(arg, "--balance=", 10) == 0)
            params.balance = ParseFloat(arg);
        else if (strncmp(arg, "--euclidean=", 12) == 0)
        {
            const char* val = ParseString(arg);
            if (strcmp(val, "auto") == 0)
                autoEuclidean = true;
            else
                params.euclideanRatio = ParseFloat(arg);
        }
        else if (strcmp(arg, "--soft-repair") == 0)
            params.applySoftRepair = true;
        else if (strcmp(arg, "--fill") == 0)
            params.inFillZone = true;
        else if (strncmp(arg, "--fill-intensity=", 17) == 0)
            params.fillIntensity = ParseFloat(arg);
        else if (strncmp(arg, "--genre=", 8) == 0)
            params.genre = ParseGenre(arg);
        else if (strncmp(arg, "--aux-density=", 14) == 0)
            params.auxDensity = ParseAuxDensity(arg);
        else if (strncmp(arg, "--voice-coupling=", 17) == 0)
            params.voiceCoupling = ParseVoiceCoupling(arg);
        else if (strncmp(arg, "--density-mult=", 15) == 0)
            params.densityMultiplier = ParseFloat(arg);
        // Debug options
        else if (strcmp(arg, "--debug-weights") == 0)
            debugWeights = true;
        else if (strcmp(arg, "--debug-euclidean") == 0)
            debugEuclidean = true;
        // Configuration
        else if (strncmp(arg, "--config=", 9) == 0)
            configFile = ParseString(arg);
        // Weight overrides (for sensitivity analysis)
        else if (strncmp(arg, "--euclidean-fade-start=", 23) == 0) {
            g_weightOverrides.euclideanFadeStart = ParseFloat(arg);
            g_weightOverrides.enabled = true;
        }
        else if (strncmp(arg, "--euclidean-fade-end=", 21) == 0) {
            g_weightOverrides.euclideanFadeEnd = ParseFloat(arg);
            g_weightOverrides.enabled = true;
        }
        else if (strncmp(arg, "--syncopation-center=", 21) == 0) {
            g_weightOverrides.syncopationCenter = ParseFloat(arg);
            g_weightOverrides.enabled = true;
        }
        else if (strncmp(arg, "--syncopation-width=", 20) == 0) {
            g_weightOverrides.syncopationWidth = ParseFloat(arg);
            g_weightOverrides.enabled = true;
        }
        else if (strncmp(arg, "--random-fade-start=", 20) == 0) {
            g_weightOverrides.randomFadeStart = ParseFloat(arg);
            g_weightOverrides.enabled = true;
        }
        else if (strncmp(arg, "--random-fade-end=", 18) == 0) {
            g_weightOverrides.randomFadeEnd = ParseFloat(arg);
            g_weightOverrides.enabled = true;
        }
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

    // Load and display config file if specified
    if (!configFile.empty())
    {
        LoadedWeightConfig loadedConfig = LoadWeightConfigFromJSON(configFile);
        if (!loadedConfig.isLoaded)
        {
            std::cerr << "Error: Failed to load config file: " << configFile << "\n";
            return 1;
        }
        PrintLoadedConfig(loadedConfig);

        // Note: For full runtime config switching, AlgorithmWeights would need
        // to accept runtime parameters. Currently pattern generation uses
        // compile-time constexpr values from algorithm_config.h.
        // The loaded config is displayed for verification but doesn't affect
        // pattern generation. Use 'make weights-header CONFIG=...' to change
        // firmware weights.
        std::cout << "Note: Pattern generation uses compiled-in weights.\n";
        std::cout << "To use this config, run: make weights-header CONFIG=" << configFile << "\n\n";
    }

    // Compute auto-euclidean if requested (like firmware does)
    if (autoEuclidean)
    {
        EnergyZone zone = GetEnergyZone(params.energy);
        params.euclideanRatio = GetGenreEuclideanRatio(params.genre, params.axisX, zone);
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

            // Recompute auto-euclidean for each sweep value
            if (autoEuclidean)
            {
                EnergyZone zone = GetEnergyZone(sweepParams.energy);
                sweepParams.euclideanRatio = GetGenreEuclideanRatio(
                    sweepParams.genre, sweepParams.axisX, zone);
            }

            PatternResult pattern;
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
        PatternResult pattern;
        GeneratePattern(params, pattern);

        if (format == "grid")
            PrintPatternGrid(*out, params, pattern);
        else if (format == "csv")
            PrintPatternCSV(*out, params, pattern);
        else if (format == "mask")
            PrintPatternMask(*out, params, pattern);

        // Print debug info if requested
        if (debugWeights || debugEuclidean)
        {
            PrintDebugWeights(*out, params);
        }
    }

    return 0;
}
