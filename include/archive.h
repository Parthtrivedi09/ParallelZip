#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <string>

#include "parallel_compressor.h"

// Store all independently compressed chunks
// inside one .pzip archive.
void writeArchive(
    const std::string& archivePath,
    const ParallelCompressedData& compressed
);

// Read all compressed chunks from a .pzip archive.
ParallelCompressedData readArchive(
    const std::string& archivePath
);

#endif