#include "BSPTree.h"
#include "camera/Camera.h"
#include "renderer/shader/Shader.h"
#include "renderer/texture/Texture2D.h"
#include "core/Logger.h"
#include <glad/glad.h>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "math/Frustum.h"

namespace Genesis {

// ============================================================================
// BSPStats Implementation
// ============================================================================

void BSPStats::Print() const {
    std::cout << "=== BSP Stats ===" << std::endl;
    std::cout << "  Planes:    " << numPlanes << std::endl;
    std::cout << "  Nodes:     " << numNodes << std::endl;
    std::cout << "  Leafs:     " << numLeafs << std::endl;
    std::cout << "  Faces:     " << numFaces << std::endl;
    std::cout << "  Vertices:  " << numVertices << std::endl;
    std::cout << "  Indices:   " << numIndices << std::endl;
    std::cout << "  Brushes:   " << numBrushes << std::endl;
    std::cout << "  Materials: " << numMaterials << std::endl;
    std::cout << "  Max Depth: " << maxTreeDepth << std::endl;
    std::cout << "=================" << std::endl;
}

// ============================================================================
// BSPTree Implementation
// ============================================================================

BSPTree::~BSPTree() {
    ClearGPUResources();
}

void BSPTree::Clear() {
    ClearGPUResources();

    m_planes.clear();
    m_nodes.clear();
    m_leafs.clear();
    m_faces.clear();
    m_vertices.clear();
    m_indices.clear();
    m_leafFaces.clear();
    m_brushRefs.clear();
    m_materials.clear();
    m_materialBatches.clear();

    // Clear collision data (Phase 2)
    m_collision.Clear();

    // Clear PVS data (Phase 3)
    m_pvs.Clear();

    m_lastFrameFaces = 0;
    m_lastFrameLeafs = 0;
    m_lastFrameNodes = 0;
}

void BSPTree::ClearGPUResources() {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ebo) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    if (m_lightmapTexture) {
        glDeleteTextures(1, &m_lightmapTexture);
        m_lightmapTexture = 0;
    }
    if (m_sceneColorTexture) {
        glDeleteTextures(1, &m_sceneColorTexture);
        m_sceneColorTexture = 0;
    }
    m_gpuReady = false;
    m_lightmapUploaded = false;
}

// ============================================================================
// Lightmap Texture Management (Phase 4B)
// ============================================================================

void BSPTree::UploadLightmapTexture() {
    if (m_lightmapAtlas.GetPixelDataSize() == 0) return;
    
    // Delete old texture if exists
    if (m_lightmapTexture) {
        glDeleteTextures(1, &m_lightmapTexture);
    }
    
    glGenTextures(1, &m_lightmapTexture);
    glBindTexture(GL_TEXTURE_2D, m_lightmapTexture);
    
    // Upload RGB8 data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                 m_lightmapAtlas.GetWidth(), m_lightmapAtlas.GetHeight(),
                 0, GL_RGB, GL_UNSIGNED_BYTE, m_lightmapAtlas.GetPixelData());
    
    // Lightmap filtering: bilinear for smooth lighting
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Clamp to edge to prevent bleeding
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    m_lightmapUploaded = true;
    
    LOG_INFO("BSPTree", "Uploaded lightmap atlas: " + 
             std::to_string(m_lightmapAtlas.GetWidth()) + "x" +
             std::to_string(m_lightmapAtlas.GetHeight()) + 
             " (" + std::to_string(static_cast<int>(m_lightmapAtlas.GetUtilization() * 100)) + "% used)");
}

void BSPTree::BindLightmapTexture(uint32_t unit) const {
    if (!m_lightmapUploaded) return;
    
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_lightmapTexture);
}

// ============================================================================
// Tree Queries
// ============================================================================

int32_t BSPTree::FindLeaf(const Vec3& position) const {
    if (m_nodes.empty()) {
        // No tree, return first leaf if exists
        return m_leafs.empty() ? -1 : 0;
    }

    int32_t nodeIndex = 0;

    while (nodeIndex >= 0) {
        const BSPNode& node = m_nodes[nodeIndex];
        const BSPPlane& plane = m_planes[node.planeIndex];

        float dist = plane.ClassifyPoint(position);

        if (dist >= 0) {
            // Front side
            if (node.IsFrontLeaf()) {
                return node.GetFrontLeafIndex();
            }
            nodeIndex = node.frontChild;
        } else {
            // Back side
            if (node.IsBackLeaf()) {
                return node.GetBackLeafIndex();
            }
            nodeIndex = node.backChild;
        }
    }

    return -1;
}

const BSPLeaf* BSPTree::GetLeaf(uint32_t index) const {
    if (index < m_leafs.size()) {
        return &m_leafs[index];
    }
    return nullptr;
}

bool BSPTree::IsPointSolid(const Vec3& position) const {
    int32_t leafIndex = FindLeaf(position);
    if (leafIndex < 0 || leafIndex >= static_cast<int32_t>(m_leafs.size())) {
        return false;
    }
    return (m_leafs[leafIndex].contents & BSPContents::Solid) != BSPContents::Empty;
}

// ============================================================================
// Rendering
// ============================================================================

bool BSPTree::InitializeRendering() {
    if (m_vertices.empty() || m_indices.empty()) {
        LOG_WARNING("BSPTree", "No geometry to initialize");
        return false;
    }

    UploadGeometry();
    BuildMaterialBatches();

    m_gpuReady = true;
    LOG_INFO("BSPTree", "Initialized rendering with " + std::to_string(m_vertices.size()) +
             " vertices, " + std::to_string(m_indices.size()) + " indices");

    return true;
}

void BSPTree::UploadGeometry() {
    ClearGPUResources();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    // Upload vertex data
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(BSPVertex),
                 m_vertices.data(), GL_STATIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BSPVertex),
                          (void*)offsetof(BSPVertex, position));
    glEnableVertexAttribArray(0);

    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(BSPVertex),
                          (void*)offsetof(BSPVertex, normal));
    glEnableVertexAttribArray(1);

    // TexCoord attribute (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(BSPVertex),
                          (void*)offsetof(BSPVertex, texCoord));
    glEnableVertexAttribArray(2);

    // Color attribute (location 3)
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(BSPVertex),
                          (void*)offsetof(BSPVertex, color));
    glEnableVertexAttribArray(3);

    // Lightmap TexCoord attribute (location 4) - Phase 4A
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(BSPVertex),
                          (void*)offsetof(BSPVertex, lightmapCoord));
    glEnableVertexAttribArray(4);

    // Tangent attribute (location 5) - For normal mapping TBN matrix
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(BSPVertex),
                          (void*)offsetof(BSPVertex, tangent));
    glEnableVertexAttribArray(5);

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(uint32_t),
                 m_indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void BSPTree::UpdateMesh() {
    if (!m_gpuReady || m_vbo == 0) return;
    
    // Bind existing VBO and update content
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(BSPVertex),
                 m_vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    LOG_INFO("BSPTree", "Updated mesh data on GPU (post-bake)");
}

void BSPTree::BuildMaterialBatches() {
    m_materialBatches.clear();

    if (m_faces.empty()) return;

    // Simple approach: just create one batch for all geometry
    // No index reordering needed - just use RenderAll for fastest path
    
    MaterialBatch batch;
    batch.materialIndex = 0;
    batch.firstIndex = 0;
    batch.indexCount = static_cast<uint32_t>(m_indices.size());
    batch.material = MaterialLibrary::Instance().Get("default");
    batch.cachedColor = Vec3(0.5f, 0.5f, 0.5f);
    
    m_materialBatches.push_back(batch);

    LOG_DEBUG("BSPTree", "Built 1 material batch with " + std::to_string(m_indices.size()) + " indices");
}

void BSPTree::Render(const FPSCamera& camera, Shader& shader) {
    if (!m_gpuReady) {
        RenderAll(shader);
        return;
    }

    m_lastFrameFaces = 0;
    m_lastFrameLeafs = 0;
    m_lastFrameNodes = 0;

    Vec3 camPos = camera.GetPosition();
    
    // Store camera state for use by DrawFace (needed for glass shader)
    m_currentViewMatrix = camera.GetViewMatrix();
    m_currentProjMatrix = camera.GetProjectionMatrix();
    m_currentCameraPos = camPos;

    // Two-pass global rendering for proper transparency:
    // Pass 1: Render all OPAQUE faces (skip glass)
    m_renderingGlassPass = false;
    RenderBSPPass(camPos, shader);
    
    // === Capture scene color for glass refraction ===
    // Get current viewport dimensions
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int vpWidth = viewport[2];
    int vpHeight = viewport[3];
    
    // Create or resize scene color texture if needed
    if (m_sceneColorTexture == 0 || m_sceneColorWidth != vpWidth || m_sceneColorHeight != vpHeight) {
        if (m_sceneColorTexture == 0) {
            glGenTextures(1, &m_sceneColorTexture);
        }
        
        glBindTexture(GL_TEXTURE_2D, m_sceneColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, vpWidth, vpHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        m_sceneColorWidth = vpWidth;
        m_sceneColorHeight = vpHeight;
    }
    
    // Copy current framebuffer to scene color texture
    glBindTexture(GL_TEXTURE_2D, m_sceneColorTexture);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, viewport[0], viewport[1], vpWidth, vpHeight);
    
    // Pass 2: Render all GLASS faces (after all opaque)
    m_renderingGlassPass = true;
    RenderBSPPass(camPos, shader);
    m_renderingGlassPass = false;
}

void BSPTree::RenderBSPPass(const Vec3& camPos, Shader& shader) {
    if (m_rootNode >= 0 && !m_nodes.empty()) {
        DrawNode(m_rootNode, camPos, shader);
    } else if (m_rootNode < 0) {
        uint32_t leafIdx = static_cast<uint32_t>(-(m_rootNode + 1));
        DrawLeaf(leafIdx, shader);
    } else if (!m_leafs.empty()) {
        for (uint32_t i = 0; i < m_leafs.size(); i++) {
            DrawLeaf(i, shader);
        }
    }
}

void BSPTree::RenderWithCulling(const FPSCamera& camera, Shader& shader) {
    if (!m_gpuReady) {
        if (!m_vertices.empty() && !m_indices.empty()) {
            UploadGeometry();
            m_gpuReady = true;
        } else {
            return;
        }
    }

    m_lastFrameFaces = 0;
    m_lastFrameLeafs = 0;
    m_lastFrameNodes = 0;

    // Use tree traversal to properly bind textures per-face
    // This calls DrawFace() which binds the correct texture for each face
    Render(camera, shader);
}

void BSPTree::RenderAll(Shader& shader) {
    if (m_vertices.empty() || m_indices.empty()) return;

    // Make sure we have GPU resources
    if (!m_gpuReady) {
        UploadGeometry();
        m_gpuReady = true;
    }

    shader.Bind();

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    m_lastFrameFaces = static_cast<uint32_t>(m_faces.size());
}

void BSPTree::RenderWireframe(const FPSCamera& camera, Shader& shader) {
    if (!m_gpuReady && !m_vertices.empty()) {
        UploadGeometry();
        m_gpuReady = true;
    }

    if (!m_gpuReady) return;

    shader.Bind();

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}



void BSPTree::DrawNode(int32_t nodeIndex, const Vec3& cameraPos, Shader& shader) {
    // Original DrawNode (no culling)
    if (nodeIndex < 0 || nodeIndex >= static_cast<int32_t>(m_nodes.size())) return;
    m_lastFrameNodes++;
    const BSPNode& node = m_nodes[nodeIndex];
    if (node.planeIndex >= m_planes.size()) return; // Safety check
    const BSPPlane& plane = m_planes[node.planeIndex];
    
    float dist = plane.ClassifyPoint(cameraPos);
    if (dist >= 0) {
        if (node.IsFrontLeaf()) DrawLeaf(node.GetFrontLeafIndex(), shader);
        else DrawNode(node.frontChild, cameraPos, shader);
        if (node.IsBackLeaf()) DrawLeaf(node.GetBackLeafIndex(), shader);
        else DrawNode(node.backChild, cameraPos, shader);
    } else {
        if (node.IsBackLeaf()) DrawLeaf(node.GetBackLeafIndex(), shader);
        else DrawNode(node.backChild, cameraPos, shader);
        if (node.IsFrontLeaf()) DrawLeaf(node.GetFrontLeafIndex(), shader);
        else DrawNode(node.frontChild, cameraPos, shader);
    }
}

void BSPTree::DrawNodeWithCulling(int32_t nodeIndex, const Vec3& cameraPos, Shader& shader, const Frustum& frustum) {
    if (nodeIndex < 0 || nodeIndex >= static_cast<int32_t>(m_nodes.size())) return;

    const BSPNode& node = m_nodes[nodeIndex];
    
    // Safety check for plane index
    if (node.planeIndex >= m_planes.size()) return;

    // Check node bounds against frustum
    if (!frustum.IsBoxVisible(node.boundsMin, node.boundsMax)) {
        return; // Culled!
    }

    m_lastFrameNodes++;
    
    const BSPPlane& plane = m_planes[node.planeIndex];
    float dist = plane.ClassifyPoint(cameraPos);

    // Front-to-back traversal
    if (dist >= 0) { // Camera in front
        // Draw Front
        if (node.IsFrontLeaf()) {
            const BSPLeaf& leaf = m_leafs[node.GetFrontLeafIndex()];
            if (frustum.IsBoxVisible(leaf.boundsMin, leaf.boundsMax)) {
                DrawLeaf(node.GetFrontLeafIndex(), shader);
            }
        } else {
            DrawNodeWithCulling(node.frontChild, cameraPos, shader, frustum);
        }
        
        // Draw Back (if possibly visible)
        // Since we are recursive, we trust the child node's bounds check to cull it if irrelevant
        if (node.IsBackLeaf()) {
            const BSPLeaf& leaf = m_leafs[node.GetBackLeafIndex()];
            if (frustum.IsBoxVisible(leaf.boundsMin, leaf.boundsMax)) {
                DrawLeaf(node.GetBackLeafIndex(), shader);
            }
        } else {
            DrawNodeWithCulling(node.backChild, cameraPos, shader, frustum);
        }
    } else { // Camera in back
        // Draw Back first
        if (node.IsBackLeaf()) {
            const BSPLeaf& leaf = m_leafs[node.GetBackLeafIndex()];
            if (frustum.IsBoxVisible(leaf.boundsMin, leaf.boundsMax)) {
                DrawLeaf(node.GetBackLeafIndex(), shader);
            }
        } else {
            DrawNodeWithCulling(node.backChild, cameraPos, shader, frustum);
        }
        
        // Draw Front
        if (node.IsFrontLeaf()) {
            const BSPLeaf& leaf = m_leafs[node.GetFrontLeafIndex()];
            if (frustum.IsBoxVisible(leaf.boundsMin, leaf.boundsMax)) {
                DrawLeaf(node.GetFrontLeafIndex(), shader);
            }
        } else {
            DrawNodeWithCulling(node.frontChild, cameraPos, shader, frustum);
        }
    }

    }


void BSPTree::DrawLeaf(uint32_t leafIndex, Shader& shader) {
    if (leafIndex >= m_leafs.size()) return;

    m_lastFrameLeafs++;

    const BSPLeaf& leaf = m_leafs[leafIndex];

    // Skip empty/solid leafs with no visible faces
    if (leaf.numFaces == 0) return;

    // Draw faces based on current rendering pass
    for (uint32_t i = 0; i < leaf.numFaces; i++) {
        uint32_t faceIdx = m_leafFaces[leaf.firstFace + i];
        if (faceIdx < m_faces.size()) {
            const BSPFace& face = m_faces[faceIdx];
            
            // Check if this is a glass face (either __glass or __glass_real)
            bool isGlass = false;
            if (face.materialIndex < m_materials.size()) {
                const std::string& matName = m_materials[face.materialIndex].name;
                // Check for __glass_real first (12 chars)
                if (matName.size() > 12) {
                    std::string suffix = matName.substr(matName.size() - 12);
                    if (suffix == "__glass_real") {
                        isGlass = true;
                    }
                }
                // Then check for __glass (7 chars)
                if (!isGlass && matName.size() > 7) {
                    std::string suffix = matName.substr(matName.size() - 7);
                    if (suffix == "__glass" || suffix == "__Glass") {
                        isGlass = true;
                    }
                }
            }
            
            // Opaque pass: skip glass, Glass pass: only glass
            if (m_renderingGlassPass != isGlass) {
                continue;
            }
            
            DrawFace(face, shader);
        }
    }
}

void BSPTree::DrawFace(const BSPFace& face, Shader& shader) {
    if (face.numIndices == 0) return;

    m_lastFrameFaces++;

    // Set material color and texture on the shader
    bool hasTexture = false;
    bool isGlass = false;
    bool isGlassReal = false;
    float transparency = 0.5f;  // Default glass transparency
    
    if (face.materialIndex < m_materials.size()) {
        const std::string& matName = m_materials[face.materialIndex].name;
        
        // Check if this is a glass_real material (key ends with __glass_real)
        if (matName.size() > 12) {
            std::string suffix = matName.substr(matName.size() - 12);
            if (suffix == "__glass_real") {
                isGlassReal = true;
                isGlass = true;  // Also set isGlass for transparency handling
            }
        }
        
        // Check if this is a regular glass material (key ends with __glass)
        if (!isGlassReal && matName.size() > 7) {
            std::string suffix = matName.substr(matName.size() - 7);
            if (suffix == "__glass" || suffix == "__Glass") {
                isGlass = true;
            }
        }
        
        MaterialPtr mat = MaterialLibrary::Instance().Get(matName);
        if (mat) {
            // Get the color from the material and set it on the BSP shader
            Vec3 color = mat->GetVec3("u_Color", Vec3(1.0f, 1.0f, 1.0f));
            shader.SetVec3("u_Color", color);
            
            // Get transparency for glass materials
            if (isGlass) {
                // Try both possible property names (u_Transparency from MapLoader, transparency from SAU)
                transparency = mat->GetFloat("u_Transparency", 0.0f);
                if (transparency == 0.0f) {
                    transparency = mat->GetFloat("transparency", 0.3f);
                }
            }
            
            // Get and bind texture if available
            const TextureSlot* texSlot = mat->GetTextureSlot("u_BaseTexture");
            if (texSlot && texSlot->texture) {
                texSlot->texture->Bind(0);
                hasTexture = true;
                // Debug: Log first time this material's texture is bound
                static std::unordered_set<std::string> loggedMats;
                if (loggedMats.find(matName) == loggedMats.end()) {
                    LOG_INFO("BSPTree", "DrawFace: Binding texture for material '" + matName + "'" + 
                             (isGlass ? " [GLASS trans=" + std::to_string(transparency) + "]" : ""));
                    loggedMats.insert(matName);
                }
            } else {
                // Debug: Log missing texture
                static std::unordered_set<std::string> loggedMissing;
                if (loggedMissing.find(matName) == loggedMissing.end()) {
                    LOG_WARNING("BSPTree", "DrawFace: Material '" + matName + "' has no u_BaseTexture");
                    loggedMissing.insert(matName);
                }
            }
        } else {
            // Debug: Log material not found
            static std::unordered_set<std::string> loggedNotFound;
            if (loggedNotFound.find(matName) == loggedNotFound.end()) {
                LOG_WARNING("BSPTree", "DrawFace: Material '" + matName + "' not found in MaterialLibrary");
                loggedNotFound.insert(matName);
            }
            // Default white color
            shader.SetVec3("u_Color", Vec3(1.0f, 1.0f, 1.0f));
        }
    } else {
        shader.SetVec3("u_Color", Vec3(1.0f, 1.0f, 1.0f));
    }
    
    // Set hasDiffuseTexture uniform
    shader.SetInt("hasDiffuseTexture", hasTexture ? 1 : 0);
    
    // Bind normal map if available (texture unit 2)
    bool hasNormalMap = false;
    if (face.materialIndex < m_materials.size()) {
        const std::string& matName = m_materials[face.materialIndex].name;
        MaterialPtr mat = MaterialLibrary::Instance().Get(matName);
        if (mat) {
            const TextureSlot* normalSlot = mat->GetTextureSlot("u_NormalTexture");
            if (normalSlot && normalSlot->texture) {
                normalSlot->texture->Bind(2);
                hasNormalMap = true;
            }
        }
    }
    shader.SetInt("hasNormalMap", hasNormalMap ? 1 : 0);

    // Enable blending for glass materials - texture alpha controls transparency
    Shader* activeShader = &shader;
    bool usingGlassRealShader = false;
    
    if (isGlassReal) {
        // Load glass_real shader on demand
        if (!m_glassRealShaderLoaded) {
            m_glassRealShader = std::make_shared<Shader>();
#ifdef ASSETS_DIR
            std::string vertPath = std::string(ASSETS_DIR) + "/shaders/glass_real.vert";
            std::string fragPath = std::string(ASSETS_DIR) + "/shaders/glass_real.frag";
#else
            std::string vertPath = "assets/shaders/glass_real.vert";
            std::string fragPath = "assets/shaders/glass_real.frag";
#endif
            m_glassRealShader->LoadFromFiles(vertPath, fragPath);
            m_glassRealShaderLoaded = true;
            if (m_glassRealShader->IsValid()) {
                LOG_INFO("BSPTree", "Loaded glass_real shader");
            } else {
                LOG_ERROR("BSPTree", "Failed to load glass_real shader");
            }
        }
        
        if (m_glassRealShader && m_glassRealShader->IsValid()) {
            m_glassRealShader->Bind();
            activeShader = m_glassRealShader.get();
            usingGlassRealShader = true;
            
            // Set matrices
            activeShader->SetMat4("u_Projection", m_currentProjMatrix);
            activeShader->SetMat4("u_View", m_currentViewMatrix);
            activeShader->SetMat4("u_Model", Mat4(1.0f));
            
            // Set glass_real properties
            // Get values from material if available, otherwise use realistic defaults
            MaterialPtr mat = MaterialLibrary::Instance().Get(m_materials[face.materialIndex].name);
            Vec3 glassTint = Vec3(0.92f, 0.96f, 1.0f);  // Visible blue tint
            float ior = 1.45f;           // Standard glass IOR
            float thickness = 0.5f;      // Higher thickness for visible effect
            float fresnelPower = 5.0f;   // Standard Schlick
            float absorption = 0.5f;     // Subtle absorption
            float roughness = 0.0f;      // Clear glass by default
            float alpha = 0.6f;          // Semi-transparent (user requested 0.6-0.85)
            
            if (mat) {
                glassTint = mat->GetVec3("u_GlassTint", glassTint);
                ior = mat->GetFloat("u_IOR", ior);
                thickness = mat->GetFloat("u_Thickness", thickness);
                fresnelPower = mat->GetFloat("u_FresnelPower", fresnelPower);
                absorption = mat->GetFloat("u_Absorption", absorption);
                roughness = mat->GetFloat("u_Roughness", roughness);
                alpha = mat->GetFloat("u_Alpha", alpha);
            }
            
            activeShader->SetVec3("u_GlassTint", glassTint);
            activeShader->SetFloat("u_IOR", ior);
            activeShader->SetFloat("u_Thickness", thickness);
            activeShader->SetFloat("u_FresnelPower", fresnelPower);
            activeShader->SetFloat("u_Absorption", absorption);
            activeShader->SetFloat("u_Roughness", roughness);
            activeShader->SetFloat("u_Alpha", alpha);
            
            // Environment info
            activeShader->SetVec3("u_LightDir", Vec3(-0.5f, -1.0f, -0.3f));
            activeShader->SetVec3("u_LightColor", Vec3(1.0f, 0.95f, 0.9f));
            activeShader->SetVec3("u_AmbientColor", Vec3(0.3f, 0.35f, 0.4f));
            
            // Texture uniforms
            activeShader->SetInt("hasDiffuseTexture", hasTexture ? 1 : 0);
            activeShader->SetInt("diffuseTexture", 0);
            
            // Scene color texture for refraction (bind to texture unit 2)
            if (m_sceneColorTexture != 0) {
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, m_sceneColorTexture);
                activeShader->SetInt("sceneColor", 2);
                activeShader->SetVec2("screenSize", Vec2(static_cast<float>(m_sceneColorWidth), 
                                                          static_cast<float>(m_sceneColorHeight)));
                glActiveTexture(GL_TEXTURE0);  // Reset to default
            }
            
            // Noise texture (not available, set to 0)
            activeShader->SetInt("hasNoiseTexture", 0);
        }
    }
    
    if (isGlass) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);    // Don't write to depth buffer (critical for glass)
        glDisable(GL_CULL_FACE);  // Render both sides of glass
        
        if (!usingGlassRealShader) {
            shader.SetInt("hasLightmap", 0);   // Glass is unlit (bright)
        }
    }

    // Draw the face
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, face.numIndices, GL_UNSIGNED_INT,
                   (void*)(face.firstIndex * sizeof(uint32_t)));
    glBindVertexArray(0);
    
    // Restore state after glass
    if (isGlass) {
        if (!usingGlassRealShader) {
            shader.SetInt("hasLightmap", 1);  // Restore for next face
        }
        glDepthMask(GL_TRUE);     // Re-enable depth writes
        glEnable(GL_CULL_FACE);   // Re-enable backface culling
        glDisable(GL_BLEND);
    }
    
    // Switch back to original shader after glass_real
    if (usingGlassRealShader) {
        shader.Bind();
    }
}

// ============================================================================
// Statistics
// ============================================================================

BSPStats BSPTree::GetStats() const {
    BSPStats stats;
    stats.numPlanes = static_cast<uint32_t>(m_planes.size());
    stats.numNodes = static_cast<uint32_t>(m_nodes.size());
    stats.numLeafs = static_cast<uint32_t>(m_leafs.size());
    stats.numFaces = static_cast<uint32_t>(m_faces.size());
    stats.numVertices = static_cast<uint32_t>(m_vertices.size());
    stats.numIndices = static_cast<uint32_t>(m_indices.size());
    stats.numBrushes = static_cast<uint32_t>(m_brushRefs.size());
    stats.numMaterials = static_cast<uint32_t>(m_materials.size());

    // Calculate max depth
    std::function<uint32_t(int32_t, uint32_t)> calcDepth = [&](int32_t nodeIdx, uint32_t depth) -> uint32_t {
        if (nodeIdx < 0 || nodeIdx >= static_cast<int32_t>(m_nodes.size())) {
            return depth;
        }

        const BSPNode& node = m_nodes[nodeIdx];
        uint32_t frontDepth = depth;
        uint32_t backDepth = depth;

        if (!node.IsFrontLeaf()) {
            frontDepth = calcDepth(node.frontChild, depth + 1);
        }
        if (!node.IsBackLeaf()) {
            backDepth = calcDepth(node.backChild, depth + 1);
        }

        return std::max(frontDepth, backDepth);
    };

    if (!m_nodes.empty()) {
        stats.maxTreeDepth = calcDepth(0, 1);
    }

    return stats;
}

// ============================================================================
// Collision (Phase 2)
// ============================================================================

TraceResult BSPTree::TraceCapsule(const Vec3& start, const Vec3& end,
                                   float radius, float halfHeight) const {
    CollisionCapsule capsule(radius, halfHeight);
    return m_collision.TraceCapsule(start, end, capsule);
}

Vec3 BSPTree::SlideMove(const Vec3& start, const Vec3& velocity, float deltaTime,
                         float radius, float halfHeight, Vec3& outVelocity) const {
    CollisionCapsule capsule(radius, halfHeight);
    return m_collision.SlideMove(start, velocity, deltaTime, capsule, outVelocity);
}

// ============================================================================
// PVS Rendering (Phase 3)
// ============================================================================

void BSPTree::RenderWithPVS(const FPSCamera& camera, Shader& shader) {
    if (!m_gpuReady) {
        RenderAll(shader);
        return;
    }

    m_lastFrameFaces = 0;
    m_lastFrameLeafs = 0;
    m_lastFrameNodes = 0;

    Vec3 camPos = camera.GetPosition();

    // Find which leaf the camera is in
    int32_t cameraLeaf = FindLeaf(camPos);
    
    if (cameraLeaf < 0 || !m_pvs.IsBuilt()) {
        // No valid leaf or no PVS - fall back to regular rendering
        Render(camera, shader);
        return;
    }

    // Get list of visible leafs from PVS
    const auto& visibleLeafs = m_pvs.GetVisibleLeafs(static_cast<uint32_t>(cameraLeaf));

    // Render only the visible leafs
    for (uint32_t leafIndex : visibleLeafs) {
        DrawLeaf(leafIndex, shader);
    }
}

// ============================================================================
// OPTIMIZED RENDERING FUNCTIONS
// ============================================================================

void BSPTree::BuildLeafFaceLookup() {
    // Build a direct lookup: each leaf -> contiguous index range
    // This enables O(1) access to each leaf's geometry
    
    m_leafIndexRanges.clear();
    m_leafIndexRanges.resize(m_leafs.size());
    
    // For each leaf, find its faces in the reordered index buffer
    // Since BuildMaterialBatches reordered indices, we need to track where each leaf's
    // geometry ended up. For now, we'll just compute bounds.
    
    // Simpler approach: each leaf has direct index ranges via firstFace/numFaces
    // pointing into m_leafFaces -> m_faces -> indices
    // We don't need to recompute - just use this existing structure
    
    for (uint32_t i = 0; i < m_leafs.size(); i++) {
        const BSPLeaf& leaf = m_leafs[i];
        // Count total indices for this leaf
        uint32_t totalIndices = 0;
        for (uint32_t f = 0; f < leaf.numFaces; f++) {
            uint32_t faceIdx = m_leafFaces[leaf.firstFace + f];
            if (faceIdx < m_faces.size()) {
                totalIndices += m_faces[faceIdx].numIndices;
            }
        }
        m_leafIndexRanges[i].indexCount = totalIndices;
        m_leafIndexRanges[i].firstIndex = 0; // Will be set if we rebuild per-leaf
    }
    
    m_batchedIndicesBuilt = true;
    LOG_DEBUG("BSPTree", "Built leaf index ranges for " + std::to_string(m_leafs.size()) + " leaves");
}

void BSPTree::RenderBatched(Shader& shader) {
    // Render using pre-built material batches - ONE draw call per material
    if (!m_gpuReady || m_materialBatches.empty()) {
        RenderAll(shader);
        return;
    }
    
    shader.Bind();
    glBindVertexArray(m_vao);
    
    uint32_t totalDrawn = 0;
    
    for (const auto& batch : m_materialBatches) {
        if (batch.indexCount == 0) continue;
        
        // Set material color (cached, no lookup overhead)
        shader.SetVec3("u_BaseColor", batch.cachedColor);
        
        // Single draw call for all geometry with this material
        glDrawElements(GL_TRIANGLES,
                      static_cast<GLsizei>(batch.indexCount),
                      GL_UNSIGNED_INT,
                      reinterpret_cast<void*>(batch.firstIndex * sizeof(uint32_t)));
        
        totalDrawn += batch.indexCount;
    }
    
    glBindVertexArray(0);
    m_lastFrameFaces = totalDrawn / 3;
}

void BSPTree::RenderBatchedWithVisibility(Shader& shader, const std::vector<bool>& leafVisibility) {
    // Render only visible leaves using glMultiDrawElements
    if (!m_gpuReady) {
        RenderAll(shader);
        return;
    }
    
    // Collect draw commands for visible leaves
    static std::vector<GLsizei> counts;
    static std::vector<const void*> offsets;
    counts.clear();
    offsets.clear();
    
    uint32_t visibleLeafCount = 0;
    
    for (uint32_t i = 0; i < m_leafs.size() && i < leafVisibility.size(); i++) {
        if (!leafVisibility[i]) continue;
        
        const BSPLeaf& leaf = m_leafs[i];
        if (leaf.numFaces == 0) continue;
        
        visibleLeafCount++;
        
        // Add all faces from this leaf
        for (uint32_t f = 0; f < leaf.numFaces; f++) {
            uint32_t faceIdx = m_leafFaces[leaf.firstFace + f];
            if (faceIdx < m_faces.size()) {
                const BSPFace& face = m_faces[faceIdx];
                if (face.numIndices > 0) {
                    counts.push_back(static_cast<GLsizei>(face.numIndices));
                    offsets.push_back(reinterpret_cast<const void*>(face.firstIndex * sizeof(uint32_t)));
                }
            }
        }
    }
    
    m_lastFrameLeafs = visibleLeafCount;
    
    if (counts.empty()) return;
    
    shader.Bind();
    glBindVertexArray(m_vao);
    
    glMultiDrawElements(GL_TRIANGLES,
                        counts.data(),
                        GL_UNSIGNED_INT,
                        offsets.data(),
                        static_cast<GLsizei>(counts.size()));
    
    glBindVertexArray(0);
    m_lastFrameFaces = static_cast<uint32_t>(counts.size());
}

// ============================================================================
// COMBINED FRUSTUM + PVS OPTIMIZED RENDERING
// ============================================================================

void BSPTree::BuildOptimizedLeafData() {
    if (m_optimizedDataBuilt) return;
    
    m_optimizedLeafData.clear();
    m_optimizedLeafData.resize(m_leafs.size());
    
    for (uint32_t i = 0; i < m_leafs.size(); ++i) {
        const BSPLeaf& leaf = m_leafs[i];
        OptimizedLeafData& data = m_optimizedLeafData[i];
        
        // Calculate bounding sphere from AABB
        data.boundsCenter = (leaf.boundsMin + leaf.boundsMax) * 0.5f;
        Vec3 halfExtent = (leaf.boundsMax - leaf.boundsMin) * 0.5f;
        data.boundsRadius = glm::length(halfExtent);
        
        // Calculate index range for this leaf
        data.firstIndex = 0;  // Will be set per-draw from face data
        data.indexCount = 0;
        
        for (uint32_t f = 0; f < leaf.numFaces; ++f) {
            uint32_t faceIdx = m_leafFaces[leaf.firstFace + f];
            if (faceIdx < m_faces.size()) {
                data.indexCount += m_faces[faceIdx].numIndices;
            }
        }
    }
    
    m_optimizedDataBuilt = true;
    LOG_INFO("BSPTree", "Built optimized leaf data for " + std::to_string(m_leafs.size()) + " leaves");
}

void BSPTree::RenderOptimized(const FPSCamera& camera, Shader& shader) {
    if (!m_gpuReady) {
        if (!m_vertices.empty() && !m_indices.empty()) {
            UploadGeometry();
            m_gpuReady = true;
        } else {
            return;
        }
    }
    
    // Ensure optimized data is built
    if (!m_optimizedDataBuilt) {
        BuildOptimizedLeafData();
    }
    
    m_lastFrameFaces = 0;
    m_lastFrameLeafs = 0;
    m_lastFrameNodes = 0;
    
    Vec3 camPos = camera.GetPosition();
    
    // 1. Find camera leaf
    int32_t cameraLeaf = FindLeaf(camPos);
    
    // 2. Get PVS visible leafs (or all if no PVS)
    const std::vector<uint32_t>* visibleLeafsPtr = nullptr;
    std::vector<uint32_t> allLeafs;
    
    if (cameraLeaf >= 0 && m_pvs.IsBuilt()) {
        visibleLeafsPtr = &m_pvs.GetVisibleLeafs(static_cast<uint32_t>(cameraLeaf));
    } else {
        // Fallback: all leafs with faces
        allLeafs.reserve(m_leafs.size());
        for (uint32_t i = 0; i < m_leafs.size(); ++i) {
            if (m_leafs[i].numFaces > 0) {
                allLeafs.push_back(i);
            }
        }
        visibleLeafsPtr = &allLeafs;
    }
    
    const std::vector<uint32_t>& visibleLeafs = *visibleLeafsPtr;
    
    // 3. Combined frustum test + distance sorting for front-to-back
    m_sortedLeafsBuffer.clear();
    m_sortedLeafsBuffer.reserve(visibleLeafs.size());
    
    // Build frustum from camera matrices
    Frustum frustum;
    frustum.Update(camera.GetProjectionMatrix() * camera.GetViewMatrix());
    
    for (uint32_t leafIdx : visibleLeafs) {
        if (leafIdx >= m_optimizedLeafData.size()) continue;
        
        const OptimizedLeafData& data = m_optimizedLeafData[leafIdx];
        if (data.indexCount == 0) continue;
        
        // Use faster bounding sphere test (cheaper than AABB)
        if (frustum.IsSphereVisible(data.boundsCenter, data.boundsRadius)) {
            float distSq = glm::distance2(camPos, data.boundsCenter);
            m_sortedLeafsBuffer.emplace_back(distSq, leafIdx);
        }
    }
    
    // 4. Sort front-to-back (enables early-Z rejection on GPU)
    std::sort(m_sortedLeafsBuffer.begin(), m_sortedLeafsBuffer.end());
    
    // 5. Render using batched draw calls
    RenderLeafsBatched(m_sortedLeafsBuffer, shader);
}

void BSPTree::RenderLeafsBatched(const std::vector<std::pair<float, uint32_t>>& sortedLeafs, Shader& shader) {
    if (sortedLeafs.empty()) return;
    
    // Prepare draw commands
    m_drawCountsBuffer.clear();
    m_drawOffsetsBuffer.clear();
    m_drawCountsBuffer.reserve(sortedLeafs.size() * 4);  // Estimate
    m_drawOffsetsBuffer.reserve(sortedLeafs.size() * 4);
    
    uint32_t triangleCount = 0;
    
    for (const auto& [distSq, leafIdx] : sortedLeafs) {
        const BSPLeaf& leaf = m_leafs[leafIdx];
        
        // Add all faces from this leaf
        for (uint32_t f = 0; f < leaf.numFaces; ++f) {
            uint32_t faceIdx = m_leafFaces[leaf.firstFace + f];
            if (faceIdx < m_faces.size()) {
                const BSPFace& face = m_faces[faceIdx];
                if (face.numIndices > 0) {
                    m_drawCountsBuffer.push_back(static_cast<GLsizei>(face.numIndices));
                    m_drawOffsetsBuffer.push_back(
                        reinterpret_cast<const void*>(face.firstIndex * sizeof(uint32_t)));
                    triangleCount += face.numIndices / 3;
                }
            }
        }
    }
    
    if (m_drawCountsBuffer.empty()) return;
    
    m_lastFrameLeafs = static_cast<uint32_t>(sortedLeafs.size());
    m_lastFrameFaces = triangleCount;
    
    // Single batched draw call using glMultiDrawElements
    shader.Bind();
    glBindVertexArray(m_vao);
    
    glMultiDrawElements(GL_TRIANGLES,
                        m_drawCountsBuffer.data(),
                        GL_UNSIGNED_INT,
                        m_drawOffsetsBuffer.data(),
                        static_cast<GLsizei>(m_drawCountsBuffer.size()));
    
    glBindVertexArray(0);
}

} // namespace Genesis

