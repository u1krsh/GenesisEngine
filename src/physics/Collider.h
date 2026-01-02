#pragma once

#include "math/Math.h"
#include <memory>
#include <vector>
#include <string>

namespace Genesis {

// ============================================================================
// Collider Types - Different collision shape types
// ============================================================================
enum class ColliderType {
    None,       // No collision (decorative only)
    Box,        // Axis-aligned bounding box
    Sphere,     // Sphere collider
    Capsule,    // Capsule collider (for characters)
    Mesh        // Mesh collider (expensive, for complex static geometry)
};

// ============================================================================
// CollisionLayer - For filtering collision detection
// ============================================================================
enum class CollisionLayer : uint32_t {
    Default     = 1 << 0,
    Static      = 1 << 1,   // Static world geometry
    Dynamic     = 1 << 2,   // Moving objects
    Player      = 1 << 3,   // Player character
    Trigger     = 1 << 4,   // Trigger volumes (no physical response)
    Projectile  = 1 << 5,   // Bullets, etc.
    Debris      = 1 << 6,   // Non-essential collision
    All         = 0xFFFFFFFF
};

inline CollisionLayer operator|(CollisionLayer a, CollisionLayer b) {
    return static_cast<CollisionLayer>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline CollisionLayer operator&(CollisionLayer a, CollisionLayer b) {
    return static_cast<CollisionLayer>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// ============================================================================
// Collider - Base class for collision shapes (decoupled from visual mesh)
// ============================================================================
class Collider {
public:
    virtual ~Collider() = default;

    virtual ColliderType GetType() const = 0;

    // Get world-space AABB for broad phase
    virtual AABB GetWorldAABB(const Mat4& transform) const = 0;

    // Check if point is inside collider
    virtual bool ContainsPoint(const Vec3& point, const Mat4& transform) const = 0;

    // Collision layer for filtering
    void SetLayer(CollisionLayer layer) { m_layer = layer; }
    CollisionLayer GetLayer() const { return m_layer; }

    // Collision mask (what layers this collides with)
    void SetMask(CollisionLayer mask) { m_mask = mask; }
    CollisionLayer GetMask() const { return m_mask; }

    // Check if should collide with another layer
    bool ShouldCollideWith(CollisionLayer otherLayer) const {
        return (static_cast<uint32_t>(m_mask) & static_cast<uint32_t>(otherLayer)) != 0;
    }

    // Is this a trigger (no physical response)?
    void SetTrigger(bool isTrigger) { m_isTrigger = isTrigger; }
    bool IsTrigger() const { return m_isTrigger; }

    // Is this a stair/step?
    void SetStair(bool isStair) { m_isStair = isStair; }
    bool IsStair() const { return m_isStair; }

protected:
    CollisionLayer m_layer = CollisionLayer::Static;
    CollisionLayer m_mask = CollisionLayer::All;
    bool m_isTrigger = false;
    bool m_isStair = false;
};

using ColliderPtr = std::shared_ptr<Collider>;

// ============================================================================
// BoxCollider - Axis-aligned box collision shape
// ============================================================================
class BoxCollider : public Collider {
public:
    BoxCollider() = default;
    BoxCollider(const Vec3& halfExtents) : m_halfExtents(halfExtents) {}
    BoxCollider(float hx, float hy, float hz) : m_halfExtents(hx, hy, hz) {}

    // Create from size (full extents, not half)
    static std::shared_ptr<BoxCollider> FromSize(const Vec3& size) {
        return std::make_shared<BoxCollider>(size * 0.5f);
    }

    static std::shared_ptr<BoxCollider> FromSize(float sx, float sy, float sz) {
        return std::make_shared<BoxCollider>(sx * 0.5f, sy * 0.5f, sz * 0.5f);
    }

    ColliderType GetType() const override { return ColliderType::Box; }

    AABB GetWorldAABB(const Mat4& transform) const override {
        // Get the 8 corners of the box
        Vec3 corners[8] = {
            Vec3(-m_halfExtents.x, -m_halfExtents.y, -m_halfExtents.z),
            Vec3( m_halfExtents.x, -m_halfExtents.y, -m_halfExtents.z),
            Vec3(-m_halfExtents.x,  m_halfExtents.y, -m_halfExtents.z),
            Vec3( m_halfExtents.x,  m_halfExtents.y, -m_halfExtents.z),
            Vec3(-m_halfExtents.x, -m_halfExtents.y,  m_halfExtents.z),
            Vec3( m_halfExtents.x, -m_halfExtents.y,  m_halfExtents.z),
            Vec3(-m_halfExtents.x,  m_halfExtents.y,  m_halfExtents.z),
            Vec3( m_halfExtents.x,  m_halfExtents.y,  m_halfExtents.z)
        };

        AABB result;
        result.min = Vec3(std::numeric_limits<float>::max());
        result.max = Vec3(std::numeric_limits<float>::lowest());

        for (const auto& corner : corners) {
            Vec4 transformed = transform * Vec4(corner, 1.0f);
            Vec3 worldCorner(transformed.x, transformed.y, transformed.z);
            result.min = glm::min(result.min, worldCorner);
            result.max = glm::max(result.max, worldCorner);
        }

        return result;
    }

    bool ContainsPoint(const Vec3& point, const Mat4& transform) const override {
        // Transform point to local space
        Mat4 invTransform = glm::inverse(transform);
        Vec4 localPoint4 = invTransform * Vec4(point, 1.0f);
        Vec3 localPoint(localPoint4.x, localPoint4.y, localPoint4.z);

        return std::abs(localPoint.x) <= m_halfExtents.x &&
               std::abs(localPoint.y) <= m_halfExtents.y &&
               std::abs(localPoint.z) <= m_halfExtents.z;
    }

    const Vec3& GetHalfExtents() const { return m_halfExtents; }
    void SetHalfExtents(const Vec3& halfExtents) { m_halfExtents = halfExtents; }

    Vec3 GetSize() const { return m_halfExtents * 2.0f; }

private:
    Vec3 m_halfExtents = Vec3(0.5f);  // Default 1x1x1 box
};

// ============================================================================
// SphereCollider - Sphere collision shape
// ============================================================================
class SphereCollider : public Collider {
public:
    SphereCollider() = default;
    SphereCollider(float radius) : m_radius(radius) {}

    ColliderType GetType() const override { return ColliderType::Sphere; }

    AABB GetWorldAABB(const Mat4& transform) const override {
        // Extract position from transform
        Vec3 center(transform[3]);

        // For a sphere, the AABB is simply center +/- radius
        // (ignoring scale for now - could extract scale from transform)
        return AABB{
            center - Vec3(m_radius),
            center + Vec3(m_radius)
        };
    }

    bool ContainsPoint(const Vec3& point, const Mat4& transform) const override {
        Vec3 center(transform[3]);
        float distSq = glm::length2(point - center);
        return distSq <= m_radius * m_radius;
    }

    float GetRadius() const { return m_radius; }
    void SetRadius(float radius) { m_radius = radius; }

private:
    float m_radius = 0.5f;
};

// ============================================================================
// Collision Helper Functions
// ============================================================================
namespace CollisionUtils {
    // Create a box collider that matches a unit cube scaled by transform
    inline ColliderPtr CreateBoxFromTransform(const Mat4& transform) {
        // Extract scale from transform matrix
        Vec3 scale(
            glm::length(Vec3(transform[0])),
            glm::length(Vec3(transform[1])),
            glm::length(Vec3(transform[2]))
        );
        return std::make_shared<BoxCollider>(scale * 0.5f);
    }
}

} // namespace Genesis

