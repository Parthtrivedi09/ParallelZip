#include "file_utils.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

bool fileExists(const std::string& path) {
    // We specifically want a normal file, not just any filesystem entry.
    return std::filesystem::exists(path) &&
           std::filesystem::is_regular_file(path);
}

std::uintmax_t getFileSize(const std::string& path) {
    // file_size() returns the size of the file in bytes.
    return std::filesystem::file_size(path);
}

std::vector<unsigned char> readBinaryFile(const std::string& path) {

    // Binary mode ensures that the file is read as raw bytes.
    std::ifstream file(path, std::ios::binary);

    if (!file) {
        throw std::runtime_error(
            "Failed to open file: " + path
        );
    }

    // Read every byte from the file into a vector.
    return std::vector<unsigned char>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

void writeBinaryFile(
    const std::string& path,
    const std::vector<unsigned char>& data
) {
    // Open in binary mode so bytes are written exactly as provided.
    std::ofstream file(path, std::ios::binary);

    if (!file) {
        throw std::runtime_error(
            "Failed to create file: " + path
        );
    }

    if (!data.empty()) {
        file.write(
            reinterpret_cast<const char*>(data.data()),
            static_cast<std::streamsize>(data.size())
        );
    }

    if (!file) {
        throw std::runtime_error(
            "Failed while writing file: " + path
        );
    }
}