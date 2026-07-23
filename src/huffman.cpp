#include "huffman.h"

#include <queue>
#include <stdexcept>
#include <utility>


// ============================================================
// HuffmanNode implementation
// ============================================================

// Constructor used when creating a leaf node.
// A leaf represents an actual byte from the input file.
HuffmanNode::HuffmanNode(
    unsigned char byteValue,
    std::uint64_t freq
)
    : byte(byteValue),
      frequency(freq),
      left(nullptr),
      right(nullptr) {
}


// Constructor used when combining two Huffman nodes.
//
// Internal nodes do not represent a real input byte.
// Their frequency is the sum of their child frequencies.
HuffmanNode::HuffmanNode(
    std::uint64_t freq,
    std::shared_ptr<HuffmanNode> leftChild,
    std::shared_ptr<HuffmanNode> rightChild
)
    : byte(0),
      frequency(freq),
      left(std::move(leftChild)),
      right(std::move(rightChild)) {
}


// A node is a leaf when it has no children.
bool HuffmanNode::isLeaf() const {
    return left == nullptr && right == nullptr;
}



// ============================================================
// Step 1: Build frequency table
// ============================================================

FrequencyTable buildFrequencyTable(
    const std::vector<unsigned char>& data
) {
    // {} initializes all 256 positions to zero.
    FrequencyTable frequencies{};

    // The byte value itself can be used as an array index.
    //
    // Example:
    // 'A' = 65
    //
    // frequencies[65] stores the number of A bytes.
    for (unsigned char byte : data) {
        frequencies[byte]++;
    }

    return frequencies;
}



// ============================================================
// Step 2: Build Huffman tree
// ============================================================

namespace {

// std::priority_queue is normally a max-heap.
//
// Huffman requires the two LOWEST-frequency nodes,
// so this comparator turns it into a min-heap.
struct CompareNodes {

    bool operator()(
        const std::shared_ptr<HuffmanNode>& first,
        const std::shared_ptr<HuffmanNode>& second
    ) const {

        return first->frequency > second->frequency;
    }
};

} // namespace


std::shared_ptr<HuffmanNode> buildHuffmanTree(
    const FrequencyTable& frequencies
) {

    std::priority_queue<
        std::shared_ptr<HuffmanNode>,
        std::vector<std::shared_ptr<HuffmanNode>>,
        CompareNodes
    > minHeap;


    // Create one leaf node for every byte that appears
    // at least once in the input.
    for (int i = 0; i < 256; i++) {

        if (frequencies[i] > 0) {

            auto node = std::make_shared<HuffmanNode>(
                static_cast<unsigned char>(i),
                frequencies[i]
            );

            minHeap.push(node);
        }
    }


    // An empty file has no Huffman tree.
    if (minHeap.empty()) {
        return nullptr;
    }


    // Huffman's greedy algorithm:
    //
    // 1. Remove the two nodes with the lowest frequencies.
    // 2. Combine them into a new parent.
    // 3. Push that parent back into the min-heap.
    //
    // Continue until only one node remains.
    while (minHeap.size() > 1) {

        auto left = minHeap.top();
        minHeap.pop();

        auto right = minHeap.top();
        minHeap.pop();


        auto parent = std::make_shared<HuffmanNode>(
            left->frequency + right->frequency,
            left,
            right
        );


        minHeap.push(parent);
    }


    // The final remaining node is the root of the tree.
    return minHeap.top();
}



// ============================================================
// Step 3: Generate Huffman codes
// ============================================================

namespace {

void generateCodes(
    const std::shared_ptr<HuffmanNode>& node,
    const std::string& currentCode,
    HuffmanCodeTable& codes
) {

    if (!node) {
        return;
    }


    // Reaching a leaf means currentCode is the Huffman
    // representation of that byte.
    if (node->isLeaf()) {

        // Special case:
        //
        // If the input contains only one unique byte,
        // the Huffman tree contains only one node.
        //
        // Without this special case its code would be "",
        // so we explicitly assign "0".
        codes[node->byte] =
            currentCode.empty() ? "0" : currentCode;

        return;
    }


    // Convention:
    //
    // left edge  -> 0
    // right edge -> 1

    generateCodes(
        node->left,
        currentCode + "0",
        codes
    );

    generateCodes(
        node->right,
        currentCode + "1",
        codes
    );
}

} // namespace


HuffmanCodeTable buildHuffmanCodes(
    const std::shared_ptr<HuffmanNode>& root
) {

    HuffmanCodeTable codes;

    generateCodes(
        root,
        "",
        codes
    );

    return codes;
}



// ============================================================
// Step 4: Huffman compression
// ============================================================

HuffmanEncodedData huffmanCompress(
    const std::vector<unsigned char>& input
) {
    HuffmanEncodedData result;

    // Step 1: Count byte frequencies.
    result.frequencies = buildFrequencyTable(input);

    if (input.empty()) {
        return result;
    }

    // Step 2: Build Huffman tree and generate codes.
    auto root = buildHuffmanTree(result.frequencies);

    HuffmanCodeTable codes =
        buildHuffmanCodes(root);

    std::uint8_t currentByte = 0;
    int bitsInCurrentByte = 0;

    // Step 3: Replace each input byte with its Huffman code.
    for (unsigned char byte : input) {

        const std::string& code = codes.at(byte);

        for (char bit : code) {

            // Make room for the next bit.
            currentByte <<= 1;

            // Set the lowest bit when the Huffman bit is 1.
            if (bit == '1') {
                currentByte |= 1;
            }

            bitsInCurrentByte++;
            result.bitCount++;

            // Once 8 bits have been collected, we have
            // one complete compressed byte.
            if (bitsInCurrentByte == 8) {
                result.data.push_back(currentByte);

                currentByte = 0;
                bitsInCurrentByte = 0;
            }
        }
    }

    // The encoded stream may not end exactly on an 8-bit boundary.
    //
    // Example:
    // 10110
    //
    // Store it as:
    // 10110000
    //
    // bitCount tells decompression that only the first
    // five bits are meaningful.
    if (bitsInCurrentByte > 0) {

        currentByte <<= (8 - bitsInCurrentByte);

        result.data.push_back(currentByte);
    }

    return result;
}


// ============================================================
// Step 5: Huffman decompression
// ============================================================

std::vector<unsigned char> huffmanDecompress(
    const HuffmanEncodedData& encoded
) {
    std::vector<unsigned char> output;

    std::uint64_t originalSize = 0;

    for (std::uint64_t frequency : encoded.frequencies) {
        originalSize += frequency;
    }

    if (originalSize == 0) {
        return output;
    }

    auto root = buildHuffmanTree(encoded.frequencies);

    if (!root) {
        throw std::runtime_error(
            "Failed to reconstruct Huffman tree."
        );
    }

    // Special case: input contained only one unique byte.
    if (root->isLeaf()) {

        output.assign(
            static_cast<std::size_t>(originalSize),
            root->byte
        );

        return output;
    }

    auto current = root;

    std::uint64_t bitsProcessed = 0;

    // Read every compressed byte.
    for (std::uint8_t byte : encoded.data) {

        // Extract bits from most significant to least significant.
        for (int bitPosition = 7;
             bitPosition >= 0;
             bitPosition--) {

            // Ignore padding bits in the final byte.
            if (bitsProcessed >= encoded.bitCount) {
                break;
            }

            // Move the desired bit to position 0 and mask
            // everything else.
            int bit = (byte >> bitPosition) & 1;

            if (bit == 0) {
                current = current->left;
            }
            else {
                current = current->right;
            }

            if (!current) {
                throw std::runtime_error(
                    "Corrupted Huffman data."
                );
            }

            // A leaf represents one reconstructed byte.
            if (current->isLeaf()) {
                output.push_back(current->byte);
                current = root;
            }

            bitsProcessed++;
        }
    }

    if (output.size() != originalSize) {
        throw std::runtime_error(
            "Decoded size does not match original size."
        );
    }

    return output;
}