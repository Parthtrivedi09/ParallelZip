#include "file_utils.h"
#include <fstream>
#include <stdexcept>
#include <filesystem>

bool fileExists(const std::string& path) {
    return std::filesystem::exists(path);
}

std::uintmax_t getFileSize(const std::string& path) {
    return std::filesystem::file_size(path);
}

std::vector<unsigned char> readBinaryFile(const std::string& path) {

    std::ifstream file(path, std::ios::binary);

    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    return std::vector<unsigned char>(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}