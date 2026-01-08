#pragma once

// ============================================================================
// OracularV2 Grid System
// Enforces vertex snapping for BSP-safe geometry
// ============================================================================

#include "math/Math.h"

class Grid {
public:
    // Valid grid sizes
    static constexpr float GRID_SIZES[] = {1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f};
    static constexpr int NUM_GRID_SIZES = 7;
    
    Grid() = default;
    
    // Grid snapping - THE critical operation
    // All brush vertices MUST go through this
    Genesis::Vec3 Snap(const Genesis::Vec3& pos) const {
        return Genesis::Vec3(
            std::round(pos.x / m_snapSize) * m_snapSize,
            std::round(pos.y / m_snapSize) * m_snapSize,
            std::round(pos.z / m_snapSize) * m_snapSize
        );
    }
    
    float SnapValue(float value) const {
        return std::round(value / m_snapSize) * m_snapSize;
    }
    
    // Configuration
    float GetSnapSize() const { return m_snapSize; }
    void SetSnapSize(float size) { m_snapSize = size; }
    
    void IncreaseGridSize() {
        for (int i = 0; i < NUM_GRID_SIZES - 1; i++) {
            if (m_snapSize == GRID_SIZES[i]) {
                m_snapSize = GRID_SIZES[i + 1];
                return;
            }
        }
    }
    
    void DecreaseGridSize() {
        for (int i = 1; i < NUM_GRID_SIZES; i++) {
            if (m_snapSize == GRID_SIZES[i]) {
                m_snapSize = GRID_SIZES[i - 1];
                return;
            }
        }
    }
    
    // Visibility
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    
    // Grid rendering parameters
    int GetGridLines() const { return m_gridLines; }
    void SetGridLines(int lines) { m_gridLines = lines; }
    
    Genesis::Vec4 GetGridColor() const { return m_gridColor; }
    void SetGridColor(const Genesis::Vec4& color) { m_gridColor = color; }
    
    Genesis::Vec4 GetAxisColor() const { return m_axisColor; }

private:
    float m_snapSize = 8.0f;
    bool m_visible = true;
    int m_gridLines = 64;
    
    Genesis::Vec4 m_gridColor{0.3f, 0.3f, 0.35f, 1.0f};
    Genesis::Vec4 m_axisColor{0.6f, 0.6f, 0.7f, 1.0f};
};
