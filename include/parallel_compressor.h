#ifndef PARALLEL_COMPRESSOR_H
#define PARALLEL_COMPRESSOR_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "huffman.h"


// Represents the complete result of parallel compression.
struct ParallelCompressedData {

    // Each element contains one independently
    // Huffman-compressed chunk.
    std::vector<HuffmanEncodedData> chunks;

    // Stores the original size of every chunk.
    // This helps us validate decompression later.
    std::vector<std::size_t> originalChunkSizes;

    // CRC32 checksum of the COMPLETE original file.
    //
    // We calculate this before compression and store it
    // inside the .pzip archive.
    //
    // After decompression we calculate CRC32 again.
    // If both checksums match, the reconstructed file
    // passed our integrity check.
    std::uint32_t checksum = 0;
};


// Compress a file using multiple worker threads.
ParallelCompressedData parallelCompress(
    const std::vector<unsigned char>& data,
    std::size_t threadCount
);


// Decompress chunks using multiple worker threads
// and reconstruct the original file.
std::vector<unsigned char> parallelDecompress(
    const ParallelCompressedData& compressed,
    std::size_t threadCount
);


#endif