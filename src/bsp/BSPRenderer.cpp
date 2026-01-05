#include "BSPRenderer.h"
#include "map/MapLoader.h"
#include "core/Logger.h"
#include <iostream>

namespace Genesis {

bool BSPRenderer::LoadMap(const std::string& filepath) {
    // Load map using MapLoader
    MapPtr map = MapLoader::Instance().Load(filepath);
    if (!map) {
        LOG_ERROR("BSPRenderer", "Failed to load map: " + filepath);
        return false;
    }

    return CompileMap(map);
}

bool BSPRenderer::CompileMap(MapPtr map) {
    if (!map) {
        LOG_ERROR("BSPRenderer", "Cannot compile null map");
        return false;
    }

    m_sourceMap = map;

    LOG_INFO("BSPRenderer", "Compiling map: " + map->GetName());

    // Build brushes first (resolves materials, creates meshes)
    MapLoader::Instance().BuildMap(*map);

    // Compile to BSP
    BSPCompiler compiler;
    BSPCompiler::Options options;
    options.verbose = true;

    m_bsp = compiler.Compile(*map, options);

    if (!m_bsp) {
        LOG_ERROR("BSPRenderer", "BSP compilation failed: " + compiler.GetLastError());
        return false;
    }

    // Initialize GPU resources
    if (!m_bsp->InitializeRendering()) {
        LOG_WARNING("BSPRenderer", "Failed to initialize BSP rendering");
    }

    LOG_INFO("BSPRenderer", "BSP compilation complete");
    return true;
}

void BSPRenderer::SetBSP(BSPTreePtr bsp) {
    m_bsp = bsp;
    if (m_bsp && !m_bsp->HasGeometry()) {
        LOG_WARNING("BSPRenderer", "BSP tree has no geometry");
    }
}

void BSPRenderer::Unload() {
    if (m_bsp) {
        m_bsp->Clear();
        m_bsp = nullptr;
    }
    m_sourceMap = nullptr;
    LOG_INFO("BSPRenderer", "Unloaded BSP");
}

void BSPRenderer::Render(const FPSCamera& camera) {
    if (!m_bsp || !m_bsp->HasGeometry()) {
        return;
    }

    if (!m_shader) {
        LOG_WARNING("BSPRenderer", "No shader set for rendering");
        return;
    }

    m_shader->Bind();

    // Set camera matrices - use correct uniform names matching mesh.vert
    m_shader->SetMat4("u_View", camera.GetViewMatrix());
    m_shader->SetMat4("u_Proj", camera.GetProjectionMatrix());
    m_shader->SetMat4("u_Model", Mat4(1.0f));  // World geometry uses identity

    // Set lighting uniforms for mesh.frag
    m_shader->SetVec3("u_LightDir", glm::normalize(Vec3(0.5f, 1.0f, 0.3f)));
    m_shader->SetVec3("u_LightColor", Vec3(1.0f, 0.98f, 0.95f));
    m_shader->SetVec3("u_AmbientColor", Vec3(0.15f, 0.15f, 0.2f));

    // Render BSP - ALWAYS use real-time frustum culling
    // This matches the minimap visibility exactly
    m_bsp->RenderWithCulling(camera, *m_shader);

    // Optional wireframe overlay
    if (m_showWireframe && m_wireframeShader) {
        RenderWireframe(camera);
    }
}

bool BSPRenderer::HasPVS() const {
    return m_bsp && m_bsp->HasPVS();
}

void BSPRenderer::RenderWireframe(const FPSCamera& camera) {
    if (!m_bsp || !m_wireframeShader) return;

    m_wireframeShader->Bind();
    m_wireframeShader->SetMat4("view", camera.GetViewMatrix());
    m_wireframeShader->SetMat4("projection", camera.GetProjectionMatrix());
    m_wireframeShader->SetMat4("model", Mat4(1.0f));
    m_wireframeShader->SetVec3("color", Vec3(0.0f, 1.0f, 0.0f));  // Green wireframe

    m_bsp->RenderWireframe(camera, *m_wireframeShader);
}

bool BSPRenderer::InitializeShaders() {
    // Create default BSP shader if not set
    if (!m_shader) {
        m_shader = std::make_shared<Shader>();
        if (!m_shader->LoadFromFiles("assets/shaders/mesh.vert", "assets/shaders/mesh.frag")) {
            LOG_WARNING("BSPRenderer", "Failed to load default mesh shader, trying bsp shader");
            if (!m_shader->LoadFromFiles("assets/shaders/bsp.vert", "assets/shaders/bsp.frag")) {
                LOG_ERROR("BSPRenderer", "Failed to load any shader for BSP rendering");
                return false;
            }
        }
    }

    // Create wireframe shader if not set
    if (!m_wireframeShader) {
        m_wireframeShader = std::make_shared<Shader>();
        if (!m_wireframeShader->LoadFromFiles("assets/shaders/debug.vert", "assets/shaders/debug.frag")) {
            LOG_WARNING("BSPRenderer", "Wireframe shader not available");
        }
    }

    return m_shader->IsValid();
}

BSPStats BSPRenderer::GetStats() const {
    if (m_bsp) {
        return m_bsp->GetStats();
    }
    return BSPStats();
}

uint32_t BSPRenderer::GetRenderedFaces() const {
    return m_bsp ? m_bsp->GetLastFrameFaceCount() : 0;
}

uint32_t BSPRenderer::GetRenderedLeafs() const {
    return m_bsp ? m_bsp->GetLastFrameLeafCount() : 0;
}

uint32_t BSPRenderer::GetRenderedNodes() const {
    return m_bsp ? m_bsp->GetLastFrameNodeCount() : 0;
}

} // namespace Genesis

