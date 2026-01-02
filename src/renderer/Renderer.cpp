#include "Renderer.h"
#include "camera/Camera.h"
#include "renderer/shader/Shader.h"
#include "renderer/mesh/Mesh.h"
#include "core/Logger.h"
#include <algorithm>

namespace Genesis {

// ============================================================================
// Constructor
// ============================================================================

Renderer::Renderer() {
    m_renderQueue.reserve(1000);  // Pre-allocate for typical scene
}

// ============================================================================
// Frame Management
// ============================================================================

void Renderer::BeginFrame(const FPSCamera& camera) {
    if (m_inFrame) {
        LOG_WARNING("Renderer", "BeginFrame called while already in frame");
        EndFrame();
    }

    m_inFrame = true;

    // Cache camera matrices
    m_viewMatrix = camera.GetViewMatrix();
    m_projectionMatrix = camera.GetProjectionMatrix();
    m_viewProjMatrix = m_projectionMatrix * m_viewMatrix;
    m_cameraPosition = camera.GetPosition();

    // Clear render queue
    m_renderQueue.clear();

    // Reset per-frame state
    m_currentShader = nullptr;
    m_currentMaterial = nullptr;

    // Reset statistics
    ResetStats();
}

void Renderer::EndFrame() {
    if (!m_inFrame) {
        return;
    }

    // Sort and execute all queued commands
    if (!m_renderQueue.empty()) {
        SortCommands();

        for (const auto& cmd : m_renderQueue) {
            ExecuteCommand(cmd);
        }

        m_renderQueue.clear();
    }

    // Unbind current material
    if (m_currentMaterial) {
        m_currentMaterial->Unbind();
        m_currentMaterial = nullptr;
    }
    m_currentShader = nullptr;

    m_inFrame = false;
}

// ============================================================================
// Lighting
// ============================================================================

void Renderer::SetDirectionalLight(const DirectionalLight& light) {
    m_directionalLight = light;
}

void Renderer::SetAmbientLight(const AmbientLight& light) {
    m_ambientLight = light;
}

// ============================================================================
// Immediate Mode Drawing
// ============================================================================

void Renderer::Draw(const MeshPtr& mesh, const MaterialPtr& material, const Mat4& transform) {
    if (!mesh || !material) {
        return;
    }
    Draw(*mesh, *material, transform);
}

void Renderer::Draw(Mesh& mesh, Material& material, const Mat4& transform) {
    if (!m_inFrame) {
        LOG_WARNING("Renderer", "Draw called outside of BeginFrame/EndFrame");
        return;
    }

    // Bind material with global uniforms
    BindMaterial(material, transform);

    // Draw mesh
    mesh.Draw();

    // Update stats
    m_drawCalls++;
    if (mesh.HasIndices()) {
        m_triangleCount += mesh.GetIndexCount() / 3;
    } else {
        m_triangleCount += mesh.GetVertexCount() / 3;
    }
}

// ============================================================================
// Deferred Mode
// ============================================================================

void Renderer::Submit(const RenderCommand& command) {
    if (!m_inFrame) {
        LOG_WARNING("Renderer", "Submit called outside of BeginFrame/EndFrame");
        return;
    }
    m_renderQueue.push_back(command);
}

void Renderer::Submit(const MeshPtr& mesh, const MaterialPtr& material, const Mat4& transform) {
    Submit(RenderCommand(mesh, material, transform));
}

// ============================================================================
// Material Binding
// ============================================================================

bool Renderer::BindMaterial(const MaterialPtr& material, const Mat4& modelMatrix) {
    if (!material) {
        return false;
    }
    return BindMaterial(*material, modelMatrix);
}

bool Renderer::BindMaterial(Material& material, const Mat4& modelMatrix) {
    bool shaderSwitched = false;

    auto shader = material.GetShader();
    if (!shader || !shader->IsValid()) {
        LOG_WARNING("Renderer", "Material '" + material.GetName() + "' has no valid shader");
        return false;
    }

    // Check if we need to switch shaders
    Shader* shaderPtr = shader.get();
    if (m_currentShader != shaderPtr) {
        // Bind the full material (shader + properties + render state)
        material.Bind();

        m_currentShader = shaderPtr;
        m_currentMaterial = &material;
        m_shaderSwitches++;
        shaderSwitched = true;
    } else if (m_currentMaterial != &material) {
        // Same shader, different material - just upload properties
        material.ApplyRenderState();
        material.UploadProperties();
        m_currentMaterial = &material;
    }

    // Always upload global uniforms with current model matrix
    UploadGlobalUniforms(*shader, modelMatrix);

    return shaderSwitched;
}

void Renderer::UploadGlobalUniforms(Shader& shader, const Mat4& modelMatrix) {
    // Transform matrices - only set if shader has them
    if (shader.HasUniform("u_Model"))
        shader.SetMat4("u_Model", modelMatrix);
    if (shader.HasUniform("u_View"))
        shader.SetMat4("u_View", m_viewMatrix);
    if (shader.HasUniform("u_Proj"))
        shader.SetMat4("u_Proj", m_projectionMatrix);
    if (shader.HasUniform("u_ViewProj"))
        shader.SetMat4("u_ViewProj", m_viewProjMatrix);

    // Camera
    if (shader.HasUniform("u_CameraPos"))
        shader.SetVec3("u_CameraPos", m_cameraPosition);

    // Directional light
    Vec3 lightDir = glm::normalize(m_directionalLight.direction);
    if (shader.HasUniform("u_LightDir"))
        shader.SetVec3("u_LightDir", lightDir);
    if (shader.HasUniform("u_LightColor"))
        shader.SetVec3("u_LightColor", m_directionalLight.color * m_directionalLight.intensity);

    // Ambient light
    if (shader.HasUniform("u_AmbientColor"))
        shader.SetVec3("u_AmbientColor", m_ambientLight.color * m_ambientLight.intensity);
}

// ============================================================================
// Statistics
// ============================================================================

void Renderer::ResetStats() {
    m_drawCalls = 0;
    m_shaderSwitches = 0;
    m_triangleCount = 0;
}

// ============================================================================
// Private Methods
// ============================================================================

void Renderer::ExecuteCommand(const RenderCommand& cmd) {
    if (!cmd.mesh || !cmd.material) {
        return;
    }
    Draw(*cmd.mesh, *cmd.material, cmd.transform);
}

void Renderer::SortCommands() {
    // Sort by:
    // 1. Render queue (background -> geometry -> transparent -> overlay)
    // 2. Shader (minimize shader switches)
    // 3. Material (minimize uniform uploads)

    std::sort(m_renderQueue.begin(), m_renderQueue.end(),
        [](const RenderCommand& a, const RenderCommand& b) {
            // First by render queue
            int queueA = static_cast<int>(a.material->GetRenderQueue());
            int queueB = static_cast<int>(b.material->GetRenderQueue());
            if (queueA != queueB) {
                return queueA < queueB;
            }

            // Then by shader pointer (same shader grouped together)
            Shader* shaderA = a.material->GetShader().get();
            Shader* shaderB = b.material->GetShader().get();
            if (shaderA != shaderB) {
                return shaderA < shaderB;
            }

            // Then by material pointer
            return a.material.get() < b.material.get();
        });
}

} // namespace Genesis

