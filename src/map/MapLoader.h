#pragma once

#include "Map.h"
#include "MeshLibrary.h"
#include "renderer/material/MaterialLibrary.h"
#include <string>
#include <functional>

namespace Genesis {

// ============================================================================
// MapLoader - Loads and parses map files
//
// Supported formats:
// - JSON (.json) - Primary format, human-readable
// - Simple Text (.map) - Compact format for quick editing
//
// The loader:
// 1. Parses the file format
// 2. Creates Brush objects with geometry data
// 3. Resolves materials from MaterialLibrary
// 4. Creates meshes from MeshLibrary
// 5. Builds colliders from brush geometry
// ============================================================================
class MapLoader {
public:
    static MapLoader& Instance() {
        static MapLoader instance;
        return instance;
    }

    // ========================================================================
    // Loading
    // ========================================================================

    // Load a map from file (auto-detects format)
    MapPtr Load(const std::string& filepath);

    // Load from JSON file
    MapPtr LoadJSON(const std::string& filepath);

    // Load from simple text format
    MapPtr LoadSimple(const std::string& filepath);

    // Load from string (JSON)
    MapPtr LoadFromString(const std::string& jsonString);

    // ========================================================================
    // Saving
    // ========================================================================

    // Save map to JSON file
    bool SaveJSON(const Map& map, const std::string& filepath);

    // Save map to simple text format
    bool SaveSimple(const Map& map, const std::string& filepath);

    // ========================================================================
    // Map Building
    // ========================================================================

    // Build runtime data for all brushes in a map
    // This resolves materials, creates meshes, and builds colliders
    void BuildMap(Map& map);

    // Build a single brush (mesh, material, collider, transform)
    void BuildBrush(Brush& brush);

    // ========================================================================
    // Configuration
    // ========================================================================

    // Set base path for map files
    void SetBasePath(const std::string& path) { m_basePath = path; }
    const std::string& GetBasePath() const { return m_basePath; }

    // Set default material (used when material not found)
    void SetDefaultMaterial(const std::string& name) { m_defaultMaterial = name; }

    // ========================================================================
    // Error Handling
    // ========================================================================

    bool HasError() const { return !m_lastError.empty(); }
    const std::string& GetLastError() const { return m_lastError; }
    void ClearError() { m_lastError.clear(); }

    // Set error callback
    using ErrorCallback = std::function<void(const std::string& error)>;
    void SetErrorCallback(ErrorCallback callback) { m_errorCallback = std::move(callback); }

private:
    MapLoader();
    ~MapLoader() = default;
    MapLoader(const MapLoader&) = delete;
    MapLoader& operator=(const MapLoader&) = delete;

    // Internal parsing
    bool ParseJSONBrush(const std::string& jsonObj, Brush& brush);
    bool ParseSimpleLine(const std::string& line, Brush& brush);
    BrushFlags ParseFlags(const std::string& flagsStr);

    // Error reporting
    void SetError(const std::string& error);

    // Create collider from brush shape and size
    ColliderPtr CreateCollider(const Brush& brush);

private:
    std::string m_basePath = "assets/maps/";
    std::string m_defaultMaterial = "default";
    std::string m_lastError;
    ErrorCallback m_errorCallback;
};

} // namespace Genesis

