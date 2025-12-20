#include "Persistence.h"
#include "Crc32.h"
#include <cstring>

namespace daisysp_idm_grids
{

// =============================================================================
// CRC32 Implementation (using self-validating Crc32 class)
// =============================================================================

/**
 * Global CRC32 instance - initialized on first use
 *
 * The Crc32 class generates its lookup table at runtime and validates
 * against known test vectors, eliminating the possibility of corrupted
 * hardcoded tables.
 */
static Crc32 s_crc32;
static bool  s_crc32Initialized = false;

/**
 * Ensure CRC32 is initialized and validated
 *
 * @return true if CRC is ready to use, false if self-test failed
 */
static bool EnsureCRC32Initialized()
{
    if (!s_crc32Initialized)
    {
        s_crc32Initialized = s_crc32.Init();
    }
    return s_crc32.IsValid();
}

uint32_t ComputeCRC32(const uint8_t* data, size_t length)
{
    if (!EnsureCRC32Initialized())
    {
        // CRC self-test failed - return 0 to indicate error
        return 0;
    }

    return s_crc32.Calculate(data, length);
}

uint32_t ComputeConfigChecksum(const PersistentConfig& config)
{
    // Checksum all bytes except the checksum field itself
    // checksum field is at the end of the struct
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&config);
    size_t checksumOffset = offsetof(PersistentConfig, checksum);

    return ComputeCRC32(data, checksumOffset);
}

bool ValidateConfigChecksum(const PersistentConfig& config)
{
    uint32_t computed = ComputeConfigChecksum(config);
    return computed == config.checksum;
}

// =============================================================================
// Auto-Save Functions
// =============================================================================

void MarkConfigDirty(AutoSaveState& autoSave)
{
    autoSave.dirty = true;
    autoSave.ResetDebounce();
}

bool ProcessAutoSave(AutoSaveState& autoSave)
{
    if (!autoSave.savePending)
    {
        return false;
    }

    autoSave.AdvanceSample();

    if (autoSave.DebounceElapsed())
    {
        return true;  // Signal that save should be performed
    }

    return false;
}

// =============================================================================
// Config Serialization
// =============================================================================

void PackConfig(
    int patternLength,
    float swing,
    AuxMode auxMode,
    ResetMode resetMode,
    int phraseLength,
    int clockDivision,
    AuxDensity auxDensity,
    VoiceCoupling voiceCoupling,
    Genre genre,
    uint32_t patternSeed,
    PersistentConfig& config
)
{
    config.magic   = kPersistenceMagic;
    config.version = kPersistenceVersion;

    config.reserved[0] = 0;
    config.reserved[1] = 0;
    config.reserved[2] = 0;

    // Config Mode Primary
    config.patternLength = static_cast<uint8_t>(patternLength);

    // Clamp and scale swing (0.0-1.0 -> 0-255)
    float swingClamped = swing;
    if (swingClamped < 0.0f) swingClamped = 0.0f;
    if (swingClamped > 1.0f) swingClamped = 1.0f;
    config.swing = static_cast<uint8_t>(swingClamped * 255.0f);

    config.auxMode   = static_cast<uint8_t>(auxMode);
    config.resetMode = static_cast<uint8_t>(resetMode);

    // Config Mode Shift
    config.phraseLength  = static_cast<uint8_t>(phraseLength);
    config.clockDivision = static_cast<uint8_t>(clockDivision);
    config.auxDensity    = static_cast<uint8_t>(auxDensity);
    config.voiceCoupling = static_cast<uint8_t>(voiceCoupling);

    // Performance Shift
    config.genre       = static_cast<uint8_t>(genre);
    config.reserved2[0] = 0;
    config.reserved2[1] = 0;
    config.reserved2[2] = 0;

    // Pattern seed
    config.patternSeed = patternSeed;

    // Compute and store checksum
    config.checksum = ComputeConfigChecksum(config);
}

void UnpackConfig(
    const PersistentConfig& config,
    int& patternLength,
    float& swing,
    AuxMode& auxMode,
    ResetMode& resetMode,
    int& phraseLength,
    int& clockDivision,
    AuxDensity& auxDensity,
    VoiceCoupling& voiceCoupling,
    Genre& genre,
    uint32_t& patternSeed
)
{
    // Config Mode Primary
    patternLength = static_cast<int>(config.patternLength);

    // Validate pattern length (must be 16, 24, 32, or 64)
    if (patternLength != 16 && patternLength != 24 &&
        patternLength != 32 && patternLength != 64)
    {
        patternLength = 32;  // Default
    }

    // Scale swing (0-255 -> 0.0-1.0)
    swing = static_cast<float>(config.swing) / 255.0f;

    // Validate and cast enum values
    auxMode = static_cast<AuxMode>(config.auxMode);
    if (config.auxMode > static_cast<uint8_t>(AuxMode::EVENT))
    {
        auxMode = AuxMode::HAT;
    }

    resetMode = static_cast<ResetMode>(config.resetMode);
    if (config.resetMode > static_cast<uint8_t>(ResetMode::STEP))
    {
        resetMode = ResetMode::PHRASE;
    }

    // Config Mode Shift
    phraseLength = static_cast<int>(config.phraseLength);
    if (phraseLength != 1 && phraseLength != 2 &&
        phraseLength != 4 && phraseLength != 8)
    {
        phraseLength = 4;  // Default
    }

    clockDivision = static_cast<int>(config.clockDivision);
    if (clockDivision != 1 && clockDivision != 2 &&
        clockDivision != 4 && clockDivision != 8)
    {
        clockDivision = 1;  // Default
    }

    auxDensity = static_cast<AuxDensity>(config.auxDensity);
    if (config.auxDensity > static_cast<uint8_t>(AuxDensity::BUSY))
    {
        auxDensity = AuxDensity::NORMAL;
    }

    voiceCoupling = static_cast<VoiceCoupling>(config.voiceCoupling);
    if (config.voiceCoupling > static_cast<uint8_t>(VoiceCoupling::SHADOW))
    {
        voiceCoupling = VoiceCoupling::INDEPENDENT;
    }

    // Performance Shift
    genre = static_cast<Genre>(config.genre);
    if (config.genre > static_cast<uint8_t>(Genre::IDM))
    {
        genre = Genre::TECHNO;
    }

    // Pattern seed
    patternSeed = config.patternSeed;
}

bool ConfigChanged(const PersistentConfig& current, const PersistentConfig& lastSaved)
{
    // Compare relevant fields (not magic/version/checksum)
    if (current.patternLength != lastSaved.patternLength) return true;
    if (current.swing != lastSaved.swing) return true;
    if (current.auxMode != lastSaved.auxMode) return true;
    if (current.resetMode != lastSaved.resetMode) return true;
    if (current.phraseLength != lastSaved.phraseLength) return true;
    if (current.clockDivision != lastSaved.clockDivision) return true;
    if (current.auxDensity != lastSaved.auxDensity) return true;
    if (current.voiceCoupling != lastSaved.voiceCoupling) return true;
    if (current.genre != lastSaved.genre) return true;
    if (current.patternSeed != lastSaved.patternSeed) return true;

    return false;
}

// =============================================================================
// Flash Storage Functions (Stub implementations for unit testing)
// =============================================================================

// Note: These are stub implementations. Real flash I/O will be performed
// in main.cpp using the Daisy hardware API. These functions provide a
// testable interface without hardware dependencies.

#ifndef DAISY_HARDWARE

// Static storage for testing (simulates flash)
static PersistentConfig s_simulatedFlash;
static bool s_flashHasData = false;

bool LoadConfigFromFlash(PersistentConfig& config)
{
    if (!s_flashHasData)
    {
        return false;
    }

    config = s_simulatedFlash;

    // Validate magic and checksum
    if (!config.IsValid())
    {
        return false;
    }

    if (!ValidateConfigChecksum(config))
    {
        return false;
    }

    return true;
}

bool SaveConfigToFlash(const PersistentConfig& config)
{
    s_simulatedFlash = config;
    s_flashHasData = true;
    return true;
}

bool EraseConfigFromFlash()
{
    s_flashHasData = false;
    memset(&s_simulatedFlash, 0, sizeof(s_simulatedFlash));
    return true;
}

#endif // DAISY_HARDWARE

} // namespace daisysp_idm_grids
