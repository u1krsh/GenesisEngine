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
    m_adjacency.clear();
    m_visibility.clear();
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

    // Initialize data structures
    m_adjacency.resize(m_numLeafs);
    m_visibility.resize(m_numLeafs);
    m_visibleLeafLists.resize(m_numLeafs);
    m_visibleLeafListsValid.resize(m_numLeafs, false);

    for (uint32_t i = 0; i < m_numLeafs; ++i) {
        m_visibility[i].resize(m_numLeafs, false);
    }

    // Step 1: Build adjacency graph
    BuildAdjacencyGraph(tree);

    // Step 2: Compute visibility for each leaf
    for (uint32_t i = 0; i < m_numLeafs; ++i) {
        ComputeLeafVisibility(i);
    }

    m_built = true;

    // Log stats
    LOG_INFO("BSPPVS", "PVS built! Average visibility: " + 
             std::to_string(static_cast<int>(GetAverageVisibility() * 100)) + "%");
    LOG_INFO("BSPPVS", "PVS memory usage: " + std::to_string(GetMemoryUsage() / 1024) + " KB");
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
            // (not overlap on all three)
            const float epsilon = 0.1f;  // Small epsilon for touching
            
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
    m_visibility[leafIndex][leafIndex] = true;

    // Flood fill through adjacent leafs
    // We use BFS with a maximum depth limit - lower depth = more aggressive culling
    constexpr uint32_t MAX_VISIBILITY_DEPTH = 3;  // Reduced for tighter culling

    std::queue<std::pair<uint32_t, uint32_t>> queue;  // (leafIndex, depth)
    std::vector<bool> visited(m_numLeafs, false);

    queue.push({leafIndex, 0});
    visited[leafIndex] = true;

    while (!queue.empty()) {
        auto [currentLeaf, depth] = queue.front();
        queue.pop();

        // Mark as visible
        m_visibility[leafIndex][currentLeaf] = true;

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
        m_visibility[fromLeaf][toLeaf] = true;
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
    return m_visibility[leafA][leafB];
}

const std::vector<uint32_t>& BSPPVS::GetVisibleLeafs(uint32_t leafIndex) const {
    if (!m_built || leafIndex >= m_numLeafs) {
        return s_emptyList;
    }

    // Build cache if needed
    if (!m_visibleLeafListsValid[leafIndex]) {
        m_visibleLeafLists[leafIndex].clear();
        for (uint32_t i = 0; i < m_numLeafs; ++i) {
            if (m_visibility[leafIndex][i]) {
                m_visibleLeafLists[leafIndex].push_back(i);
            }
        }
        m_visibleLeafListsValid[leafIndex] = true;
    }

    return m_visibleLeafLists[leafIndex];
}

const std::vector<bool>& BSPPVS::GetVisibilityBitset(uint32_t leafIndex) const {
    if (!m_built || leafIndex >= m_numLeafs) {
        return s_emptyBitset;
    }
    return m_visibility[leafIndex];
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
        for (uint32_t j = 0; j < m_numLeafs; ++j) {
            if (m_visibility[i][j]) {
                totalVisible++;
            }
        }
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

    // Visibility data
    bytes += m_visibility.capacity() * sizeof(std::vector<bool>);
    for (const auto& vis : m_visibility) {
        bytes += (vis.size() + 7) / 8;  // Bits to bytes
    }

    // Cached lists
    bytes += m_visibleLeafLists.capacity() * sizeof(std::vector<uint32_t>);
    for (const auto& list : m_visibleLeafLists) {
        bytes += list.capacity() * sizeof(uint32_t);
    }

    return bytes;
}

} // namespace Genesis
