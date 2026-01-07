#pragma once

#include "BSPTypes.h"
#include "math/Math.h"
#include <vector>
#include <cstdint>
#include <bitset>
#include <cstring>

namespace Genesis {

// Forward declaration
class BSPTree;

// ============================================================================
// PVS Constants & Compression Settings
// ============================================================================
constexpr uint32_t PVS_BITS_PER_WORD = 64;
constexpr uint32_t PVS_MAX_LEAFS = 8192;  // Maximum supported leafs for PVS

// ============================================================================
// BSPPVS - Potentially Visible Set
//
// Precomputed visibility data for BSP leafs.
// For each leaf, stores which other leafs are potentially visible.
//
// Phase 3 uses a conservative flood-fill approach:
// 1. Build adjacency graph (which leafs share portals)
// 2. For each leaf, flood fill through portals
// 3. Store visible leafs in compressed bitset
// ============================================================================
class BSPPVS {
public:
    BSPPVS() = default;
    ~BSPPVS() = default;

    // ========================================================================
    // Building
    // ========================================================================

    // Build PVS for the given BSP tree
    void Build(const BSPTree& tree);

    // Clear all PVS data
    void Clear();

    // Check if PVS has been built
    bool IsBuilt() const { return m_built; }

    // ========================================================================
    // Queries
    // ========================================================================

    // Is leafB potentially visible from leafA?
    bool IsLeafVisible(uint32_t leafA, uint32_t leafB) const;

    // Get list of visible leaf indices for a given leaf
    // Returns reference to cached list (valid until next Build() or Clear())
    const std::vector<uint32_t>& GetVisibleLeafs(uint32_t leafIndex) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    uint32_t GetNumLeafs() const { return m_numLeafs; }
    
    // Average percentage of leafs visible from any leaf (0.0 - 1.0)
    float GetAverageVisibility() const;
    
    // Total bytes used by PVS data
    size_t GetMemoryUsage() const;

    // ========================================================================
    // Debugging
    // ========================================================================

    // Get the visibility bitset for a leaf (for debugging)
    const std::vector<bool>& GetVisibilityBitset(uint32_t leafIndex) const;

private:
    // ========================================================================
    // Internal Methods
    // ========================================================================

    // Build adjacency graph - which leafs are adjacent (share a portal)?
    void BuildAdjacencyGraph(const BSPTree& tree);

    // Compute visibility for a single leaf using flood fill
    void ComputeLeafVisibility(uint32_t leafIndex);

    // Add visible leaf to the set
    void MarkLeafVisible(uint32_t fromLeaf, uint32_t toLeaf);

    // Check if two leafs share a face (are adjacent)
    bool AreLeafsAdjacent(const BSPTree& tree, uint32_t leafA, uint32_t leafB) const;

    // Find portal between two adjacent leafs (returns bounding box of shared area)
    AABB FindPortal(const BSPTree& tree, uint32_t leafA, uint32_t leafB) const;

private:
    // ========================================================================
    // Compression Helpers
    // ========================================================================
    
    // Compress visibility bitset using simple RLE encoding
    void CompressVisibility(uint32_t leafIndex);
    
    // Decompress visibility to working buffer
    void DecompressVisibility(uint32_t leafIndex) const;
    
    // Set/get bits in working buffer
    void SetVisibleBit(uint32_t leafIndex, bool visible);
    bool GetVisibleBit(uint32_t leafIndex) const;
    
private:
    bool m_built = false;
    uint32_t m_numLeafs = 0;
    uint32_t m_numWords = 0;  // Number of uint64_t words needed

    // Adjacency graph: for each leaf, list of adjacent leaf indices
    std::vector<std::vector<uint32_t>> m_adjacency;

    // ========================================================================
    // Optimized Visibility Storage
    // ========================================================================
    
    // RLE-compressed visibility data per leaf
    // Format: [count][value][count][value]... where:
    //   - count is number of consecutive 64-bit words
    //   - value is the 64-bit visibility mask (or 0x00/0xFF for runs)
    struct CompressedPVS {
        std::vector<uint8_t> data;      // RLE-compressed stream
        uint32_t visibleCount = 0;      // Cached count of visible leafs
    };
    std::vector<CompressedPVS> m_compressedVisibility;
    
    // Working buffer for building visibility (reused to avoid allocations)
    mutable std::vector<uint64_t> m_workingBuffer;
    mutable int32_t m_workingBufferLeaf = -1;  // Which leaf is in buffer, -1 = none

    // Cached visible leaf lists (computed on demand from compressed data)
    mutable std::vector<std::vector<uint32_t>> m_visibleLeafLists;
    mutable std::vector<bool> m_visibleLeafListsValid;

    // Empty containers for invalid queries
    static const std::vector<uint32_t> s_emptyList;
    static const std::vector<bool> s_emptyBitset;
};

} // namespace Genesis
