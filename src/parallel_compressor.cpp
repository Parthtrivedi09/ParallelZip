#include "parallel_compressor.h"

#include <algorithm>
#include <stdexcept>
#include <vector>
#include "checksum.h"
#include "thread_pool.h"


namespace {

// Each compression task processes approximately 4 MB.
//
// Chunk count and thread count are intentionally independent:
// a file may contain hundreds of chunks while only eight
// worker threads process them.
constexpr std::size_t CHUNK_SIZE =
    4 * 1024 * 1024;

}


ParallelCompressedData parallelCompress(
    const std::vector<unsigned char>& data,
    std::size_t threadCount
) {
    ParallelCompressedData result;
    // Calculate the checksum of the COMPLETE original file
    // before splitting it into chunks.
    //
    // This value will later be stored in the .pzip archive
    // and used to verify the decompressed file.
    result.checksum = calculateCRC32(data);

    if (data.empty()) {
        return result;
    }

    if (threadCount == 0) {
        throw std::invalid_argument(
            "Thread count must be greater than zero."
        );
    }

    // Calculate how many fixed-size chunks are required.
    std::size_t chunkCount =
        (data.size() + CHUNK_SIZE - 1) /
        CHUNK_SIZE;

    result.chunks.resize(chunkCount);
    result.originalChunkSizes.resize(chunkCount);

    // Never create more worker threads than useful tasks.
    std::size_t workerCount =
        std::min(threadCount, chunkCount);

    ThreadPool pool(workerCount);


    for (std::size_t i = 0; i < chunkCount; i++) {

        std::size_t start =
            i * CHUNK_SIZE;

        std::size_t end =
            std::min(
                start + CHUNK_SIZE,
                data.size()
            );

        result.originalChunkSizes[i] =
            end - start;

        pool.enqueue(
            [&, i, start, end]() {

                // Each worker creates a private chunk from
                // its assigned section of the input.
                std::vector<unsigned char> chunk(
                    data.begin() + start,
                    data.begin() + end
                );

                // Each result index belongs to exactly one task,
                // so workers do not overwrite one another.
                result.chunks[i] =
                    huffmanCompress(chunk);
            }
        );
    }

    pool.wait();

    return result;
}


std::vector<unsigned char> parallelDecompress(
    const ParallelCompressedData& compressed,
    std::size_t threadCount
) {
    if (compressed.chunks.empty()) {
        return {};
    }

    if (threadCount == 0) {
        throw std::invalid_argument(
            "Thread count must be greater than zero."
        );
    }

    std::size_t chunkCount =
        compressed.chunks.size();

    std::vector<std::vector<unsigned char>>
        decompressedChunks(chunkCount);

    std::size_t workerCount =
        std::min(threadCount, chunkCount);

    ThreadPool pool(workerCount);


    for (std::size_t i = 0; i < chunkCount; i++) {

        pool.enqueue(
            [&, i]() {

                decompressedChunks[i] =
                    huffmanDecompress(
                        compressed.chunks[i]
                    );

                // Validate against the original size recorded
                // in the archive metadata.
                if (decompressedChunks[i].size() !=
                    compressed.originalChunkSizes[i]) {

                    throw std::runtime_error(
                        "Decompressed chunk size mismatch."
                    );
                }
            }
        );
    }

    pool.wait();


    // Determine the final file size before merging so the
    // output vector only needs one allocation.
    std::size_t totalSize = 0;

    for (const auto& chunk : decompressedChunks) {
        totalSize += chunk.size();
    }

    std::vector<unsigned char> output;

    output.reserve(totalSize);


    // Chunks must be concatenated in their original order.
    for (const auto& chunk : decompressedChunks) {

        output.insert(
            output.end(),
            chunk.begin(),
            chunk.end()
        );
    }

    return output;
}