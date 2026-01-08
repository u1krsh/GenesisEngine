#pragma once

// ============================================================================
// OracularV2 Editor Brush
// Wrapper around engine Brush with editor-specific state
// ============================================================================

#include "map/Brush.h"
#include <array>
#include <string>

// ============================================================================
// EditorBrush - Editor wrapper for engine brushes
// ============================================================================
struct EditorBrush {
    // The actual engine brush data
    Genesis::Brush brush;
    
    // Editor-only state (not saved to file)
    bool isSelected = false;
    bool isHovered = false;
    bool isVisible = true;
    bool isLocked = false;
    
    // Per-face materials (for future CSG)
    std::array<std::string, 6> faceMaterials = {
        "default", "default", "default",
        "default", "default", "default"
    };
    
    // Unique editor ID (different from brush.id for undo/redo)
    uint32_t editorId = 0;
    
    // Group/layer assignment
    std::string editorLayer = "default";
    
    // ========================================================================
    // Convenience accessors
    // ========================================================================
    
    Genesis::Vec3& Min() { 
        return brush.position; 
    }
    
    Genesis::Vec3 Max() const { 
        return brush.position + brush.size; 
    }
    
    void SetBounds(const Genesis::Vec3& min, const Genesis::Vec3& max) {
        brush.position = min;
        brush.size = max - min;
    }
    
    Genesis::Vec3 GetCenter() const {
        return brush.position + brush.size * 0.5f;
    }
    
    Genesis::AABB GetAABB() const {
        return Genesis::AABB(brush.position, brush.position + brush.size);
    }
    
    // ========================================================================
    // Creation helpers
    // ========================================================================
    
    static EditorBrush CreateBox(const Genesis::Vec3& min, 
                                  const Genesis::Vec3& max,
                                  const std::string& material = "default") {
        EditorBrush eb;
        eb.brush.shape = Genesis::BrushShape::Cube;
        eb.brush.position = min;
        eb.brush.size = max - min;
        eb.brush.materialName = material;
        eb.brush.flags = Genesis::BrushFlags::CastShadow | 
                         Genesis::BrushFlags::ReceiveShadow;
        return eb;
    }
};

using EditorBrushPtr = std::shared_ptr<EditorBrush>;
