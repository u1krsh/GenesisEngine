#pragma once

#include "math/Math.h"
#include <string>
#include <memory>
#include <vector>

namespace Genesis {

// ============================================================================
// Texture Filtering Mode
// ============================================================================
enum class TextureFilter {
    Nearest,        // Pixelated (no filtering)
    Linear,         // Smooth (bilinear)
    Trilinear       // Smooth with mipmaps
};

// ============================================================================
// Texture Wrap Mode
// ============================================================================
enum class TextureWrap {
    Repeat,         // Tile the texture
    Clamp,          // Clamp to edge
    Mirror          // Mirror at edges
};

// ============================================================================
// Texture2D - 2D texture loaded from file or created programmatically
// ============================================================================
class Texture2D {
public:
    Texture2D();
    explicit Texture2D(const std::string& name);
    ~Texture2D();

    // Non-copyable but movable
    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    // ========================================================================
    // Loading
    // ========================================================================

    // Load from file (PNG, JPG, TGA, BMP)
    bool LoadFromFile(const std::string& filepath);

    // Create from raw pixel data
    bool Create(int width, int height, int channels, const unsigned char* data);

    // Create a solid color texture
    bool CreateSolidColor(int width, int height, const Vec4& color);

    // ========================================================================
    // OpenGL Binding
    // ========================================================================

    void Bind(int textureUnit = 0) const;
    void Unbind() const;

    // ========================================================================
    // Properties
    // ========================================================================

    const std::string& GetName() const { return m_name; }
    const std::string& GetFilePath() const { return m_filepath; }
    
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    int GetChannels() const { return m_channels; }
    
    unsigned int GetID() const { return m_textureID; }
    bool IsLoaded() const { return m_textureID != 0; }
    
    // Sample the alpha channel at UV coordinates (0-1), returns 0-1
    // For CPU-side texture sampling (used in light baking)
    float SampleAlpha(float u, float v) const;

    // ========================================================================
    // Settings
    // ========================================================================

    void SetFilter(TextureFilter filter);
    TextureFilter GetFilter() const { return m_filter; }

    void SetWrap(TextureWrap wrap);
    TextureWrap GetWrap() const { return m_wrap; }

    void GenerateMipmaps();

    // ========================================================================
    // Thumbnail (for editor)
    // ========================================================================

    // Get a downscaled version for UI display (lazy-generated)
    unsigned int GetThumbnailID(int maxSize = 64);

private:
    void Destroy();
    void ApplyFilterSettings();
    void ApplyWrapSettings();

private:
    std::string m_name;
    std::string m_filepath;

    unsigned int m_textureID = 0;
    int m_width = 0;
    int m_height = 0;
    int m_channels = 0;

    TextureFilter m_filter = TextureFilter::Linear;
    TextureWrap m_wrap = TextureWrap::Repeat;

    // Thumbnail cache
    unsigned int m_thumbnailID = 0;
    int m_thumbnailSize = 0;
    
    // CPU-side pixel data for sampling (used in light baking)
    std::vector<unsigned char> m_pixelData;
};

using Texture2DPtr = std::shared_ptr<Texture2D>;

} // namespace Genesis
