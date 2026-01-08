#pragma once

// ============================================================================
// OracularV2 Gizmo System
// Move and Scale gizmos for brush manipulation
// ============================================================================

#include "math/Math.h"

class Grid;
struct EditorBrush;

// ============================================================================
// Gizmo Mode
// ============================================================================
enum class GizmoMode {
    None,
    Translate,
    Scale
};

// ============================================================================
// Gizmo Axis
// ============================================================================
enum class GizmoAxis {
    None = 0,
    X = 1,
    Y = 2,
    Z = 4,
    XY = X | Y,
    XZ = X | Z,
    YZ = Y | Z,
    XYZ = X | Y | Z
};

inline GizmoAxis operator|(GizmoAxis a, GizmoAxis b) {
    return static_cast<GizmoAxis>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool HasAxis(GizmoAxis mask, GizmoAxis axis) {
    return (static_cast<int>(mask) & static_cast<int>(axis)) != 0;
}

// ============================================================================
// Gizmo - Transform gizmo for selected brushes
// ============================================================================
class Gizmo {
public:
    Gizmo() = default;
    
    // ========================================================================
    // Mode
    // ========================================================================
    
    void SetMode(GizmoMode mode) { m_mode = mode; }
    GizmoMode GetMode() const { return m_mode; }
    
    // ========================================================================
    // Positioning
    // ========================================================================
    
    void SetPosition(const Genesis::Vec3& pos) { m_position = pos; }
    Genesis::Vec3 GetPosition() const { return m_position; }
    
    void SetSize(float size) { m_size = size; }
    float GetSize() const { return m_size; }
    
    // ========================================================================
    // Interaction
    // ========================================================================
    
    // Check if a ray hits any gizmo handle
    GizmoAxis HitTest(const Genesis::Ray& ray) const;
    
    // Begin a drag operation
    void BeginDrag(GizmoAxis axis, const Genesis::Ray& ray);
    
    // Update drag (returns delta movement, snapped to grid)
    Genesis::Vec3 UpdateDrag(const Genesis::Ray& ray, Grid* grid);
    
    // End drag
    void EndDrag();
    
    // Is currently being dragged?
    bool IsDragging() const { return m_isDragging; }
    GizmoAxis GetActiveAxis() const { return m_activeAxis; }
    
    // ========================================================================
    // Rendering
    // ========================================================================
    
    void Render();

private:
    // Ray-axis intersection for translation
    Genesis::Vec3 ProjectRayOntoAxis(const Genesis::Ray& ray, GizmoAxis axis) const;
    
private:
    GizmoMode m_mode = GizmoMode::Translate;
    Genesis::Vec3 m_position{0.0f};
    float m_size = 5.0f;
    
    // Drag state
    bool m_isDragging = false;
    GizmoAxis m_activeAxis = GizmoAxis::None;
    Genesis::Vec3 m_dragStart{0.0f};
    Genesis::Vec3 m_lastDragPos{0.0f};
    
    // Hover state
    GizmoAxis m_hoveredAxis = GizmoAxis::None;
};
