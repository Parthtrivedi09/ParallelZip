#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <string>
#include <vector>
#include <cstdint>

bool fileExists(const std::string& path);

std::uintmax_t getFileSize(const std::string& path);

std::vector<unsigned char> readBinaryFile(const std::string& path);

#endif