// ============================================================================
// OracularV2 Block Tool - Implementation
// ============================================================================

#include "BlockTool.h"
#include "../Grid.h"
#include "../Viewport.h"

// ============================================================================
// Input Handling
// ============================================================================

void BlockTool::OnMouseDown(Viewport* viewport, float x, float y, Grid* grid) {
    if (!viewport || m_state != State::Idle) return;
    
    // Get world position on grid plane
    m_startPos = ScreenToGridPlane(viewport, x, y, grid);
    
    // Initialize preview brush
    m_preview = EditorBrush::CreateBox(m_startPos, m_startPos, m_defaultMaterial);
    
    m_state = State::DrawingBase;
}

void BlockTool::OnMouseDrag(Viewport* viewport, float x, float y, Grid* grid) {
    if (!viewport || m_state == State::Idle) return;
    
    Genesis::Vec3 currentPos = ScreenToGridPlane(viewport, x, y, grid);
    
    if (m_state == State::DrawingBase) {
        // Update X/Z extents (for Top view) or appropriate axes
        Genesis::Vec3 min = glm::min(m_startPos, currentPos);
        Genesis::Vec3 max = glm::max(m_startPos, currentPos);
        
        // Set default height
        max.y = min.y + m_defaultHeight;
        
        m_preview.SetBounds(min, max);
    } else if (m_state == State::DrawingHeight) {
        // Update Y height
        Genesis::Vec3 max = m_preview.Max();
        max.y = currentPos.y;
        if (max.y < m_preview.brush.position.y + 1.0f) {
            max.y = m_preview.brush.position.y + 1.0f;
        }
        m_preview.SetBounds(m_preview.brush.position, max);
    }
}

EditorBrush BlockTool::OnMouseUp(Viewport* viewport, float x, float y, Grid* grid) {
    if (m_state == State::Idle) {
        return EditorBrush();
    }
    
    // Finalize the brush
    OnMouseDrag(viewport, x, y, grid);
    
    // Validate minimum size
    Genesis::Vec3 size = m_preview.brush.size;
    if (std::abs(size.x) < 1.0f || std::abs(size.y) < 1.0f || std::abs(size.z) < 1.0f) {
        // Too small, cancel
        Cancel();
        return EditorBrush();
    }
    
    // Return the created brush
    EditorBrush result = m_preview;
    
    // Reset state
    m_state = State::Idle;
    m_preview = EditorBrush();
    
    return result;
}

void BlockTool::Cancel() {
    m_state = State::Idle;
    m_preview = EditorBrush();
}

// ============================================================================
// Coordinate Conversion
// ============================================================================

Genesis::Vec3 BlockTool::ScreenToGridPlane(Viewport* viewport, float x, float y, Grid* grid) {
    if (!viewport) return Genesis::Vec3(0.0f);
    
    Genesis::Vec3 worldPos;
    
    if (viewport->IsOrthographic()) {
        // For 2D views, convert directly
        worldPos = viewport->ScreenToWorld(x, y, 0.0f);
    } else {
        // For 3D view, cast ray to ground plane (Y=0)
        Genesis::Ray ray = viewport->ScreenToWorldRay(x, y);
        
        // Intersect with Y=0 plane
        if (std::abs(ray.direction.y) > 0.0001f) {
            float t = -ray.origin.y / ray.direction.y;
            if (t > 0) {
                worldPos = ray.origin + ray.direction * t;
            }
        }
    }
    
    // Snap to grid
    if (grid) {
        worldPos = grid->Snap(worldPos);
    }
    
    return worldPos;
}
