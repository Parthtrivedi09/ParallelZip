cmake_minimum_required(VERSION 3.16)

project(ParallelZip VERSION 1.0)

# ParallelZip uses C++17 features.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the correct threading implementation for the platform.
find_package(Threads REQUIRED)

# Build the ParallelZip executable from all engine modules.
add_executable(
    parallelzip

    src/main.cpp
    src/file_utils.cpp
    src/huffman.cpp
    src/archive.cpp
    src/parallel_compressor.cpp
    src/thread_pool.cpp
    src/checksum.cpp
)

# Allow source files to include headers from include/.
target_include_directories(
    parallelzip
    PRIVATE
    include
)

# Link against the platform threading library.
target_link_libraries(
    parallelzip
    PRIVATE
    Threads::Threads
)