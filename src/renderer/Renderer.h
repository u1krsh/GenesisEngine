#pragma once

#include "math/Math.h"
#include "renderer/material/Material.h"
#include "renderer/mesh/Mesh.h"
#include <memory>
#include <vector>
#include <functional>

namespace Genesis {

// Forward declarations
class FPSCamera;
class Shader;

// ============================================================================
// Light Types
// ============================================================================
struct DirectionalLight {
    Vec3 direction = Vec3(0.5f, 1.0f, 0.3f);
    Vec3 color = Vec3(1.0f, 0.98f, 0.95f);
    float intensity = 1.0f;
};

struct AmbientLight {
    Vec3 color = Vec3(0.15f, 0.15f, 0.2f);
    float intensity = 1.0f;
};

// ============================================================================
// Render Command - Represents a single draw call
// ============================================================================
struct RenderCommand {
    MeshPtr mesh;
    MaterialPtr material;
    Mat4 transform = Mat4(1.0f);

    // Optional per-draw overrides
    bool castShadows = true;
    bool receiveShadows = true;

    RenderCommand() = default;
    RenderCommand(MeshPtr m, MaterialPtr mat, const Mat4& t = Mat4(1.0f))
        : mesh(std::move(m)), material(std::move(mat)), transform(t) {}
};

// ============================================================================
// Renderer - Central rendering system
//
// Handles:
// - Global render state (view/proj matrices, lighting)
// - Render queue sorting and batching
// - Material binding with automatic global uniform upload
//
// Usage pattern (Source-like):
//   renderer.BeginFrame(camera);
//   renderer.SetDirectionalLight(light);
//   renderer.Submit(mesh, material, transform);  // or use Draw() for immediate
//   renderer.EndFrame();  // Sorts and renders all submitted commands
//
// Or immediate mode:
//   renderer.BeginFrame(camera);
//   renderer.Draw(mesh, material, transform);  // Renders immediately
//   renderer.EndFrame();
// ============================================================================
class Renderer {
public:
    static Renderer& Instance() {
        static Renderer instance;
        return instance;
    }

    // ========================================================================
    // Frame Management
    // ========================================================================

    // Begin a new render frame - sets up global state
    void BeginFrame(const FPSCamera& camera);

    // End frame - executes all queued render commands
    void EndFrame();

    // ========================================================================
    // Lighting
    // ========================================================================

    void SetDirectionalLight(const DirectionalLight& light);
    void SetAmbientLight(const AmbientLight& light);

    const DirectionalLight& GetDirectionalLight() const { return m_directionalLight; }
    const AmbientLight& GetAmbientLight() const { return m_ambientLight; }

    // ========================================================================
    // Immediate Mode Drawing
    // ========================================================================

    // Draw immediately (binds material, sets uniforms, draws mesh)
    void Draw(const MeshPtr& mesh, const MaterialPtr& material, const Mat4& transform = Mat4(1.0f));
    void Draw(Mesh& mesh, Material& material, const Mat4& transform = Mat4(1.0f));

    // ========================================================================
    // Deferred Mode (Queue for later sorting/batching)
    // ========================================================================

    // Submit a render command for deferred rendering
    void Submit(const RenderCommand& command);
    void Submit(const MeshPtr& mesh, const MaterialPtr& material, const Mat4& transform = Mat4(1.0f));

    // ========================================================================
    // Material Binding
    // ========================================================================

    // Bind material and upload global uniforms
    // Returns true if shader was switched (for batching optimization)
    bool BindMaterial(const MaterialPtr& material, const Mat4& modelMatrix);
    bool BindMaterial(Material& material, const Mat4& modelMatrix);

    // Upload only global uniforms to currently bound shader
    void UploadGlobalUniforms(Shader& shader, const Mat4& modelMatrix);

    // ========================================================================
    // State Queries
    // ========================================================================

    const Mat4& GetViewMatrix() const { return m_viewMatrix; }
    const Mat4& GetProjectionMatrix() const { return m_projectionMatrix; }
    const Mat4& GetViewProjectionMatrix() const { return m_viewProjMatrix; }
    const Vec3& GetCameraPosition() const { return m_cameraPosition; }

    // Currently bound shader (for batching checks)
    Shader* GetCurrentShader() const { return m_currentShader; }

    // ========================================================================
    // Statistics
    // ========================================================================

    uint32_t GetDrawCallCount() const { return m_drawCalls; }
    uint32_t GetShaderSwitchCount() const { return m_shaderSwitches; }
    uint32_t GetTriangleCount() const { return m_triangleCount; }

    void ResetStats();

private:
    Renderer();
    ~Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Execute a single render command
    void ExecuteCommand(const RenderCommand& cmd);

    // Sort render commands by queue and material
    void SortCommands();

private:
    // Global state
    Mat4 m_viewMatrix = Mat4(1.0f);
    Mat4 m_projectionMatrix = Mat4(1.0f);
    Mat4 m_viewProjMatrix = Mat4(1.0f);
    Vec3 m_cameraPosition = Vec3(0.0f);

    // Lighting
    DirectionalLight m_directionalLight;
    AmbientLight m_ambientLight;

    // Current state (for batching)
    Shader* m_currentShader = nullptr;
    Material* m_currentMaterial = nullptr;

    // Render queue
    std::vector<RenderCommand> m_renderQueue;

    // Statistics
    uint32_t m_drawCalls = 0;
    uint32_t m_shaderSwitches = 0;
    uint32_t m_triangleCount = 0;

    // Frame state
    bool m_inFrame = false;
};

} // namespace Genesis

