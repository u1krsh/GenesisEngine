#pragma once

#include "math/Math.h"
#include <vector>
#include <cstdint>
#include <algorithm>

namespace Genesis {

// ============================================================================
// LightmapAtlas - Packs multiple face lightmaps into a single texture atlas
// Uses MaxRects bin packing algorithm for efficient space utilization
// ============================================================================
class LightmapAtlas {
public:
    // Default atlas size (can be resized if needed)
    static constexpr uint32_t DEFAULT_WIDTH = 4096;
    static constexpr uint32_t DEFAULT_HEIGHT = 4096;
    static constexpr uint32_t MAX_SIZE = 8192;
    static constexpr uint32_t MIN_LIGHTMAP_SIZE = 4;
    static constexpr uint32_t MAX_LIGHTMAP_SIZE = 32;  // Smaller for better packing

    LightmapAtlas(uint32_t width = DEFAULT_WIDTH, uint32_t height = DEFAULT_HEIGHT);
    ~LightmapAtlas() = default;

    // ========================================================================
    // Allocation
    // ========================================================================

    // Allocate a rectangle in the atlas
    // Returns true if successful, with UV coordinates in outUVMin/outUVMax
    // UV coordinates are normalized (0-1 range)
    bool Allocate(uint32_t width, uint32_t height,
                  Vec2& outUVMin, Vec2& outUVMax);

    // Allocate and return pixel coordinates (for writing pixels)
    bool AllocatePixels(uint32_t width, uint32_t height,
                        uint32_t& outX, uint32_t& outY);

    // Reset allocations (keeps pixel data)
    void ResetAllocations();

    // Clear everything (allocations + pixels)
    void Clear();

    // ========================================================================
    // Pixel Access
    // ========================================================================

    // Get pixel data pointer for GPU upload (RGB8 format)
    const uint8_t* GetPixelData() const { return m_pixels.data(); }
    
    // Get pixel data size in bytes
    size_t GetPixelDataSize() const { return m_pixels.size(); }

    // Set pixel color (RGB, each component 0-1)
    void SetPixel(uint32_t x, uint32_t y, const Vec3& color);
    void SetPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);

    // Fill a rectangle with color
    void FillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Vec3& color);

    // Get pixel color
    Vec3 GetPixel(uint32_t x, uint32_t y) const;

    // ========================================================================
    // Accessors
    // ========================================================================

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    float GetUtilization() const;  // Returns 0-1 (percentage of atlas used)
    uint32_t GetAllocationCount() const { return m_allocationCount; }

private:
    // Rectangle for bin packing
    struct Rect {
        uint32_t x, y, width, height;
        
        bool Contains(uint32_t w, uint32_t h) const {
            return width >= w && height >= h;
        }
    };

    // MaxRects algorithm: find best rectangle for given dimensions
    int FindBestRect(uint32_t width, uint32_t height) const;

    // Split a free rectangle after placing an item
    void SplitRect(int rectIndex, uint32_t width, uint32_t height);

    // Remove redundant free rectangles (contained by others)
    void PruneRects();

private:
    uint32_t m_width;
    uint32_t m_height;
    std::vector<uint8_t> m_pixels;      // RGB8 pixel data
    std::vector<Rect> m_freeRects;      // Free rectangles for bin packing
    uint32_t m_usedPixels = 0;          // Total pixels allocated
    uint32_t m_allocationCount = 0;     // Number of allocations
};

} // namespace Genesis
