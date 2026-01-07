#pragma once

#include "Math.h"
#include <array>

namespace Genesis {

// ============================================================================
// Frustum - Represents the camera view frustum for culling
// ============================================================================
class Frustum {
public:
    Frustum() = default;

    // Update frustum planes from view-projection matrix
    void Update(const Mat4& viewProjection) {
        // Gribb-Hartmann extraction
        // Left
        m_planes[0].x = viewProjection[0][3] + viewProjection[0][0];
        m_planes[0].y = viewProjection[1][3] + viewProjection[1][0];
        m_planes[0].z = viewProjection[2][3] + viewProjection[2][0];
        m_planes[0].w = viewProjection[3][3] + viewProjection[3][0];

        // Right
        m_planes[1].x = viewProjection[0][3] - viewProjection[0][0];
        m_planes[1].y = viewProjection[1][3] - viewProjection[1][0];
        m_planes[1].z = viewProjection[2][3] - viewProjection[2][0];
        m_planes[1].w = viewProjection[3][3] - viewProjection[3][0];

        // Bottom
        m_planes[2].x = viewProjection[0][3] + viewProjection[0][1];
        m_planes[2].y = viewProjection[1][3] + viewProjection[1][1];
        m_planes[2].z = viewProjection[2][3] + viewProjection[2][1];
        m_planes[2].w = viewProjection[3][3] + viewProjection[3][1];

        // Top
        m_planes[3].x = viewProjection[0][3] - viewProjection[0][1];
        m_planes[3].y = viewProjection[1][3] - viewProjection[1][1];
        m_planes[3].z = viewProjection[2][3] - viewProjection[2][1];
        m_planes[3].w = viewProjection[3][3] - viewProjection[3][1];

        // Near
        m_planes[4].x = viewProjection[0][3] + viewProjection[0][2];
        m_planes[4].y = viewProjection[1][3] + viewProjection[1][2];
        m_planes[4].z = viewProjection[2][3] + viewProjection[2][2];
        m_planes[4].w = viewProjection[3][3] + viewProjection[3][2];

        // Far
        m_planes[5].x = viewProjection[0][3] - viewProjection[0][2];
        m_planes[5].y = viewProjection[1][3] - viewProjection[1][2];
        m_planes[5].z = viewProjection[2][3] - viewProjection[2][2];
        m_planes[5].w = viewProjection[3][3] - viewProjection[3][2];

        // Normalize planes
        for (auto& plane : m_planes) {
            float length = glm::length(Vec3(plane));
            plane /= length;
        }
    }

    // Check if an Axis-Aligned Bounding Box is visible
    bool IsBoxVisible(const Vec3& min, const Vec3& max) const {
        // For each plane
        for (const auto& plane : m_planes) {
            Vec3 p;
            
            // Find the point on the box closest to the plane normal direction
            // If this point is behind the plane, the whole box is behind
            p.x = (plane.x > 0) ? max.x : min.x;
            p.y = (plane.y > 0) ? max.y : min.y;
            p.z = (plane.z > 0) ? max.z : min.z;

            // Dot product + distance check
            if (glm::dot(Vec3(plane), p) + plane.w < 0.0f) {
                return false; // Outside
            }
        }
        return true;
    }

    // Check if a bounding sphere is visible (faster than box test)
    // Sphere test is a single dot product per plane vs multiple branches for box
    bool IsSphereVisible(const Vec3& center, float radius) const {
        for (const auto& plane : m_planes) {
            // Distance from center to plane
            float distance = glm::dot(Vec3(plane), center) + plane.w;
            if (distance < -radius) {
                return false;  // Sphere is completely behind this plane
            }
        }
        return true;
    }

private:
    std::array<Vec4, 6> m_planes; // Plane equation: ax + by + cz + d = 0 (stored as Vec4)
};

} // namespace Genesis
