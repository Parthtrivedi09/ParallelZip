#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "archive.h"
#include "file_utils.h"
#include "parallel_compressor.h"


int main(int argc, char* argv[]) {

    if (argc < 3) {

        std::cout << "Usage:\n";
        std::cout
            << "  parallelzip compress <file> [threads]\n";

        std::cout
            << "  parallelzip decompress <file.pzip> [threads]\n";

        return 1;
    }


    std::string command = argv[1];
    std::string filename = argv[2];


    if (command != "compress" &&
        command != "decompress") {

        std::cerr
            << "Error: Unknown command: "
            << command
            << "\n";

        return 1;
    }


    if (!fileExists(filename)) {

        std::cerr
            << "Error: File does not exist: "
            << filename
            << "\n";

        return 1;
    }


    try {

        // hardware_concurrency() gives an estimate of how many
        // hardware execution threads are available.
        //
        // It is allowed to return 0, so we provide a fallback.
        unsigned int hardwareThreads =
            std::thread::hardware_concurrency();

        std::size_t threadCount =
            hardwareThreads == 0
                ? 1
                : hardwareThreads;


        // Allow the user to override automatic thread selection.
        //
        // Example:
        // parallelzip compress file.txt 4
        if (argc >= 4) {

            int requested =
                std::stoi(argv[3]);

            if (requested <= 0) {
                throw std::invalid_argument(
                    "Thread count must be greater than zero."
                );
            }

            threadCount =
                static_cast<std::size_t>(
                    requested
                );
        }


        std::cout
            << "Threads: "
            << threadCount
            << "\n";


        // =====================================================
        // COMPRESSION
        // =====================================================

        if (command == "compress") {

            std::vector<unsigned char> data =
                readBinaryFile(filename);


            auto start =
                std::chrono::steady_clock::now();


            ParallelCompressedData compressed =
                parallelCompress(
                    data,
                    threadCount
                );


            auto end =
                std::chrono::steady_clock::now();


            double compressionTime =
                std::chrono::duration<double>(
                    end - start
                ).count();


            std::string archivePath =
                filename + ".pzip";


            writeArchive(
                archivePath,
                compressed
            );


            std::uintmax_t archiveSize =
                getFileSize(archivePath);


            std::cout << "\n--- Compression Results ---\n";

            std::cout
                << "Original size: "
                << data.size()
                << " bytes\n";

            std::cout
                << "Archive size: "
                << archiveSize
                << " bytes\n";

            std::cout
                << "Chunks: "
                << compressed.chunks.size()
                << "\n";

            std::cout
                << "Compression time: "
                << compressionTime
                << " seconds\n";


            if (!data.empty()) {

                double ratio =
                    static_cast<double>(
                        archiveSize
                    ) /
                    static_cast<double>(
                        data.size()
                    );

                std::cout
                    << "Compression ratio: "
                    << ratio
                    << "\n";


                // Convert bytes/sec to MB/sec.
                double throughput =
                    (
                        static_cast<double>(
                            data.size()
                        ) /
                        (1024.0 * 1024.0)
                    ) /
                    compressionTime;

                std::cout
                    << "Compression throughput: "
                    << throughput
                    << " MB/s\n";
            }


            std::cout
                << "Created: "
                << archivePath
                << "\n";
        }


        // =====================================================
        // DECOMPRESSION
        // =====================================================

        else {

            ParallelCompressedData compressed =
                readArchive(filename);


            auto start =
                std::chrono::steady_clock::now();


            std::vector<unsigned char> reconstructed =
                parallelDecompress(
                    compressed,
                    threadCount
                );


            auto end =
                std::chrono::steady_clock::now();


            double decompressionTime =
                std::chrono::duration<double>(
                    end - start
                ).count();


            std::string outputPath =
                filename + ".decoded";


            writeBinaryFile(
                outputPath,
                reconstructed
            );


            std::cout << "\n--- Decompression Results ---\n";

            std::cout
                << "Reconstructed size: "
                << reconstructed.size()
                << " bytes\n";

            std::cout
                << "Chunks: "
                << compressed.chunks.size()
                << "\n";

            std::cout
                << "Decompression time: "
                << decompressionTime
                << " seconds\n";


            if (!reconstructed.empty()) {

                double throughput =
                    (
                        static_cast<double>(
                            reconstructed.size()
                        ) /
                        (1024.0 * 1024.0)
                    ) /
                    decompressionTime;

                std::cout
                    << "Decompression throughput: "
                    << throughput
                    << " MB/s\n";
            }


            std::cout
                << "Created: "
                << outputPath
                << "\n";
        }

    }
    catch (const std::exception& error) {

        std::cerr
            << "Error: "
            << error.what()
            << "\n";

        return 1;
    }


    return 0;
}