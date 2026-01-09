#pragma once

// ============================================================================
// OracularV2 Map Editor - Application Core
// Hammer-style map editor for GenesisEngine
// ============================================================================

#include <string>
#include <memory>
#include <vector>

// Must include glad BEFORE glfw to prevent OpenGL header conflicts
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE  // Tell GLFW not to include OpenGL headers
#include <GLFW/glfw3.h>

#include "EditorBrush.h"
#include "EditorEntity.h"
#include "UndoManager.h"

// Forward declarations
namespace Genesis {
    class Map;
}

class Grid;
class ViewportLayout;
class SelectionManager;
class Gizmo;
class BlockTool;

// ============================================================================
// Editor Tool Types
// ============================================================================
enum class EditorTool {
    Select,     // Selection and gizmo manipulation
    Block,      // Create box brushes
    Entity      // Place entities
};

// ============================================================================
// Entity Palette Types
// ============================================================================
enum class EntityPaletteType {
    Light,
    PlayerStart,
    Trigger
};

// ============================================================================
// EditorApp - Main editor application class
// ============================================================================
class EditorApp {
public:
    EditorApp();
    ~EditorApp();

    // Initialize the editor (creates window, sets up ImGui)
    bool Initialize(int width = 1600, int height = 900, const char* title = "OracularV2 Map Editor");
    
    // Main loop
    void Run();
    
    // Shutdown
    void Shutdown();

    // Accessors
    GLFWwindow* GetWindow() const { return m_window; }
    Grid* GetGrid() const { return m_grid.get(); }
    SelectionManager* GetSelection() const { return m_selection.get(); }
    Gizmo* GetGizmo() const { return m_gizmo.get(); }
    
    // Tool management
    EditorTool GetCurrentTool() const { return m_currentTool; }
    void SetCurrentTool(EditorTool tool) { m_currentTool = tool; }
    
    // Entity palette
    EntityPaletteType GetEntityPaletteType() const { return m_entityPaletteType; }
    void SetEntityPaletteType(EntityPaletteType type) { m_entityPaletteType = type; }
    
    // Editor brushes
    std::vector<EditorBrush>& GetEditorBrushes() { return m_editorBrushes; }
    const std::vector<EditorBrush>& GetEditorBrushes() const { return m_editorBrushes; }
    void AddEditorBrush(const EditorBrush& brush);
    void DeleteSelectedBrushes();
    
    // Editor entities
    std::vector<EditorEntity>& GetEditorEntities() { return m_editorEntities; }
    const std::vector<EditorEntity>& GetEditorEntities() const { return m_editorEntities; }
    void AddEditorEntity(const EditorEntity& entity);
    void DeleteSelectedEntities();
    
    // Map management
    void NewMap();
    bool LoadMap(const std::string& path);
    bool SaveMap(const std::string& path);
    void BuildMap();
    Genesis::Map* GetCurrentMap() const { return m_currentMap.get(); }
    
    // Undo/Redo
    void SaveUndoState(const std::string& description = "");
    void Undo();
    void Redo();
    bool CanUndo() const { return m_undoManager.CanUndo(); }
    bool CanRedo() const { return m_undoManager.CanRedo(); }

private:
    // Core systems
    void InitImGui();
    void ShutdownImGui();
    void SetupDockingLayout();
    
    // Frame rendering
    void BeginFrame();
    void EndFrame();
    
    // UI rendering
    void RenderMenuBar();
    void RenderToolbar();
    void RenderStatusBar();
    void RenderPropertiesPanel();
    void RenderEntityPalette();
    void RenderViewports();
    void RenderBuildOutput();
    
    // Input handling
    void ProcessInput();

private:
    GLFWwindow* m_window = nullptr;
    bool m_running = false;
    
    // Editor systems
    std::unique_ptr<Grid> m_grid;
    std::unique_ptr<ViewportLayout> m_viewportLayout;
    std::unique_ptr<SelectionManager> m_selection;
    std::unique_ptr<Gizmo> m_gizmo;
    std::unique_ptr<BlockTool> m_blockTool;
    
    // Editor brushes (the actual brush data for editing)
    std::vector<EditorBrush> m_editorBrushes;
    uint32_t m_nextBrushId = 1;
    
    // Editor entities
    std::vector<EditorEntity> m_editorEntities;
    uint32_t m_nextEntityId = 1;
    
    // Current tool
    EditorTool m_currentTool = EditorTool::Select;
    EntityPaletteType m_entityPaletteType = EntityPaletteType::Light;
    
    // Status message
    std::string m_statusMessage;
    float m_statusMessageTime = 0.0f;
    
    // Internal helpers
    void ShowStatusMessage(const std::string& message, float duration = 2.0f);
    
    // Current map (engine Map for export)
    std::shared_ptr<Genesis::Map> m_currentMap;
    std::string m_currentMapPath;
    bool m_mapModified = false;
    
    // Undo/Redo system
    UndoManager m_undoManager;
    
    // UI state
    bool m_showDemoWindow = false;
    bool m_showBuildOutput = false;
    bool m_firstFrame = true;
    std::string m_buildLog;
    
    float m_statusBarHeight = 24.0f;
    float m_menuBarHeight = 0.0f;
    float m_toolbarHeight = 32.0f;
    float m_propertiesPanelWidth = 300.0f;
};
