#pragma once

// ============================================================================
// OracularV2 Block Tool
// Creates box brushes by click-drag in viewports
// ============================================================================

#include "math/Math.h"
#include "EditorBrush.h"

class Grid;
class Viewport;

// ============================================================================
// BlockTool - Primary brush creation tool
// ============================================================================
class BlockTool {
public:
    BlockTool() = default;
    
    // ========================================================================
    // Tool State
    // ========================================================================
    
    enum class State {
        Idle,           // Waiting for input
        DrawingBase,    // Click-dragging to set X/Z in 2D view
        DrawingHeight   // Adjusting Y height in 3D view
    };
    
    State GetState() const { return m_state; }
    bool IsActive() const { return m_state != State::Idle; }
    
    // ========================================================================
    // Input Handling
    // ========================================================================
    
    // Called when mouse button pressed in viewport
    void OnMouseDown(Viewport* viewport, float x, float y, Grid* grid);
    
    // Called while mouse is dragging
    void OnMouseDrag(Viewport* viewport, float x, float y, Grid* grid);
    
    // Called when mouse button released
    EditorBrush OnMouseUp(Viewport* viewport, float x, float y, Grid* grid);
    
    // Cancel current operation
    void Cancel();
    
    // ========================================================================
    // Preview
    // ========================================================================
    
    // Get current preview brush (for visualization)
    const EditorBrush& GetPreview() const { return m_preview; }
    bool HasPreview() const { return m_state != State::Idle; }
    
    // ========================================================================
    // Configuration
    // ========================================================================
    
    void SetDefaultMaterial(const std::string& material) { m_defaultMaterial = material; }
    const std::string& GetDefaultMaterial() const { return m_defaultMaterial; }

private:
    // Convert screen coordinates to world position on grid plane
    Genesis::Vec3 ScreenToGridPlane(Viewport* viewport, float x, float y, Grid* grid);
    
private:
    State m_state = State::Idle;
    
    // Brush being created
    EditorBrush m_preview;
    
    // Start position of drag
    Genesis::Vec3 m_startPos{0.0f};
    
    // Default height for new brushes
    float m_defaultHeight = 64.0f;
    
    // Default material
    std::string m_defaultMaterial = "default";
};
