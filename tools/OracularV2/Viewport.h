#pragma once

// ============================================================================
// OracularV2 Viewport System
// 3D Perspective and 2D Orthographic viewports for editing
// ============================================================================

#include "math/Math.h"
#include "EditorCamera.h"
#include <memory>
#include <string>
#include <vector>

class Grid;
class Gizmo;
class SelectionManager;
struct EditorBrush;

namespace Genesis {
    class Map;
}

// ============================================================================
// Viewport Types
// ============================================================================
enum class ViewportType {
    Perspective3D,  // Free camera perspective view
    TopXZ,          // Top-down (Y-up, looking at XZ plane)
    FrontXY,        // Front view (Z-up, looking at XY plane) 
    SideYZ          // Side view (X-up, looking at YZ plane)
};

inline const char* ViewportTypeToString(ViewportType type) {
    switch (type) {
        case ViewportType::Perspective3D: return "3D View";
        case ViewportType::TopXZ: return "Top (XZ)";
        case ViewportType::FrontXY: return "Front (XY)";
        case ViewportType::SideYZ: return "Side (YZ)";
        default: return "Unknown";
    }
}

// ============================================================================
// Viewport - A view into the map
// ============================================================================
class Viewport {
public:
    Viewport(ViewportType type = ViewportType::Perspective3D);
    ~Viewport();

    // Setup/teardown
    void Initialize(int width, int height);
    void Resize(int width, int height);
    
    // Rendering
    void BeginRender();
    void EndRender();
    void RenderGrid(Grid* grid);
    void RenderBrushes(std::vector<EditorBrush>* brushes, SelectionManager* selection);
    void RenderEntities(std::vector<struct EditorEntity>* entities, SelectionManager* selection);
    void RenderBrushPreview(const EditorBrush& preview, Grid* grid);
    void RenderGizmo(Gizmo* gizmo);
    
    // Camera control
    void Pan(float dx, float dy);
    void Zoom(float delta);
    void Orbit(float dx, float dy);  // Only for 3D viewport
    void MoveOrbitTarget(const Genesis::Vec3& delta) {
        m_orbitTarget += delta;
        SetupCamera();
    }
    void FocusOn(const Genesis::Vec3& position);
    
    // Mouse interaction
    Genesis::Ray ScreenToWorldRay(float screenX, float screenY) const;
    Genesis::Vec3 ScreenToWorld(float screenX, float screenY, float depth = 0.0f) const;
    
    // Properties
    ViewportType GetType() const { return m_type; }
    void SetType(ViewportType type) {
        if (m_type == type) return;
        m_type = type;
        SetupCamera(); // Reset camera for new type
    }
    
    bool IsPerspective() const { return m_type == ViewportType::Perspective3D; }
    bool IsOrthographic() const { return !IsPerspective(); }
    
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    
    unsigned int GetFramebufferTexture() const { return m_colorTexture; }
    
    EditorCamera& GetCamera() { return m_camera; }
    const EditorCamera& GetCamera() const { return m_camera; }
    
    // 2D viewport zoom level
    float GetZoom() const { return m_zoom; }
    void SetZoom(float zoom) { m_zoom = std::max(0.1f, zoom); }
    
    Genesis::Vec2 GetPan() const { return m_panOffset; }
    void SetPan(const Genesis::Vec2& pan) { m_panOffset = pan; }

private:
    void CreateFramebuffer();
    void DestroyFramebuffer();
    void SetupCamera();
    
    // Drawing helpers
    void DrawLine(const Genesis::Vec3& start, const Genesis::Vec3& end, 
                  const Genesis::Vec4& color);
    void DrawBox(const Genesis::Vec3& min, const Genesis::Vec3& max,
                 const Genesis::Vec4& color, bool filled = false);
    void DrawAxisGizmo(const Genesis::Vec3& pos, float size);
    


private:
    ViewportType m_type;
    int m_width = 0;
    int m_height = 0;
    
    // Camera
    EditorCamera m_camera;
    float m_zoom = 0.15f;  // Start zoomed out to see whole map
    Genesis::Vec2 m_panOffset{0.0f, 0.0f};
    
    // Orbit camera state (3D only)
    float m_orbitYaw = -45.0f;
    float m_orbitPitch = 30.0f;
    float m_orbitDistance = 80.0f;  // Start further back to see map
    Genesis::Vec3 m_orbitTarget{0.0f};
    
    // FBO for offscreen rendering
    unsigned int m_framebuffer = 0;
    unsigned int m_colorTexture = 0;
    unsigned int m_depthRenderbuffer = 0;
    
    // Simple line rendering
    unsigned int m_lineVAO = 0;
    unsigned int m_lineVBO = 0;
    unsigned int m_lineShader = 0;
};
