#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <cstdint>
#include <vector>

// Calculate a CRC32 checksum for integrity verification.
std::uint32_t calculateCRC32(
    const std::vector<unsigned char>& data
);

#endif