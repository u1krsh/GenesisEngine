#pragma once

#include "BSPTypes.h"
#include "math/Math.h"
#include <vector>
#include <cstdint>
#include <unordered_map>

namespace Genesis {

// Forward declarations
class BSPTree;

// ============================================================================
// BSP Collision Constants
// ============================================================================
constexpr float COLLISION_EPSILON = 0.001f;      // Plane classification epsilon
constexpr float DIST_EPSILON = 0.03125f;         // Distance epsilon for traces
constexpr float OVERCLIP = 1.001f;               // Slight push off surfaces
constexpr int MAX_CLIP_PLANES = 5;               // Max planes for slide move
constexpr int MAX_SLIDE_BUMPS = 4;               // Max iterations for sliding

// ============================================================================
// BSP Collision Brush - Convex volume defined by planes
// ============================================================================
struct BSPCollisionBrush {
    uint32_t firstPlane = 0;       // Index into collision planes array
    uint32_t numPlanes = 0;        // Number of bounding planes
    Vec3 boundsMin = Vec3(0.0f);   // AABB for broad phase
    Vec3 boundsMax = Vec3(0.0f);
    BSPContents contents = BSPContents::Solid;
    uint32_t brushId = 0;          // Original brush ID for debugging
    
    // Check if a point is inside the AABB (broad phase)
    bool ContainsPointAABB(const Vec3& point) const {
        return point.x >= boundsMin.x && point.x <= boundsMax.x &&
               point.y >= boundsMin.y && point.y <= boundsMax.y &&
               point.z >= boundsMin.z && point.z <= boundsMax.z;
    }
    
    // Expand AABB by radius for capsule collision
    bool ContainsPointAABBExpanded(const Vec3& point, float radius) const {
        return point.x >= boundsMin.x - radius && point.x <= boundsMax.x + radius &&
               point.y >= boundsMin.y - radius && point.y <= boundsMax.y + radius &&
               point.z >= boundsMin.z - radius && point.z <= boundsMax.z + radius;
    }
};

// ============================================================================
// BSP Collision Plane - Plane with additional collision data
// ============================================================================
struct BSPCollisionPlane {
    Vec3 normal = Vec3(0.0f, 1.0f, 0.0f);
    float distance = 0.0f;
    
    BSPCollisionPlane() = default;
    BSPCollisionPlane(const Vec3& n, float d) : normal(glm::normalize(n)), distance(d) {}
    BSPCollisionPlane(const BSPPlane& plane) : normal(plane.normal), distance(plane.distance) {}
    
    // Classify point relative to plane
    float ClassifyPoint(const Vec3& point) const {
        return glm::dot(normal, point) - distance;
    }
    
    // Expand plane for capsule radius
    float ClassifyPointExpanded(const Vec3& point, float radius) const {
        return glm::dot(normal, point) - distance - radius;
    }
};

// ============================================================================
// Trace Result - Result of a collision trace
// ============================================================================
struct TraceResult {
    bool startSolid = false;      // Started inside solid geometry
    bool allSolid = false;        // Entire trace was inside solid
    float fraction = 1.0f;        // 0.0 = hit immediately, 1.0 = no hit
    Vec3 endPos = Vec3(0.0f);     // Final position after trace
    Vec3 hitNormal = Vec3(0.0f);  // Surface normal at hit point
    BSPContents contents = BSPContents::Empty;  // What was hit
    
    // Helper to check if we hit something
    bool DidHit() const { return fraction < 1.0f || startSolid; }
    
    // Merge with another trace result (keep the earliest hit)
    void MergeWith(const TraceResult& other) {
        if (other.startSolid) {
            startSolid = true;
            if (other.allSolid) {
                allSolid = true;
            }
        }
        if (other.fraction < fraction) {
            fraction = other.fraction;
            endPos = other.endPos;
            hitNormal = other.hitNormal;
            contents = other.contents;
        }
    }
};

// ============================================================================
// Capsule - Collision shape for player
// ============================================================================
struct CollisionCapsule {
    float radius = 0.3f;          // Capsule radius
    float halfHeight = 0.9f;      // Half of the cylinder height (not including caps)
    
    CollisionCapsule() = default;
    CollisionCapsule(float r, float hh) : radius(r), halfHeight(hh) {}
    
    // Get total height including spherical caps
    float GetTotalHeight() const { return (halfHeight * 2.0f) + (radius * 2.0f); }
    
    // Get the AABB that contains this capsule at a given position
    AABB GetAABB(const Vec3& position) const {
        Vec3 halfExtents(radius, halfHeight + radius, radius);
        return AABB(position - halfExtents, position + halfExtents);
    }
};

// ============================================================================
// BSP Collision System - Handles all collision queries against BSP tree
// ============================================================================
class BSPCollision {
public:
    BSPCollision() = default;
    ~BSPCollision() = default;
    
    // ========================================================================
    // Initialization
    // ========================================================================
    
    // Set the BSP tree to use for collision
    void SetBSPTree(BSPTree* tree) { m_bspTree = tree; }
    
    // ========================================================================
    // Collision Data Management
    // ========================================================================
    
    // Get collision data (for building)
    std::vector<BSPCollisionBrush>& GetBrushes() { return m_brushes; }
    std::vector<BSPCollisionPlane>& GetPlanes() { return m_planes; }
    
    const std::vector<BSPCollisionBrush>& GetBrushes() const { return m_brushes; }
    const std::vector<BSPCollisionPlane>& GetPlanes() const { return m_planes; }
    
    // Clear all collision data
    void Clear();
    
    // Build spatial grid for fast queries (call after adding all brushes)
    void BuildGrid();
    
    // ========================================================================
    // Trace Functions
    // ========================================================================
    
    // Trace a capsule through the world
    TraceResult TraceCapsule(const Vec3& start, const Vec3& end,
                             const CollisionCapsule& capsule) const;
    
    // Trace a point (zero-size trace)
    TraceResult TracePoint(const Vec3& start, const Vec3& end) const;
    
    // Trace a sphere
    TraceResult TraceSphere(const Vec3& start, const Vec3& end, float radius) const;
    
    // ========================================================================
    // Point Queries
    // ========================================================================
    
    // Check if a point is inside solid geometry
    bool IsPointSolid(const Vec3& point) const;
    
    // Get contents at a point
    BSPContents GetPointContents(const Vec3& point) const;
    
    // ========================================================================
    // Movement Functions (Quake-style sliding)
    // ========================================================================
    
    // Perform a slide move - returns the final position after sliding
    Vec3 SlideMove(const Vec3& start, const Vec3& velocity, float deltaTime,
                   const CollisionCapsule& capsule, Vec3& outVelocity) const;
    
    // Ground check - trace down to find ground
    TraceResult GroundTrace(const Vec3& position, const CollisionCapsule& capsule,
                            float maxDistance = 0.1f) const;
    
    // Step up check - can the player step up onto something?
    bool TryStepUp(const Vec3& position, const Vec3& velocity, float deltaTime,
                   const CollisionCapsule& capsule, float stepHeight,
                   Vec3& outPosition) const;
    
    // ========================================================================
    // Debug
    // ========================================================================
    
    uint32_t GetBrushCount() const { return static_cast<uint32_t>(m_brushes.size()); }
    uint32_t GetPlaneCount() const { return static_cast<uint32_t>(m_planes.size()); }
    
private:
    // ========================================================================
    // Internal Trace Functions
    // ========================================================================
    
    // Trace against a single brush
    void TraceToBrush(const BSPCollisionBrush& brush, const Vec3& start, 
                      const Vec3& end, float radius, TraceResult& result) const;
    
    // Traverse BSP tree for collision (uses leaf brush references)
    void TraceTreeRecursive(int32_t nodeIndex, const Vec3& start, const Vec3& end,
                            float radius, float p1f, float p2f,
                            const Vec3& p1, const Vec3& p2, TraceResult& result) const;
    
    // Clip velocity to a plane
    Vec3 ClipVelocity(const Vec3& velocity, const Vec3& normal) const;
    
private:
    BSPTree* m_bspTree = nullptr;
    std::vector<BSPCollisionBrush> m_brushes;
    std::vector<BSPCollisionPlane> m_planes;
    
    // ========================================================================
    // Spatial Grid for Fast Brush Queries
    // ========================================================================
    static constexpr float GRID_CELL_SIZE = 8.0f;  // World units per cell
    
    struct CollisionGrid {
        std::unordered_map<uint64_t, std::vector<uint32_t>> cells;
        Vec3 worldMin = Vec3(0.0f);
        Vec3 worldMax = Vec3(0.0f);
        bool built = false;
        
        void Clear() { cells.clear(); built = false; }
    };
    CollisionGrid m_grid;
    
    // Grid helper functions
    uint64_t HashGridCell(int32_t x, int32_t y, int32_t z) const;
    void GetCellCoords(const Vec3& pos, int32_t& x, int32_t& y, int32_t& z) const;
    void QueryGridCells(const AABB& bounds, std::vector<uint32_t>& outBrushIndices) const;
};

} // namespace Genesis
