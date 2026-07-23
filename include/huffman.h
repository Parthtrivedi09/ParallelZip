#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using FrequencyTable = std::array<std::uint64_t, 256>;
using HuffmanCodeTable = std::unordered_map<unsigned char, std::string>;

struct HuffmanNode {
    unsigned char byte;
    std::uint64_t frequency;

    std::shared_ptr<HuffmanNode> left;
    std::shared_ptr<HuffmanNode> right;

    HuffmanNode(unsigned char byteValue, std::uint64_t freq);

    HuffmanNode(
        std::uint64_t freq,
        std::shared_ptr<HuffmanNode> leftChild,
        std::shared_ptr<HuffmanNode> rightChild
    );

    bool isLeaf() const;
};

struct HuffmanEncodedData {
    // Actual packed compressed bytes.
    std::vector<std::uint8_t> data;

    // Number of meaningful bits stored in data.
    // The final byte may contain unused padding bits.
    std::uint64_t bitCount = 0;

    // Required to reconstruct the Huffman tree.
    FrequencyTable frequencies{};
};

FrequencyTable buildFrequencyTable(
    const std::vector<unsigned char>& data
);

std::shared_ptr<HuffmanNode> buildHuffmanTree(
    const FrequencyTable& frequencies
);

HuffmanCodeTable buildHuffmanCodes(
    const std::shared_ptr<HuffmanNode>& root
);

HuffmanEncodedData huffmanCompress(
    const std::vector<unsigned char>& data
);

std::vector<unsigned char> huffmanDecompress(
    const HuffmanEncodedData& encoded
);

#endif