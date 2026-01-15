#pragma once

#include "BSPTypes.h"
#include "BSPCollision.h"
#include "BSPPVS.h"
#include "LightmapAtlas.h"
#include "renderer/mesh/Mesh.h"
#include "renderer/material/Material.h"
#include "renderer/material/MaterialLibrary.h"
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

namespace Genesis {

// Forward declarations
// Forward declarations
class FPSCamera;
class Shader;
class Frustum;

// ============================================================================
// BSP Tree - The compiled BSP structure for a map
//
// Phase 1: Rendering only
// - Load/compile from brushes
// - Traverse tree from camera position
// - Render visible faces
//
// Instead of:
//   for (auto& mesh : worldMeshes)
//       mesh.Draw();
//
// We do:
//   bspTree.Render(camera);  // which calls DrawNode(rootNode)
//
// ============================================================================
class BSPTree {
public:
    BSPTree() = default;
    ~BSPTree();

    // ========================================================================
    // Tree Data Access
    // ========================================================================

    const std::vector<BSPPlane>& GetPlanes() const { return m_planes; }
    const std::vector<BSPNode>& GetNodes() const { return m_nodes; }
    const std::vector<BSPLeaf>& GetLeafs() const { return m_leafs; }
    const std::vector<BSPFace>& GetFaces() const { return m_faces; }
    const std::vector<BSPVertex>& GetVertices() const { return m_vertices; }
    const std::vector<uint32_t>& GetIndices() const { return m_indices; }
    const std::vector<BSPMaterial>& GetMaterials() const { return m_materials; }
    const std::vector<StaticLight>& GetLights() const { return m_lights; }

    std::vector<BSPPlane>& GetPlanes() { return m_planes; }
    std::vector<BSPNode>& GetNodes() { return m_nodes; }
    std::vector<BSPLeaf>& GetLeafs() { return m_leafs; }
    std::vector<BSPFace>& GetFaces() { return m_faces; }
    std::vector<BSPVertex>& GetVertices() { return m_vertices; }
    std::vector<uint32_t>& GetIndices() { return m_indices; }
    std::vector<BSPMaterial>& GetMaterials() { return m_materials; }
    std::vector<StaticLight>& GetLights() { return m_lights; }
    
    // Add a static light to the scene
    void AddLight(const StaticLight& light) { m_lights.push_back(light); }
    void ClearLights() { m_lights.clear(); }

    // ========================================================================
    // Tree Queries
    // ========================================================================

    bool IsValid() const { return !m_nodes.empty() || !m_leafs.empty(); }
    bool HasGeometry() const { return !m_faces.empty(); }

    // Find which leaf contains a point
    int32_t FindLeaf(const Vec3& position) const;

    // Get leaf at index
    const BSPLeaf* GetLeaf(uint32_t index) const;

    // Check if point is in solid
    bool IsPointSolid(const Vec3& position) const;

    // ========================================================================
    // Collision (Phase 2)
    // ========================================================================

    // Get collision system
    BSPCollision& GetCollision() { return m_collision; }
    const BSPCollision& GetCollision() const { return m_collision; }

    // Trace a capsule through the BSP tree
    TraceResult TraceCapsule(const Vec3& start, const Vec3& end,
                             float radius, float halfHeight) const;

    // Perform slide move with collision response
    Vec3 SlideMove(const Vec3& start, const Vec3& velocity, float deltaTime,
                   float radius, float halfHeight, Vec3& outVelocity) const;

    // Get all faces (for simple rendering)
    const std::vector<uint32_t>& GetLeafFaces() const { return m_leafFaces; }
    std::vector<uint32_t>& GetLeafFaces() { return m_leafFaces; }

    // Get brush refs
    std::vector<BSPBrushRef>& GetBrushRefs() { return m_brushRefs; }

    // Root node management
    void SetRootNode(int32_t rootIndex) { m_rootNode = rootIndex; }
    int32_t GetRootNode() const { return m_rootNode; }

    // ========================================================================
    // Rendering (Phase 1)
    // ========================================================================

    // Initialize GPU resources
    bool InitializeRendering();

    // Render the entire BSP tree
    void Render(const FPSCamera& camera, Shader& shader);

    // Render with frustum culling (Phase 1.5)
    void RenderWithCulling(const FPSCamera& camera, Shader& shader);

    // Simple render - just draw all faces (for debugging)
    void RenderAll(Shader& shader);

    // Render wireframe overlay (for debugging)
    void RenderWireframe(const FPSCamera& camera, Shader& shader);

    // ========================================================================
    // Post-Process Updates
    // ========================================================================
    
    // Update mesh data on GPU (e.g. after lightmap baking updates UVs)
    void UpdateMesh();

    // ========================================================================
    // PVS (Phase 3) - Potentially Visible Set
    // ========================================================================

    // Get PVS system
    BSPPVS& GetPVS() { return m_pvs; }
    const BSPPVS& GetPVS() const { return m_pvs; }

    // Check if PVS is built
    bool HasPVS() const { return m_pvs.IsBuilt(); }

    // Render using PVS culling (only visible leafs)
    void RenderWithPVS(const FPSCamera& camera, Shader& shader);

    // ========================================================================
    // Optimized Rendering (Combined Frustum + PVS + Batching)
    // ========================================================================

    // Render with combined frustum + PVS culling and front-to-back sorting
    void RenderOptimized(const FPSCamera& camera, Shader& shader);
    
    // Build optimized leaf data (bounding spheres for fast culling)
    void BuildOptimizedLeafData();

    // ========================================================================
    // Statistics
    // ========================================================================

    BSPStats GetStats() const;

    // Rendering stats (per-frame)
    uint32_t GetLastFrameFaceCount() const { return m_lastFrameFaces; }
    uint32_t GetLastFrameLeafCount() const { return m_lastFrameLeafs; }
    uint32_t GetLastFrameNodeCount() const { return m_lastFrameNodes; }

    // ========================================================================
    // Cleanup
    // ========================================================================

    void Clear();
    void ClearGPUResources();

private:
    // Draw a single node (recursive)
    void DrawNode(int32_t nodeIndex, const Vec3& cameraPos, Shader& shader);
    
    // Helper for two-pass rendering
    void RenderBSPPass(const Vec3& camPos, Shader& shader);
    
    // Draw node with frustum culling
    void DrawNodeWithCulling(int32_t nodeIndex, const Vec3& cameraPos, Shader& shader, const Frustum& frustum);

    // Draw a single leaf
    void DrawLeaf(uint32_t leafIndex, Shader& shader);
    void DrawFace(const BSPFace& face, Shader& shader);

    // GPU resource management
    void UploadGeometry();
    void BuildMaterialBatches();
    void BuildLeafFaceLookup();  // Build lookup for fast leaf visibility checks

    // Fast batched rendering (one draw call per material)
    void RenderBatched(Shader& shader);
    void RenderBatchedWithVisibility(Shader& shader, const std::vector<bool>& leafVisibility);
    
    // Render visible leafs using glMultiDrawElements
    void RenderLeafsBatched(const std::vector<std::pair<float, uint32_t>>& sortedLeafs, Shader& shader);

private:
    // BSP data
    std::vector<BSPPlane> m_planes;
    std::vector<BSPNode> m_nodes;
    std::vector<BSPLeaf> m_leafs;
    std::vector<BSPFace> m_faces;
    std::vector<BSPVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<uint32_t> m_leafFaces;    // Face indices per leaf
    std::vector<BSPBrushRef> m_brushRefs;
    std::vector<BSPMaterial> m_materials;
    std::vector<StaticLight> m_lights;    // Static lights for baking (Phase 4D)
    int32_t m_rootNode = 0;               // Root node index (or negative for leaf)

    // Collision system (Phase 2)
    BSPCollision m_collision;

    // PVS system (Phase 3)
    BSPPVS m_pvs;

    // Lightmap atlas (Phase 4B)
    LightmapAtlas m_lightmapAtlas;
    uint32_t m_lightmapTexture = 0;  // OpenGL texture ID
    bool m_lightmapUploaded = false;

public:
    // Lightmap access
    LightmapAtlas& GetLightmapAtlas() { return m_lightmapAtlas; }
    const LightmapAtlas& GetLightmapAtlas() const { return m_lightmapAtlas; }
    
    // Upload lightmap atlas to GPU
    void UploadLightmapTexture();
    
    // Bind lightmap texture to specified unit
    void BindLightmapTexture(uint32_t unit = 1) const;
    
    // Check if lightmap is available
    bool HasLightmap() const { return m_lightmapUploaded; }

private:
    // GPU resources
    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ebo = 0;
    bool m_gpuReady = false;

    // Material batches for efficient rendering
    struct MaterialBatch {
        uint32_t materialIndex;
        MaterialPtr material;
        uint32_t firstIndex;
        uint32_t indexCount;
        Vec3 cachedColor = Vec3(0.5f);  // Pre-cached material color
    };
    std::vector<MaterialBatch> m_materialBatches;

    // Per-leaf index ranges for fast culled rendering
    // Each leaf maps to a range in the batched index buffer
    struct LeafIndexRange {
        uint32_t firstIndex;
        uint32_t indexCount;
    };
    std::vector<LeafIndexRange> m_leafIndexRanges;
    bool m_batchedIndicesBuilt = false;

    // ========================================================================
    // Optimized Rendering Data
    // ========================================================================
    
    // Pre-computed leaf data for fast frustum culling
    struct OptimizedLeafData {
        Vec3 boundsCenter;     // Center of bounding box
        float boundsRadius;    // Bounding sphere radius
        uint32_t firstIndex;   // First index in the index buffer
        uint32_t indexCount;   // Number of indices for this leaf
    };
    std::vector<OptimizedLeafData> m_optimizedLeafData;
    bool m_optimizedDataBuilt = false;
    
    // Reusable buffers for optimized rendering (avoid allocations per frame)
    mutable std::vector<std::pair<float, uint32_t>> m_sortedLeafsBuffer;
    mutable std::vector<GLsizei> m_drawCountsBuffer;
    mutable std::vector<const void*> m_drawOffsetsBuffer;

    // Two-pass rendering for glass transparency
    bool m_renderingGlassPass = false;
    
    // Current camera state (stored during Render() for use in DrawFace)
    mutable Mat4 m_currentViewMatrix;
    mutable Mat4 m_currentProjMatrix;
    mutable Vec3 m_currentCameraPos;
    
    // Glass real shader (loaded on demand)
    mutable std::shared_ptr<Shader> m_glassRealShader;
    mutable bool m_glassRealShaderLoaded = false;

    // Frame stats
    mutable uint32_t m_lastFrameFaces = 0;
    mutable uint32_t m_lastFrameLeafs = 0;
    mutable uint32_t m_lastFrameNodes = 0;
};

using BSPTreePtr = std::shared_ptr<BSPTree>;

} // namespace Genesis

