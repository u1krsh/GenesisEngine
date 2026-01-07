#pragma once

#include "math/Math.h"
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

namespace Genesis {

// ============================================================================
// BSP Types - Core data structures for Binary Space Partitioning
//
// Phase 1: Basic BSP for rendering
// - Planes: Split planes for BSP tree
// - Nodes: Internal BSP tree nodes
// - Leafs: Terminal nodes containing faces
// - Faces: Renderable polygon data
// - Vertices/Indices: Raw geometry
//
// Future Phases:
// - Phase 2: Collision detection
// - Phase 3: Visibility (PVS)
// - Phase 4: Lightmaps
// ============================================================================

// ============================================================================
// BSP Plane - Splitting plane defined by normal and distance
// ============================================================================
struct BSPPlane {
    Vec3 normal = Vec3(0.0f, 1.0f, 0.0f);  // Unit normal
    float distance = 0.0f;                  // Distance from origin

    BSPPlane() = default;
    BSPPlane(const Vec3& n, float d) : normal(glm::normalize(n)), distance(d) {}
    BSPPlane(const Vec3& n, const Vec3& point)
        : normal(glm::normalize(n)), distance(glm::dot(glm::normalize(n), point)) {}

    // Classify point relative to plane
    // Returns: > 0 = front, < 0 = back, == 0 = on plane
    float ClassifyPoint(const Vec3& point) const {
        return glm::dot(normal, point) - distance;
    }

    // Classify with epsilon tolerance
    int ClassifyPointEpsilon(const Vec3& point, float epsilon = 0.001f) const {
        float d = ClassifyPoint(point);
        if (d > epsilon) return 1;   // Front
        if (d < -epsilon) return -1; // Back
        return 0;                    // On plane
    }
};

// ============================================================================
// BSP Contents - What a leaf contains (for future use)
// ============================================================================
enum class BSPContents : uint32_t {
    Empty = 0,          // Air/void
    Solid = 1,          // Solid geometry
    Water = 2,          // Water volume
    Slime = 4,          // Slime/acid
    Lava = 8,           // Lava
    Sky = 16,           // Sky volume
    PlayerClip = 32,    // Blocks players only
    Trigger = 64,       // Trigger volume
};

inline BSPContents operator|(BSPContents a, BSPContents b) {
    return static_cast<BSPContents>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline BSPContents operator&(BSPContents a, BSPContents b) {
    return static_cast<BSPContents>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// ============================================================================
// BSP Vertex - Vertex data for BSP faces
// ============================================================================
struct BSPVertex {
    Vec3 position;
    Vec3 normal;
    Vec2 texCoord;           // UV1: Diffuse texture
    Vec2 lightmapCoord;      // UV2: Lightmap texture (Phase 4A)
    Vec3 color = Vec3(1.0f); // Vertex color (fallback/tint)

    BSPVertex() = default;
    BSPVertex(const Vec3& pos, const Vec3& norm, const Vec2& tex)
        : position(pos), normal(norm), texCoord(tex), lightmapCoord(Vec2(0.0f)) {}
    BSPVertex(const Vec3& pos, const Vec3& norm, const Vec2& tex, const Vec2& lmCoord)
        : position(pos), normal(norm), texCoord(tex), lightmapCoord(lmCoord) {}
};

// ============================================================================
// BSP Face - A renderable polygon (triangle fan from brush face)
// ============================================================================
struct BSPFace {
    uint32_t firstVertex = 0;   // Index into vertex array
    uint32_t numVertices = 0;   // Number of vertices in this face
    uint32_t firstIndex = 0;    // Index into index array
    uint32_t numIndices = 0;    // Number of indices (triangulated)

    BSPPlane plane;             // Face plane (for backface culling)
    uint32_t materialIndex = 0; // Material/texture index

    // Source brush reference (for editing/debugging)
    uint32_t brushIndex = 0;

    // Bounding box (for culling)
    Vec3 boundsMin = Vec3(0.0f);
    Vec3 boundsMax = Vec3(0.0f);
    
    // Lightmap data (Phase 4A)
    uint32_t lightmapIndex = 0;           // Index into lightmap atlas/array
    Vec2 lightmapMins = Vec2(0.0f);       // UV offset in lightmap atlas
    Vec2 lightmapSize = Vec2(1.0f);       // UV size in lightmap atlas
};

// ============================================================================
// BSP Leaf - Terminal node in BSP tree (contains faces to render)
// ============================================================================
struct BSPLeaf {
    // Faces in this leaf
    uint32_t firstFace = 0;
    uint32_t numFaces = 0;

    // Brush references (for collision)
    uint32_t firstBrush = 0;
    uint32_t numBrushes = 0;

    // Contents flags
    BSPContents contents = BSPContents::Empty;

    // Bounding box
    Vec3 boundsMin = Vec3(0.0f);
    Vec3 boundsMax = Vec3(0.0f);

    // Visibility cluster (Phase 3)
    int32_t cluster = -1;

    // Area (for areaportals, Phase 3+)
    int32_t area = 0;
};

// ============================================================================
// BSP Node - Internal node in BSP tree
// ============================================================================
struct BSPNode {
    uint32_t planeIndex = 0;     // Index into planes array

    // Children: positive = node index, negative = -(leaf index + 1)
    int32_t frontChild = 0;      // Front child (in front of plane)
    int32_t backChild = 0;       // Back child (behind plane)

    // Bounding box (for frustum culling)
    Vec3 boundsMin = Vec3(0.0f);
    Vec3 boundsMax = Vec3(0.0f);

    // Face count in this subtree (for stats)
    uint32_t faceCount = 0;

    // Is child a leaf?
    bool IsFrontLeaf() const { return frontChild < 0; }
    bool IsBackLeaf() const { return backChild < 0; }

    // Get leaf index from child value
    uint32_t GetFrontLeafIndex() const { return static_cast<uint32_t>(-(frontChild + 1)); }
    uint32_t GetBackLeafIndex() const { return static_cast<uint32_t>(-(backChild + 1)); }
};

// ============================================================================
// BSP Brush Reference - Links faces back to source brushes
// ============================================================================
struct BSPBrushRef {
    uint32_t brushId = 0;        // Original brush ID from map
    uint32_t firstSide = 0;      // First side plane
    uint32_t numSides = 0;       // Number of side planes
    BSPContents contents = BSPContents::Solid;
};

// ============================================================================
// BSP Material Entry - Material lookup table
// ============================================================================
struct BSPMaterial {
    std::string name;
    uint32_t flags = 0;          // Surface flags (future)
    // Texture references, shader info, etc. (future)
};

// ============================================================================
// BSP Stats - Statistics about the BSP tree
// ============================================================================
struct BSPStats {
    uint32_t numPlanes = 0;
    uint32_t numNodes = 0;
    uint32_t numLeafs = 0;
    uint32_t numFaces = 0;
    uint32_t numVertices = 0;
    uint32_t numIndices = 0;
    uint32_t numBrushes = 0;
    uint32_t numMaterials = 0;
    uint32_t numLights = 0;
    uint32_t maxTreeDepth = 0;

    void Print() const;
};

// ============================================================================
// Static Light Types (Phase 4D)
// ============================================================================
enum class StaticLightType : uint8_t {
    Point,          // Omnidirectional light at a position
    Directional     // Sun-like parallel rays
};

// ============================================================================
// Static Light - Scene light for baking (no runtime shadows)
// ============================================================================
struct StaticLight {
    Vec3 position = Vec3(0.0f);      // Light position (Point only)
    Vec3 direction = Vec3(0.0f, -1.0f, 0.0f);  // Light direction (Directional only)
    Vec3 color = Vec3(1.0f);         // Light color (RGB, 0-1)
    float intensity = 1.0f;          // Light intensity multiplier
    float radius = 10.0f;            // Attenuation radius (Point only)
    StaticLightType type = StaticLightType::Point;
    
    StaticLight() = default;
    
    // Convenience constructors
    static StaticLight CreatePoint(const Vec3& pos, const Vec3& col = Vec3(1.0f), 
                                   float intensity = 1.0f, float radius = 10.0f) {
        StaticLight l;
        l.position = pos;
        l.color = col;
        l.intensity = intensity;
        l.radius = radius;
        l.type = StaticLightType::Point;
        return l;
    }
    
    static StaticLight CreateDirectional(const Vec3& dir, const Vec3& col = Vec3(1.0f), 
                                          float intensity = 1.0f) {
        StaticLight l;
        l.direction = glm::normalize(dir);
        l.color = col;
        l.intensity = intensity;
        l.type = StaticLightType::Directional;
        return l;
    }
};

} // namespace Genesis

