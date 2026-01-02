#include "StaticWorldRenderer.h"
#include "camera/Camera.h"
#include "renderer/shader/Shader.h"
#include "core/Logger.h"
#include <algorithm>
#include <iostream>

namespace Genesis {

// ============================================================================
// StaticObject Implementation
// ============================================================================

void StaticObject::UpdateWorldBounds() {
    if (!mesh) {
        worldBoundsMin = Vec3(0.0f);
        worldBoundsMax = Vec3(0.0f);
        return;
    }

    // Get local bounds from mesh
    Vec3 localMin = mesh->GetBoundsMin();
    Vec3 localMax = mesh->GetBoundsMax();

    // Transform all 8 corners and find new AABB
    Vec3 corners[8] = {
        Vec3(localMin.x, localMin.y, localMin.z),
        Vec3(localMax.x, localMin.y, localMin.z),
        Vec3(localMin.x, localMax.y, localMin.z),
        Vec3(localMax.x, localMax.y, localMin.z),
        Vec3(localMin.x, localMin.y, localMax.z),
        Vec3(localMax.x, localMin.y, localMax.z),
        Vec3(localMin.x, localMax.y, localMax.z),
        Vec3(localMax.x, localMax.y, localMax.z)
    };

    worldBoundsMin = Vec3(std::numeric_limits<float>::max());
    worldBoundsMax = Vec3(std::numeric_limits<float>::lowest());

    for (const auto& corner : corners) {
        Vec4 transformed = transform * Vec4(corner, 1.0f);
        Vec3 worldCorner(transformed.x, transformed.y, transformed.z);
        worldBoundsMin = glm::min(worldBoundsMin, worldCorner);
        worldBoundsMax = glm::max(worldBoundsMax, worldCorner);
    }
}

// ============================================================================
// StaticWorldRenderer Implementation
// ============================================================================

StaticWorldRenderer::StaticWorldRenderer() {
    m_objects.reserve(1000);  // Pre-allocate for typical scene
    m_batches.reserve(50);    // Typical number of unique materials
}

// ============================================================================
// Helper: Create box collider from transform scale
// ============================================================================

ColliderPtr StaticWorldRenderer::CreateBoxColliderFromTransform(const Mat4& transform) {
    // Extract scale from transform matrix
    Vec3 scale(
        glm::length(Vec3(transform[0])),
        glm::length(Vec3(transform[1])),
        glm::length(Vec3(transform[2]))
    );
    return std::make_shared<BoxCollider>(scale * 0.5f);
}

// ============================================================================
// Object Management
// ============================================================================

size_t StaticWorldRenderer::Add(const StaticObject& obj) {
    m_objects.push_back(obj);
    m_objects.back().UpdateWorldBounds();
    m_batchesDirty = true;
    return m_objects.size() - 1;
}

size_t StaticWorldRenderer::Add(MeshPtr mesh, MaterialPtr material, const Mat4& transform) {
    StaticObject obj;
    obj.mesh = std::move(mesh);
    obj.material = std::move(material);
    obj.transform = transform;
    obj.type = StaticObjectType::Generic;
    obj.collider = nullptr;  // No collision by default for generic add
    return Add(obj);
}

size_t StaticWorldRenderer::Add(MeshPtr mesh, MaterialPtr material, ColliderPtr collider, const Mat4& transform) {
    StaticObject obj;
    obj.mesh = std::move(mesh);
    obj.material = std::move(material);
    obj.transform = transform;
    obj.collider = std::move(collider);
    obj.type = StaticObjectType::Generic;
    return Add(obj);
}

size_t StaticWorldRenderer::AddFloor(MeshPtr mesh, MaterialPtr material, const Mat4& transform) {
    StaticObject obj;
    obj.mesh = std::move(mesh);
    obj.material = std::move(material);
    obj.transform = transform;
    obj.type = StaticObjectType::Floor;
    obj.name = "Floor_" + std::to_string(m_objects.size());
    obj.collider = CreateBoxColliderFromTransform(transform);  // Auto-generate collider
    return Add(obj);
}

size_t StaticWorldRenderer::AddCeiling(MeshPtr mesh, MaterialPtr material, const Mat4& transform) {
    StaticObject obj;
    obj.mesh = std::move(mesh);
    obj.material = std::move(material);
    obj.transform = transform;
    obj.type = StaticObjectType::Ceiling;
    obj.name = "Ceiling_" + std::to_string(m_objects.size());
    obj.collider = CreateBoxColliderFromTransform(transform);  // Auto-generate collider
    return Add(obj);
}

size_t StaticWorldRenderer::AddWall(MeshPtr mesh, MaterialPtr material, const Mat4& transform) {
    StaticObject obj;
    obj.mesh = std::move(mesh);
    obj.material = std::move(material);
    obj.transform = transform;
    obj.type = StaticObjectType::Wall;
    obj.name = "Wall_" + std::to_string(m_objects.size());
    obj.collider = CreateBoxColliderFromTransform(transform);  // Auto-generate collider
    return Add(obj);
}

size_t StaticWorldRenderer::AddProp(const std::string& name, MeshPtr mesh, MaterialPtr material, const Mat4& transform) {
    StaticObject obj;
    obj.mesh = std::move(mesh);
    obj.material = std::move(material);
    obj.transform = transform;
    obj.type = StaticObjectType::Prop;
    obj.name = name;
    obj.collider = CreateBoxColliderFromTransform(transform);  // Auto-generate collider
    return Add(obj);
}

size_t StaticWorldRenderer::AddProp(const std::string& name, MeshPtr mesh, MaterialPtr material, ColliderPtr collider, const Mat4& transform) {
    StaticObject obj;
    obj.mesh = std::move(mesh);
    obj.material = std::move(material);
    obj.transform = transform;
    obj.type = StaticObjectType::PropPhysics;
    obj.name = name;
    obj.collider = std::move(collider);  // Custom collider
    return Add(obj);
}

size_t StaticWorldRenderer::AddPropDecorative(const std::string& name, MeshPtr mesh, MaterialPtr material, const Mat4& transform) {
    StaticObject obj;
    obj.mesh = std::move(mesh);
    obj.material = std::move(material);
    obj.transform = transform;
    obj.type = StaticObjectType::PropDecorative;
    obj.name = name;
    obj.collider = nullptr;  // NO collision - decorative only
    return Add(obj);
}

size_t StaticWorldRenderer::AddStructural(MeshPtr mesh, MaterialPtr material, const Mat4& transform) {
    StaticObject obj;
    obj.mesh = std::move(mesh);
    obj.material = std::move(material);
    obj.transform = transform;
    obj.type = StaticObjectType::Structural;
    obj.name = "Structural_" + std::to_string(m_objects.size());
    obj.collider = CreateBoxColliderFromTransform(transform);  // Auto-generate collider
    return Add(obj);
}

void StaticWorldRenderer::AddMany(const std::vector<StaticObject>& objects) {
    m_objects.reserve(m_objects.size() + objects.size());
    for (const auto& obj : objects) {
        Add(obj);
    }
}

void StaticWorldRenderer::Remove(size_t index) {
    if (index < m_objects.size()) {
        m_objects.erase(m_objects.begin() + index);
        m_batchesDirty = true;
    }
}

void StaticWorldRenderer::Clear() {
    m_objects.clear();
    m_batches.clear();
    m_batchesDirty = true;
    LOG_INFO("StaticWorldRenderer", "Cleared all static objects");
}

StaticObject* StaticWorldRenderer::Get(size_t index) {
    if (index < m_objects.size()) {
        return &m_objects[index];
    }
    return nullptr;
}

const StaticObject* StaticWorldRenderer::Get(size_t index) const {
    if (index < m_objects.size()) {
        return &m_objects[index];
    }
    return nullptr;
}

// ============================================================================
// Visibility Control
// ============================================================================

void StaticWorldRenderer::SetVisible(size_t index, bool visible) {
    if (index < m_objects.size()) {
        m_objects[index].visible = visible;
    }
}

void StaticWorldRenderer::SetTypeVisible(StaticObjectType type, bool visible) {
    for (auto& obj : m_objects) {
        if (obj.type == type) {
            obj.visible = visible;
        }
    }
}

void StaticWorldRenderer::SetLayerVisible(uint32_t layer, bool visible) {
    m_layerVisibility[layer] = visible;
}

void StaticWorldRenderer::ShowAll() {
    for (auto& obj : m_objects) {
        obj.visible = true;
    }
    m_layerVisibility.clear();
}

void StaticWorldRenderer::HideAll() {
    for (auto& obj : m_objects) {
        obj.visible = false;
    }
}

// ============================================================================
// Rendering
// ============================================================================

void StaticWorldRenderer::Render(const FPSCamera& camera) {
    if (m_objects.empty()) {
        return;
    }

    // Rebuild batches if needed
    if (m_batchesDirty && m_autoBatching) {
        BuildBatches();
    }

    // Reset statistics
    ResetStats();

    Material* currentMaterial = nullptr;
    Shader* currentShader = nullptr;

    // Render using batches (grouped by material)
    for (const auto& batch : m_batches) {
        if (!batch.material || batch.objects.empty()) {
            continue;
        }

        auto shader = batch.material->GetShader();
        if (!shader || !shader->IsValid()) {
            continue;
        }

        // Bind material (includes shader)
        batch.material->Bind();
        m_materialSwitches++;

        // Upload global uniforms
        UploadGlobalUniforms(*shader, camera);

        currentMaterial = batch.material.get();
        currentShader = shader.get();

        // Render all objects in this batch
        for (const StaticObject* obj : batch.objects) {
            if (!obj->IsValid()) {
                continue;
            }

            // Check layer visibility
            auto layerIt = m_layerVisibility.find(obj->layer);
            if (layerIt != m_layerVisibility.end() && !layerIt->second) {
                m_objectsCulled++;
                continue;
            }

            // TODO: Frustum culling here
            // if (!IsInFrustum(obj->worldBoundsMin, obj->worldBoundsMax, camera)) {
            //     m_objectsCulled++;
            //     continue;
            // }

            RenderObject(*obj, *currentShader);
        }
    }

    // Unbind last material
    if (currentMaterial) {
        currentMaterial->Unbind();
    }
}

void StaticWorldRenderer::RenderType(const FPSCamera& camera, StaticObjectType type) {
    Material* currentMaterial = nullptr;
    Shader* currentShader = nullptr;

    ResetStats();

    for (const auto& obj : m_objects) {
        if (!obj.IsValid() || obj.type != type) {
            continue;
        }

        auto shader = obj.material->GetShader();
        if (!shader || !shader->IsValid()) {
            continue;
        }

        // Switch material if needed
        if (currentMaterial != obj.material.get()) {
            obj.material->Bind();
            UploadGlobalUniforms(*shader, camera);
            currentMaterial = obj.material.get();
            currentShader = shader.get();
            m_materialSwitches++;
        }

        RenderObject(obj, *currentShader);
    }

    if (currentMaterial) {
        currentMaterial->Unbind();
    }
}

void StaticWorldRenderer::RenderLayer(const FPSCamera& camera, uint32_t layer) {
    Material* currentMaterial = nullptr;
    Shader* currentShader = nullptr;

    ResetStats();

    for (const auto& obj : m_objects) {
        if (!obj.IsValid() || obj.layer != layer) {
            continue;
        }

        auto shader = obj.material->GetShader();
        if (!shader || !shader->IsValid()) {
            continue;
        }

        // Switch material if needed
        if (currentMaterial != obj.material.get()) {
            obj.material->Bind();
            UploadGlobalUniforms(*shader, camera);
            currentMaterial = obj.material.get();
            currentShader = shader.get();
            m_materialSwitches++;
        }

        RenderObject(obj, *currentShader);
    }

    if (currentMaterial) {
        currentMaterial->Unbind();
    }
}

void StaticWorldRenderer::RenderObject(const StaticObject& obj, Shader& shader) {
    // Set model transform
    if (shader.HasUniform("u_Model")) {
        shader.SetMat4("u_Model", obj.transform);
    }

    // Draw mesh
    obj.mesh->Draw();

    // Update stats
    m_drawCalls++;
    m_objectsRendered++;
    if (obj.mesh->HasIndices()) {
        m_trianglesRendered += obj.mesh->GetIndexCount() / 3;
    } else {
        m_trianglesRendered += obj.mesh->GetVertexCount() / 3;
    }
}

void StaticWorldRenderer::UploadGlobalUniforms(Shader& shader, const FPSCamera& camera) {
    // View/Projection matrices
    if (shader.HasUniform("u_View")) {
        shader.SetMat4("u_View", camera.GetViewMatrix());
    }
    if (shader.HasUniform("u_Proj")) {
        shader.SetMat4("u_Proj", camera.GetProjectionMatrix());
    }
    if (shader.HasUniform("u_ViewProj")) {
        shader.SetMat4("u_ViewProj", camera.GetProjectionMatrix() * camera.GetViewMatrix());
    }
    if (shader.HasUniform("u_CameraPos")) {
        shader.SetVec3("u_CameraPos", camera.GetPosition());
    }

    // Lighting
    if (shader.HasUniform("u_LightDir")) {
        shader.SetVec3("u_LightDir", glm::normalize(m_lightDirection));
    }
    if (shader.HasUniform("u_LightColor")) {
        shader.SetVec3("u_LightColor", m_lightColor * m_lightIntensity);
    }
    if (shader.HasUniform("u_AmbientColor")) {
        shader.SetVec3("u_AmbientColor", m_ambientColor * m_ambientIntensity);
    }
}

// ============================================================================
// Batching
// ============================================================================

void StaticWorldRenderer::RebuildBatches() {
    BuildBatches();
}

void StaticWorldRenderer::BuildBatches() {
    m_batches.clear();

    // Group objects by material
    std::unordered_map<Material*, size_t> materialToBatch;

    for (const auto& obj : m_objects) {
        if (!obj.material) {
            continue;
        }

        Material* matPtr = obj.material.get();
        auto it = materialToBatch.find(matPtr);

        if (it == materialToBatch.end()) {
            // Create new batch for this material
            RenderBatch batch;
            batch.material = obj.material;
            batch.objects.push_back(&obj);
            materialToBatch[matPtr] = m_batches.size();
            m_batches.push_back(std::move(batch));
        } else {
            // Add to existing batch
            m_batches[it->second].objects.push_back(&obj);
        }
    }

    // Sort batches by render queue
    std::sort(m_batches.begin(), m_batches.end(),
        [](const RenderBatch& a, const RenderBatch& b) {
            return static_cast<int>(a.material->GetRenderQueue())
                 < static_cast<int>(b.material->GetRenderQueue());
        });

    m_batchesDirty = false;

    LOG_INFO("StaticWorldRenderer", "Built " + std::to_string(m_batches.size()) +
             " render batches for " + std::to_string(m_objects.size()) + " objects");
}

// ============================================================================
// Lighting
// ============================================================================

void StaticWorldRenderer::SetDirectionalLight(const Vec3& direction, const Vec3& color, float intensity) {
    m_lightDirection = direction;
    m_lightColor = color;
    m_lightIntensity = intensity;
}

void StaticWorldRenderer::SetAmbientLight(const Vec3& color, float intensity) {
    m_ambientColor = color;
    m_ambientIntensity = intensity;
}

// ============================================================================
// Statistics
// ============================================================================

void StaticWorldRenderer::ResetStats() {
    m_drawCalls = 0;
    m_materialSwitches = 0;
    m_trianglesRendered = 0;
    m_objectsRendered = 0;
    m_objectsCulled = 0;
}

// ============================================================================
// Debug
// ============================================================================

void StaticWorldRenderer::PrintDebugInfo() const {
    std::cout << "=== StaticWorldRenderer Debug ===" << std::endl;
    std::cout << "Total Objects: " << m_objects.size() << std::endl;
    std::cout << "Render Batches: " << m_batches.size() << std::endl;

    // Count by type
    int floors = 0, ceilings = 0, walls = 0, props = 0, propsDecorative = 0, structural = 0, generic = 0;
    int withCollision = 0, withoutCollision = 0;

    for (const auto& obj : m_objects) {
        switch (obj.type) {
            case StaticObjectType::Floor: floors++; break;
            case StaticObjectType::Ceiling: ceilings++; break;
            case StaticObjectType::Wall: walls++; break;
            case StaticObjectType::Prop:
            case StaticObjectType::PropPhysics: props++; break;
            case StaticObjectType::PropDecorative: propsDecorative++; break;
            case StaticObjectType::Structural: structural++; break;
            case StaticObjectType::Generic: generic++; break;
        }

        if (obj.HasCollision()) {
            withCollision++;
        } else {
            withoutCollision++;
        }
    }

    std::cout << "  Floors: " << floors << std::endl;
    std::cout << "  Ceilings: " << ceilings << std::endl;
    std::cout << "  Walls: " << walls << std::endl;
    std::cout << "  Props (with collision): " << props << std::endl;
    std::cout << "  Props (decorative): " << propsDecorative << std::endl;
    std::cout << "  Structural: " << structural << std::endl;
    std::cout << "  Generic: " << generic << std::endl;
    std::cout << "Collision:" << std::endl;
    std::cout << "  With Collision: " << withCollision << std::endl;
    std::cout << "  Without Collision (decorative): " << withoutCollision << std::endl;

    std::cout << "Last Frame Stats:" << std::endl;
    std::cout << "  Draw Calls: " << m_drawCalls << std::endl;
    std::cout << "  Material Switches: " << m_materialSwitches << std::endl;
    std::cout << "  Triangles: " << m_trianglesRendered << std::endl;
    std::cout << "  Objects Rendered: " << m_objectsRendered << std::endl;
    std::cout << "  Objects Culled: " << m_objectsCulled << std::endl;
    std::cout << "=================================" << std::endl;
}

// ============================================================================
// Collision Queries
// ============================================================================

size_t StaticWorldRenderer::GetCollisionObjectCount() const {
    size_t count = 0;
    for (const auto& obj : m_objects) {
        if (obj.HasCollision()) {
            count++;
        }
    }
    return count;
}

std::vector<const StaticObject*> StaticWorldRenderer::GetCollisionObjects() const {
    std::vector<const StaticObject*> result;
    result.reserve(m_objects.size());

    for (const auto& obj : m_objects) {
        if (obj.HasCollision()) {
            result.push_back(&obj);
        }
    }

    return result;
}

std::vector<AABB> StaticWorldRenderer::GetCollisionAABBs() const {
    std::vector<AABB> result;
    result.reserve(m_objects.size());

    for (const auto& obj : m_objects) {
        if (obj.HasCollision()) {
            result.push_back(obj.GetCollisionAABB());
        }
    }

    return result;
}

bool StaticWorldRenderer::PointInAnyCollider(const Vec3& point) const {
    for (const auto& obj : m_objects) {
        if (obj.HasCollision() && obj.collider->ContainsPoint(point, obj.transform)) {
            return true;
        }
    }
    return false;
}

std::vector<const StaticObject*> StaticWorldRenderer::QueryAABB(const AABB& aabb) const {
    std::vector<const StaticObject*> result;

    for (const auto& obj : m_objects) {
        if (!obj.HasCollision()) continue;

        AABB objAABB = obj.GetCollisionAABB();

        // Check AABB overlap
        if (aabb.max.x >= objAABB.min.x && aabb.min.x <= objAABB.max.x &&
            aabb.max.y >= objAABB.min.y && aabb.min.y <= objAABB.max.y &&
            aabb.max.z >= objAABB.min.z && aabb.min.z <= objAABB.max.z) {
            result.push_back(&obj);
        }
    }

    return result;
}

} // namespace Genesis

