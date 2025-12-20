#include "Crc32.h"

namespace daisysp_idm_grids
{

// =============================================================================
// Table Generation
// =============================================================================

void Crc32::GenerateTable()
{
    for (uint32_t i = 0; i < 256; i++)
    {
        uint32_t crc = i;
        for (int bit = 0; bit < 8; bit++)
        {
            if (crc & 1)
            {
                crc = (crc >> 1) ^ kPolynomial;
            }
            else
            {
                crc = crc >> 1;
            }
        }
        table_[i] = crc;
    }
}

// =============================================================================
// Initialization
// =============================================================================

bool Crc32::Init()
{
    GenerateTable();
    initialized_ = SelfTest();
    return initialized_;
}

// =============================================================================
// CRC Calculation
// =============================================================================

uint32_t Crc32::Update(uint32_t crc, const uint8_t* data, size_t length) const
{
    if (!initialized_ || data == nullptr)
    {
        return crc;
    }

    for (size_t i = 0; i < length; i++)
    {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc           = (crc >> 8) ^ table_[index];
    }

    return crc;
}

uint32_t Crc32::Finalize(uint32_t crc) const
{
    return crc ^ kXorOut;
}

uint32_t Crc32::Calculate(const uint8_t* data, size_t length) const
{
    if (!initialized_ || data == nullptr)
    {
        return 0;
    }

    uint32_t crc = kInitValue;
    crc          = Update(crc, data, length);
    return Finalize(crc);
}

// =============================================================================
// Self-Test
// =============================================================================

bool Crc32::SelfTest() const
{
    // Standard test vector: ASCII "123456789" -> 0xCBF43926
    // This is the canonical check value for CRC-32/ISO-HDLC
    const uint8_t        test_data[]  = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    constexpr uint32_t   expected_crc = 0xCBF43926;

    uint32_t crc = kInitValue;
    for (size_t i = 0; i < sizeof(test_data); i++)
    {
        uint8_t index = (crc ^ test_data[i]) & 0xFF;
        crc           = (crc >> 8) ^ table_[index];
    }
    crc ^= kXorOut;

    if (crc != expected_crc)
    {
        return false;
    }

    // Additional test: empty data should return 0x00000000
    // (init XOR xorout = 0xFFFFFFFF XOR 0xFFFFFFFF = 0)
    uint32_t empty_crc = kInitValue ^ kXorOut;
    if (empty_crc != 0x00000000)
    {
        return false;
    }

    // Test: single byte 0x00
    // CRC32("\x00") = 0xD202EF8D
    crc = kInitValue;
    crc = (crc >> 8) ^ table_[(crc ^ 0x00) & 0xFF];
    crc ^= kXorOut;
    if (crc != 0xD202EF8D)
    {
        return false;
    }

    // Test incremental API matches single-shot
    uint32_t incremental = kInitValue;
    incremental          = (incremental >> 8) ^ table_[(incremental ^ '1') & 0xFF];
    incremental          = (incremental >> 8) ^ table_[(incremental ^ '2') & 0xFF];
    incremental          = (incremental >> 8) ^ table_[(incremental ^ '3') & 0xFF];
    incremental          = (incremental >> 8) ^ table_[(incremental ^ '4') & 0xFF];
    incremental          = (incremental >> 8) ^ table_[(incremental ^ '5') & 0xFF];
    incremental          = (incremental >> 8) ^ table_[(incremental ^ '6') & 0xFF];
    incremental          = (incremental >> 8) ^ table_[(incremental ^ '7') & 0xFF];
    incremental          = (incremental >> 8) ^ table_[(incremental ^ '8') & 0xFF];
    incremental          = (incremental >> 8) ^ table_[(incremental ^ '9') & 0xFF];
    incremental ^= kXorOut;

    if (incremental != expected_crc)
    {
        return false;
    }

    return true;
}

} // namespace daisysp_idm_grids
