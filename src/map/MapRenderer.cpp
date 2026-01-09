#include "MapRenderer.h"
#include "MapLoader.h"
#include "core/Logger.h"

namespace Genesis {

bool MapRenderer::LoadMap(const std::string& filepath) {
    // Unload current map
    UnloadMap();

    // Load new map
    auto& loader = MapLoader::Instance();
    MapPtr map = loader.Load(filepath);

    if (!map) {
        LOG_ERROR("MapRenderer", "Failed to load map: " + filepath);
        return false;
    }

    SetActiveMap(map);
    return true;
}

void MapRenderer::SetActiveMap(MapPtr map) {
    // Unload current
    UnloadMap();

    m_activeMap = std::move(map);

    if (m_activeMap) {
        SyncToRenderers();
        LOG_INFO("MapRenderer", "Activated map: " + m_activeMap->GetName());
    }
}

void MapRenderer::UnloadMap() {
    if (!m_activeMap) return;

    LOG_INFO("MapRenderer", "Unloading map: " + m_activeMap->GetName());

    // Clear world collision
    auto& worldCol = WorldCollision::Instance();
    worldCol.Clear();

    // Clear static world renderer
    auto& worldRender = StaticWorldRenderer::Instance();
    worldRender.Clear();

    m_activeMap = nullptr;
}

void MapRenderer::SyncToRenderers() {
    if (!m_activeMap) return;

    SyncCollision();
    SyncRendering();
}

void MapRenderer::SyncCollision() {
    if (!m_activeMap) return;

    auto& worldCol = WorldCollision::Instance();
    worldCol.Clear();
    worldCol.SetFloorHeight(-1000.0f); // Disable auto floor, use map geometry

    for (const auto& brush : m_activeMap->GetBrushes()) {
        if (brush.HasCollision() && m_activeMap->IsLayerVisible(brush.layer)) {
            AddBrushToWorld(brush);
        }
    }

    LOG_DEBUG("MapRenderer", "Synced " + std::to_string(worldCol.GetBoxes().size()) + " collision boxes");
}

void MapRenderer::SyncRendering() {
    if (!m_activeMap) return;

    auto& worldRender = StaticWorldRenderer::Instance();
    worldRender.Clear();

    for (const auto& brush : m_activeMap->GetBrushes()) {
        if (brush.IsVisible() && m_activeMap->IsLayerVisible(brush.layer)) {
            AddBrushToRenderer(brush);
        }
    }

    // SCENE LIGHTS
    worldRender.ClearLights(); // Ensure we don't duplicate lights on reload
    for (const auto& entity : m_activeMap->GetEntities()) {
        if (entity.classname == "light") {
            Vec3 color(1.0f);
            float intensity = 100.0f;
            float radius = 10.0f;

            // Parse Properties
            if (entity.properties.count("color")) {
                std::istringstream ss(entity.properties.at("color"));
                float r, g, b;
                if (ss >> r >> g >> b) {
                     // SAU stores 0-255, Renderer expects 0-1
                     if (r > 1.0f || g > 1.0f || b > 1.0f) { 
                        color = Vec3(r/255.0f, g/255.0f, b/255.0f);
                     } else {
                        color = Vec3(r, g, b);
                     }
                }
            }
            if (entity.properties.count("intensity")) {
                intensity = std::stof(entity.properties.at("intensity"));
            }
            if (entity.properties.count("radius")) {
                radius = std::stof(entity.properties.at("radius"));
            }

            worldRender.AddPointLight(entity.position, color, intensity, radius);
        }
    }

    worldRender.RebuildBatches();

    LOG_DEBUG("MapRenderer", "Synced " + std::to_string(worldRender.GetObjectCount()) + " render objects");
}

void MapRenderer::AddBrushToWorld(const Brush& brush) {
    auto& worldCol = WorldCollision::Instance();

    // Add to world collision based on brush type
    Vec3 halfSize = brush.size * 0.5f;

    if (brush.IsStair()) {
        // Add as stair (auto-climbable)
        worldCol.AddStair(
            brush.position.x, brush.position.y, brush.position.z,
            brush.size.x, brush.size.y, brush.size.z
        );
    } else {
        // Add as regular box
        worldCol.AddBox(
            brush.position.x, brush.position.y, brush.position.z,
            brush.size.x, brush.size.y, brush.size.z
        );
    }
}

void MapRenderer::AddBrushToRenderer(const Brush& brush) {
    auto& worldRender = StaticWorldRenderer::Instance();

    if (!brush.mesh || !brush.material) {
        LOG_WARNING("MapRenderer", "Brush missing mesh or material: " + brush.name);
        return;
    }

    // Create static object from brush
    StaticObject obj;
    obj.mesh = brush.mesh;
    obj.material = brush.material;
    obj.transform = brush.transform;
    obj.collider = brush.collider;
    obj.name = brush.name;
    obj.visible = true;
    obj.castShadow = HasFlag(brush.flags, BrushFlags::CastShadow);

    // Determine object type based on material name
    std::string matName = brush.materialName;
    std::transform(matName.begin(), matName.end(), matName.begin(), ::tolower);

    if (matName == "floor" || matName == "ground") {
        obj.type = StaticObjectType::Floor;
    } else if (matName == "wall") {
        obj.type = StaticObjectType::Wall;
    } else if (matName == "ceiling") {
        obj.type = StaticObjectType::Ceiling;
    } else {
        obj.type = StaticObjectType::Generic;
    }

    worldRender.Add(obj);
}

Vec3 MapRenderer::GetSpawnPosition() const {
    if (m_activeMap) {
        return m_activeMap->GetSpawnPosition();
    }
    return Vec3(0.0f, 1.0f, 0.0f);
}

Vec3 MapRenderer::GetSpawnRotation() const {
    if (m_activeMap) {
        return m_activeMap->GetSpawnRotation();
    }
    return Vec3(0.0f);
}

size_t MapRenderer::GetBrushCount() const {
    return m_activeMap ? m_activeMap->GetBrushCount() : 0;
}

void MapRenderer::SetLayerVisible(const std::string& layer, bool visible) {
    if (m_activeMap) {
        m_activeMap->SetLayerVisible(layer, visible);
        SyncToRenderers(); // Re-sync after visibility change
    }
}

bool MapRenderer::IsLayerVisible(const std::string& layer) const {
    return m_activeMap ? m_activeMap->IsLayerVisible(layer) : true;
}

} // namespace Genesis

