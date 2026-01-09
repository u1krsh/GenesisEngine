#pragma once

// ============================================================================
// OracularV2 Viewport Layout
// Manages the quad-split viewport arrangement
// ============================================================================

#include "Viewport.h"
#include "EditorApp.h"
#include "EditorEntity.h"
#include <array>
#include <memory>

class Grid;
class SelectionManager;
class Gizmo;
class BlockTool;
struct EditorBrush;

namespace Genesis {
    class Map;
}

// ============================================================================
// ViewportLayout - Manages multiple viewports in a grid
// ============================================================================
class ViewportLayout {
public:
    ViewportLayout();
    ~ViewportLayout();

    // Initialize all viewports
    void Initialize(int totalWidth, int totalHeight);
    
    // Render all viewports to their FBOs and ImGui
    void Render(Grid* grid, Genesis::Map* map,
                std::vector<EditorBrush>* brushes,
                std::vector<EditorEntity>* entities,
                SelectionManager* selection,
                Gizmo* gizmo,
                BlockTool* blockTool,
                EditorTool currentTool,
                EntityPaletteType entityType);
    
    // Get viewport at screen position
    Viewport* GetViewportAt(float x, float y);
    
    // Get specific viewport
    Viewport* GetPerspective() { return m_viewports[0].get(); }
    Viewport* GetTop() { return m_viewports[1].get(); }
    Viewport* GetFront() { return m_viewports[2].get(); }
    Viewport* GetSide() { return m_viewports[3].get(); }

private:
    // Handle input for a viewport
    void HandleViewportInput(Viewport* viewport, int viewportIndex,
                            Grid* grid,
                            std::vector<EditorBrush>* brushes,
                            std::vector<EditorEntity>* entities,
                            SelectionManager* selection,
                            Gizmo* gizmo,
                            BlockTool* blockTool,
                            EditorTool currentTool,
                            EntityPaletteType entityType);
    
    // Ray-cast to find brush at screen position
    EditorBrush* RaycastBrush(Viewport* viewport, float x, float y,
                               std::vector<EditorBrush>* brushes);
    
    // Ray-cast to find entity at screen position
    EditorEntity* RaycastEntity(Viewport* viewport, float x, float y,
                                 std::vector<EditorEntity>* entities);

private:
    // 0 = 3D, 1 = Top, 2 = Front, 3 = Side
    std::array<std::unique_ptr<Viewport>, 4> m_viewports;
    
    // Active viewport for input
    int m_activeViewport = 0;
    
    // Layout initialized flag
    bool m_initialized = false;
    
    // ------------------------------------------------------------------------
    // Transform Modal State
    // ------------------------------------------------------------------------
    enum class TransformMode {
        None,
        Translate,
        Rotate,
        Scale
    };

    enum class TransformAxis {
        None,
        X,
        Y,
        Z
    };

    struct TransformState {
        TransformMode mode = TransformMode::None;
        TransformAxis axis = TransformAxis::None;
        
        Genesis::Vec2 startMousePos{0.0f};
        Genesis::Vec3 center{0.0f}; // Average center of selection
        float startDistance = 0.0f; // For scale (dist from center)
        float startAngle = 0.0f;    // For rotation
        
        // Stored initial state for revert/delta calculation
        struct ItemState {
            void* ptr; // Pointer to EditorBrush or EditorEntity
            bool isBrush;
            Genesis::Vec3 startPos;
            Genesis::Vec3 startSize;
            Genesis::Vec3 startRot;
            
            ItemState(void* p, bool b, Genesis::Vec3 pos, Genesis::Vec3 size, Genesis::Vec3 rot)
                : ptr(p), isBrush(b), startPos(pos), startSize(size), startRot(rot) {}
        };
        std::vector<ItemState> items;
    } m_transform;
    
    // Helpers for transform
    void StartTransform(TransformMode mode, SelectionManager* selection, const Genesis::Vec2& startMouse);
    void UpdateTransform(const Genesis::Vec2& currentMouse, Viewport* viewport);
    void ApplyTransform();
    void CancelTransform();
    
    // ------------------------------------------------------------------------
    // Box Selection State
    // ------------------------------------------------------------------------
    struct BoxSelectState {
        bool active = false;
        Genesis::Vec2 startPos{0.0f};
        Genesis::Vec2 endPos{0.0f};
        int viewportIndex = -1;
    } m_boxSelect;
    
    void RenderBoxSelectOverlay();
    
    // ------------------------------------------------------------------------
    // Click-through selection (cycle through overlapping objects)
    // ------------------------------------------------------------------------
    struct ClickThroughState {
        Genesis::Vec2 lastClickPos{0.0f};
        int viewportIndex = -1;
        int cycleIndex = 0;  // Which object in the stack to select
        float clickTimeout = 0.3f;  // Max time between clicks to cycle
        double lastClickTime = 0.0;
    } m_clickThrough;
};
