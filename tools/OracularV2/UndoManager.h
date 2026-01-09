#pragma once

// ============================================================================
// OracularV2 Undo/Redo System
// Simple snapshot-based undo manager
// ============================================================================

#include "EditorBrush.h"
#include "EditorEntity.h"
#include <vector>
#include <deque>

// ============================================================================
// EditorSnapshot - Complete state of the editor at a point in time
// ============================================================================
struct EditorSnapshot {
    std::vector<EditorBrush> brushes;
    std::vector<EditorEntity> entities;
    std::string description;  // What action created this state
};

// ============================================================================
// UndoManager - Manages undo/redo stack
// ============================================================================
class UndoManager {
public:
    static constexpr size_t MAX_UNDO_LEVELS = 50;
    
    UndoManager() = default;
    
    // ========================================================================
    // Push state for undo
    // ========================================================================
    void PushState(const std::vector<EditorBrush>& brushes,
                   const std::vector<EditorEntity>& entities,
                   const std::string& description = "") {
        // Remove any redo states when new action is performed
        m_redoStack.clear();
        
        // Push current state to undo stack
        EditorSnapshot snapshot;
        snapshot.brushes = brushes;
        snapshot.entities = entities;
        snapshot.description = description;
        
        m_undoStack.push_back(std::move(snapshot));
        
        // Limit undo stack size
        while (m_undoStack.size() > MAX_UNDO_LEVELS) {
            m_undoStack.pop_front();
        }
    }
    
    // ========================================================================
    // Undo - Returns true if undo was performed
    // ========================================================================
    bool Undo(std::vector<EditorBrush>& brushes,
              std::vector<EditorEntity>& entities) {
        if (m_undoStack.empty()) return false;
        
        // Save current state to redo stack
        EditorSnapshot currentState;
        currentState.brushes = brushes;
        currentState.entities = entities;
        currentState.description = "Redo point";
        m_redoStack.push_back(std::move(currentState));
        
        // Restore previous state
        EditorSnapshot& prev = m_undoStack.back();
        brushes = std::move(prev.brushes);
        entities = std::move(prev.entities);
        m_undoStack.pop_back();
        
        return true;
    }
    
    // ========================================================================
    // Redo - Returns true if redo was performed
    // ========================================================================
    bool Redo(std::vector<EditorBrush>& brushes,
              std::vector<EditorEntity>& entities) {
        if (m_redoStack.empty()) return false;
        
        // Save current state to undo stack
        EditorSnapshot currentState;
        currentState.brushes = brushes;
        currentState.entities = entities;
        currentState.description = "Undo point";
        m_undoStack.push_back(std::move(currentState));
        
        // Restore redo state
        EditorSnapshot& next = m_redoStack.back();
        brushes = std::move(next.brushes);
        entities = std::move(next.entities);
        m_redoStack.pop_back();
        
        return true;
    }
    
    // ========================================================================
    // Queries
    // ========================================================================
    bool CanUndo() const { return !m_undoStack.empty(); }
    bool CanRedo() const { return !m_redoStack.empty(); }
    size_t GetUndoCount() const { return m_undoStack.size(); }
    size_t GetRedoCount() const { return m_redoStack.size(); }
    
    void Clear() {
        m_undoStack.clear();
        m_redoStack.clear();
    }

private:
    std::deque<EditorSnapshot> m_undoStack;
    std::deque<EditorSnapshot> m_redoStack;
};
