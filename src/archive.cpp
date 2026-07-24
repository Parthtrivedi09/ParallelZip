#include "archive.h"

#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>


namespace {

// Every valid ParallelZip archive starts with these four bytes.
//
// This allows us to verify that the input is actually
// a ParallelZip archive and not some unrelated file.
constexpr char MAGIC[4] = {
    'P', 'Z', 'I', 'P'
};


// Version history:
//
// Version 2 -> introduced multiple independently compressed chunks.
// Version 3 -> added CRC32 checksum to the global archive header.
constexpr std::uint8_t VERSION = 3;


// ============================================================
// 32-BIT SERIALIZATION
// ============================================================

// Write a 32-bit unsigned integer in little-endian format.
//
// CRC32 is a 32-bit value, so we use this function
// specifically for storing the checksum.
void writeUint32(
    std::ofstream& file,
    std::uint32_t value
) {
    for (int i = 0; i < 4; i++) {

        // Extract one byte at a time from the integer.
        std::uint8_t byte =
            static_cast<std::uint8_t>(
                (value >> (i * 8)) & 0xFF
            );

        file.put(
            static_cast<char>(byte)
        );
    }

    if (!file) {
        throw std::runtime_error(
            "Failed while writing archive."
        );
    }
}


// Read a 32-bit unsigned integer stored
// in little-endian format.
std::uint32_t readUint32(
    std::ifstream& file
) {
    std::uint32_t value = 0;

    for (int i = 0; i < 4; i++) {

        int byte = file.get();

        if (byte == EOF) {
            throw std::runtime_error(
                "Unexpected end of archive."
            );
        }

        // Reconstruct the original 32-bit value
        // one byte at a time.
        value |=
            static_cast<std::uint32_t>(
                static_cast<std::uint8_t>(byte)
            ) << (i * 8);
    }

    return value;
}


// ============================================================
// 64-BIT SERIALIZATION
// ============================================================

// Write a 64-bit unsigned integer in little-endian format.
//
// We explicitly serialize integers instead of writing
// raw C++ structs because structs may contain compiler
// padding and may not be portable between systems.
void writeUint64(
    std::ofstream& file,
    std::uint64_t value
) {
    for (int i = 0; i < 8; i++) {

        std::uint8_t byte =
            static_cast<std::uint8_t>(
                (value >> (i * 8)) & 0xFF
            );

        file.put(
            static_cast<char>(byte)
        );
    }

    if (!file) {
        throw std::runtime_error(
            "Failed while writing archive."
        );
    }
}


// Read a 64-bit unsigned integer stored
// in little-endian format.
std::uint64_t readUint64(
    std::ifstream& file
) {
    std::uint64_t value = 0;

    for (int i = 0; i < 8; i++) {

        int byte = file.get();

        if (byte == EOF) {
            throw std::runtime_error(
                "Unexpected end of archive."
            );
        }

        value |=
            static_cast<std::uint64_t>(
                static_cast<std::uint8_t>(byte)
            ) << (i * 8);
    }

    return value;
}

} // namespace



// ============================================================
// WRITE ARCHIVE
// ============================================================

void writeArchive(
    const std::string& archivePath,
    const ParallelCompressedData& compressed
) {
    // Open the archive in binary mode so every byte
    // is written exactly as intended.
    std::ofstream file(
        archivePath,
        std::ios::binary
    );

    if (!file) {
        throw std::runtime_error(
            "Failed to create archive: " +
            archivePath
        );
    }


    // ========================================================
    // GLOBAL HEADER
    // ========================================================

    // --------------------------------------------------------
    // MAGIC NUMBER
    // --------------------------------------------------------
    //
    // First four bytes:
    //
    // P Z I P

    file.write(
        MAGIC,
        4
    );


    // --------------------------------------------------------
    // ARCHIVE VERSION
    // --------------------------------------------------------

    file.put(
        static_cast<char>(VERSION)
    );


    // --------------------------------------------------------
    // NUMBER OF CHUNKS
    // --------------------------------------------------------

    writeUint64(
        file,
        static_cast<std::uint64_t>(
            compressed.chunks.size()
        )
    );


    // --------------------------------------------------------
    // CRC32 CHECKSUM
    // --------------------------------------------------------
    //
    // This checksum belongs to the COMPLETE original file,
    // not to an individual chunk.
    //
    // During decompression we calculate CRC32 again and
    // compare it with this stored value.

    writeUint32(
        file,
        compressed.checksum
    );


    // ========================================================
    // COMPRESSED CHUNKS
    // ========================================================

    for (
        std::size_t i = 0;
        i < compressed.chunks.size();
        i++
    ) {

        const HuffmanEncodedData& chunk =
            compressed.chunks[i];


        // ----------------------------------------------------
        // ORIGINAL CHUNK SIZE
        // ----------------------------------------------------
        //
        // Number of bytes before Huffman compression.

        writeUint64(
            file,
            static_cast<std::uint64_t>(
                compressed.originalChunkSizes[i]
            )
        );


        // ----------------------------------------------------
        // NUMBER OF MEANINGFUL BITS
        // ----------------------------------------------------
        //
        // The final compressed byte may contain padding,
        // so we need to know exactly how many bits are valid.

        writeUint64(
            file,
            chunk.bitCount
        );


        // ----------------------------------------------------
        // COMPRESSED PAYLOAD SIZE
        // ----------------------------------------------------
        //
        // Allows the archive reader to know exactly how
        // many bytes belong to this chunk.

        writeUint64(
            file,
            static_cast<std::uint64_t>(
                chunk.data.size()
            )
        );


        // ----------------------------------------------------
        // HUFFMAN FREQUENCY TABLE
        // ----------------------------------------------------
        //
        // Every chunk has its own Huffman tree.
        //
        // We store its 256 byte frequencies so that the
        // same Huffman tree can be reconstructed later.

        for (
            std::uint64_t frequency :
            chunk.frequencies
        ) {

            writeUint64(
                file,
                frequency
            );
        }


        // ----------------------------------------------------
        // COMPRESSED PAYLOAD
        // ----------------------------------------------------

        if (!chunk.data.empty()) {

            file.write(
                reinterpret_cast<const char*>(
                    chunk.data.data()
                ),
                static_cast<std::streamsize>(
                    chunk.data.size()
                )
            );
        }


        if (!file) {
            throw std::runtime_error(
                "Failed while writing compressed chunk."
            );
        }
    }
}



// ============================================================
// READ ARCHIVE
// ============================================================

ParallelCompressedData readArchive(
    const std::string& archivePath
) {
    // Open the archive as raw binary data.
    std::ifstream file(
        archivePath,
        std::ios::binary
    );

    if (!file) {
        throw std::runtime_error(
            "Failed to open archive: " +
            archivePath
        );
    }


    // ========================================================
    // MAGIC NUMBER
    // ========================================================

    char magic[4];

    file.read(
        magic,
        4
    );


    // Verify that the first four bytes are PZIP.
    if (
        !file ||
        magic[0] != 'P' ||
        magic[1] != 'Z' ||
        magic[2] != 'I' ||
        magic[3] != 'P'
    ) {

        throw std::runtime_error(
            "Invalid ParallelZip archive."
        );
    }


    // ========================================================
    // VERSION
    // ========================================================

    int version =
        file.get();


    if (
        version == EOF ||
        static_cast<std::uint8_t>(version)
            != VERSION
    ) {

        throw std::runtime_error(
            "Unsupported ParallelZip archive version."
        );
    }


    // ========================================================
    // CHUNK COUNT
    // ========================================================

    std::uint64_t chunkCount =
        readUint64(file);


    // Prevent corrupted or malicious archives from
    // requesting an unreasonable number of chunks.
    if (chunkCount > 1000000) {

        throw std::runtime_error(
            "Invalid chunk count."
        );
    }


    // ========================================================
    // CREATE RESULT OBJECT
    // ========================================================

    ParallelCompressedData result;


    // ========================================================
    // CRC32 CHECKSUM
    // ========================================================
    //
    // IMPORTANT:
    //
    // We wrote the archive as:
    //
    // MAGIC
    // VERSION
    // CHUNK COUNT
    // CHECKSUM
    // CHUNKS
    //
    // Therefore we must read it in exactly the same order.

    result.checksum =
        readUint32(file);


    // Allocate storage for all compressed chunks.
    result.chunks.resize(
        static_cast<std::size_t>(
            chunkCount
        )
    );


    result.originalChunkSizes.resize(
        static_cast<std::size_t>(
            chunkCount
        )
    );


    // ========================================================
    // READ EACH CHUNK
    // ========================================================

    for (
        std::size_t i = 0;
        i < result.chunks.size();
        i++
    ) {

        // ----------------------------------------------------
        // CHUNK METADATA
        // ----------------------------------------------------

        std::uint64_t originalSize =
            readUint64(file);


        std::uint64_t bitCount =
            readUint64(file);


        std::uint64_t payloadSize =
            readUint64(file);


        // ----------------------------------------------------
        // VALIDATE ORIGINAL SIZE
        // ----------------------------------------------------

        if (
            originalSize >
            static_cast<std::uint64_t>(
                std::numeric_limits<std::size_t>::max()
            )
        ) {

            throw std::runtime_error(
                "Chunk is too large."
            );
        }


        // ----------------------------------------------------
        // VALIDATE PAYLOAD SIZE
        // ----------------------------------------------------

        if (
            payloadSize >
            static_cast<std::uint64_t>(
                std::numeric_limits<std::size_t>::max()
            )
        ) {

            throw std::runtime_error(
                "Compressed chunk is too large."
            );
        }


        // Convert bit count to the expected number
        // of physical bytes.
        //
        // Equivalent to:
        //
        // ceil(bitCount / 8)
        //
        // without using floating point.
        std::uint64_t expectedPayload =
            bitCount / 8 +
            (bitCount % 8 != 0 ? 1 : 0);


        if (payloadSize != expectedPayload) {

            throw std::runtime_error(
                "Invalid compressed chunk size."
            );
        }


        result.originalChunkSizes[i] =
            static_cast<std::size_t>(
                originalSize
            );


        HuffmanEncodedData& chunk =
            result.chunks[i];


        chunk.bitCount =
            bitCount;


        // ====================================================
        // READ HUFFMAN FREQUENCY TABLE
        // ====================================================

        std::uint64_t frequencySum = 0;


        for (int j = 0; j < 256; j++) {

            chunk.frequencies[j] =
                readUint64(file);


            // Prevent integer overflow if the archive
            // contains corrupted frequency values.
            if (
                chunk.frequencies[j] >
                std::numeric_limits<std::uint64_t>::max()
                    - frequencySum
            ) {

                throw std::runtime_error(
                    "Invalid frequency table."
                );
            }


            frequencySum +=
                chunk.frequencies[j];
        }


        // The total number of symbols represented by
        // the frequency table must equal the original
        // number of bytes in this chunk.
        if (frequencySum != originalSize) {

            throw std::runtime_error(
                "Chunk metadata is inconsistent."
            );
        }


        // ====================================================
        // READ COMPRESSED PAYLOAD
        // ====================================================

        chunk.data.resize(
            static_cast<std::size_t>(
                payloadSize
            )
        );


        if (!chunk.data.empty()) {

            file.read(
                reinterpret_cast<char*>(
                    chunk.data.data()
                ),
                static_cast<std::streamsize>(
                    chunk.data.size()
                )
            );


            if (!file) {

                throw std::runtime_error(
                    "Archive contains truncated chunk data."
                );
            }
        }
    }


    return result;
}