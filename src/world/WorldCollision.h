#pragma once

#include "math/Math.h"
#include "player/PlayerController.h"
#include <vector>

namespace Genesis {

// ============================================================================
// World Box Tags - Identifies special geometry types
// ============================================================================
enum class BoxTag {
    Default,    // Normal collision box
    Stair,      // Auto-climb stair step
    Ramp,       // Smooth ramp (future use)
    Platform,   // Moving platform (future use)
    Trigger     // Non-solid trigger volume (future use)
};

// ============================================================================
// World Box - A collidable box in the world
// ============================================================================
struct WorldBox {
    Vec3 center;
    Vec3 halfExtents;
    bool isSolid = true;  // Can be walked on and collided with
    BoxTag tag = BoxTag::Default;

    WorldBox() = default;
    WorldBox(const Vec3& c, const Vec3& he, BoxTag t = BoxTag::Default)
        : center(c), halfExtents(he), isSolid(true), tag(t) {}

    AABB GetAABB() const {
        return AABB(center - halfExtents, center + halfExtents);
    }

    float GetTop() const { return center.y + halfExtents.y; }
    float GetBottom() const { return center.y - halfExtents.y; }
    bool IsStair() const { return tag == BoxTag::Stair; }
};

// ============================================================================
// Simple World Collision System
// ============================================================================
class WorldCollision {
public:
    static WorldCollision& Instance() {
        static WorldCollision instance;
        return instance;
    }

    // ========================================================================
    // Box Management
    // ========================================================================
    void Clear() { m_boxes.clear(); }

    void AddBox(const Vec3& center, const Vec3& halfExtents) {
        m_boxes.emplace_back(center, halfExtents);
    }

    // Add a box from position and size (like DrawCube uses)
    void AddCube(float x, float y, float z, float size) {
        float half = size * 0.5f;
        m_boxes.emplace_back(Vec3(x, y, z), Vec3(half, half, half));
    }

    // Add a box from center position and dimensions (width, height, depth)
    void AddBox(float x, float y, float z, float width, float height, float depth) {
        m_boxes.emplace_back(Vec3(x, y, z), Vec3(width * 0.5f, height * 0.5f, depth * 0.5f));
    }

    // Add a stair step (auto-climbable)
    void AddStair(float x, float y, float z, float width, float height, float depth) {
        m_boxes.emplace_back(Vec3(x, y, z), Vec3(width * 0.5f, height * 0.5f, depth * 0.5f), BoxTag::Stair);
    }

    // Add a cube as a stair step
    void AddStairCube(float x, float y, float z, float size) {
        float half = size * 0.5f;
        m_boxes.emplace_back(Vec3(x, y, z), Vec3(half, half, half), BoxTag::Stair);
    }

    const std::vector<WorldBox>& GetBoxes() const { return m_boxes; }

    // ========================================================================
    // Stair Detection
    // ========================================================================
    // Check if there's a stair in front of the player that can be auto-climbed
    // Returns the height to climb to, or -1 if no stair found
    float GetStairClimbHeight(float x, float z, float playerY, float playerRadius,
                               float maxStairHeight, const Vec3& moveDir) const {
        float bestStairTop = -1.0f;

        for (const auto& box : m_boxes) {
            if (!box.IsStair() || !box.isSolid) continue;

            AABB aabb = box.GetAABB();
            float boxTop = box.GetTop();
            float boxBottom = box.GetBottom();

            // Check height constraints:
            // - Stair top must be above player feet
            // - Stair must be climbable (not too high)
            float heightAbovePlayer = boxTop - playerY;
            if (heightAbovePlayer <= 0.0f || heightAbovePlayer > maxStairHeight) continue;

            // Check if player is approaching/touching this stair horizontally
            // Expand check area slightly in movement direction
            float checkRadius = playerRadius + 0.1f;
            float checkX = x + moveDir.x * checkRadius;
            float checkZ = z + moveDir.z * checkRadius;

            if (checkX >= aabb.min.x - checkRadius && checkX <= aabb.max.x + checkRadius &&
                checkZ >= aabb.min.z - checkRadius && checkZ <= aabb.max.z + checkRadius) {

                // This stair is reachable, take the highest valid one
                if (boxTop > bestStairTop) {
                    bestStairTop = boxTop;
                }
            }
        }

        return bestStairTop;
    }

    // ========================================================================
    // Ground Height Query
    // ========================================================================
    // Returns the highest ground point at the given XZ position that the player can stand on
    // A box counts as ground only if the player is above or very close to the box's top
    float GetGroundHeight(float x, float z, float radius = 0.3f, float playerY = 1000.0f) const {
        float highestGround = m_floorHeight;

        for (const auto& box : m_boxes) {
            if (!box.isSolid) continue;

            AABB aabb = box.GetAABB();

            // Check if player CENTER is within the box XZ bounds
            // Use a small inset to prevent standing on the very edge
            float inset = radius * 0.5f;  // Increased from 0.3f for more reliable edge handling

            if (x >= aabb.min.x - inset && x <= aabb.max.x + inset &&
                z >= aabb.min.z - inset && z <= aabb.max.z + inset) {

                float boxTop = box.GetTop();

                // Only consider this box as ground if:
                // 1. The player is above the box top (can land on it)
                // 2. The player is slightly below the box top (already standing on it or just landed)
                // Use stricter tolerance (0.3) to prevent teleporting onto boxes from the side
                if (playerY >= boxTop - 0.3f) {
                    if (boxTop > highestGround) {
                        highestGround = boxTop;
                    }
                }
            }
        }

        return highestGround;
    }

    // ========================================================================
    // Collision Query
    // ========================================================================
    // Check if an AABB collides with any world geometry
    // Uses a small "skin" to prevent getting stuck on edges
    // maxClimbHeight: stairs below this height are ignored (can be climbed)
    bool CheckCollision(const Vec3& position, const AABB& playerBounds, float maxClimbHeight = 0.5f) const {
        // Shrink the player bounds more aggressively to prevent edge catching
        const float skinXZ = 0.05f;  // More forgiving on horizontal
        const float skinY = 0.02f;   // Tighter on vertical
        AABB shrunkBounds(
            playerBounds.min + Vec3(skinXZ, skinY, skinXZ),
            playerBounds.max - Vec3(skinXZ, skinY, skinXZ)
        );

        float playerBottom = shrunkBounds.min.y;

        for (const auto& box : m_boxes) {
            if (!box.isSolid) continue;

            AABB boxAABB = box.GetAABB();
            if (shrunkBounds.Intersects(boxAABB)) {
                float boxTop = boxAABB.max.y;

                // Check if this is a climbable stair
                if (box.IsStair()) {
                    float heightAbovePlayer = boxTop - playerBottom;
                    // Skip this stair if it's within climbable range
                    if (heightAbovePlayer > 0.0f && heightAbovePlayer <= maxClimbHeight) {
                        continue;  // This stair can be climbed, don't block
                    }
                }

                // Check if player is on TOP of the box (or close to it)

                // Use generous tolerance - if player's bottom is near box top, they're standing on it
                // Increased tolerance to 0.6 to prevent edge sticking
                if (playerBottom < boxTop - 0.6f) {
                    return true;  // Collision with side
                }
            }
        }
        return false;
    }

    // ========================================================================
    // Swept Collision (Continuous Collision Detection)
    // ========================================================================
    // Returns the penetration depth and pushout direction if colliding
    // maxClimbHeight: stairs below this height are ignored (can be climbed)
    bool GetPenetration(const AABB& playerBounds, Vec3& pushOut, float maxClimbHeight = 0.5f) const {
        pushOut = Vec3(0.0f);
        float smallestPenetration = 1000.0f;
        bool anyCollision = false;

        float playerBottom = playerBounds.min.y;

        for (const auto& box : m_boxes) {
            if (!box.isSolid) continue;

            AABB boxAABB = box.GetAABB();

            // Check if there's overlap
            float overlapX = std::min(playerBounds.max.x - boxAABB.min.x, boxAABB.max.x - playerBounds.min.x);
            float overlapY = std::min(playerBounds.max.y - boxAABB.min.y, boxAABB.max.y - playerBounds.min.y);
            float overlapZ = std::min(playerBounds.max.z - boxAABB.min.z, boxAABB.max.z - playerBounds.min.z);

            if (overlapX > 0 && overlapY > 0 && overlapZ > 0) {
                float boxTop = boxAABB.max.y;

                // Skip climbable stairs
                if (box.IsStair()) {
                    float heightAbovePlayer = boxTop - playerBottom;
                    if (heightAbovePlayer > 0.0f && heightAbovePlayer <= maxClimbHeight) {
                        continue;  // This stair can be climbed, don't push out
                    }
                }

                // We have penetration
                anyCollision = true;

                // Check if standing on top - don't push horizontally if on top
                if (playerBottom >= boxTop - 0.1f) {
                    continue; // Standing on top, no horizontal push needed
                }

                // Find minimum penetration axis (for horizontal only)
                // Ignore Y axis for pushout to prevent weird vertical behavior
                float playerCenterX = (playerBounds.min.x + playerBounds.max.x) * 0.5f;
                float playerCenterZ = (playerBounds.min.z + playerBounds.max.z) * 0.5f;
                float boxCenterX = (boxAABB.min.x + boxAABB.max.x) * 0.5f;
                float boxCenterZ = (boxAABB.min.z + boxAABB.max.z) * 0.5f;

                // Determine push direction based on which axis has less overlap
                if (overlapX < overlapZ) {
                    float pushDist = overlapX + 0.01f; // Small extra push
                    if (playerCenterX < boxCenterX) {
                        if (pushDist < smallestPenetration) {
                            smallestPenetration = pushDist;
                            pushOut = Vec3(-pushDist, 0.0f, 0.0f);
                        }
                    } else {
                        if (pushDist < smallestPenetration) {
                            smallestPenetration = pushDist;
                            pushOut = Vec3(pushDist, 0.0f, 0.0f);
                        }
                    }
                } else {
                    float pushDist = overlapZ + 0.01f;
                    if (playerCenterZ < boxCenterZ) {
                        if (pushDist < smallestPenetration) {
                            smallestPenetration = pushDist;
                            pushOut = Vec3(0.0f, 0.0f, -pushDist);
                        }
                    } else {
                        if (pushDist < smallestPenetration) {
                            smallestPenetration = pushDist;
                            pushOut = Vec3(0.0f, 0.0f, pushDist);
                        }
                    }
                }
            }
        }

        return anyCollision;
    }

    // ========================================================================
    // Raycast Query (for ground detection)
    // ========================================================================
    // Cast a ray down from a point, return the Y position of the hit
    bool RaycastDown(const Vec3& origin, float maxDistance, float& hitY) const {
        hitY = m_floorHeight;
        bool hit = false;

        // Check floor first
        if (origin.y >= m_floorHeight && origin.y - maxDistance <= m_floorHeight) {
            hitY = m_floorHeight;
            hit = true;
        }

        // Check all boxes
        for (const auto& box : m_boxes) {
            if (!box.isSolid) continue;

            AABB aabb = box.GetAABB();

            // Is the ray within the XZ bounds of the box?
            if (origin.x >= aabb.min.x && origin.x <= aabb.max.x &&
                origin.z >= aabb.min.z && origin.z <= aabb.max.z) {

                float boxTop = aabb.max.y;

                // Is the box top below us and within range?
                if (boxTop <= origin.y && boxTop >= origin.y - maxDistance) {
                    if (boxTop > hitY) {
                        hitY = boxTop;
                        hit = true;
                    }
                }
            }
        }

        return hit;
    }

    // ========================================================================
    // Configuration
    // ========================================================================
    void SetFloorHeight(float height) { m_floorHeight = height; }
    float GetFloorHeight() const { return m_floorHeight; }

private:
    WorldCollision() = default;

    std::vector<WorldBox> m_boxes;
    float m_floorHeight = 0.0f;  // Base floor level
};

} // namespace Genesis

