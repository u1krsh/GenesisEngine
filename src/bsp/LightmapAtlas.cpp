#include "LightmapAtlas.h"
#include "core/Logger.h"
#include <limits>
#include <cstring>

namespace Genesis {

LightmapAtlas::LightmapAtlas(uint32_t width, uint32_t height)
    : m_width(width), m_height(height) {
    // Initialize pixel buffer (RGB8)
    m_pixels.resize(m_width * m_height * 3, 128);  // Default gray
    
    // Start with one free rectangle covering entire atlas
    m_freeRects.push_back({0, 0, m_width, m_height});
    
    LOG_INFO("LightmapAtlas", "Created " + std::to_string(m_width) + "x" + 
             std::to_string(m_height) + " atlas (" + 
             std::to_string(m_pixels.size() / 1024) + " KB)");
}

void LightmapAtlas::Clear() {
    std::fill(m_pixels.begin(), m_pixels.end(), 128);  // Gray
    ResetAllocations();
}

void LightmapAtlas::ResetAllocations() {
    m_freeRects.clear();
    m_freeRects.push_back({0, 0, m_width, m_height});
    m_usedPixels = 0;
    m_allocationCount = 0;
}

bool LightmapAtlas::Allocate(uint32_t width, uint32_t height,
                              Vec2& outUVMin, Vec2& outUVMax) {
    uint32_t x, y;
    if (!AllocatePixels(width, height, x, y)) {
        return false;
    }
    
    // Convert to normalized UVs
    float invWidth = 1.0f / static_cast<float>(m_width);
    float invHeight = 1.0f / static_cast<float>(m_height);
    
    outUVMin = Vec2(x * invWidth, y * invHeight);
    outUVMax = Vec2((x + width) * invWidth, (y + height) * invHeight);
    
    return true;
}

bool LightmapAtlas::AllocatePixels(uint32_t width, uint32_t height,
                                    uint32_t& outX, uint32_t& outY) {
    // Clamp to valid sizes
    width = std::max(width, MIN_LIGHTMAP_SIZE);
    height = std::max(height, MIN_LIGHTMAP_SIZE);
    
    // Find best fitting rectangle
    int bestIdx = FindBestRect(width, height);
    if (bestIdx < 0) {
        LOG_WARNING("LightmapAtlas", "Failed to allocate " + 
                    std::to_string(width) + "x" + std::to_string(height) + 
                    " (atlas full)");
        return false;
    }
    
    const Rect& rect = m_freeRects[bestIdx];
    outX = rect.x;
    outY = rect.y;
    
    // Split the remaining space
    SplitRect(bestIdx, width, height);
    
    // Update stats
    m_usedPixels += width * height;
    m_allocationCount++;
    
    return true;
}

int LightmapAtlas::FindBestRect(uint32_t width, uint32_t height) const {
    // Best Area Fit (BAF) heuristic
    int bestIdx = -1;
    uint32_t bestArea = std::numeric_limits<uint32_t>::max();
    uint32_t bestShortSide = std::numeric_limits<uint32_t>::max();
    
    for (size_t i = 0; i < m_freeRects.size(); ++i) {
        const Rect& rect = m_freeRects[i];
        
        if (!rect.Contains(width, height)) continue;
        
        uint32_t area = rect.width * rect.height;
        uint32_t shortSide = std::min(rect.width - width, rect.height - height);
        
        // Prefer smaller area, then smaller leftover on short side
        if (area < bestArea || (area == bestArea && shortSide < bestShortSide)) {
            bestIdx = static_cast<int>(i);
            bestArea = area;
            bestShortSide = shortSide;
        }
    }
    
    return bestIdx;
}

void LightmapAtlas::SplitRect(int rectIndex, uint32_t width, uint32_t height) {
    Rect rect = m_freeRects[rectIndex];
    
    // Remove the old rectangle
    m_freeRects.erase(m_freeRects.begin() + rectIndex);
    
    // Create new rectangles for remaining space
    // Right remainder
    if (rect.width > width) {
        m_freeRects.push_back({
            rect.x + width,
            rect.y,
            rect.width - width,
            height  // Only height of allocated rect
        });
    }
    
    // Bottom remainder (full width)
    if (rect.height > height) {
        m_freeRects.push_back({
            rect.x,
            rect.y + height,
            rect.width,
            rect.height - height
        });
    }
    
    // Prune overlapping/contained rectangles occasionally
    if (m_freeRects.size() > 100) {
        PruneRects();
    }
}

void LightmapAtlas::PruneRects() {
    // Remove rectangles that are fully contained by others
    for (size_t i = 0; i < m_freeRects.size(); ) {
        bool contained = false;
        const Rect& a = m_freeRects[i];
        
        for (size_t j = 0; j < m_freeRects.size() && !contained; ++j) {
            if (i == j) continue;
            const Rect& b = m_freeRects[j];
            
            // Check if 'a' is fully contained in 'b'
            if (a.x >= b.x && a.y >= b.y &&
                a.x + a.width <= b.x + b.width &&
                a.y + a.height <= b.y + b.height) {
                contained = true;
            }
        }
        
        if (contained) {
            m_freeRects.erase(m_freeRects.begin() + i);
        } else {
            ++i;
        }
    }
}

void LightmapAtlas::SetPixel(uint32_t x, uint32_t y, const Vec3& color) {
    SetPixel(x, y,
             static_cast<uint8_t>(std::clamp(color.r * 255.0f, 0.0f, 255.0f)),
             static_cast<uint8_t>(std::clamp(color.g * 255.0f, 0.0f, 255.0f)),
             static_cast<uint8_t>(std::clamp(color.b * 255.0f, 0.0f, 255.0f)));
}

void LightmapAtlas::SetPixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b) {
    if (x >= m_width || y >= m_height) return;
    
    size_t idx = (y * m_width + x) * 3;
    m_pixels[idx + 0] = r;
    m_pixels[idx + 1] = g;
    m_pixels[idx + 2] = b;
}

void LightmapAtlas::FillRect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const Vec3& color) {
    uint8_t r = static_cast<uint8_t>(std::clamp(color.r * 255.0f, 0.0f, 255.0f));
    uint8_t g = static_cast<uint8_t>(std::clamp(color.g * 255.0f, 0.0f, 255.0f));
    uint8_t b = static_cast<uint8_t>(std::clamp(color.b * 255.0f, 0.0f, 255.0f));
    
    for (uint32_t py = y; py < y + h && py < m_height; ++py) {
        for (uint32_t px = x; px < x + w && px < m_width; ++px) {
            size_t idx = (py * m_width + px) * 3;
            m_pixels[idx + 0] = r;
            m_pixels[idx + 1] = g;
            m_pixels[idx + 2] = b;
        }
    }
}

Vec3 LightmapAtlas::GetPixel(uint32_t x, uint32_t y) const {
    if (x >= m_width || y >= m_height) return Vec3(0.0f);
    
    size_t idx = (y * m_width + x) * 3;
    return Vec3(
        m_pixels[idx + 0] / 255.0f,
        m_pixels[idx + 1] / 255.0f,
        m_pixels[idx + 2] / 255.0f
    );
}

float LightmapAtlas::GetUtilization() const {
    return static_cast<float>(m_usedPixels) / static_cast<float>(m_width * m_height);
}

} // namespace Genesis
