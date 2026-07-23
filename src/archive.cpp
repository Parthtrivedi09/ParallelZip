#include "archive.h"

#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>


namespace {

constexpr char MAGIC[4] = {
    'P', 'Z', 'I', 'P'
};

// Version 2 introduces multiple independently
// compressed Huffman chunks.
constexpr std::uint8_t VERSION = 2;


// Serialize uint64_t explicitly using little-endian order.
//
// We don't write raw structs because compiler padding and
// platform representation could make the format unreliable.
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



void writeArchive(
    const std::string& archivePath,
    const ParallelCompressedData& compressed
) {
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


    // -------------------------------------------------
    // GLOBAL HEADER
    // -------------------------------------------------

    file.write(MAGIC, 4);

    file.put(
        static_cast<char>(VERSION)
    );

    writeUint64(
        file,
        static_cast<std::uint64_t>(
            compressed.chunks.size()
        )
    );


    // -------------------------------------------------
    // CHUNKS
    // -------------------------------------------------

    for (std::size_t i = 0;
         i < compressed.chunks.size();
         i++) {

        const HuffmanEncodedData& chunk =
            compressed.chunks[i];


        // Store original uncompressed chunk size.
        writeUint64(
            file,
            static_cast<std::uint64_t>(
                compressed.originalChunkSizes[i]
            )
        );


        // Number of meaningful Huffman bits.
        writeUint64(
            file,
            chunk.bitCount
        );


        // Store compressed payload size so the archive
        // parser knows exactly where this chunk ends.
        writeUint64(
            file,
            static_cast<std::uint64_t>(
                chunk.data.size()
            )
        );


        // Store the Huffman frequency table required
        // to rebuild this chunk's Huffman tree.
        for (std::uint64_t frequency :
             chunk.frequencies) {

            writeUint64(
                file,
                frequency
            );
        }


        // Store compressed payload.
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



ParallelCompressedData readArchive(
    const std::string& archivePath
) {
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


    // -------------------------------------------------
    // MAGIC
    // -------------------------------------------------

    char magic[4];

    file.read(magic, 4);

    if (!file ||
        magic[0] != 'P' ||
        magic[1] != 'Z' ||
        magic[2] != 'I' ||
        magic[3] != 'P') {

        throw std::runtime_error(
            "Invalid ParallelZip archive."
        );
    }


    // -------------------------------------------------
    // VERSION
    // -------------------------------------------------

    int version = file.get();

    if (version == EOF ||
        static_cast<std::uint8_t>(version) != VERSION) {

        throw std::runtime_error(
            "Unsupported ParallelZip archive version."
        );
    }


    // -------------------------------------------------
    // CHUNK COUNT
    // -------------------------------------------------

    std::uint64_t chunkCount =
        readUint64(file);


    // Prevent obviously malicious/corrupted archives from
    // requesting absurd numbers of vector elements.
    if (chunkCount > 1000000) {

        throw std::runtime_error(
            "Invalid chunk count."
        );
    }


    ParallelCompressedData result;

    result.chunks.resize(
        static_cast<std::size_t>(chunkCount)
    );

    result.originalChunkSizes.resize(
        static_cast<std::size_t>(chunkCount)
    );


    // -------------------------------------------------
    // READ EACH CHUNK
    // -------------------------------------------------

    for (std::size_t i = 0;
         i < result.chunks.size();
         i++) {

        std::uint64_t originalSize =
            readUint64(file);

        std::uint64_t bitCount =
            readUint64(file);

        std::uint64_t payloadSize =
            readUint64(file);


        if (originalSize >
            static_cast<std::uint64_t>(
                std::numeric_limits<std::size_t>::max()
            )) {

            throw std::runtime_error(
                "Chunk is too large."
            );
        }


        if (payloadSize >
            static_cast<std::uint64_t>(
                std::numeric_limits<std::size_t>::max()
            )) {

            throw std::runtime_error(
                "Compressed chunk is too large."
            );
        }


        // A bitstream cannot require more payload bytes
        // than ceil(bitCount / 8).
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


        // Read this chunk's frequency table.
        std::uint64_t frequencySum = 0;

        for (int j = 0; j < 256; j++) {

            chunk.frequencies[j] =
                readUint64(file);

            if (chunk.frequencies[j] >
                std::numeric_limits<std::uint64_t>::max()
                    - frequencySum) {

                throw std::runtime_error(
                    "Invalid frequency table."
                );
            }

            frequencySum +=
                chunk.frequencies[j];
        }


        // Frequencies must describe exactly the number
        // of original bytes stored for this chunk.
        if (frequencySum != originalSize) {

            throw std::runtime_error(
                "Chunk metadata is inconsistent."
            );
        }


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