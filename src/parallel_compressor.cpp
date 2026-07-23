#include "parallel_compressor.h"

#include <algorithm>
#include <stdexcept>
#include <thread>
#include <vector>


ParallelCompressedData parallelCompress(
    const std::vector<unsigned char>& data,
    std::size_t threadCount
) {
    ParallelCompressedData result;

    if (data.empty()) {
        return result;
    }

    if (threadCount == 0) {
        throw std::invalid_argument(
            "Thread count must be greater than zero."
        );
    }

    // Creating more chunks than bytes would be pointless.
    //
    // Example:
    // 3-byte file + 8 requested threads
    // -> use at most 3 chunks.
    std::size_t chunkCount =
        std::min(threadCount, data.size());

    result.chunks.resize(chunkCount);
    result.originalChunkSizes.resize(chunkCount);

    std::vector<std::thread> workers;

    workers.reserve(chunkCount);

    // Divide the file as evenly as possible.
    //
    // Example:
    // 10 bytes / 3 chunks
    //
    // baseChunkSize = 3
    // remainder     = 1
    //
    // sizes become:
    // 4, 3, 3
    std::size_t baseChunkSize =
        data.size() / chunkCount;

    std::size_t remainder =
        data.size() % chunkCount;

    std::size_t start = 0;

    for (std::size_t i = 0; i < chunkCount; i++) {

        // Distribute leftover bytes among the first chunks.
        std::size_t currentChunkSize =
            baseChunkSize + (i < remainder ? 1 : 0);

        std::size_t end =
            start + currentChunkSize;

        result.originalChunkSizes[i] =
            currentChunkSize;

        // Each worker gets its own copy of its chunk.
        //
        // This keeps workers independent and avoids multiple
        // threads reading/modifying shared chunk state.
        std::vector<unsigned char> chunk(
            data.begin() + start,
            data.begin() + end
        );

        workers.emplace_back(
            [chunk = std::move(chunk), &result, i]() {

                // Each thread independently:
                //
                // chunk
                //   ↓
                // frequency table
                //   ↓
                // Huffman tree
                //   ↓
                // bit-packed compressed data
                result.chunks[i] =
                    huffmanCompress(chunk);
            }
        );

        start = end;
    }

    // join() blocks until each worker has completed.
    //
    // We MUST wait before using result.chunks because the
    // worker threads are still writing their results.
    for (std::thread& worker : workers) {

        if (worker.joinable()) {
            worker.join();
        }
    }

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

    // One output vector for each compressed chunk.
    std::vector<std::vector<unsigned char>>
        decompressedChunks(chunkCount);

    std::vector<std::thread> workers;

    workers.reserve(chunkCount);

    // For this first version, each compressed chunk gets
    // its own worker. The archive's chunk count was created
    // from the requested compression thread count.
    //
    // Later a fixed worker pool can schedule arbitrary
    // numbers of chunks onto fewer threads.
    for (std::size_t i = 0; i < chunkCount; i++) {

        workers.emplace_back(
            [&compressed, &decompressedChunks, i]() {

                decompressedChunks[i] =
                    huffmanDecompress(
                        compressed.chunks[i]
                    );
            }
        );
    }

    for (std::thread& worker : workers) {

        if (worker.joinable()) {
            worker.join();
        }
    }

    // Merge chunks AFTER all threads finish.
    //
    // Ordering matters:
    //
    // chunk 0 + chunk 1 + chunk 2 ...
    //
    // reconstructs the original file.
    std::vector<unsigned char> output;

    std::size_t totalSize = 0;

    for (const auto& chunk : decompressedChunks) {
        totalSize += chunk.size();
    }

    output.reserve(totalSize);

    for (const auto& chunk : decompressedChunks) {

        output.insert(
            output.end(),
            chunk.begin(),
            chunk.end()
        );
    }

    return output;
}


