#pragma once

#include "math/Math.h"
#include "renderer/mesh/Mesh.h"
#include "renderer/material/Material.h"
#include "physics/Collider.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace Genesis {

// Forward declarations
class FPSCamera;

// ============================================================================
// Static Object Types - For categorization and culling
// ============================================================================
enum class StaticObjectType {
    Generic,        // Uncategorized static object
    Floor,          // Floor geometry
    Ceiling,        // Ceiling geometry
    Wall,           // Wall geometry
    Prop,           // Static prop (furniture, decorations) - may or may not have collision
    PropPhysics,    // Physics prop - has collision
    PropDecorative, // Decorative prop - NO collision (like Source's prop_static)
    Structural      // Pillars, beams, etc.
};

// ============================================================================
// Static Object - Represents a single static world object
//
// DECOUPLED DESIGN (Source Engine style):
// - mesh/material: Visual representation only
// - collider: Optional collision shape (can be nullptr for decorative props)
// - The collision shape can be DIFFERENT from the visual mesh
// ============================================================================
struct StaticObject {
    // Visual (Render)
    MeshPtr mesh = nullptr;
    MaterialPtr material = nullptr;
    Mat4 transform = Mat4(1.0f);

    // Collision (Physics) - DECOUPLED from visual
    ColliderPtr collider = nullptr;  // nullptr = no collision (decorative)

    // Metadata
    std::string name;
    StaticObjectType type = StaticObjectType::Generic;
    bool visible = true;
    bool castShadow = true;

    // Bounding box in world space (for culling)
    Vec3 worldBoundsMin = Vec3(0.0f);
    Vec3 worldBoundsMax = Vec3(0.0f);

    // Layer for visibility grouping (e.g., interior vs exterior)
    uint32_t layer = 0;

    StaticObject() = default;

    // Constructor with mesh + material (no collision - decorative)
    StaticObject(MeshPtr m, MaterialPtr mat, const Mat4& t = Mat4(1.0f))
        : mesh(std::move(m)), material(std::move(mat)), transform(t), collider(nullptr) {
        UpdateWorldBounds();
    }

    // Constructor with mesh + material + collider (has collision)
    StaticObject(MeshPtr m, MaterialPtr mat, ColliderPtr col, const Mat4& t = Mat4(1.0f))
        : mesh(std::move(m)), material(std::move(mat)), transform(t), collider(std::move(col)) {
        UpdateWorldBounds();
    }

    // Update world bounds from mesh bounds and transform
    void UpdateWorldBounds();

    // Check if object is valid for rendering
    bool IsValid() const { return mesh != nullptr && material != nullptr && visible; }

    // Check if object has collision
    bool HasCollision() const { return collider != nullptr; }

    // Get collision AABB in world space
    AABB GetCollisionAABB() const {
        if (collider) {
            return collider->GetWorldAABB(transform);
        }
        return AABB{worldBoundsMin, worldBoundsMax};
    }
};

// ============================================================================
// Render Batch - Groups objects by material for efficient rendering
// ============================================================================
struct RenderBatch {
    MaterialPtr material;
    std::vector<const StaticObject*> objects;
};

// ============================================================================
// Static World Renderer - Renders all static world geometry
//
// DECOUPLED COLLISION (Source Engine style):
// - Visual mesh and collision shape are separate
// - Objects can have collision (walls, floors) or be decorative (props)
// - Collision shapes can be simpler than visual meshes for performance
//
// Usage:
//   auto& world = StaticWorldRenderer::Instance();
//
//   // WITH collision (floors, walls, platforms)
//   world.AddFloor(floorMesh, floorMaterial, floorTransform);  // Auto-generates box collider
//   world.AddWall(wallMesh, wallMaterial, wallTransform);      // Auto-generates box collider
//
//   // WITHOUT collision (decorative props)
//   world.AddPropDecorative("Vase", vaseMesh, vaseMaterial, vaseTransform);
//
//   // WITH custom collider (different from visual)
//   auto simpleBox = BoxCollider::FromSize(1.0f, 2.0f, 1.0f);
//   world.AddProp("Chair", chairMesh, chairMaterial, simpleBox, chairTransform);
//
//   // In render loop:
//   world.Render(camera);
//
//   // Get all collision shapes for physics:
//   auto& colliders = world.GetCollisionObjects();
// ============================================================================
class StaticWorldRenderer {
public:
    static StaticWorldRenderer& Instance() {
        static StaticWorldRenderer instance;
        return instance;
    }

    // ========================================================================
    // Object Management - WITH auto-generated collision
    // ========================================================================

    // Add a generic static object
    size_t Add(const StaticObject& obj);
    size_t Add(MeshPtr mesh, MaterialPtr material, const Mat4& transform = Mat4(1.0f));

    // Add with explicit collider
    size_t Add(MeshPtr mesh, MaterialPtr material, ColliderPtr collider, const Mat4& transform = Mat4(1.0f));

    // Convenience methods - these AUTO-GENERATE box colliders from transform scale
    size_t AddFloor(MeshPtr mesh, MaterialPtr material, const Mat4& transform = Mat4(1.0f));
    size_t AddCeiling(MeshPtr mesh, MaterialPtr material, const Mat4& transform = Mat4(1.0f));
    size_t AddWall(MeshPtr mesh, MaterialPtr material, const Mat4& transform = Mat4(1.0f));
    size_t AddStructural(MeshPtr mesh, MaterialPtr material, const Mat4& transform = Mat4(1.0f));

    // ========================================================================
    // Prop Management - Decorative vs Physics props
    // ========================================================================

    // Prop WITH collision (auto-generates box collider)
    size_t AddProp(const std::string& name, MeshPtr mesh, MaterialPtr material, const Mat4& transform = Mat4(1.0f));

    // Prop WITH custom collider
    size_t AddProp(const std::string& name, MeshPtr mesh, MaterialPtr material, ColliderPtr collider, const Mat4& transform = Mat4(1.0f));

    // Prop WITHOUT collision (purely decorative - like Source's prop_static with no collision)
    size_t AddPropDecorative(const std::string& name, MeshPtr mesh, MaterialPtr material, const Mat4& transform = Mat4(1.0f));

    // Bulk operations
    void AddMany(const std::vector<StaticObject>& objects);

    // Remove object by index
    void Remove(size_t index);

    // Clear all objects
    void Clear();

    // Get object by index
    StaticObject* Get(size_t index);
    const StaticObject* Get(size_t index) const;

    // Get all objects
    const std::vector<StaticObject>& GetObjects() const { return m_objects; }
    std::vector<StaticObject>& GetObjects() { return m_objects; }

    // Get object count
    size_t GetObjectCount() const { return m_objects.size(); }

    // ========================================================================
    // Visibility Control
    // ========================================================================

    // Set visibility of specific object
    void SetVisible(size_t index, bool visible);

    // Set visibility by type
    void SetTypeVisible(StaticObjectType type, bool visible);

    // Set visibility by layer
    void SetLayerVisible(uint32_t layer, bool visible);

    // Hide/show all
    void ShowAll();
    void HideAll();

    // ========================================================================
    // Rendering
    // ========================================================================

    // Render all visible static objects
    void Render(const FPSCamera& camera);

    // Render only specific types
    void RenderType(const FPSCamera& camera, StaticObjectType type);

    // Render only specific layer
    void RenderLayer(const FPSCamera& camera, uint32_t layer);

    // ========================================================================
    // Batching Control
    // ========================================================================

    // Rebuild render batches (call after adding/removing many objects)
    void RebuildBatches();

    // Enable/disable automatic batch rebuilding
    void SetAutoBatching(bool enabled) { m_autoBatching = enabled; }
    bool IsAutoBatching() const { return m_autoBatching; }

    // ========================================================================
    // Lighting (Global for all static objects)
    // ========================================================================

    void SetDirectionalLight(const Vec3& direction, const Vec3& color, float intensity = 1.0f);
    void SetAmbientLight(const Vec3& color, float intensity = 1.0f);

    // ========================================================================
    // Statistics
    // ========================================================================

    uint32_t GetDrawCalls() const { return m_drawCalls; }
    uint32_t GetMaterialSwitches() const { return m_materialSwitches; }
    uint32_t GetTrianglesRendered() const { return m_trianglesRendered; }
    uint32_t GetVerticesRendered() const { return m_verticesRendered; }
    uint32_t GetObjectsRendered() const { return m_objectsRendered; }
    uint32_t GetObjectsCulled() const { return m_objectsCulled; }

    void ResetStats();

    // ========================================================================
    // Collision Queries - Get collision data for physics
    // ========================================================================

    // Get number of objects with collision
    size_t GetCollisionObjectCount() const;

    // Get all objects that have collision (for physics system)
    std::vector<const StaticObject*> GetCollisionObjects() const;

    // Get all collision AABBs (for legacy WorldCollision integration)
    std::vector<AABB> GetCollisionAABBs() const;

    // Query: Does any collider contain this point?
    bool PointInAnyCollider(const Vec3& point) const;

    // Query: Get all colliders overlapping an AABB
    std::vector<const StaticObject*> QueryAABB(const AABB& aabb) const;

    // ========================================================================
    // Debug
    // ========================================================================

    void PrintDebugInfo() const;

private:
    StaticWorldRenderer();
    ~StaticWorldRenderer() = default;
    StaticWorldRenderer(const StaticWorldRenderer&) = delete;
    StaticWorldRenderer& operator=(const StaticWorldRenderer&) = delete;

    // Internal rendering
    void RenderObject(const StaticObject& obj, Shader& shader);
    void UploadGlobalUniforms(Shader& shader, const FPSCamera& camera);

    // Batching
    void BuildBatches();

    // Helper: Create a box collider from transform (extracts scale)
    static ColliderPtr CreateBoxColliderFromTransform(const Mat4& transform);

private:
    // All static objects
    std::vector<StaticObject> m_objects;

    // Render batches (grouped by material)
    std::vector<RenderBatch> m_batches;
    bool m_batchesDirty = true;
    bool m_autoBatching = true;

    // Lighting
    Vec3 m_lightDirection = Vec3(0.5f, 1.0f, 0.3f);
    Vec3 m_lightColor = Vec3(1.0f, 0.98f, 0.95f);
    float m_lightIntensity = 1.0f;
    Vec3 m_ambientColor = Vec3(0.15f, 0.15f, 0.2f);
    float m_ambientIntensity = 1.0f;

    // Layer visibility
    std::unordered_map<uint32_t, bool> m_layerVisibility;

    // Statistics
    uint32_t m_drawCalls = 0;
    uint32_t m_materialSwitches = 0;
    uint32_t m_trianglesRendered = 0;
    uint32_t m_verticesRendered = 0;
    uint32_t m_objectsRendered = 0;
    uint32_t m_objectsCulled = 0;
};

} // namespace Genesis

