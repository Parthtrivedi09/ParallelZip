#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <cstdint>
#include <string>
#include <vector>

// Returns true if the given path exists and points to a regular file.
bool fileExists(const std::string& path);

// Returns the size of the file in bytes.
std::uintmax_t getFileSize(const std::string& path);

// Reads the entire file as raw bytes.
// Using raw bytes allows ParallelZip to work with text files,
// images, PDFs, executables, and other binary formats.
std::vector<unsigned char> readBinaryFile(const std::string& path);

// Write raw bytes to a file.
void writeBinaryFile(
    const std::string& path,
    const std::vector<unsigned char>& data
);

#endif