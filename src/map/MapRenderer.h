#pragma once

#include "Map.h"
#include "renderer/world/StaticWorldRenderer.h"
#include "world/WorldCollision.h"
#include <memory>

namespace Genesis {

// ============================================================================
// MapRenderer - Bridges Map to StaticWorldRenderer and WorldCollision
//
// Takes a loaded Map and:
// 1. Adds all visible brushes to StaticWorldRenderer for rendering
// 2. Adds all collision brushes to WorldCollision for physics
// 3. Handles map unloading/switching
//
// This keeps the map system decoupled from the rendering system.
// ============================================================================
class MapRenderer {
public:
    static MapRenderer& Instance() {
        static MapRenderer instance;
        return instance;
    }

    // ========================================================================
    // Map Management
    // ========================================================================

    // Load and activate a map (clears previous map)
    bool LoadMap(const std::string& filepath);

    // Set the active map (already loaded)
    void SetActiveMap(MapPtr map);

    // Get the active map
    MapPtr GetActiveMap() const { return m_activeMap; }

    // Unload the current map
    void UnloadMap();

    // Check if a map is loaded
    bool HasMap() const { return m_activeMap != nullptr; }

    // ========================================================================
    // Syncing
    // ========================================================================

    // Sync map to renderers (call after modifying brushes)
    void SyncToRenderers();

    // Sync only collision (for physics-only updates)
    void SyncCollision();

    // Sync only rendering (for visual-only updates)
    void SyncRendering();

    // ========================================================================
    // Queries
    // ========================================================================

    // Get spawn point from active map
    Vec3 GetSpawnPosition() const;
    Vec3 GetSpawnRotation() const;

    // Get brush count
    size_t GetBrushCount() const;

    // ========================================================================
    // Layer Control
    // ========================================================================

    void SetLayerVisible(const std::string& layer, bool visible);
    bool IsLayerVisible(const std::string& layer) const;

private:
    MapRenderer() = default;
    ~MapRenderer() = default;
    MapRenderer(const MapRenderer&) = delete;
    MapRenderer& operator=(const MapRenderer&) = delete;

    // Internal sync helpers
    void AddBrushToWorld(const Brush& brush);
    void AddBrushToRenderer(const Brush& brush);

private:
    MapPtr m_activeMap;
};

} // namespace Genesis

