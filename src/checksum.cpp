#include "checksum.h"

std::uint32_t calculateCRC32(
    const std::vector<unsigned char>& data
) {
    std::uint32_t crc = 0xFFFFFFFF;

    for (unsigned char byte : data) {

        crc ^= byte;

        for (int i = 0; i < 8; i++) {

            if (crc & 1) {

                crc =
                    (crc >> 1) ^
                    0xEDB88320;
            }
            else {

                crc >>= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}