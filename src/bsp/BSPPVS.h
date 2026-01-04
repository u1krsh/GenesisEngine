#pragma once

#include "BSPTypes.h"
#include "math/Math.h"
#include <vector>
#include <cstdint>
#include <bitset>

namespace Genesis {

// Forward declaration
class BSPTree;

// ============================================================================
// PVS Constants
// ============================================================================
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
    bool m_built = false;
    uint32_t m_numLeafs = 0;

    // Adjacency graph: for each leaf, list of adjacent leaf indices
    std::vector<std::vector<uint32_t>> m_adjacency;

    // Visibility data: for each leaf, bitset of visible leafs
    std::vector<std::vector<bool>> m_visibility;

    // Cached visible leaf lists (computed on demand)
    mutable std::vector<std::vector<uint32_t>> m_visibleLeafLists;
    mutable std::vector<bool> m_visibleLeafListsValid;

    // Empty list for invalid queries
    static const std::vector<uint32_t> s_emptyList;
    static const std::vector<bool> s_emptyBitset;
};

} // namespace Genesis
