#pragma once
// =============================================================================
// CRC32 Implementation - Safe, Self-Validating
// =============================================================================
//
// Uses runtime table generation to avoid hardcoded lookup table errors.
// Implements standard CRC-32/ISO-HDLC (used by Ethernet, ZIP, PNG, etc.)
//
// Polynomial: 0xEDB88320 (reflected form of 0x04C11DB7)
// Init:       0xFFFFFFFF
// XorOut:     0xFFFFFFFF
// RefIn/Out:  true
//
// Test vector: CRC32("123456789") = 0xCBF43926
//
// Usage:
//   Crc32 crc;
//   crc.Init();  // Call once at startup - generates table and validates
//
//   uint32_t checksum = crc.Calculate(data, length);
//
// =============================================================================

#include <cstdint>
#include <cstddef>

namespace daisysp_idm_grids
{

class Crc32
{
  public:
    Crc32() : initialized_(false) {}

    /**
     * Initialize the CRC table and validate against known test vector.
     *
     * @return true if self-test passes, false if implementation is broken.
     *         Call once at startup.
     */
    bool Init();

    /**
     * Calculate CRC32 of a byte buffer.
     *
     * @param data Pointer to data buffer
     * @param length Number of bytes
     * @return CRC32 checksum, or 0 if not initialized
     */
    uint32_t Calculate(const uint8_t* data, size_t length) const;

    /**
     * Update CRC32 incrementally (for streaming or large data).
     *
     * Start with crc = 0xFFFFFFFF, call Update for each chunk,
     * then call Finalize on the result.
     *
     * @param crc Current CRC state
     * @param data Pointer to data chunk
     * @param length Number of bytes in chunk
     * @return Updated CRC state
     */
    uint32_t Update(uint32_t crc, const uint8_t* data, size_t length) const;

    /**
     * Finalize CRC32 after incremental updates.
     *
     * @param crc CRC state from Update calls
     * @return Final CRC32 checksum
     */
    uint32_t Finalize(uint32_t crc) const;

    /**
     * Check if initialized and validated.
     */
    bool IsValid() const { return initialized_; }

    /**
     * Run self-test against known test vectors.
     *
     * @return true if all tests pass
     */
    bool SelfTest() const;

    /**
     * Get the initial CRC value for incremental calculation.
     */
    static constexpr uint32_t GetInitValue() { return kInitValue; }

  private:
    static constexpr uint32_t kPolynomial = 0xEDB88320;
    static constexpr uint32_t kInitValue  = 0xFFFFFFFF;
    static constexpr uint32_t kXorOut     = 0xFFFFFFFF;

    uint32_t table_[256];
    bool     initialized_;

    void GenerateTable();
};

} // namespace daisysp_idm_grids
