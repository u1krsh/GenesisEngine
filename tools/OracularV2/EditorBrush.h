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
    // NOTE: brush.position is the CENTER of the brush
    //       brush.size is the FULL dimensions
    // ========================================================================
    
    Genesis::Vec3 Min() const { 
        return brush.position - brush.size * 0.5f; 
    }
    
    Genesis::Vec3 Max() const { 
        return brush.position + brush.size * 0.5f; 
    }
    
    void SetBounds(const Genesis::Vec3& min, const Genesis::Vec3& max) {
        brush.size = max - min;
        brush.position = min + brush.size * 0.5f;  // Center position
    }
    
    Genesis::Vec3 GetCenter() const {
        return brush.position;  // Position IS the center
    }
    
    Genesis::AABB GetAABB() const {
        Genesis::Vec3 halfSize = brush.size * 0.5f;
        return Genesis::AABB(brush.position - halfSize, brush.position + halfSize);
    }
    
    // ========================================================================
    // Creation helpers
    // ========================================================================
    
    static EditorBrush CreateBox(const Genesis::Vec3& min, 
                                  const Genesis::Vec3& max,
                                  const std::string& material = "default") {
        EditorBrush eb;
        eb.brush.shape = Genesis::BrushShape::Cube;
        eb.brush.size = max - min;
        eb.brush.position = min + eb.brush.size * 0.5f;  // Center position
        eb.brush.materialName = material;
        eb.brush.flags = Genesis::BrushFlags::CastShadow | 
                         Genesis::BrushFlags::ReceiveShadow;
        return eb;
    }
};

using EditorBrushPtr = std::shared_ptr<EditorBrush>;
