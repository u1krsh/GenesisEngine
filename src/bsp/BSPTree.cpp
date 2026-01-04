#include "BSPTree.h"
#include "camera/Camera.h"
#include "renderer/shader/Shader.h"
#include "core/Logger.h"
#include <glad/glad.h>
#include <iostream>
#include <algorithm>

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
    m_gpuReady = false;
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

    // Upload index data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(uint32_t),
                 m_indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void BSPTree::BuildMaterialBatches() {
    m_materialBatches.clear();

    if (m_faces.empty()) return;

    // Sort faces by material
    std::vector<size_t> sortedFaces(m_faces.size());
    for (size_t i = 0; i < m_faces.size(); i++) {
        sortedFaces[i] = i;
    }

    std::sort(sortedFaces.begin(), sortedFaces.end(),
              [this](size_t a, size_t b) {
                  return m_faces[a].materialIndex < m_faces[b].materialIndex;
              });

    // Build batches
    // For Phase 1, we'll just batch all faces together
    // A more sophisticated approach would reorder indices

    uint32_t currentMaterial = UINT32_MAX;
    for (size_t faceIdx : sortedFaces) {
        const BSPFace& face = m_faces[faceIdx];

        if (face.materialIndex != currentMaterial) {
            currentMaterial = face.materialIndex;

            MaterialBatch batch;
            batch.materialIndex = currentMaterial;

            // Get material from library
            if (currentMaterial < m_materials.size()) {
                batch.material = MaterialLibrary::Instance().Get(m_materials[currentMaterial].name);
            }
            if (!batch.material) {
                batch.material = MaterialLibrary::Instance().Get("default");
            }

            batch.firstIndex = 0;
            batch.indexCount = 0;
            m_materialBatches.push_back(batch);
        }
    }

    LOG_DEBUG("BSPTree", "Built " + std::to_string(m_materialBatches.size()) + " material batches");
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

    // Phase 1: Simple tree traversal
    // Draw nodes front-to-back based on camera position
    if (m_rootNode >= 0 && !m_nodes.empty()) {
        // Root is a node - traverse the tree
        DrawNode(m_rootNode, camPos, shader);
    } else if (m_rootNode < 0) {
        // Root is a leaf (all geometry in one leaf)
        uint32_t leafIdx = static_cast<uint32_t>(-(m_rootNode + 1));
        DrawLeaf(leafIdx, shader);
    } else if (!m_leafs.empty()) {
        // Fallback: no tree structure, just draw all leafs
        for (uint32_t i = 0; i < m_leafs.size(); i++) {
            DrawLeaf(i, shader);
        }
    }
}

void BSPTree::RenderWithCulling(const FPSCamera& camera, Shader& shader) {
    // Phase 1.5: Add frustum culling
    // For now, same as Render()
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
    if (nodeIndex < 0 || nodeIndex >= static_cast<int32_t>(m_nodes.size())) {
        return;
    }

    m_lastFrameNodes++;

    const BSPNode& node = m_nodes[nodeIndex];
    const BSPPlane& plane = m_planes[node.planeIndex];

    float dist = plane.ClassifyPoint(cameraPos);

    // Draw front-to-back for better z-buffer utilization
    if (dist >= 0) {
        // Camera is in front of plane
        // Draw front first, then back
        if (node.IsFrontLeaf()) {
            DrawLeaf(node.GetFrontLeafIndex(), shader);
        } else {
            DrawNode(node.frontChild, cameraPos, shader);
        }

        if (node.IsBackLeaf()) {
            DrawLeaf(node.GetBackLeafIndex(), shader);
        } else {
            DrawNode(node.backChild, cameraPos, shader);
        }
    } else {
        // Camera is behind plane
        // Draw back first, then front
        if (node.IsBackLeaf()) {
            DrawLeaf(node.GetBackLeafIndex(), shader);
        } else {
            DrawNode(node.backChild, cameraPos, shader);
        }

        if (node.IsFrontLeaf()) {
            DrawLeaf(node.GetFrontLeafIndex(), shader);
        } else {
            DrawNode(node.frontChild, cameraPos, shader);
        }
    }
}

void BSPTree::DrawLeaf(uint32_t leafIndex, Shader& shader) {
    if (leafIndex >= m_leafs.size()) return;

    m_lastFrameLeafs++;

    const BSPLeaf& leaf = m_leafs[leafIndex];

    // Skip empty/solid leafs with no visible faces
    if (leaf.numFaces == 0) return;

    // Draw all faces in this leaf
    for (uint32_t i = 0; i < leaf.numFaces; i++) {
        uint32_t faceIdx = m_leafFaces[leaf.firstFace + i];
        if (faceIdx < m_faces.size()) {
            DrawFace(m_faces[faceIdx], shader);
        }
    }
}

void BSPTree::DrawFace(const BSPFace& face, Shader& shader) {
    if (face.numIndices == 0) return;

    m_lastFrameFaces++;

    // Set material color on the shader (don't call mat->Bind() as it would change the shader!)
    if (face.materialIndex < m_materials.size()) {
        MaterialPtr mat = MaterialLibrary::Instance().Get(m_materials[face.materialIndex].name);
        if (mat) {
            // Get the color from the material and set it on the BSP shader
            Vec3 color = mat->GetVec3("u_Color", Vec3(0.5f, 0.5f, 0.5f));
            shader.SetVec3("u_Color", color);
        } else {
            // Default gray color
            shader.SetVec3("u_Color", Vec3(0.5f, 0.5f, 0.5f));
        }
    } else {
        shader.SetVec3("u_Color", Vec3(0.5f, 0.5f, 0.5f));
    }

    // Draw the face
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, face.numIndices, GL_UNSIGNED_INT,
                   (void*)(face.firstIndex * sizeof(uint32_t)));
    glBindVertexArray(0);
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

} // namespace Genesis

