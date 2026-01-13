#pragma once

#include "Texture2D.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Genesis {

// ============================================================================
// TextureLibrary - Manages texture loading, caching, and discovery
// ============================================================================
class TextureLibrary {
public:
    static TextureLibrary& Instance() {
        static TextureLibrary instance;
        return instance;
    }

    // ========================================================================
    // Texture Loading
    // ========================================================================

    // Load a texture from file (cached)
    Texture2DPtr Load(const std::string& filepath);

    // Get a previously loaded texture
    Texture2DPtr Get(const std::string& name) const;

    // Check if texture is loaded
    bool Exists(const std::string& name) const;

    // Unload a texture
    void Unload(const std::string& name);

    // Clear all textures
    void Clear();

    // ========================================================================
    // Directory Scanning
    // ========================================================================

    // Scan a directory for texture files (recursive)
    void ScanDirectory(const std::string& path);

    // Get all discovered texture paths
    const std::vector<std::string>& GetDiscoveredTextures() const { return m_discoveredPaths; }

    // Refresh discovered textures
    void RefreshDiscovery();

    // ========================================================================
    // Procedural Textures
    // ========================================================================

    // Create a checkerboard pattern
    Texture2DPtr CreateCheckerboard(const std::string& name, int size, 
                                     const Vec4& color1, const Vec4& color2);

    // Create a solid color texture
    Texture2DPtr CreateSolid(const std::string& name, int size, const Vec4& color);

    // Create a grid pattern (for dev textures)
    Texture2DPtr CreateGrid(const std::string& name, int size, 
                            const Vec4& bgColor, const Vec4& lineColor);

    // ========================================================================
    // Info
    // ========================================================================

    // Get all loaded texture names
    std::vector<std::string> GetLoadedTextureNames() const;

    // Get texture count
    size_t GetTextureCount() const { return m_textures.size(); }

    // Get discovered texture count
    size_t GetDiscoveredCount() const { return m_discoveredPaths.size(); }

    // ========================================================================
    // Configuration
    // ========================================================================

    void SetBasePath(const std::string& path) { m_basePath = path; }
    const std::string& GetBasePath() const { return m_basePath; }

private:
    TextureLibrary();
    ~TextureLibrary() = default;
    TextureLibrary(const TextureLibrary&) = delete;
    TextureLibrary& operator=(const TextureLibrary&) = delete;

    bool IsTextureFile(const std::string& path) const;
    std::string NormalizePath(const std::string& path) const;

private:
    std::unordered_map<std::string, Texture2DPtr> m_textures;
    std::vector<std::string> m_discoveredPaths;
    std::string m_basePath = "assets/textures/";
    std::string m_lastScanPath;
};

} // namespace Genesis
