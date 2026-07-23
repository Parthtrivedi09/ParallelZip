#ifndef PARALLEL_COMPRESSOR_H
#define PARALLEL_COMPRESSOR_H

#include <cstddef>
#include <vector>

#include "huffman.h"

// Contains all independently compressed chunks.
struct ParallelCompressedData {
    std::vector<HuffmanEncodedData> chunks;

    // Original number of bytes in each chunk.
    // This is useful for validation and reconstruction.
    std::vector<std::size_t> originalChunkSizes;
};

// Compress input using multiple worker threads.
ParallelCompressedData parallelCompress(
    const std::vector<unsigned char>& data,
    std::size_t threadCount
);

// Decompress all chunks concurrently and reconstruct
// the original byte sequence in the correct order.
std::vector<unsigned char> parallelDecompress(
    const ParallelCompressedData& compressed,
    std::size_t threadCount
);



#endif

