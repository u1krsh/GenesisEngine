#pragma once

// ============================================================================
// OracularV2 Selection Manager
// Handles brush and entity selection
// ============================================================================

#include "EditorBrush.h"
#include "map/Map.h"
#include "math/Math.h"
#include <vector>

// ============================================================================
// SelectionManager - Manages selected objects
// ============================================================================
class SelectionManager {
public:
    SelectionManager() = default;
    
    // ========================================================================
    // Selection operations
    // ========================================================================
    
    void Select(EditorBrush* brush, bool addToSelection = false) {
        if (!addToSelection) {
            ClearSelection();
        }
        
        if (brush && !brush->isSelected) {
            brush->isSelected = true;
            m_selectedBrushes.push_back(brush);
        }
    }
    
    void Deselect(EditorBrush* brush) {
        if (!brush) return;
        
        brush->isSelected = false;
        m_selectedBrushes.erase(
            std::remove(m_selectedBrushes.begin(), m_selectedBrushes.end(), brush),
            m_selectedBrushes.end()
        );
    }
    
    void ToggleSelection(EditorBrush* brush) {
        if (!brush) return;
        
        if (brush->isSelected) {
            Deselect(brush);
        } else {
            Select(brush, true);
        }
    }
    
    void ClearSelection() {
        for (auto* brush : m_selectedBrushes) {
            brush->isSelected = false;
        }
        m_selectedBrushes.clear();
        
        for (auto* entity : m_selectedEntities) {
            // TODO: Clear entity selection state
        }
        m_selectedEntities.clear();
    }
    
    void SelectAll(std::vector<EditorBrush>& brushes) {
        ClearSelection();
        for (auto& brush : brushes) {
            brush.isSelected = true;
            m_selectedBrushes.push_back(&brush);
        }
    }
    
    // ========================================================================
    // Entity selection
    // ========================================================================
    
    void SelectEntity(Genesis::MapEntity* entity, bool addToSelection = false) {
        if (!addToSelection) {
            ClearSelection();
        }
        
        if (entity) {
            m_selectedEntities.push_back(entity);
        }
    }
    
    // ========================================================================
    // Queries
    // ========================================================================
    
    bool HasSelection() const { 
        return !m_selectedBrushes.empty() || !m_selectedEntities.empty(); 
    }
    
    size_t GetSelectedCount() const { 
        return m_selectedBrushes.size() + m_selectedEntities.size(); 
    }
    
    const std::vector<EditorBrush*>& GetSelectedBrushes() const { 
        return m_selectedBrushes; 
    }
    
    const std::vector<Genesis::MapEntity*>& GetSelectedEntities() const { 
        return m_selectedEntities; 
    }
    
    EditorBrush* GetFirstSelectedBrush() const {
        return m_selectedBrushes.empty() ? nullptr : m_selectedBrushes[0];
    }
    
    // ========================================================================
    // Selection bounds
    // ========================================================================
    
    Genesis::AABB GetSelectionBounds() const {
        if (m_selectedBrushes.empty()) {
            return Genesis::AABB();
        }
        
        Genesis::Vec3 min(std::numeric_limits<float>::max());
        Genesis::Vec3 max(std::numeric_limits<float>::lowest());
        
        for (const auto* brush : m_selectedBrushes) {
            Genesis::AABB bounds = brush->GetAABB();
            min = glm::min(min, bounds.min);
            max = glm::max(max, bounds.max);
        }
        
        return Genesis::AABB(min, max);
    }
    
    Genesis::Vec3 GetSelectionCenter() const {
        Genesis::AABB bounds = GetSelectionBounds();
        return (bounds.min + bounds.max) * 0.5f;
    }

private:
    std::vector<EditorBrush*> m_selectedBrushes;
    std::vector<Genesis::MapEntity*> m_selectedEntities;
};
