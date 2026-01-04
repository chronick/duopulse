#include <catch2/catch_test_macros.hpp>

#include "../src/Engine/Crc32.h"

using namespace daisysp_idm_grids;

// =============================================================================
// Initialization and Self-Test
// =============================================================================

TEST_CASE("Crc32 initialization and self-test", "[crc32]")
{
    Crc32 crc;

    SECTION("Not valid before Init")
    {
        REQUIRE(crc.IsValid() == false);
    }

    SECTION("Init succeeds and validates")
    {
        bool initResult = crc.Init();
        REQUIRE(initResult == true);
        REQUIRE(crc.IsValid() == true);
    }

    SECTION("SelfTest passes after Init")
    {
        crc.Init();
        REQUIRE(crc.SelfTest() == true);
    }
}

// =============================================================================
// Known Test Vectors
// =============================================================================

TEST_CASE("CRC32 produces correct values for known test vectors", "[crc32]")
{
    Crc32 crc;
    REQUIRE(crc.Init() == true);

    SECTION("Canonical test vector: '123456789' = 0xCBF43926")
    {
        const uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
        uint32_t      result = crc.Calculate(data, sizeof(data));
        REQUIRE(result == 0xCBF43926);
    }

    SECTION("Single character 'a' = 0xE8B7BE43")
    {
        const uint8_t data[] = {'a'};
        uint32_t      result = crc.Calculate(data, 1);
        REQUIRE(result == 0xE8B7BE43);
    }

    SECTION("String 'abc' = 0x352441C2")
    {
        const uint8_t data[] = {'a', 'b', 'c'};
        uint32_t      result = crc.Calculate(data, 3);
        REQUIRE(result == 0x352441C2);
    }

    SECTION("String 'Hello, World!' = 0xEC4AC3D0")
    {
        const uint8_t data[] = "Hello, World!";
        uint32_t      result = crc.Calculate(data, 13);  // Don't include null terminator
        REQUIRE(result == 0xEC4AC3D0);
    }

    SECTION("Four zero bytes = 0x2144DF1C")
    {
        const uint8_t data[] = {0x00, 0x00, 0x00, 0x00};
        uint32_t      result = crc.Calculate(data, 4);
        REQUIRE(result == 0x2144DF1C);
    }

    SECTION("Four 0xFF bytes = 0xFFFFFFFF")
    {
        const uint8_t data[] = {0xFF, 0xFF, 0xFF, 0xFF};
        uint32_t      result = crc.Calculate(data, 4);
        REQUIRE(result == 0xFFFFFFFF);
    }

    SECTION("Single byte 0x00 = 0xD202EF8D")
    {
        const uint8_t data[] = {0x00};
        uint32_t      result = crc.Calculate(data, 1);
        REQUIRE(result == 0xD202EF8D);
    }

    SECTION("Single byte 0xFF = 0xFF000000")
    {
        const uint8_t data[] = {0xFF};
        uint32_t      result = crc.Calculate(data, 1);
        REQUIRE(result == 0xFF000000);
    }
}

// =============================================================================
// Incremental API
// =============================================================================

TEST_CASE("CRC32 incremental API matches single-shot", "[crc32]")
{
    Crc32 crc;
    REQUIRE(crc.Init() == true);

    SECTION("'123456789' in two chunks")
    {
        const uint8_t chunk1[] = {'1', '2', '3', '4'};
        const uint8_t chunk2[] = {'5', '6', '7', '8', '9'};

        uint32_t incremental = Crc32::GetInitValue();
        incremental          = crc.Update(incremental, chunk1, 4);
        incremental          = crc.Update(incremental, chunk2, 5);
        incremental          = crc.Finalize(incremental);

        const uint8_t full[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
        uint32_t      single = crc.Calculate(full, 9);

        REQUIRE(incremental == single);
        REQUIRE(incremental == 0xCBF43926);
    }

    SECTION("'abc' in three chunks (one byte each)")
    {
        const uint8_t a[] = {'a'};
        const uint8_t b[] = {'b'};
        const uint8_t c[] = {'c'};

        uint32_t incremental = Crc32::GetInitValue();
        incremental          = crc.Update(incremental, a, 1);
        incremental          = crc.Update(incremental, b, 1);
        incremental          = crc.Update(incremental, c, 1);
        incremental          = crc.Finalize(incremental);

        const uint8_t full[] = {'a', 'b', 'c'};
        uint32_t      single = crc.Calculate(full, 3);

        REQUIRE(incremental == single);
        REQUIRE(incremental == 0x352441C2);
    }

    SECTION("Single chunk equals single-shot")
    {
        const uint8_t data[] = "Hello, World!";

        uint32_t incremental = Crc32::GetInitValue();
        incremental          = crc.Update(incremental, data, 13);
        incremental          = crc.Finalize(incremental);

        uint32_t single = crc.Calculate(data, 13);

        REQUIRE(incremental == single);
    }
}

// =============================================================================
// Error Handling
// =============================================================================

TEST_CASE("CRC32 handles edge cases gracefully", "[crc32]")
{
    Crc32 crc;

    SECTION("Calculate returns 0 before Init")
    {
        const uint8_t data[] = {'t', 'e', 's', 't'};
        uint32_t      result = crc.Calculate(data, 4);
        REQUIRE(result == 0);
    }

    SECTION("Calculate returns 0 for null pointer after Init")
    {
        crc.Init();
        uint32_t result = crc.Calculate(nullptr, 10);
        REQUIRE(result == 0);
    }

    SECTION("Calculate handles zero length")
    {
        crc.Init();
        const uint8_t data[] = {'t', 'e', 's', 't'};
        // CRC of empty data = init XOR xorout = 0
        uint32_t result = crc.Calculate(data, 0);
        REQUIRE(result == 0x00000000);
    }

    SECTION("Update returns unchanged CRC for null pointer")
    {
        crc.Init();
        uint32_t initial = 0x12345678;
        uint32_t result  = crc.Update(initial, nullptr, 10);
        REQUIRE(result == initial);
    }
}

// =============================================================================
// Consistency Tests
// =============================================================================

TEST_CASE("CRC32 is deterministic", "[crc32]")
{
    Crc32 crc;
    crc.Init();

    SECTION("Same data produces same checksum")
    {
        const uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};

        uint32_t crc1 = crc.Calculate(data, sizeof(data));
        uint32_t crc2 = crc.Calculate(data, sizeof(data));

        REQUIRE(crc1 == crc2);
    }

    SECTION("Multiple instances produce same results")
    {
        Crc32 crc2;
        crc2.Init();

        const uint8_t data[] = {'t', 'e', 's', 't'};

        uint32_t result1 = crc.Calculate(data, sizeof(data));
        uint32_t result2 = crc2.Calculate(data, sizeof(data));

        REQUIRE(result1 == result2);
    }
}

TEST_CASE("CRC32 detects changes", "[crc32]")
{
    Crc32 crc;
    crc.Init();

    SECTION("Different data produces different checksum")
    {
        const uint8_t data1[] = {0x01, 0x02, 0x03, 0x04, 0x05};
        const uint8_t data2[] = {0x01, 0x02, 0x03, 0x04, 0x06};

        uint32_t crc1 = crc.Calculate(data1, sizeof(data1));
        uint32_t crc2 = crc.Calculate(data2, sizeof(data2));

        REQUIRE(crc1 != crc2);
    }

    SECTION("Single bit flip changes checksum")
    {
        const uint8_t data1[] = {0x00, 0x00, 0x00, 0x00};
        const uint8_t data2[] = {0x01, 0x00, 0x00, 0x00};

        uint32_t crc1 = crc.Calculate(data1, sizeof(data1));
        uint32_t crc2 = crc.Calculate(data2, sizeof(data2));

        REQUIRE(crc1 != crc2);
    }

    SECTION("Order matters")
    {
        const uint8_t data1[] = {0x01, 0x02};
        const uint8_t data2[] = {0x02, 0x01};

        uint32_t crc1 = crc.Calculate(data1, 2);
        uint32_t crc2 = crc.Calculate(data2, 2);

        REQUIRE(crc1 != crc2);
    }
}

// =============================================================================
// Integration with Persistence
// =============================================================================

TEST_CASE("CRC32 works with typical config data", "[crc32][integration]")
{
    Crc32 crc;
    crc.Init();

    // Simulate a PersistentConfig-like structure
    struct TestConfig
    {
        uint32_t magic;
        uint8_t  version;
        uint8_t  data[20];
        uint32_t checksum;
    };

    SECTION("Checksum covers all data except checksum field")
    {
        TestConfig config;
        config.magic   = 0x44505634;
        config.version = 1;
        for (int i = 0; i < 20; i++)
        {
            config.data[i] = static_cast<uint8_t>(i);
        }

        // Compute checksum of everything except the checksum field
        size_t   checksumOffset = offsetof(TestConfig, checksum);
        uint32_t computed       = crc.Calculate(
            reinterpret_cast<const uint8_t*>(&config),
            checksumOffset
        );

        config.checksum = computed;

        // Verify the checksum validates
        uint32_t verify = crc.Calculate(
            reinterpret_cast<const uint8_t*>(&config),
            checksumOffset
        );

        REQUIRE(verify == config.checksum);
    }

    SECTION("Modified data fails checksum")
    {
        TestConfig config;
        config.magic   = 0x44505634;
        config.version = 1;
        for (int i = 0; i < 20; i++)
        {
            config.data[i] = static_cast<uint8_t>(i);
        }

        size_t checksumOffset = offsetof(TestConfig, checksum);
        config.checksum       = crc.Calculate(
            reinterpret_cast<const uint8_t*>(&config),
            checksumOffset
        );

        // Modify data
        config.data[10] = 0xFF;

        // Verify checksum now fails
        uint32_t verify = crc.Calculate(
            reinterpret_cast<const uint8_t*>(&config),
            checksumOffset
        );

        REQUIRE(verify != config.checksum);
    }
}
