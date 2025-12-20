#pragma once

#include <cstddef>
#include <cstdint>
#include "DuoPulseTypes.h"

namespace daisysp_idm_grids
{

/**
 * Magic number for validating stored config
 * Changes when config format changes (breaking change)
 */
constexpr uint32_t kPersistenceMagic = 0x44505634;  // "DPV4" in ASCII

/**
 * Version number for config format
 * Increment for compatible changes
 */
constexpr uint8_t kPersistenceVersion = 1;

/**
 * Auto-save debounce time in samples
 * 2 seconds at 48kHz = 96000 samples
 */
constexpr int kAutoSaveDebounceMs = 2000;

/**
 * Flash storage address offset (within QSPI region)
 * Using offset to avoid first sector which may have bootloader data
 */
constexpr uint32_t kFlashStorageOffset = 0x10000;  // 64KB offset

/**
 * PersistentConfig: Data that gets saved to flash
 *
 * This struct contains all parameters that should survive power cycles.
 * Performance primary controls (ENERGY, BUILD, FIELD X/Y) are NOT saved
 * as they should be read from knobs on boot.
 *
 * Reference: docs/specs/main.md section 12.2
 */
struct PersistentConfig
{
    // =========================================================================
    // Header (for validation)
    // =========================================================================

    /// Magic number to identify valid config
    uint32_t magic;

    /// Version for forward compatibility
    uint8_t version;

    /// Reserved bytes for alignment
    uint8_t reserved[3];

    // =========================================================================
    // Config Mode Primary
    // =========================================================================

    /// Pattern length in steps (16, 24, 32, 64)
    uint8_t patternLength;

    /// Base swing amount (0-255 maps to 0.0-1.0)
    uint8_t swing;

    /// AUX output mode (0-3)
    uint8_t auxMode;

    /// Reset behavior (0-2)
    uint8_t resetMode;

    // =========================================================================
    // Config Mode Shift
    // =========================================================================

    /// Phrase length in bars (1, 2, 4, 8)
    uint8_t phraseLength;

    /// Clock division (1, 2, 4, 8)
    uint8_t clockDivision;

    /// AUX density (0-3)
    uint8_t auxDensity;

    /// Voice coupling (0-2)
    uint8_t voiceCoupling;

    // =========================================================================
    // Performance Shift (saved because less frequently changed)
    // =========================================================================

    /// Genre selection (0-2)
    uint8_t genre;

    /// Reserved for future use
    uint8_t reserved2[3];

    // =========================================================================
    // Pattern Seed
    // =========================================================================

    /// Current pattern seed for reproducible patterns
    uint32_t patternSeed;

    // =========================================================================
    // Footer
    // =========================================================================

    /// CRC32 checksum of all preceding bytes
    uint32_t checksum;

    // =========================================================================
    // Methods
    // =========================================================================

    /**
     * Initialize with default values
     */
    void Init()
    {
        magic   = kPersistenceMagic;
        version = kPersistenceVersion;

        reserved[0] = 0;
        reserved[1] = 0;
        reserved[2] = 0;

        patternLength = 32;
        swing         = 0;
        auxMode       = static_cast<uint8_t>(AuxMode::HAT);
        resetMode     = static_cast<uint8_t>(ResetMode::PHRASE);

        phraseLength  = 4;
        clockDivision = 1;
        auxDensity    = static_cast<uint8_t>(AuxDensity::NORMAL);
        voiceCoupling = static_cast<uint8_t>(VoiceCoupling::INDEPENDENT);

        genre       = static_cast<uint8_t>(Genre::TECHNO);
        reserved2[0] = 0;
        reserved2[1] = 0;
        reserved2[2] = 0;

        patternSeed = 0x12345678;  // Default seed

        checksum = 0;  // Will be computed before save
    }

    /**
     * Check if magic and version are valid
     */
    bool IsValid() const
    {
        return (magic == kPersistenceMagic) && (version <= kPersistenceVersion);
    }
};

/**
 * AutoSaveState: State management for auto-save system
 *
 * Implements 2-second debounce to minimize flash wear.
 * Config is only written when:
 * 1. A config parameter changed (dirty flag set)
 * 2. 2 seconds have passed since last change
 *
 * Reference: docs/specs/main.md section 12.1
 */
struct AutoSaveState
{
    /// Whether config has changed since last save
    bool dirty;

    /// Sample counter for debounce timing
    int debounceSamples;

    /// Debounce threshold in samples (computed from sample rate)
    int debounceThreshold;

    /// Whether a save is pending (debounce timer active)
    bool savePending;

    /// Last saved config (for change detection)
    PersistentConfig lastSaved;

    /**
     * Initialize auto-save state
     *
     * @param sampleRate Sample rate in Hz for computing debounce threshold
     */
    void Init(float sampleRate)
    {
        dirty             = false;
        debounceSamples   = 0;
        debounceThreshold = static_cast<int>((sampleRate * kAutoSaveDebounceMs) / 1000.0f);
        savePending       = false;
        lastSaved.Init();
    }

    /**
     * Reset debounce timer
     */
    void ResetDebounce()
    {
        debounceSamples = 0;
        savePending     = true;
    }

    /**
     * Check if debounce time has elapsed
     */
    bool DebounceElapsed() const
    {
        return savePending && (debounceSamples >= debounceThreshold);
    }

    /**
     * Advance debounce timer by one sample
     */
    void AdvanceSample()
    {
        if (savePending)
        {
            debounceSamples++;
        }
    }

    /**
     * Clear pending save after successful write
     */
    void ClearPending()
    {
        dirty       = false;
        savePending = false;
        debounceSamples = 0;
    }
};

// =============================================================================
// CRC32 Checksum Functions
// =============================================================================

/**
 * Compute CRC32 checksum of a data buffer
 *
 * Uses standard CRC-32 polynomial (0xEDB88320, reflected)
 *
 * @param data Pointer to data buffer
 * @param length Number of bytes to checksum
 * @return CRC32 checksum
 */
uint32_t ComputeCRC32(const uint8_t* data, size_t length);

/**
 * Compute checksum for a PersistentConfig struct
 * (checksums all bytes except the checksum field itself)
 *
 * @param config Config to checksum
 * @return CRC32 checksum
 */
uint32_t ComputeConfigChecksum(const PersistentConfig& config);

/**
 * Validate config checksum
 *
 * @param config Config to validate
 * @return true if checksum matches
 */
bool ValidateConfigChecksum(const PersistentConfig& config);

// =============================================================================
// Config Serialization Functions
// =============================================================================

/**
 * Mark config as dirty (needs to be saved)
 *
 * Resets the debounce timer so save won't happen immediately.
 *
 * @param autoSave Auto-save state to update
 */
void MarkConfigDirty(AutoSaveState& autoSave);

/**
 * Process auto-save logic (call each sample)
 *
 * Advances debounce timer and triggers save when ready.
 * Returns true if a save should be performed now.
 *
 * @param autoSave Auto-save state
 * @return true if save should be performed
 */
bool ProcessAutoSave(AutoSaveState& autoSave);

/**
 * Pack control state into persistent config
 *
 * @param patternLength Pattern length in steps
 * @param swing Base swing amount (0.0-1.0)
 * @param auxMode AUX output mode
 * @param resetMode Reset behavior
 * @param phraseLength Phrase length in bars
 * @param clockDivision Clock division
 * @param auxDensity AUX density setting
 * @param voiceCoupling Voice coupling mode
 * @param genre Current genre
 * @param patternSeed Current pattern seed
 * @param config Output config struct
 */
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
);

/**
 * Unpack persistent config into individual parameters
 *
 * @param config Input config struct
 * @param patternLength Output pattern length
 * @param swing Output swing amount
 * @param auxMode Output AUX mode
 * @param resetMode Output reset mode
 * @param phraseLength Output phrase length
 * @param clockDivision Output clock division
 * @param auxDensity Output AUX density
 * @param voiceCoupling Output voice coupling
 * @param genre Output genre
 * @param patternSeed Output pattern seed
 */
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
);

/**
 * Check if config has changed from last saved state
 *
 * @param current Current config
 * @param lastSaved Last saved config
 * @return true if any saveable parameter changed
 */
bool ConfigChanged(const PersistentConfig& current, const PersistentConfig& lastSaved);

// =============================================================================
// Flash Storage Functions (Platform-specific implementations)
// =============================================================================

/**
 * Load config from flash storage
 *
 * @param config Output config struct
 * @return true if valid config was loaded, false if defaults should be used
 */
bool LoadConfigFromFlash(PersistentConfig& config);

/**
 * Save config to flash storage
 *
 * @param config Config to save
 * @return true if save succeeded
 */
bool SaveConfigToFlash(const PersistentConfig& config);

/**
 * Erase config from flash (factory reset)
 *
 * @return true if erase succeeded
 */
bool EraseConfigFromFlash();

} // namespace daisysp_idm_grids
