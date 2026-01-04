#pragma once

#include "BSPTree.h"
#include "BSPCompiler.h"
#include "map/Map.h"
#include "renderer/shader/Shader.h"
#include "camera/Camera.h"
#include <memory>

namespace Genesis {

// ============================================================================
// BSPRenderer - Renders BSP-compiled maps
//
// Phase 1: Simple BSP rendering
// - Compiles maps to BSP trees
// - Renders using tree traversal (DrawNode)
// - Tracks rendering statistics
//
// Instead of:
//   for (auto& mesh : worldMeshes)
//       mesh.Draw();
//
// We do:
//   bspRenderer.Render(camera);
//
// ============================================================================
class BSPRenderer {
public:
    static BSPRenderer& Instance() {
        static BSPRenderer instance;
        return instance;
    }

    // ========================================================================
    // Map Management
    // ========================================================================

    // Load and compile a map to BSP
    bool LoadMap(const std::string& filepath);

    // Compile an already-loaded map to BSP
    bool CompileMap(MapPtr map);

    // Set pre-compiled BSP tree
    void SetBSP(BSPTreePtr bsp);

    // Get current BSP tree
    BSPTreePtr GetBSP() const { return m_bsp; }

    // Unload current BSP
    void Unload();

    // Check if BSP is loaded
    bool HasBSP() const { return m_bsp && m_bsp->IsValid(); }

    // ========================================================================
    // Rendering
    // ========================================================================

    // Render the BSP world
    void Render(const FPSCamera& camera);

    // Render wireframe overlay
    void RenderWireframe(const FPSCamera& camera);

    // ========================================================================
    // Debug Visualization
    // ========================================================================

    // Toggle debug modes
    void SetShowWireframe(bool show) { m_showWireframe = show; }
    bool GetShowWireframe() const { return m_showWireframe; }

    void SetShowNodes(bool show) { m_showNodes = show; }
    bool GetShowNodes() const { return m_showNodes; }

    void SetShowLeafs(bool show) { m_showLeafs = show; }
    bool GetShowLeafs() const { return m_showLeafs; }

    // Track if BSP rendering is currently active (for debug overlay)
    void SetRenderingActive(bool active) { m_renderingActive = active; }
    bool IsRenderingActive() const { return m_renderingActive; }

    // ========================================================================
    // Statistics
    // ========================================================================

    BSPStats GetStats() const;

    // Per-frame stats
    uint32_t GetRenderedFaces() const;
    uint32_t GetRenderedLeafs() const;
    uint32_t GetRenderedNodes() const;

    // ========================================================================
    // Shader Management
    // ========================================================================

    void SetShader(std::shared_ptr<Shader> shader) { m_shader = shader; }
    void SetWireframeShader(std::shared_ptr<Shader> shader) { m_wireframeShader = shader; }

    bool InitializeShaders();

private:
    BSPRenderer() = default;
    ~BSPRenderer() = default;
    BSPRenderer(const BSPRenderer&) = delete;
    BSPRenderer& operator=(const BSPRenderer&) = delete;

private:
    BSPTreePtr m_bsp;
    MapPtr m_sourceMap;  // Keep reference to source map

    std::shared_ptr<Shader> m_shader;
    std::shared_ptr<Shader> m_wireframeShader;

    // Debug options
    bool m_showWireframe = false;
    bool m_showNodes = false;
    bool m_showLeafs = false;
    bool m_renderingActive = false;  // True when BSP is being used for rendering
};

} // namespace Genesis

