#include "BSPPVS.h"
#include "BSPTree.h"
#include "core/Logger.h"
#include <queue>
#include <algorithm>
#include <cmath>

namespace Genesis {

// Static empty containers for invalid queries
const std::vector<uint32_t> BSPPVS::s_emptyList;
const std::vector<bool> BSPPVS::s_emptyBitset;

// ============================================================================
// Building
// ============================================================================

void BSPPVS::Clear() {
    m_built = false;
    m_numLeafs = 0;
    m_numWords = 0;
    m_adjacency.clear();
    m_compressedVisibility.clear();
    m_workingBuffer.clear();
    m_workingBufferLeaf = -1;
    m_visibleLeafLists.clear();
    m_visibleLeafListsValid.clear();
}

void BSPPVS::Build(const BSPTree& tree) {
    Clear();

    const auto& leafs = tree.GetLeafs();
    m_numLeafs = static_cast<uint32_t>(leafs.size());

    if (m_numLeafs == 0) {
        LOG_WARNING("BSPPVS", "No leafs to build PVS for");
        return;
    }

    if (m_numLeafs > PVS_MAX_LEAFS) {
        LOG_WARNING("BSPPVS", "Too many leafs (" + std::to_string(m_numLeafs) + 
                    "), max is " + std::to_string(PVS_MAX_LEAFS));
        return;
    }

    LOG_INFO("BSPPVS", "Building PVS for " + std::to_string(m_numLeafs) + " leafs...");

    // Calculate number of 64-bit words needed
    m_numWords = (m_numLeafs + PVS_BITS_PER_WORD - 1) / PVS_BITS_PER_WORD;

    // Initialize data structures
    m_adjacency.resize(m_numLeafs);
    m_compressedVisibility.resize(m_numLeafs);
    m_workingBuffer.resize(m_numWords, 0);
    m_visibleLeafLists.resize(m_numLeafs);
    m_visibleLeafListsValid.resize(m_numLeafs, false);

    // Step 1: Build adjacency graph
    BuildAdjacencyGraph(tree);

    // Step 2: Compute visibility for each leaf and compress
    for (uint32_t i = 0; i < m_numLeafs; ++i) {
        // Clear working buffer
        std::memset(m_workingBuffer.data(), 0, m_numWords * sizeof(uint64_t));
        m_workingBufferLeaf = static_cast<int32_t>(i);
        
        ComputeLeafVisibility(i);
        CompressVisibility(i);
    }

    // Clear working buffer after build
    m_workingBufferLeaf = -1;

    m_built = true;

    // Log stats
    LOG_INFO("BSPPVS", "PVS built! Average visibility: " + 
             std::to_string(static_cast<int>(GetAverageVisibility() * 100)) + "%");
    LOG_INFO("BSPPVS", "PVS memory usage: " + std::to_string(GetMemoryUsage() / 1024) + " KB");
}

// ============================================================================
// Compression Helpers
// ============================================================================

void BSPPVS::SetVisibleBit(uint32_t leafIndex, bool visible) {
    uint32_t wordIdx = leafIndex / PVS_BITS_PER_WORD;
    uint32_t bitIdx = leafIndex % PVS_BITS_PER_WORD;
    
    if (wordIdx < m_workingBuffer.size()) {
        if (visible) {
            m_workingBuffer[wordIdx] |= (1ULL << bitIdx);
        } else {
            m_workingBuffer[wordIdx] &= ~(1ULL << bitIdx);
        }
    }
}

bool BSPPVS::GetVisibleBit(uint32_t leafIndex) const {
    uint32_t wordIdx = leafIndex / PVS_BITS_PER_WORD;
    uint32_t bitIdx = leafIndex % PVS_BITS_PER_WORD;
    
    if (wordIdx < m_workingBuffer.size()) {
        return (m_workingBuffer[wordIdx] & (1ULL << bitIdx)) != 0;
    }
    return false;
}

void BSPPVS::CompressVisibility(uint32_t leafIndex) {
    CompressedPVS& compressed = m_compressedVisibility[leafIndex];
    compressed.data.clear();
    compressed.visibleCount = 0;
    
    // Simple RLE compression optimized for sparse visibility
    // Format: For each run of identical words:
    //   [1 byte: count] [8 bytes: word value]
    // Special cases:
    //   count = 0xFF means next byte is extended count
    
    uint32_t i = 0;
    while (i < m_numWords) {
        uint64_t currentWord = m_workingBuffer[i];
        uint32_t runLength = 1;
        
        // Count consecutive identical words
        while (i + runLength < m_numWords && 
               m_workingBuffer[i + runLength] == currentWord &&
               runLength < 254) {
            runLength++;
        }
        
        // Write run
        compressed.data.push_back(static_cast<uint8_t>(runLength));
        
        // Write word value (8 bytes, little-endian)
        for (int b = 0; b < 8; b++) {
            compressed.data.push_back(static_cast<uint8_t>((currentWord >> (b * 8)) & 0xFF));
        }
        
        // Count visible bits using popcount
        #if defined(__GNUC__) || defined(__clang__)
            compressed.visibleCount += __builtin_popcountll(currentWord) * runLength;
        #else
            // Fallback bit counting
            uint64_t v = currentWord;
            uint32_t c = 0;
            while (v) { c++; v &= v - 1; }
            compressed.visibleCount += c * runLength;
        #endif
        
        i += runLength;
    }
}

void BSPPVS::DecompressVisibility(uint32_t leafIndex) const {
    // Skip if already decompressed
    if (m_workingBufferLeaf == static_cast<int32_t>(leafIndex)) {
        return;
    }
    
    // Resize buffer if needed
    if (m_workingBuffer.size() < m_numWords) {
        m_workingBuffer.resize(m_numWords);
    }
    
    // Clear buffer
    std::memset(m_workingBuffer.data(), 0, m_numWords * sizeof(uint64_t));
    
    const CompressedPVS& compressed = m_compressedVisibility[leafIndex];
    
    uint32_t wordIdx = 0;
    size_t dataIdx = 0;
    
    while (dataIdx < compressed.data.size() && wordIdx < m_numWords) {
        uint8_t runLength = compressed.data[dataIdx++];
        
        // Read 8-byte word value
        uint64_t wordValue = 0;
        for (int b = 0; b < 8 && dataIdx < compressed.data.size(); b++) {
            wordValue |= static_cast<uint64_t>(compressed.data[dataIdx++]) << (b * 8);
        }
        
        // Fill run
        for (uint32_t r = 0; r < runLength && wordIdx < m_numWords; r++) {
            m_workingBuffer[wordIdx++] = wordValue;
        }
    }
    
    m_workingBufferLeaf = static_cast<int32_t>(leafIndex);
}

// ============================================================================
// Adjacency Graph
// ============================================================================

void BSPPVS::BuildAdjacencyGraph(const BSPTree& tree) {
    const auto& leafs = tree.GetLeafs();

    // Count how many leafs have faces (non-empty leafs for rendering)
    uint32_t leafsWithFaces = 0;
    for (uint32_t i = 0; i < m_numLeafs; ++i) {
        if (leafs[i].numFaces > 0) {
            leafsWithFaces++;
        }
    }

    // For each pair of leafs, check if they share a face (strict adjacency)
    for (uint32_t i = 0; i < m_numLeafs; ++i) {
        if (leafs[i].numFaces == 0) continue;
        
        for (uint32_t j = i + 1; j < m_numLeafs; ++j) {
            if (leafs[j].numFaces == 0) continue;

            // STRICT adjacency: boxes must TOUCH on exactly one axis
            const float epsilon = 0.1f;
            
            bool xTouching = std::abs(leafs[i].boundsMax.x - leafs[j].boundsMin.x) < epsilon ||
                            std::abs(leafs[j].boundsMax.x - leafs[i].boundsMin.x) < epsilon;
            bool yTouching = std::abs(leafs[i].boundsMax.y - leafs[j].boundsMin.y) < epsilon ||
                            std::abs(leafs[j].boundsMax.y - leafs[i].boundsMin.y) < epsilon;
            bool zTouching = std::abs(leafs[i].boundsMax.z - leafs[j].boundsMin.z) < epsilon ||
                            std::abs(leafs[j].boundsMax.z - leafs[i].boundsMin.z) < epsilon;
            
            // Check overlap on the OTHER two axes
            bool xOverlap = leafs[i].boundsMax.x > leafs[j].boundsMin.x + epsilon &&
                           leafs[i].boundsMin.x < leafs[j].boundsMax.x - epsilon;
            bool yOverlap = leafs[i].boundsMax.y > leafs[j].boundsMin.y + epsilon &&
                           leafs[i].boundsMin.y < leafs[j].boundsMax.y - epsilon;
            bool zOverlap = leafs[i].boundsMax.z > leafs[j].boundsMin.z + epsilon &&
                           leafs[i].boundsMin.z < leafs[j].boundsMax.z - epsilon;

            // Adjacent only if touching on ONE axis and overlapping on the OTHER TWO
            bool adjacent = false;
            if (xTouching && yOverlap && zOverlap) adjacent = true;
            if (yTouching && xOverlap && zOverlap) adjacent = true;
            if (zTouching && xOverlap && yOverlap) adjacent = true;

            if (adjacent) {
                m_adjacency[i].push_back(j);
                m_adjacency[j].push_back(i);
            }
        }
    }

    // Log adjacency stats
    uint32_t totalAdjacent = 0;
    for (const auto& adj : m_adjacency) {
        totalAdjacent += static_cast<uint32_t>(adj.size());
    }
    LOG_INFO("BSPPVS", "Built adjacency: " + std::to_string(leafsWithFaces) + " leafs with faces, " 
             + std::to_string(totalAdjacent / 2) + " portal connections");
}

// ============================================================================
// Visibility Computation
// ============================================================================

void BSPPVS::ComputeLeafVisibility(uint32_t leafIndex) {
    // A leaf is always visible to itself
    SetVisibleBit(leafIndex, true);

    // Flood fill through adjacent leafs with increased depth for better accuracy
    constexpr uint32_t MAX_VISIBILITY_DEPTH = 5;  // Increased from 3 for better visibility

    std::queue<std::pair<uint32_t, uint32_t>> queue;  // (leafIndex, depth)
    std::vector<bool> visited(m_numLeafs, false);

    queue.push({leafIndex, 0});
    visited[leafIndex] = true;

    while (!queue.empty()) {
        auto [currentLeaf, depth] = queue.front();
        queue.pop();

        // Mark as visible
        SetVisibleBit(currentLeaf, true);

        // Don't go deeper than max depth
        if (depth >= MAX_VISIBILITY_DEPTH) {
            continue;
        }

        // Add all adjacent leafs to queue
        for (uint32_t adjacent : m_adjacency[currentLeaf]) {
            if (!visited[adjacent]) {
                visited[adjacent] = true;
                queue.push({adjacent, depth + 1});
            }
        }
    }
}

void BSPPVS::MarkLeafVisible(uint32_t fromLeaf, uint32_t toLeaf) {
    if (fromLeaf < m_numLeafs && toLeaf < m_numLeafs) {
        // Need to decompress, modify, and recompress
        DecompressVisibility(fromLeaf);
        SetVisibleBit(toLeaf, true);
        CompressVisibility(fromLeaf);
        m_visibleLeafListsValid[fromLeaf] = false;  // Invalidate cache
    }
}

// ============================================================================
// Queries
// ============================================================================

bool BSPPVS::IsLeafVisible(uint32_t leafA, uint32_t leafB) const {
    if (!m_built || leafA >= m_numLeafs || leafB >= m_numLeafs) {
        return true;  // If not built, assume everything visible
    }
    
    // Decompress and check bit
    DecompressVisibility(leafA);
    return GetVisibleBit(leafB);
}

const std::vector<uint32_t>& BSPPVS::GetVisibleLeafs(uint32_t leafIndex) const {
    if (!m_built || leafIndex >= m_numLeafs) {
        return s_emptyList;
    }

    // Build cache if needed
    if (!m_visibleLeafListsValid[leafIndex]) {
        m_visibleLeafLists[leafIndex].clear();
        
        // Decompress and iterate bits
        DecompressVisibility(leafIndex);
        
        for (uint32_t word = 0; word < m_numWords; ++word) {
            uint64_t bits = m_workingBuffer[word];
            if (bits == 0) continue;  // Skip empty words (common case)
            
            uint32_t baseIdx = word * PVS_BITS_PER_WORD;
            
            // Fast bit iteration using trailing zeros
            while (bits != 0) {
                #if defined(__GNUC__) || defined(__clang__)
                    int bitPos = __builtin_ctzll(bits);
                #else
                    int bitPos = 0;
                    while ((bits & (1ULL << bitPos)) == 0) bitPos++;
                #endif
                
                uint32_t leafIdx = baseIdx + bitPos;
                if (leafIdx < m_numLeafs) {
                    m_visibleLeafLists[leafIndex].push_back(leafIdx);
                }
                
                bits &= bits - 1;  // Clear lowest set bit
            }
        }
        m_visibleLeafListsValid[leafIndex] = true;
    }

    return m_visibleLeafLists[leafIndex];
}

const std::vector<bool>& BSPPVS::GetVisibilityBitset(uint32_t leafIndex) const {
    // This method is for debugging - we'll return empty for now
    // as the internal format changed to compressed storage
    return s_emptyBitset;
}

// ============================================================================
// Statistics
// ============================================================================

float BSPPVS::GetAverageVisibility() const {
    if (!m_built || m_numLeafs == 0) {
        return 1.0f;  // Everything visible by default
    }

    uint64_t totalVisible = 0;
    for (uint32_t i = 0; i < m_numLeafs; ++i) {
        totalVisible += m_compressedVisibility[i].visibleCount;
    }

    return static_cast<float>(totalVisible) / static_cast<float>(m_numLeafs * m_numLeafs);
}

size_t BSPPVS::GetMemoryUsage() const {
    size_t bytes = 0;

    // Adjacency graph
    bytes += m_adjacency.capacity() * sizeof(std::vector<uint32_t>);
    for (const auto& adj : m_adjacency) {
        bytes += adj.capacity() * sizeof(uint32_t);
    }

    // Compressed visibility data
    bytes += m_compressedVisibility.capacity() * sizeof(CompressedPVS);
    for (const auto& pvs : m_compressedVisibility) {
        bytes += pvs.data.capacity();
    }
    
    // Working buffer
    bytes += m_workingBuffer.capacity() * sizeof(uint64_t);

    // Cached lists
    bytes += m_visibleLeafLists.capacity() * sizeof(std::vector<uint32_t>);
    for (const auto& list : m_visibleLeafLists) {
        bytes += list.capacity() * sizeof(uint32_t);
    }

    return bytes;
}

} // namespace Genesis
