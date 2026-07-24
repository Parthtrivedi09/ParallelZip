# ParallelZip

ParallelZip is a multithreaded lossless file compression engine built
from scratch in C++17 using Huffman coding, bit-level encoding,
fixed-size chunking, and a reusable thread pool.

The compression engine is exposed through a FastAPI backend and a
React-based web interface.

## Features

- Lossless Huffman compression and decompression
- Bit-level compressed representation
- Custom `.pzip` binary archive format
- Fixed-size chunk-based processing
- Parallel compression and decompression
- Reusable C++ thread pool and task queue
- CRC32 integrity verification
- Configurable worker thread count
- Compression ratio and throughput measurement
- FastAPI backend
- React web interface
- CMake build system

## Architecture

```text
                     ParallelZip

                       React UI
                           |
                           | HTTP
                           v
                       FastAPI
                           |
                           v
                    C++17 Engine
                           |
                           v
                    Binary File I/O
                           |
                           v
                     4 MB Chunking
                           |
                           v
                       Task Queue
                    /      |      \
                   v       v       v
                Worker  Worker  Worker
                   |       |       |
                   v       v       v
                 Huffman Compression
                    \      |      /
                           v
                     Bit Packing
                           |
                           v
                     .pzip Archive
                           |
                    CRC32 Integrity
```

## Compression Pipeline

1. Read the input file as binary data.
2. Calculate its CRC32 checksum.
3. Divide the input into fixed-size chunks.
4. Submit chunks to the thread pool.
5. Build an independent Huffman representation for each chunk.
6. Pack encoded bits into bytes.
7. Serialize metadata and compressed data into `.pzip`.

Decompression performs the reverse operation and validates the
reconstructed file using CRC32.

## Archive Format

```text
PZIP Archive
|
|-- Magic Number
|-- Version
|-- Chunk Count
|-- CRC32
|
|-- Chunk 0
|   |-- Original Size
|   |-- Bit Count
|   |-- Payload Size
|   |-- Frequency Table
|   `-- Compressed Data
|
|-- Chunk 1
|   `-- ...
```

## Building the Engine

Requirements:

- C++17 compiler
- CMake
- Threading support

Build:

```bash
cmake -S . -B build
cmake --build build
```

## CLI

Compression:

```bash
parallelzip compress input.txt 4
```

Decompression:

```bash
parallelzip decompress input.txt.pzip 4
```

The final argument specifies the number of worker threads.

## Backend

Install dependencies:

```bash
pip install -r backend/requirements.txt
```

Run:

```bash
python -m uvicorn backend.main:app --reload
```

## Frontend

```bash
cd frontend
npm install
npm run dev
```

## Correctness

ParallelZip performs lossless compression.

On Windows the reconstructed file can be verified using:

```cmd
fc /b original.txt original.txt.pzip.decoded
```

CRC32 is also calculated before compression and verified after
decompression.

## Performance

ParallelZip supports configurable worker counts, allowing compression
performance to be measured using 1, 2, 4, or 8 threads.

Add your measured benchmark table here.

## Compression Characteristics

Huffman coding performs best when the input contains uneven symbol
frequencies, such as many text and structured-data files.

Formats such as JPEG, PNG, MP4, ZIP, and some document formats may show
little additional compression because their contents are already
compressed.

## Future Improvements

- LZ77 + Huffman compression
- Canonical Huffman codes
- More compact frequency metadata
- Streaming compression for very large files
- Automatic raw-storage fallback for incompressible chunks
- Additional benchmarking and profiling

## Tech Stack

**Core:** C++17, STL, CMake  
**Concurrency:** std::thread, mutexes, condition variables, thread pool  
**Compression:** Huffman coding, bit packing, CRC32  
**Backend:** Python, FastAPI  
**Frontend:** React, Vite

File size = 100MB
Threads 	 Time	    Throughput	Speedup vs 1 thread
1	        52.05 s	    1.9 MB/s	    1.00×
2	        43.03 s	    2.3 MB/s	    1.21×
4	        37.99 s	    2.6 MB/s	    1.37×
8	        35.08 s	    2.9 MB/s	    1.48×