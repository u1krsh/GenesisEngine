#include "BSPCollision.h"
#include "BSPTree.h"
#include <algorithm>
#include <cmath>

namespace Genesis {

// ============================================================================
// BSPCollision - Clear
// ============================================================================
void BSPCollision::Clear() {
    m_brushes.clear();
    m_planes.clear();
}

// ============================================================================
// BSPCollision - Point Queries
// ============================================================================
bool BSPCollision::IsPointSolid(const Vec3& point) const {
    return GetPointContents(point) == BSPContents::Solid;
}

BSPContents BSPCollision::GetPointContents(const Vec3& point) const {
    // Test against all brushes (could be optimized with BSP tree traversal)
    for (const auto& brush : m_brushes) {
        // Broad phase - AABB check
        if (!brush.ContainsPointAABB(point)) {
            continue;
        }
        
        // Narrow phase - check against all planes
        bool inside = true;
        for (uint32_t i = 0; i < brush.numPlanes && inside; ++i) {
            const BSPCollisionPlane& plane = m_planes[brush.firstPlane + i];
            // If point is in front of any plane, it's outside the brush
            if (plane.ClassifyPoint(point) > COLLISION_EPSILON) {
                inside = false;
            }
        }
        
        if (inside) {
            return brush.contents;
        }
    }
    
    return BSPContents::Empty;
}

// ============================================================================
// BSPCollision - Trace To Brush
// ============================================================================
void BSPCollision::TraceToBrush(const BSPCollisionBrush& brush, const Vec3& start,
                                 const Vec3& end, float radius, TraceResult& result) const {
    if (brush.numPlanes == 0) {
        return;
    }
    
    float enterFrac = -1.0f;  // Fraction where we enter the brush
    float leaveFrac = 1.0f;   // Fraction where we leave the brush
    Vec3 hitNormal(0.0f);
    bool getOut = false;
    bool startOut = false;
    
    for (uint32_t i = 0; i < brush.numPlanes; ++i) {
        const BSPCollisionPlane& plane = m_planes[brush.firstPlane + i];
        
        // Expand plane by radius for capsule collision
        float dist1 = plane.ClassifyPointExpanded(start, radius);
        float dist2 = plane.ClassifyPointExpanded(end, radius);
        
        // If start is in front of any plane, we started outside
        if (dist1 > 0.0f) {
            startOut = true;
        }
        
        // If end is in front, we got outside at some point
        if (dist2 > 0.0f) {
            getOut = true;
        }
        
        // Both points in front of plane - we never hit this brush
        if (dist1 > 0.0f && dist2 >= dist1) {
            return;
        }
        
        // Both points behind plane - this plane doesn't clip the trace
        if (dist1 <= 0.0f && dist2 <= 0.0f) {
            continue;
        }
        
        // Crossing the plane
        if (dist1 > dist2) {
            // Entering the brush
            float frac = (dist1 - DIST_EPSILON) / (dist1 - dist2);
            if (frac > enterFrac) {
                enterFrac = frac;
                hitNormal = plane.normal;
            }
        } else {
            // Leaving the brush
            float frac = (dist1 + DIST_EPSILON) / (dist1 - dist2);
            if (frac < leaveFrac) {
                leaveFrac = frac;
            }
        }
    }
    
    // Started inside the brush
    if (!startOut) {
        result.startSolid = true;
        if (!getOut) {
            result.allSolid = true;
            result.fraction = 0.0f;
        }
        return;
    }
    
    // Check if we actually hit the brush
    if (enterFrac < leaveFrac) {
        if (enterFrac > -1.0f && enterFrac < result.fraction) {
            // Clamp to [0, 1]
            enterFrac = std::max(0.0f, enterFrac);
            
            result.fraction = enterFrac;
            result.hitNormal = hitNormal;
            result.contents = brush.contents;
        }
    }
}

// ============================================================================
// BSPCollision - Trace Capsule
// ============================================================================
TraceResult BSPCollision::TraceCapsule(const Vec3& start, const Vec3& end,
                                        const CollisionCapsule& capsule) const {
    TraceResult result;
    result.endPos = end;
    result.fraction = 1.0f;
    
    // For a capsule, we approximate by doing a sphere trace at the center
    // The capsule is treated as a sphere with the combined radius for horizontal
    // and we do separate checks for vertical
    
    // Use the largest dimension for the trace radius
    float traceRadius = capsule.radius;
    
    // Calculate the AABB of the trace for broad phase
    Vec3 traceMin = glm::min(start, end) - Vec3(traceRadius + capsule.halfHeight);
    Vec3 traceMax = glm::max(start, end) + Vec3(traceRadius + capsule.halfHeight);
    
    // Test against all brushes
    for (const auto& brush : m_brushes) {
        // Broad phase - check if brush AABB intersects trace AABB
        if (brush.boundsMax.x < traceMin.x || brush.boundsMin.x > traceMax.x ||
            brush.boundsMax.y < traceMin.y || brush.boundsMin.y > traceMax.y ||
            brush.boundsMax.z < traceMin.z || brush.boundsMin.z > traceMax.z) {
            continue;
        }
        
        // For proper capsule collision, we trace the center point with an expanded radius
        // This accounts for both the capsule radius and half height
        TraceToBrush(brush, start, end, traceRadius, result);
    }
    
    // Calculate end position
    if (result.fraction < 1.0f) {
        result.endPos = start + (end - start) * result.fraction;
    }
    
    return result;
}

// ============================================================================
// BSPCollision - Trace Point
// ============================================================================
TraceResult BSPCollision::TracePoint(const Vec3& start, const Vec3& end) const {
    TraceResult result;
    result.endPos = end;
    result.fraction = 1.0f;
    
    for (const auto& brush : m_brushes) {
        // Broad phase
        Vec3 traceMin = glm::min(start, end);
        Vec3 traceMax = glm::max(start, end);
        
        if (brush.boundsMax.x < traceMin.x || brush.boundsMin.x > traceMax.x ||
            brush.boundsMax.y < traceMin.y || brush.boundsMin.y > traceMax.y ||
            brush.boundsMax.z < traceMin.z || brush.boundsMin.z > traceMax.z) {
            continue;
        }
        
        TraceToBrush(brush, start, end, 0.0f, result);
    }
    
    if (result.fraction < 1.0f) {
        result.endPos = start + (end - start) * result.fraction;
    }
    
    return result;
}

// ============================================================================
// BSPCollision - Trace Sphere
// ============================================================================
TraceResult BSPCollision::TraceSphere(const Vec3& start, const Vec3& end, float radius) const {
    TraceResult result;
    result.endPos = end;
    result.fraction = 1.0f;
    
    Vec3 traceMin = glm::min(start, end) - Vec3(radius);
    Vec3 traceMax = glm::max(start, end) + Vec3(radius);
    
    for (const auto& brush : m_brushes) {
        if (brush.boundsMax.x < traceMin.x || brush.boundsMin.x > traceMax.x ||
            brush.boundsMax.y < traceMin.y || brush.boundsMin.y > traceMax.y ||
            brush.boundsMax.z < traceMin.z || brush.boundsMin.z > traceMax.z) {
            continue;
        }
        
        TraceToBrush(brush, start, end, radius, result);
    }
    
    if (result.fraction < 1.0f) {
        result.endPos = start + (end - start) * result.fraction;
    }
    
    return result;
}

// ============================================================================
// BSPCollision - Clip Velocity
// ============================================================================
Vec3 BSPCollision::ClipVelocity(const Vec3& velocity, const Vec3& normal) const {
    // Remove the component of velocity that goes into the plane
    float backoff = glm::dot(velocity, normal) * OVERCLIP;
    
    Vec3 result = velocity - normal * backoff;
    
    // Avoid tiny velocities
    const float minVelocity = 0.001f;
    if (std::abs(result.x) < minVelocity) result.x = 0.0f;
    if (std::abs(result.y) < minVelocity) result.y = 0.0f;
    if (std::abs(result.z) < minVelocity) result.z = 0.0f;
    
    return result;
}

// ============================================================================
// BSPCollision - Slide Move (Quake-style)
// ============================================================================
Vec3 BSPCollision::SlideMove(const Vec3& start, const Vec3& velocity, float deltaTime,
                              const CollisionCapsule& capsule, Vec3& outVelocity) const {
    Vec3 position = start;
    Vec3 vel = velocity;
    float timeLeft = deltaTime;
    
    Vec3 planes[MAX_CLIP_PLANES];
    int numPlanes = 0;
    
    // Store original velocity for clamping later
    Vec3 originalVelocity = velocity;
    
    for (int bump = 0; bump < MAX_SLIDE_BUMPS; ++bump) {
        if (glm::length2(vel) < 0.0001f) {
            break;  // Velocity too small
        }
        
        // Calculate desired end position
        Vec3 end = position + vel * timeLeft;
        
        // Trace from current position to desired end
        TraceResult trace = TraceCapsule(position, end, capsule);
        
        // If we started in solid, try to nudge out
        if (trace.allSolid || trace.startSolid) {
            // We're stuck - try to unstick by moving slightly
            bool unstuck = false;
            const float nudgeDistance = 0.1f;
            const Vec3 nudgeDirections[] = {
                Vec3(1, 0, 0), Vec3(-1, 0, 0),
                Vec3(0, 1, 0), Vec3(0, -1, 0),
                Vec3(0, 0, 1), Vec3(0, 0, -1)
            };
            
            for (const auto& dir : nudgeDirections) {
                Vec3 testPos = position + dir * nudgeDistance;
                TraceResult testTrace = TraceCapsule(testPos, testPos, capsule);
                if (!testTrace.startSolid) {
                    position = testPos;
                    unstuck = true;
                    break;
                }
            }
            
            if (!unstuck) {
                // Can't unstick, just stop
                outVelocity = Vec3(0.0f);
                return position;
            }
            continue;
        }
        
        // If we moved some distance, update position
        if (trace.fraction > 0.0f) {
            position = trace.endPos;
        }
        
        // If we made it all the way, we're done
        if (trace.fraction == 1.0f) {
            break;
        }
        
        // We hit something - reduce time left proportionally
        timeLeft *= (1.0f - trace.fraction);
        
        // Check for duplicate planes
        bool duplicatePlane = false;
        for (int i = 0; i < numPlanes; ++i) {
            if (glm::dot(trace.hitNormal, planes[i]) > 0.99f) {
                // Same plane - nudge velocity away
                vel += trace.hitNormal * 0.01f;
                duplicatePlane = true;
                break;
            }
        }
        
        if (duplicatePlane) {
            continue;
        }
        
        // Store this plane
        if (numPlanes < MAX_CLIP_PLANES) {
            planes[numPlanes++] = trace.hitNormal;
        }
        
        // Clip velocity to all planes we've hit
        bool clipped = false;
        for (int i = 0; i < numPlanes; ++i) {
            Vec3 clippedVel = ClipVelocity(vel, planes[i]);
            
            // Check if this velocity moves us into any other plane
            bool movesIntoPlane = false;
            for (int j = 0; j < numPlanes; ++j) {
                if (j != i && glm::dot(clippedVel, planes[j]) < 0.0f) {
                    movesIntoPlane = true;
                    break;
                }
            }
            
            if (!movesIntoPlane) {
                vel = clippedVel;
                clipped = true;
                break;
            }
        }
        
        // If we couldn't clip to any single plane, try sliding along crease
        if (!clipped && numPlanes >= 2) {
            Vec3 dir = glm::cross(planes[0], planes[1]);
            float d = glm::length(dir);
            if (d > 0.001f) {
                dir /= d;
                float speed = glm::dot(vel, dir);
                vel = dir * speed;
            } else {
                // Planes are nearly parallel - just stop
                vel = Vec3(0.0f);
            }
        }
    }
    
    outVelocity = vel;
    return position;
}

// ============================================================================
// BSPCollision - Ground Trace
// ============================================================================
TraceResult BSPCollision::GroundTrace(const Vec3& position, const CollisionCapsule& capsule,
                                       float maxDistance) const {
    Vec3 end = position - Vec3(0.0f, maxDistance, 0.0f);
    return TraceCapsule(position, end, capsule);
}

// ============================================================================
// BSPCollision - Try Step Up
// ============================================================================
bool BSPCollision::TryStepUp(const Vec3& position, const Vec3& velocity, float deltaTime,
                              const CollisionCapsule& capsule, float stepHeight,
                              Vec3& outPosition) const {
    // First, check if we're blocked horizontally
    Vec3 horizontalVel(velocity.x, 0.0f, velocity.z);
    if (glm::length2(horizontalVel) < 0.0001f) {
        return false;  // Not moving horizontally
    }
    
    Vec3 horizontalEnd = position + horizontalVel * deltaTime;
    TraceResult horizontalTrace = TraceCapsule(position, horizontalEnd, capsule);
    
    // If we're not blocked, no need to step up
    if (horizontalTrace.fraction == 1.0f) {
        return false;
    }
    
    // Try stepping up
    Vec3 stepUpPos = position + Vec3(0.0f, stepHeight, 0.0f);
    TraceResult stepUpTrace = TraceCapsule(position, stepUpPos, capsule);
    
    if (stepUpTrace.fraction < 1.0f || stepUpTrace.startSolid) {
        return false;  // Can't step up (ceiling or obstacle)
    }
    
    // Now try moving horizontally from the stepped-up position
    Vec3 stepTopPos = stepUpTrace.endPos;
    Vec3 stepMoveEnd = stepTopPos + horizontalVel * deltaTime;
    TraceResult stepMoveTrace = TraceCapsule(stepTopPos, stepMoveEnd, capsule);
    
    if (stepMoveTrace.fraction < horizontalTrace.fraction) {
        return false;  // Stepping up didn't help
    }
    
    // Now step back down to find the ground
    Vec3 stepDownStart = stepMoveTrace.endPos;
    Vec3 stepDownEnd = stepDownStart - Vec3(0.0f, stepHeight + 0.1f, 0.0f);
    TraceResult stepDownTrace = TraceCapsule(stepDownStart, stepDownEnd, capsule);
    
    if (stepDownTrace.fraction == 1.0f) {
        return false;  // No ground found after stepping
    }
    
    // Check if we landed on a walkable surface (not a steep slope)
    if (stepDownTrace.hitNormal.y < 0.7f) {
        return false;  // Too steep to stand on
    }
    
    outPosition = stepDownTrace.endPos;
    return true;
}

} // namespace Genesis
