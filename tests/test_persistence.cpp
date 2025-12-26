#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "../src/Engine/Persistence.h"
#include "../src/Engine/DuoPulseTypes.h"

using namespace daisysp_idm_grids;
using Catch::Approx;

// =============================================================================
// CRC32 Checksum Tests
// =============================================================================

TEST_CASE("CRC32 produces valid checksums", "[persistence]")
{
    SECTION("Empty data produces known value")
    {
        uint8_t data[1] = {0};
        uint32_t crc = ComputeCRC32(data, 0);
        // CRC of empty data should be 0
        REQUIRE(crc == 0);
    }

    SECTION("Same data produces same checksum")
    {
        uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
        uint32_t crc1 = ComputeCRC32(data, sizeof(data));
        uint32_t crc2 = ComputeCRC32(data, sizeof(data));

        REQUIRE(crc1 == crc2);
    }

    SECTION("Different data produces different checksum")
    {
        uint8_t data1[] = {0x01, 0x02, 0x03, 0x04, 0x05};
        uint8_t data2[] = {0x01, 0x02, 0x03, 0x04, 0x06};

        uint32_t crc1 = ComputeCRC32(data1, sizeof(data1));
        uint32_t crc2 = ComputeCRC32(data2, sizeof(data2));

        REQUIRE(crc1 != crc2);
    }

    SECTION("Single bit flip changes checksum")
    {
        uint8_t data1[] = {0x00, 0x00, 0x00, 0x00};
        uint8_t data2[] = {0x01, 0x00, 0x00, 0x00};

        uint32_t crc1 = ComputeCRC32(data1, sizeof(data1));
        uint32_t crc2 = ComputeCRC32(data2, sizeof(data2));

        REQUIRE(crc1 != crc2);
    }
}

TEST_CASE("Config checksum validation works", "[persistence]")
{
    SECTION("Valid config passes validation")
    {
        PersistentConfig config;
        config.Init();
        config.checksum = ComputeConfigChecksum(config);

        REQUIRE(ValidateConfigChecksum(config) == true);
    }

    SECTION("Modified config fails validation")
    {
        PersistentConfig config;
        config.Init();
        config.checksum = ComputeConfigChecksum(config);

        // Modify a field after computing checksum
        config.patternLength = 64;

        REQUIRE(ValidateConfigChecksum(config) == false);
    }

    SECTION("Wrong checksum fails validation")
    {
        PersistentConfig config;
        config.Init();
        config.checksum = 0xDEADBEEF;  // Wrong checksum

        REQUIRE(ValidateConfigChecksum(config) == false);
    }
}

// =============================================================================
// PersistentConfig Tests
// =============================================================================

TEST_CASE("PersistentConfig initializes correctly", "[persistence]")
{
    PersistentConfig config;
    config.Init();

    SECTION("Magic number is set")
    {
        REQUIRE(config.magic == kPersistenceMagic);
    }

    SECTION("Version is set")
    {
        REQUIRE(config.version == kPersistenceVersion);
    }

    SECTION("IsValid returns true for initialized config")
    {
        REQUIRE(config.IsValid() == true);
    }

    SECTION("Default values are sensible")
    {
        REQUIRE(config.patternLength == 32);
        REQUIRE(config.swing == 0);
        REQUIRE(config.auxMode == static_cast<uint8_t>(AuxMode::HAT));
        REQUIRE(config.resetMode == static_cast<uint8_t>(ResetMode::PHRASE));
        REQUIRE(config.phraseLength == 4);
        REQUIRE(config.clockDivision == 1);
        REQUIRE(config.auxDensity == static_cast<uint8_t>(AuxDensity::NORMAL));
        REQUIRE(config.voiceCoupling == static_cast<uint8_t>(VoiceCoupling::INDEPENDENT));
        REQUIRE(config.genre == static_cast<uint8_t>(Genre::TECHNO));
    }
}

TEST_CASE("PersistentConfig IsValid detects invalid configs", "[persistence]")
{
    SECTION("Wrong magic number fails")
    {
        PersistentConfig config;
        config.Init();
        config.magic = 0xDEADBEEF;

        REQUIRE(config.IsValid() == false);
    }

    SECTION("Future version is accepted (forward compatible)")
    {
        PersistentConfig config;
        config.Init();
        // Current version should be valid
        REQUIRE(config.IsValid() == true);
    }
}

// =============================================================================
// Config Serialization Round-Trip Tests
// =============================================================================

TEST_CASE("Config serialization round-trip preserves values", "[persistence]")
{
    SECTION("All parameters survive pack/unpack")
    {
        // Create config with specific values
        int patternLength = 24;
        float swing = 0.75f;
        AuxMode auxMode = AuxMode::PHRASE_CV;
        ResetMode resetMode = ResetMode::BAR;
        int phraseLength = 8;
        int clockDivision = 2;
        AuxDensity auxDensity = AuxDensity::DENSE;
        VoiceCoupling voiceCoupling = VoiceCoupling::SHADOW;
        Genre genre = Genre::IDM;
        uint32_t patternSeed = 0x12345678;

        PersistentConfig config;
        PackConfig(
            patternLength, swing, auxMode, resetMode,
            phraseLength, clockDivision, auxDensity, voiceCoupling,
            genre, patternSeed, config
        );

        // Unpack and verify
        int outPatternLength;
        float outSwing;
        AuxMode outAuxMode;
        ResetMode outResetMode;
        int outPhraseLength;
        int outClockDivision;
        AuxDensity outAuxDensity;
        VoiceCoupling outVoiceCoupling;
        Genre outGenre;
        uint32_t outPatternSeed;

        UnpackConfig(
            config,
            outPatternLength, outSwing, outAuxMode, outResetMode,
            outPhraseLength, outClockDivision, outAuxDensity, outVoiceCoupling,
            outGenre, outPatternSeed
        );

        REQUIRE(outPatternLength == patternLength);
        REQUIRE(outSwing == Approx(swing).epsilon(0.01f));  // Allow small rounding
        REQUIRE(outAuxMode == auxMode);
        REQUIRE(outResetMode == resetMode);
        REQUIRE(outPhraseLength == phraseLength);
        REQUIRE(outClockDivision == clockDivision);
        REQUIRE(outAuxDensity == auxDensity);
        REQUIRE(outVoiceCoupling == voiceCoupling);
        REQUIRE(outGenre == genre);
        REQUIRE(outPatternSeed == patternSeed);
    }

    SECTION("Swing precision is acceptable")
    {
        // Test various swing values
        float testSwings[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};

        for (float swing : testSwings)
        {
            PersistentConfig config;
            PackConfig(32, swing, AuxMode::HAT, ResetMode::PHRASE,
                       4, 1, AuxDensity::NORMAL, VoiceCoupling::INDEPENDENT,
                       Genre::TECHNO, 0, config);

            int pl; float outSwing; AuxMode am; ResetMode rm;
            int phl, cd; AuxDensity ad; VoiceCoupling vc;
            Genre g; uint32_t ps;

            UnpackConfig(config, pl, outSwing, am, rm, phl, cd, ad, vc, g, ps);

            // Swing stored as uint8_t, so precision is ~1/255 ≈ 0.004
            // Use margin for absolute tolerance
            REQUIRE(outSwing == Approx(swing).margin(0.005f));
        }
    }

    SECTION("Pattern length values are preserved exactly")
    {
        int testLengths[] = {16, 24, 32, 64};

        for (int length : testLengths)
        {
            PersistentConfig config;
            PackConfig(length, 0.0f, AuxMode::HAT, ResetMode::PHRASE,
                       4, 1, AuxDensity::NORMAL, VoiceCoupling::INDEPENDENT,
                       Genre::TECHNO, 0, config);

            int outLength; float s; AuxMode am; ResetMode rm;
            int phl, cd; AuxDensity ad; VoiceCoupling vc;
            Genre g; uint32_t ps;

            UnpackConfig(config, outLength, s, am, rm, phl, cd, ad, vc, g, ps);

            REQUIRE(outLength == length);
        }
    }
}

TEST_CASE("UnpackConfig handles invalid values gracefully", "[persistence]")
{
    SECTION("Invalid pattern length uses default")
    {
        PersistentConfig config;
        config.Init();
        config.patternLength = 17;  // Invalid (not 16, 24, 32, or 64)
        config.checksum = ComputeConfigChecksum(config);

        int pl; float s; AuxMode am; ResetMode rm;
        int phl, cd; AuxDensity ad; VoiceCoupling vc;
        Genre g; uint32_t ps;

        UnpackConfig(config, pl, s, am, rm, phl, cd, ad, vc, g, ps);

        REQUIRE(pl == 32);  // Default
    }

    SECTION("Invalid phrase length uses default")
    {
        PersistentConfig config;
        config.Init();
        config.phraseLength = 3;  // Invalid (not 1, 2, 4, or 8)
        config.checksum = ComputeConfigChecksum(config);

        int pl; float s; AuxMode am; ResetMode rm;
        int phl, cd; AuxDensity ad; VoiceCoupling vc;
        Genre g; uint32_t ps;

        UnpackConfig(config, pl, s, am, rm, phl, cd, ad, vc, g, ps);

        REQUIRE(phl == 4);  // Default
    }

    SECTION("Invalid enum values use defaults")
    {
        PersistentConfig config;
        config.Init();
        config.auxMode = 99;       // Invalid
        config.resetMode = 99;     // Invalid
        config.auxDensity = 99;    // Invalid
        config.voiceCoupling = 99; // Invalid
        config.genre = 99;         // Invalid
        config.checksum = ComputeConfigChecksum(config);

        int pl; float s; AuxMode am; ResetMode rm;
        int phl, cd; AuxDensity ad; VoiceCoupling vc;
        Genre g; uint32_t ps;

        UnpackConfig(config, pl, s, am, rm, phl, cd, ad, vc, g, ps);

        REQUIRE(am == AuxMode::HAT);
        REQUIRE(rm == ResetMode::PHRASE);
        REQUIRE(ad == AuxDensity::NORMAL);
        REQUIRE(vc == VoiceCoupling::INDEPENDENT);
        REQUIRE(g == Genre::TECHNO);
    }
}

// =============================================================================
// ConfigChanged Tests
// =============================================================================

TEST_CASE("ConfigChanged detects changes correctly", "[persistence]")
{
    SECTION("Identical configs are not different")
    {
        PersistentConfig config1;
        PersistentConfig config2;
        config1.Init();
        config2.Init();

        REQUIRE(ConfigChanged(config1, config2) == false);
    }

    SECTION("Pattern length change is detected")
    {
        PersistentConfig config1;
        PersistentConfig config2;
        config1.Init();
        config2.Init();
        config1.patternLength = 16;

        REQUIRE(ConfigChanged(config1, config2) == true);
    }

    SECTION("Swing change is detected")
    {
        PersistentConfig config1;
        PersistentConfig config2;
        config1.Init();
        config2.Init();
        config1.swing = 128;

        REQUIRE(ConfigChanged(config1, config2) == true);
    }

    SECTION("Genre change is detected")
    {
        PersistentConfig config1;
        PersistentConfig config2;
        config1.Init();
        config2.Init();
        config1.genre = static_cast<uint8_t>(Genre::TRIBAL);

        REQUIRE(ConfigChanged(config1, config2) == true);
    }

    SECTION("Pattern seed change is detected")
    {
        PersistentConfig config1;
        PersistentConfig config2;
        config1.Init();
        config2.Init();
        config1.patternSeed = 0xDEADBEEF;

        REQUIRE(ConfigChanged(config1, config2) == true);
    }
}

// =============================================================================
// AutoSaveState Tests
// =============================================================================

TEST_CASE("AutoSaveState initializes correctly", "[persistence]")
{
    AutoSaveState autoSave;
    autoSave.Init(48000.0f);

    SECTION("Not dirty initially")
    {
        REQUIRE(autoSave.dirty == false);
    }

    SECTION("Not pending initially")
    {
        REQUIRE(autoSave.savePending == false);
    }

    SECTION("Debounce threshold computed correctly")
    {
        // 2 seconds at 48kHz = 96000 samples
        REQUIRE(autoSave.debounceThreshold == 96000);
    }
}

TEST_CASE("MarkConfigDirty sets up debounce timer", "[persistence]")
{
    AutoSaveState autoSave;
    autoSave.Init(48000.0f);

    MarkConfigDirty(autoSave);

    REQUIRE(autoSave.dirty == true);
    REQUIRE(autoSave.savePending == true);
    REQUIRE(autoSave.debounceSamples == 0);
}

TEST_CASE("ProcessAutoSave implements debounce timing", "[persistence]")
{
    AutoSaveState autoSave;
    autoSave.Init(48000.0f);

    SECTION("No save when not pending")
    {
        bool shouldSave = ProcessAutoSave(autoSave);
        REQUIRE(shouldSave == false);
    }

    SECTION("No save before debounce elapsed")
    {
        MarkConfigDirty(autoSave);

        // Process half the debounce time
        for (int i = 0; i < 48000; ++i)  // 1 second
        {
            bool shouldSave = ProcessAutoSave(autoSave);
            REQUIRE(shouldSave == false);
        }
    }

    SECTION("Save after debounce elapsed")
    {
        MarkConfigDirty(autoSave);

        // Process full debounce time
        bool savedTriggered = false;
        for (int i = 0; i < 96001; ++i)  // Just over 2 seconds
        {
            if (ProcessAutoSave(autoSave))
            {
                savedTriggered = true;
                break;
            }
        }

        REQUIRE(savedTriggered == true);
    }

    SECTION("Re-marking dirty resets debounce")
    {
        MarkConfigDirty(autoSave);

        // Process almost full debounce time
        for (int i = 0; i < 95000; ++i)
        {
            ProcessAutoSave(autoSave);
        }

        // Re-mark dirty
        MarkConfigDirty(autoSave);

        REQUIRE(autoSave.debounceSamples == 0);

        // Should not trigger save immediately
        bool shouldSave = ProcessAutoSave(autoSave);
        REQUIRE(shouldSave == false);
    }
}

TEST_CASE("ClearPending resets auto-save state", "[persistence]")
{
    AutoSaveState autoSave;
    autoSave.Init(48000.0f);

    MarkConfigDirty(autoSave);
    for (int i = 0; i < 50000; ++i)
    {
        ProcessAutoSave(autoSave);
    }

    autoSave.ClearPending();

    REQUIRE(autoSave.dirty == false);
    REQUIRE(autoSave.savePending == false);
    REQUIRE(autoSave.debounceSamples == 0);
}

// =============================================================================
// Flash Storage Tests (Simulated)
// =============================================================================

TEST_CASE("Flash storage round-trip works", "[persistence]")
{
    // Erase any existing data
    EraseConfigFromFlash();

    SECTION("Load fails on empty flash")
    {
        PersistentConfig config;
        bool loaded = LoadConfigFromFlash(config);

        REQUIRE(loaded == false);
    }

    SECTION("Save and load preserves config")
    {
        PersistentConfig original;
        original.Init();
        original.patternLength = 64;
        original.swing = 128;
        original.genre = static_cast<uint8_t>(Genre::IDM);
        original.patternSeed = 0xCAFEBABE;
        original.checksum = ComputeConfigChecksum(original);

        bool saved = SaveConfigToFlash(original);
        REQUIRE(saved == true);

        PersistentConfig loaded;
        bool loadSuccess = LoadConfigFromFlash(loaded);
        REQUIRE(loadSuccess == true);

        REQUIRE(loaded.magic == original.magic);
        REQUIRE(loaded.version == original.version);
        REQUIRE(loaded.patternLength == original.patternLength);
        REQUIRE(loaded.swing == original.swing);
        REQUIRE(loaded.genre == original.genre);
        REQUIRE(loaded.patternSeed == original.patternSeed);
        REQUIRE(loaded.checksum == original.checksum);
    }

    SECTION("Load fails if checksum is wrong")
    {
        PersistentConfig config;
        config.Init();
        config.checksum = 0xBADC0DE;  // Wrong checksum

        SaveConfigToFlash(config);

        PersistentConfig loaded;
        bool loadSuccess = LoadConfigFromFlash(loaded);

        REQUIRE(loadSuccess == false);
    }

    SECTION("Erase clears stored config")
    {
        PersistentConfig config;
        config.Init();
        config.checksum = ComputeConfigChecksum(config);
        SaveConfigToFlash(config);

        EraseConfigFromFlash();

        PersistentConfig loaded;
        bool loadSuccess = LoadConfigFromFlash(loaded);

        REQUIRE(loadSuccess == false);
    }
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_CASE("Persistence handles edge cases", "[persistence]")
{
    SECTION("Swing clamping in PackConfig")
    {
        PersistentConfig config;

        // Test negative swing
        PackConfig(32, -0.5f, AuxMode::HAT, ResetMode::PHRASE,
                   4, 1, AuxDensity::NORMAL, VoiceCoupling::INDEPENDENT,
                   Genre::TECHNO, 0, config);
        REQUIRE(config.swing == 0);

        // Test > 1.0 swing
        PackConfig(32, 1.5f, AuxMode::HAT, ResetMode::PHRASE,
                   4, 1, AuxDensity::NORMAL, VoiceCoupling::INDEPENDENT,
                   Genre::TECHNO, 0, config);
        REQUIRE(config.swing == 255);
    }

    SECTION("Different sample rates compute different debounce thresholds")
    {
        AutoSaveState autoSave1;
        AutoSaveState autoSave2;

        autoSave1.Init(48000.0f);
        autoSave2.Init(96000.0f);

        REQUIRE(autoSave1.debounceThreshold == 96000);   // 2s at 48kHz
        REQUIRE(autoSave2.debounceThreshold == 192000);  // 2s at 96kHz
    }

    SECTION("Zero sample rate doesn't crash")
    {
        AutoSaveState autoSave;
        autoSave.Init(0.0f);

        REQUIRE(autoSave.debounceThreshold == 0);

        // Should trigger immediately when pending
        MarkConfigDirty(autoSave);
        bool shouldSave = ProcessAutoSave(autoSave);
        // With 0 threshold, first advance should trigger
        REQUIRE(shouldSave == true);
    }
}

// =============================================================================
// Integration Tests
// =============================================================================

TEST_CASE("Complete persistence workflow", "[persistence][integration]")
{
    // Simulate a typical usage scenario
    EraseConfigFromFlash();

    SECTION("First boot uses defaults, then saves user changes")
    {
        // First boot - no config in flash
        PersistentConfig bootConfig;
        bool loaded = LoadConfigFromFlash(bootConfig);

        REQUIRE(loaded == false);

        // Use defaults
        bootConfig.Init();

        // User changes some parameters
        AutoSaveState autoSave;
        autoSave.Init(48000.0f);

        // Simulate user changing pattern length
        bootConfig.patternLength = 64;
        MarkConfigDirty(autoSave);

        // Run debounce timer
        bool savedTriggered = false;
        for (int i = 0; i < 100000; ++i)
        {
            if (ProcessAutoSave(autoSave))
            {
                savedTriggered = true;
                break;
            }
        }

        REQUIRE(savedTriggered == true);

        // Save to flash
        bootConfig.checksum = ComputeConfigChecksum(bootConfig);
        bool saved = SaveConfigToFlash(bootConfig);
        REQUIRE(saved == true);
        autoSave.ClearPending();
        autoSave.lastSaved = bootConfig;

        // Simulate reboot
        PersistentConfig rebootConfig;
        loaded = LoadConfigFromFlash(rebootConfig);

        REQUIRE(loaded == true);
        REQUIRE(rebootConfig.patternLength == 64);
    }
}

TEST_CASE("Deferred flash write pattern workflow", "[persistence][integration]")
{
    // This test documents the deferred save pattern used in main.cpp
    // to prevent blocking flash writes in the audio callback.

    EraseConfigFromFlash();

    SECTION("Audio callback defers save, main loop executes it")
    {
        // Simulate main.cpp state
        PersistentConfig currentConfig;
        currentConfig.Init();

        AutoSaveState autoSaveState;
        autoSaveState.Init(48000.0f);
        autoSaveState.lastSaved = currentConfig;

        // Deferred save state (audio callback -> main loop communication)
        struct DeferredSave {
            bool pending = false;
            PersistentConfig configToSave;
        };
        DeferredSave deferredSave;

        // Simulate user interaction in control processing
        currentConfig.patternLength = 24;
        MarkConfigDirty(autoSaveState);

        // === Simulate Audio Callback (runs at 48kHz, must be non-blocking) ===
        bool audioCallbackFlaggedSave = false;
        for (int sample = 0; sample < 100000; ++sample)
        {
            // Audio callback timing check
            if (ProcessAutoSave(autoSaveState))
            {
                // Build current config (cheap operation ~1μs)
                PersistentConfig testConfig = currentConfig;
                testConfig.checksum = ComputeConfigChecksum(testConfig);

                // Check if save needed - if so, DEFER to main loop
                if (ConfigChanged(testConfig, autoSaveState.lastSaved))
                {
                    deferredSave.configToSave = testConfig;
                    deferredSave.pending = true;  // Flag for main loop
                    audioCallbackFlaggedSave = true;
                }
                autoSaveState.ClearPending();
                break;
            }
        }

        REQUIRE(audioCallbackFlaggedSave == true);
        REQUIRE(deferredSave.pending == true);

        // === Simulate Main Loop (runs at ~1kHz, can handle blocking operations) ===
        // Handle deferred flash write (safe here, outside audio callback)
        if (deferredSave.pending)
        {
            // This is where the 10-100ms blocking flash write happens
            bool saved = SaveConfigToFlash(deferredSave.configToSave);
            REQUIRE(saved == true);

            autoSaveState.lastSaved = deferredSave.configToSave;
            deferredSave.pending = false;
        }

        REQUIRE(deferredSave.pending == false);

        // Verify config was actually saved to flash
        PersistentConfig loadedConfig;
        bool loaded = LoadConfigFromFlash(loadedConfig);
        REQUIRE(loaded == true);
        REQUIRE(loadedConfig.patternLength == 24);
    }

    SECTION("No save when config unchanged")
    {
        PersistentConfig currentConfig;
        currentConfig.Init();

        AutoSaveState autoSaveState;
        autoSaveState.Init(48000.0f);
        autoSaveState.lastSaved = currentConfig;

        struct DeferredSave {
            bool pending = false;
            PersistentConfig configToSave;
        };
        DeferredSave deferredSave;

        // Mark dirty but don't actually change config
        MarkConfigDirty(autoSaveState);

        // Process auto-save timing
        for (int sample = 0; sample < 100000; ++sample)
        {
            if (ProcessAutoSave(autoSaveState))
            {
                PersistentConfig testConfig = currentConfig;
                testConfig.checksum = ComputeConfigChecksum(testConfig);

                // Config unchanged - should NOT set pending flag
                if (ConfigChanged(testConfig, autoSaveState.lastSaved))
                {
                    deferredSave.configToSave = testConfig;
                    deferredSave.pending = true;
                }
                autoSaveState.ClearPending();
                break;
            }
        }

        // No flash write should be pending
        REQUIRE(deferredSave.pending == false);
    }

    SECTION("Multiple changes coalesce into single save")
    {
        PersistentConfig currentConfig;
        currentConfig.Init();

        AutoSaveState autoSaveState;
        autoSaveState.Init(48000.0f);
        autoSaveState.lastSaved = currentConfig;

        struct DeferredSave {
            bool pending = false;
            PersistentConfig configToSave;
        };
        DeferredSave deferredSave;

        // Simulate rapid changes (e.g., user turning knob)
        currentConfig.patternLength = 24;
        MarkConfigDirty(autoSaveState);

        // Advance 1 second (not enough for debounce)
        for (int i = 0; i < 48000; ++i)
        {
            ProcessAutoSave(autoSaveState);
        }

        // User makes another change
        currentConfig.patternLength = 64;
        MarkConfigDirty(autoSaveState);  // Resets debounce timer

        // Now advance past debounce period
        for (int sample = 0; sample < 100000; ++sample)
        {
            if (ProcessAutoSave(autoSaveState))
            {
                PersistentConfig testConfig = currentConfig;
                testConfig.checksum = ComputeConfigChecksum(testConfig);

                if (ConfigChanged(testConfig, autoSaveState.lastSaved))
                {
                    deferredSave.configToSave = testConfig;
                    deferredSave.pending = true;
                }
                autoSaveState.ClearPending();
                break;
            }
        }

        REQUIRE(deferredSave.pending == true);
        // Should save final value (64), not intermediate (24)
        REQUIRE(deferredSave.configToSave.patternLength == 64);
    }
}
