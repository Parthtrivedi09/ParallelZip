#include <iostream>
#include <string>

#include "file_utils.h"
#include <vector>

int main(int argc, char* argv[]) {

    if (argc < 3) {
        std::cout << "Usage:\n";
        std::cout << "  parallelzip compress <file>\n";
        std::cout << "  parallelzip decompress <file>\n";
        return 1;
    }

    std::string command = argv[1];
    std::string filename = argv[2];

    if (!fileExists(filename)) {
        std::cerr << "Error: File does not exist: "
                  << filename << "\n";

        return 1;
    }

    std::cout << "File: " << filename << "\n";
    std::cout << "Size: " << getFileSize(filename)
              << " bytes\n";
    std::vector<unsigned char> data = readBinaryFile(filename);

    std::cout << "Bytes read: " << data.size() << "\n";


    for (unsigned char byte : data) {
        std::cout << byte << " ";
    }

    std::cout << "\n";

    if (command == "compress") {
        std::cout << "Ready for compression.\n";
    }
    else if (command == "decompress") {
        std::cout << "Ready for decompression.\n";
    }
    else {
        std::cerr << "Unknown command: "
                  << command << "\n";

        return 1;
    }

    return 0;
}