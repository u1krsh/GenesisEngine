#pragma once

#include "math/Math.h"
#include "physics/Collider.h"
#include "renderer/mesh/Mesh.h"
#include "renderer/material/Material.h"
#include <string>
#include <memory>

namespace Genesis {

// ============================================================================
// Brush Shape Type - The geometric primitive
// ============================================================================
enum class BrushShape {
    Cube,       // Box/cube primitive
    Sphere,     // Sphere primitive
    Cylinder,   // Cylinder primitive
    Cone,       // Cone primitive
    Wedge,      // Wedge/ramp shape (future)
    Custom      // Custom mesh (future - for BSP)
};

// ============================================================================
// Shader Type - Different rendering modes for brushes
// Moved to Material.h
// ============================================================================

inline const char* BrushShapeToString(BrushShape shape) {
    switch (shape) {
        case BrushShape::Cube:     return "cube";
        case BrushShape::Sphere:   return "sphere";
        case BrushShape::Cylinder: return "cylinder";
        case BrushShape::Cone:     return "cone";
        case BrushShape::Wedge:    return "wedge";
        case BrushShape::Custom:   return "custom";
        default:                   return "unknown";
    }
}

inline BrushShape StringToBrushShape(const std::string& str) {
    if (str == "cube" || str == "box") return BrushShape::Cube;
    if (str == "sphere") return BrushShape::Sphere;
    if (str == "cylinder") return BrushShape::Cylinder;
    if (str == "cone") return BrushShape::Cone;
    if (str == "wedge" || str == "ramp") return BrushShape::Wedge;
    if (str == "custom") return BrushShape::Custom;
    return BrushShape::Cube; // Default
}

// ============================================================================
// Brush Flags - Special properties for brushes
// ============================================================================
enum class BrushFlags : uint32_t {
    None          = 0,
    NoCollision   = 1 << 0,   // No collision (decorative only)
    Stair         = 1 << 1,   // Auto-climbable stair
    Trigger       = 1 << 2,   // Trigger volume
    NoRender      = 1 << 3,   // Invisible (collision only)
    CastShadow    = 1 << 4,   // Casts shadows
    ReceiveShadow = 1 << 5,   // Receives shadows
    Detail        = 1 << 6,   // Detail brush (for future vis/BSP)
};

inline BrushFlags operator|(BrushFlags a, BrushFlags b) {
    return static_cast<BrushFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline BrushFlags operator&(BrushFlags a, BrushFlags b) {
    return static_cast<BrushFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline bool HasFlag(BrushFlags flags, BrushFlags flag) {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// ============================================================================
// Brush - The fundamental map building block
//
// Like Source Engine brushes:
// - Shape defines the geometric primitive
// - Material defines the surface appearance
// - Collider defines the physics shape
// - Later: will be used for BSP, lightmaps, visibility
// ============================================================================
struct Brush {
    // Identification
    std::string name;
    uint32_t id = 0;

    // Geometry definition
    BrushShape shape = BrushShape::Cube;
    Vec3 position = Vec3(0.0f);      // Center position
    Vec3 size = Vec3(1.0f);          // Full size (not half-extents)
    Vec3 rotation = Vec3(0.0f);      // Euler angles in degrees

    // Material name (resolved at load time)
    std::string materialName = "default";
    
    // Shader type preference
    ShaderType shaderType = ShaderType::Standard;
    
    // Explicit texture paths
    std::string normalMapPath;
    std::string maskMapPath;
    
    // Material parameters (embedded for now)
    float transparency = 0.3f;
    float fresnelPower = 2.0f;
    float roughness = 0.1f;
    float metallic = 0.0f;
    Vec3 tintColor = Vec3(1.0f);
    bool tileTexture = false;  // If true, texture tiles; if false, texture stretches
    
    // Glass Real properties
    float ior = 1.5f;          // Index of refraction (1.5 for standard glass)
    float thickness = 0.1f;     // Simulated glass thickness for absorption
    float absorption = 1.0f;    // Light absorption strength

    // Flags
    BrushFlags flags = BrushFlags::CastShadow | BrushFlags::ReceiveShadow;

    // Layer/group for organization
    std::string layer = "default";
    uint32_t visGroup = 0;

    // ========================================================================
    // Runtime data (populated by MapLoader)
    // ========================================================================
    MeshPtr mesh = nullptr;
    MaterialPtr material = nullptr;
    ColliderPtr collider = nullptr;
    Mat4 transform = Mat4(1.0f);

    // ========================================================================
    // Helpers
    // ========================================================================

    // Build transform matrix from position, size, rotation
    void BuildTransform() {
        transform = Mat4(1.0f);
        transform = glm::translate(transform, position);
        transform = glm::rotate(transform, glm::radians(rotation.x), Vec3(1, 0, 0));
        transform = glm::rotate(transform, glm::radians(rotation.y), Vec3(0, 1, 0));
        transform = glm::rotate(transform, glm::radians(rotation.z), Vec3(0, 0, 1));
        transform = glm::scale(transform, size);
    }

    // Get world-space AABB
    AABB GetWorldAABB() const {
        if (collider) {
            return collider->GetWorldAABB(transform);
        }
        // Fallback: compute from position and size
        Vec3 halfSize = size * 0.5f;
        return AABB(position - halfSize, position + halfSize);
    }

    // Check flags
    bool HasCollision() const { return !HasFlag(flags, BrushFlags::NoCollision); }
    bool IsStair() const { return HasFlag(flags, BrushFlags::Stair); }
    bool IsTrigger() const { return HasFlag(flags, BrushFlags::Trigger); }
    bool IsVisible() const { return !HasFlag(flags, BrushFlags::NoRender); }
    bool IsDetail() const { return HasFlag(flags, BrushFlags::Detail); }
};

using BrushPtr = std::shared_ptr<Brush>;

} // namespace Genesis

