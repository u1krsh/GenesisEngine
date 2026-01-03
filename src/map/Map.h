#pragma once

#include "Brush.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

namespace Genesis {

// ============================================================================
// Map Metadata - Information about the map
// ============================================================================
struct MapMetadata {
    std::string name = "Untitled";
    std::string author = "Unknown";
    std::string version = "1.0";
    std::string description;

    // Default spawn point
    Vec3 spawnPosition = Vec3(0.0f, 1.0f, 0.0f);
    Vec3 spawnRotation = Vec3(0.0f);

    // Environment settings
    Vec3 ambientColor = Vec3(0.15f, 0.15f, 0.2f);
    Vec3 sunDirection = Vec3(0.5f, 1.0f, 0.3f);
    Vec3 sunColor = Vec3(1.0f, 0.98f, 0.95f);
    float sunIntensity = 1.0f;

    // Sky settings (for future use)
    std::string skybox;
    Vec3 fogColor = Vec3(0.7f, 0.8f, 0.9f);
    float fogStart = 100.0f;
    float fogEnd = 500.0f;
};

// ============================================================================
// Map Entity - Generic entity in the map (spawn points, lights, etc.)
// ============================================================================
struct MapEntity {
    std::string classname;   // e.g., "info_player_start", "light", "trigger_once"
    std::string targetname;  // Unique name for targeting
    Vec3 position = Vec3(0.0f);
    Vec3 rotation = Vec3(0.0f);
    std::unordered_map<std::string, std::string> properties;

    // Get property with default value
    std::string GetProperty(const std::string& key, const std::string& defaultValue = "") const {
        auto it = properties.find(key);
        return (it != properties.end()) ? it->second : defaultValue;
    }

    float GetFloat(const std::string& key, float defaultValue = 0.0f) const {
        auto it = properties.find(key);
        return (it != properties.end()) ? std::stof(it->second) : defaultValue;
    }

    int GetInt(const std::string& key, int defaultValue = 0) const {
        auto it = properties.find(key);
        return (it != properties.end()) ? std::stoi(it->second) : defaultValue;
    }
};

// ============================================================================
// Map - A complete game level
//
// Structure (Source Engine-like):
// Map
//  ├── Metadata (name, author, environment settings)
//  ├── Brushes[] (world geometry)
//  │    └── Brush
//  │         ├── Shape
//  │         ├── Material
//  │         └── Collider
//  ├── Entities[] (spawn points, triggers, lights)
//  └── Layers[] (organizational groups)
//
// Future extensions:
// - BSP tree for visibility/collision
// - Lightmaps
// - Vis data
// - Area portals
// ============================================================================
class Map {
public:
    Map() = default;
    explicit Map(const std::string& name) { m_metadata.name = name; }

    // ========================================================================
    // Metadata
    // ========================================================================

    MapMetadata& GetMetadata() { return m_metadata; }
    const MapMetadata& GetMetadata() const { return m_metadata; }
    void SetMetadata(const MapMetadata& metadata) { m_metadata = metadata; }

    const std::string& GetName() const { return m_metadata.name; }
    void SetName(const std::string& name) { m_metadata.name = name; }

    // ========================================================================
    // Brush Management
    // ========================================================================

    // Add a brush (takes ownership)
    size_t AddBrush(const Brush& brush) {
        m_brushes.push_back(brush);
        m_brushes.back().id = m_nextBrushId++;
        return m_brushes.size() - 1;
    }

    size_t AddBrush(Brush&& brush) {
        brush.id = m_nextBrushId++;
        m_brushes.push_back(std::move(brush));
        return m_brushes.size() - 1;
    }

    // Get brush by index
    Brush* GetBrush(size_t index) {
        return (index < m_brushes.size()) ? &m_brushes[index] : nullptr;
    }

    const Brush* GetBrush(size_t index) const {
        return (index < m_brushes.size()) ? &m_brushes[index] : nullptr;
    }

    // Get brush by ID
    Brush* GetBrushById(uint32_t id) {
        for (auto& brush : m_brushes) {
            if (brush.id == id) return &brush;
        }
        return nullptr;
    }

    // Get all brushes
    std::vector<Brush>& GetBrushes() { return m_brushes; }
    const std::vector<Brush>& GetBrushes() const { return m_brushes; }

    size_t GetBrushCount() const { return m_brushes.size(); }

    // Remove brush
    void RemoveBrush(size_t index) {
        if (index < m_brushes.size()) {
            m_brushes.erase(m_brushes.begin() + index);
        }
    }

    // Clear all brushes
    void ClearBrushes() { m_brushes.clear(); }

    // ========================================================================
    // Entity Management
    // ========================================================================

    size_t AddEntity(const MapEntity& entity) {
        m_entities.push_back(entity);
        return m_entities.size() - 1;
    }

    MapEntity* GetEntity(size_t index) {
        return (index < m_entities.size()) ? &m_entities[index] : nullptr;
    }

    std::vector<MapEntity>& GetEntities() { return m_entities; }
    const std::vector<MapEntity>& GetEntities() const { return m_entities; }

    size_t GetEntityCount() const { return m_entities.size(); }

    // Find entities by classname
    std::vector<const MapEntity*> FindEntitiesByClass(const std::string& classname) const {
        std::vector<const MapEntity*> result;
        for (const auto& entity : m_entities) {
            if (entity.classname == classname) {
                result.push_back(&entity);
            }
        }
        return result;
    }

    // Find entity by targetname
    const MapEntity* FindEntityByName(const std::string& targetname) const {
        for (const auto& entity : m_entities) {
            if (entity.targetname == targetname) {
                return &entity;
            }
        }
        return nullptr;
    }

    void ClearEntities() { m_entities.clear(); }

    // ========================================================================
    // Layer Management
    // ========================================================================

    void AddLayer(const std::string& layer) {
        if (m_layers.find(layer) == m_layers.end()) {
            m_layers[layer] = true;
        }
    }

    void SetLayerVisible(const std::string& layer, bool visible) {
        m_layers[layer] = visible;
    }

    bool IsLayerVisible(const std::string& layer) const {
        auto it = m_layers.find(layer);
        return (it != m_layers.end()) ? it->second : true;
    }

    const std::unordered_map<std::string, bool>& GetLayers() const { return m_layers; }

    // ========================================================================
    // Queries
    // ========================================================================

    // Get brushes in a layer
    std::vector<const Brush*> GetBrushesInLayer(const std::string& layer) const {
        std::vector<const Brush*> result;
        for (const auto& brush : m_brushes) {
            if (brush.layer == layer) {
                result.push_back(&brush);
            }
        }
        return result;
    }

    // Get brushes with collision
    std::vector<const Brush*> GetCollisionBrushes() const {
        std::vector<const Brush*> result;
        for (const auto& brush : m_brushes) {
            if (brush.HasCollision()) {
                result.push_back(&brush);
            }
        }
        return result;
    }

    // Get visible brushes
    std::vector<const Brush*> GetVisibleBrushes() const {
        std::vector<const Brush*> result;
        for (const auto& brush : m_brushes) {
            if (brush.IsVisible() && IsLayerVisible(brush.layer)) {
                result.push_back(&brush);
            }
        }
        return result;
    }

    // Iterate over all brushes
    void ForEachBrush(const std::function<void(Brush&)>& callback) {
        for (auto& brush : m_brushes) {
            callback(brush);
        }
    }

    void ForEachBrush(const std::function<void(const Brush&)>& callback) const {
        for (const auto& brush : m_brushes) {
            callback(brush);
        }
    }

    // ========================================================================
    // Map Operations
    // ========================================================================

    // Clear entire map
    void Clear() {
        m_brushes.clear();
        m_entities.clear();
        m_layers.clear();
        m_metadata = MapMetadata();
        m_nextBrushId = 1;
    }

    // Check if map is empty
    bool IsEmpty() const {
        return m_brushes.empty() && m_entities.empty();
    }

    // Get spawn point (from metadata or first info_player_start entity)
    Vec3 GetSpawnPosition() const {
        auto spawns = FindEntitiesByClass("info_player_start");
        if (!spawns.empty()) {
            return spawns[0]->position;
        }
        return m_metadata.spawnPosition;
    }

    Vec3 GetSpawnRotation() const {
        auto spawns = FindEntitiesByClass("info_player_start");
        if (!spawns.empty()) {
            return spawns[0]->rotation;
        }
        return m_metadata.spawnRotation;
    }

private:
    MapMetadata m_metadata;
    std::vector<Brush> m_brushes;
    std::vector<MapEntity> m_entities;
    std::unordered_map<std::string, bool> m_layers;
    uint32_t m_nextBrushId = 1;
};

using MapPtr = std::shared_ptr<Map>;

} // namespace Genesis

