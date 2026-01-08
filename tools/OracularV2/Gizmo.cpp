// ============================================================================
// OracularV2 Gizmo System - Implementation
// ============================================================================

#include "Gizmo.h"
#include "Grid.h"
#include <glm/gtc/matrix_transform.hpp>

// ============================================================================
// Hit Testing
// ============================================================================

GizmoAxis Gizmo::HitTest(const Genesis::Ray& ray) const {
    if (m_mode == GizmoMode::None) return GizmoAxis::None;
    
    const float handleLength = m_size;
    const float handleRadius = m_size * 0.1f;
    
    // Test each axis
    auto testAxisHit = [&](GizmoAxis axis, const Genesis::Vec3& dir) -> bool {
        // Simple cylinder test along axis
        Genesis::Vec3 start = m_position;
        Genesis::Vec3 end = m_position + dir * handleLength;
        
        // Ray-cylinder intersection (simplified)
        Genesis::Vec3 d = ray.direction;
        Genesis::Vec3 m = ray.origin - start;
        Genesis::Vec3 n = end - start;
        
        float md = glm::dot(m, d);
        float nd = glm::dot(n, d);
        float nn = glm::dot(n, n);
        
        if (nn < 0.0001f) return false;
        
        float t = -md / nd;
        if (t < 0 || t > 1) return false;
        
        Genesis::Vec3 closest = ray.origin + d * (md + t * nd);
        float dist = glm::length(closest - (start + n * t));
        
        return dist < handleRadius;
    };
    
    if (testAxisHit(GizmoAxis::X, Genesis::Vec3(1, 0, 0))) return GizmoAxis::X;
    if (testAxisHit(GizmoAxis::Y, Genesis::Vec3(0, 1, 0))) return GizmoAxis::Y;
    if (testAxisHit(GizmoAxis::Z, Genesis::Vec3(0, 0, 1))) return GizmoAxis::Z;
    
    return GizmoAxis::None;
}

// ============================================================================
// Drag Operations
// ============================================================================

void Gizmo::BeginDrag(GizmoAxis axis, const Genesis::Ray& ray) {
    m_isDragging = true;
    m_activeAxis = axis;
    m_dragStart = ProjectRayOntoAxis(ray, axis);
    m_lastDragPos = m_dragStart;
}

Genesis::Vec3 Gizmo::UpdateDrag(const Genesis::Ray& ray, Grid* grid) {
    if (!m_isDragging) return Genesis::Vec3(0.0f);
    
    Genesis::Vec3 currentPos = ProjectRayOntoAxis(ray, m_activeAxis);
    Genesis::Vec3 delta = currentPos - m_lastDragPos;
    
    // Snap to grid
    if (grid) {
        delta = grid->Snap(delta);
    }
    
    m_lastDragPos = currentPos;
    
    return delta;
}

void Gizmo::EndDrag() {
    m_isDragging = false;
    m_activeAxis = GizmoAxis::None;
}

Genesis::Vec3 Gizmo::ProjectRayOntoAxis(const Genesis::Ray& ray, GizmoAxis axis) const {
    Genesis::Vec3 axisDir(0.0f);
    
    if (HasAxis(axis, GizmoAxis::X)) axisDir.x = 1.0f;
    if (HasAxis(axis, GizmoAxis::Y)) axisDir.y = 1.0f;
    if (HasAxis(axis, GizmoAxis::Z)) axisDir.z = 1.0f;
    
    axisDir = glm::normalize(axisDir);
    
    // Project ray onto axis line
    Genesis::Vec3 w = ray.origin - m_position;
    float b = glm::dot(ray.direction, axisDir);
    float d = glm::dot(axisDir, axisDir);
    
    if (std::abs(d) < 0.0001f) return m_position;
    
    float t = -glm::dot(w, axisDir) / d;
    
    return m_position + axisDir * t;
}

// ============================================================================
// Rendering
// ============================================================================

void Gizmo::Render() {
    // TODO: Use DebugRenderer to draw gizmo handles
    // X axis = Red, Y axis = Green, Z axis = Blue
    // Highlight active/hovered axis
}
