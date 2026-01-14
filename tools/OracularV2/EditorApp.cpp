// ============================================================================
// OracularV2 Map Editor - Application Core Implementation
// Complete implementation with all phases (1-10)
// ============================================================================

#include "EditorApp.h"
#include "Grid.h"
#include "ViewportLayout.h"
#include "SelectionManager.h"
#include "Gizmo.h"
#include "tools/BlockTool.h"
#include "TextureBrowser.h"
#include "BuildMap.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include "map/MapLoader.h"
#include "SAUFormat.h"
#include <iostream>

// Constructor
EditorApp::EditorApp() 
    : m_window(nullptr)
    , m_running(false)
    , m_firstFrame(true)
    , m_mapModified(false)
    , m_currentTool(EditorTool::Select)
    , m_entityPaletteType(EntityPaletteType::Light)
    , m_statusBarHeight(24.0f)
    , m_textureBrowserForMaterial(true)
{
}

// Destructor
EditorApp::~EditorApp() {
    Shutdown();
}

bool EditorApp::Initialize(int width, int height, const char* title) {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Configure OpenGL context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);  // MSAA
    
    // Create window
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);  // V-sync
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(m_window);
        glfwTerminate();
        return false;
    }
    
    // OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
    
    // Initialize ImGui
    InitImGui();
    
    // Initialize editor subsystems
    m_grid = std::make_unique<Grid>();
    m_viewportLayout = std::make_unique<ViewportLayout>();
    m_selection = std::make_unique<SelectionManager>();
    m_gizmo = std::make_unique<Gizmo>();
    m_blockTool = std::make_unique<BlockTool>();
    m_textureBrowser = std::make_unique<TextureBrowser>();

    // Try to load the demo map - SAU first (editor native), then JSON
    if (!LoadMap("assets/maps/moody_demo.sau")) {
        if (!LoadMap("assets/maps/moody_demo.json")) {
            if (!LoadMap("assets/maps/bsp_demo.json")) {
                if (!LoadMap("assets/maps/testmap.json")) {
                    NewMap();
                }
            }
        }
    }
    
    // Set default save path for editor output (SAU for editor work)
    m_currentMapPath = "assets/maps/moody_demo.sau";

    m_running = true;
    m_firstFrame = true;
    return true;
}

void EditorApp::InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Enable docking
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Purple/Violet theme
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    
    // Purple/Violet color palette
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.93f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.45f, 0.55f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.08f, 0.06f, 0.12f, 1.00f);  // Deep purple-black
    colors[ImGuiCol_ChildBg]                = ImVec4(0.10f, 0.08f, 0.14f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.12f, 0.10f, 0.18f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.40f, 0.25f, 0.55f, 0.50f);  // Purple border
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.18f, 0.12f, 0.25f, 1.00f);  // Purple frame
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.30f, 0.20f, 0.45f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.45f, 0.30f, 0.60f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.12f, 0.08f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.35f, 0.15f, 0.50f, 1.00f);  // Bright violet active
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.08f, 0.05f, 0.12f, 0.75f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.10f, 0.07f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.08f, 0.06f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.40f, 0.25f, 0.55f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.50f, 0.35f, 0.65f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.60f, 0.45f, 0.75f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.75f, 0.50f, 0.90f, 1.00f);  // Bright violet
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.55f, 0.35f, 0.70f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.70f, 0.50f, 0.85f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.30f, 0.18f, 0.42f, 1.00f);  // Purple button
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.45f, 0.28f, 0.58f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.55f, 0.35f, 0.70f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.35f, 0.20f, 0.48f, 1.00f);  // Violet header
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.30f, 0.60f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.55f, 0.40f, 0.72f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.40f, 0.25f, 0.55f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.55f, 0.35f, 0.70f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.65f, 0.45f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.50f, 0.30f, 0.65f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.60f, 0.40f, 0.75f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.70f, 0.50f, 0.85f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.20f, 0.12f, 0.30f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.45f, 0.28f, 0.58f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.40f, 0.22f, 0.52f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.12f, 0.08f, 0.18f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.25f, 0.15f, 0.35f, 1.00f);
    colors[ImGuiCol_DockingPreview]         = ImVec4(0.60f, 0.40f, 0.80f, 0.70f);  // Purple docking
    colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.08f, 0.06f, 0.12f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.70f, 0.50f, 0.85f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.85f, 0.60f, 0.95f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.65f, 0.45f, 0.80f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(0.80f, 0.55f, 0.90f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.15f, 0.10f, 0.22f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.40f, 0.25f, 0.55f, 1.00f);
    colors[ImGuiCol_TableBorderLight]       = ImVec4(0.30f, 0.18f, 0.42f, 1.00f);
    colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt]          = ImVec4(0.20f, 0.12f, 0.28f, 0.40f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.50f, 0.30f, 0.65f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(0.75f, 0.50f, 0.90f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.65f, 0.45f, 0.80f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(0.75f, 0.55f, 0.90f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.20f, 0.15f, 0.25f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.10f, 0.08f, 0.15f, 0.60f);
    
    // Initialize ImGui backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void EditorApp::SetupDockingLayout() {
    ImGuiID dockspaceId = ImGui::GetID("EditorDockspace");
    
    // Clear existing layout
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);
    
    // Split the dockspace into regions
    ImGuiID dockLeft, dockCenter, dockRight, dockBottom;
    
    // Main split: Left (tools) | Center+Right
    ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.15f, &dockLeft, &dockCenter);
    
    // Split center: Center (viewports) | Right (properties)
    ImGuiID dockViewports;
    ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Right, 0.22f, &dockRight, &dockViewports);
    
    // Split viewports into 2x2 grid
    ImGuiID dockTop, dockBot;
    ImGui::DockBuilderSplitNode(dockViewports, ImGuiDir_Down, 0.5f, &dockBot, &dockTop);
    
    ImGuiID dockTopLeft, dockTopRight, dockBotLeft, dockBotRight;
    ImGui::DockBuilderSplitNode(dockTop, ImGuiDir_Right, 0.5f, &dockTopRight, &dockTopLeft);
    ImGui::DockBuilderSplitNode(dockBot, ImGuiDir_Right, 0.5f, &dockBotRight, &dockBotLeft);
    
    // Dock windows to regions
    ImGui::DockBuilderDockWindow("Toolbar", dockLeft);
    ImGui::DockBuilderDockWindow("Entity Palette", dockLeft);
    
    ImGui::DockBuilderDockWindow("3D Perspective", dockTopLeft);
    ImGui::DockBuilderDockWindow("Top View", dockTopRight);
    ImGui::DockBuilderDockWindow("Front View", dockBotLeft);
    ImGui::DockBuilderDockWindow("Side View", dockBotRight);
    
    ImGui::DockBuilderDockWindow("Properties", dockRight);
    ImGui::DockBuilderDockWindow("Build Output", dockRight);
    
    ImGui::DockBuilderFinish(dockspaceId);
}

// ============================================================================
// Shutdown
// ============================================================================

void EditorApp::Shutdown() {
    if (!m_running && !m_window) return;
    
    m_textureBrowser.reset();
    m_blockTool.reset();
    m_gizmo.reset();
    m_selection.reset();
    m_viewportLayout.reset();
    m_grid.reset();
    m_currentMap.reset();
    m_editorBrushes.clear();
    m_editorEntities.clear();
    
    ShutdownImGui();
    
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
    
    m_running = false;
}

void EditorApp::ShutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// ============================================================================
// Main Loop
// ============================================================================

void EditorApp::Run() {
    while (m_running && !glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        ProcessInput();
        
        BeginFrame();
        
        // Main dockspace over entire window
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        
        ImGuiWindowFlags dockspaceFlags = 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_MenuBar;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", nullptr, dockspaceFlags);
        ImGui::PopStyleVar(3);
        
        // Create dockspace
        ImGuiID dockspaceId = ImGui::GetID("EditorDockspace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
        
        // Setup initial layout and load default map on first frame
        if (m_firstFrame) {
            SetupDockingLayout();
#ifdef ASSETS_DIR
            std::string mapPath = std::string(ASSETS_DIR) + "/maps/moody_demo.sau";
            LoadMap(mapPath);
#else
            LoadMap("assets/maps/moody_demo.sau");
#endif
            m_firstFrame = false;
        }
        
        // Render UI components
        RenderMenuBar();
        
        ImGui::End();
        
        // Individual panels
        RenderToolbar();
        RenderEntityPalette();
        RenderPropertiesPanel();
        RenderViewports();
        RenderStatusBar();
        
        if (m_showBuildOutput) {
            RenderBuildOutput();
        }
        
        // Demo window for development
        if (m_showDemoWindow) {
            ImGui::ShowDemoWindow(&m_showDemoWindow);
        }
        
        EndFrame();
    }
}

// ============================================================================
// Frame Rendering
// ============================================================================

void EditorApp::BeginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Clear background
    glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void EditorApp::EndFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(m_window);
}

// ============================================================================
// Menu Bar
// ============================================================================

void EditorApp::RenderMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New", "Ctrl+N")) {
                NewMap();
            }
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                // Simple hard-coded path for now
#ifdef ASSETS_DIR
                std::string mapPath = std::string(ASSETS_DIR) + "/maps/moody_demo.sau";
                LoadMap(mapPath);
#else
                LoadMap("assets/maps/moody_demo.sau");
#endif
            }
            if (ImGui::MenuItem("Save", "Ctrl+S")) {
                if (m_currentMapPath.empty()) {
                    m_currentMapPath = "test.sau";
                }
                SaveMap(m_currentMapPath);
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {
                SaveMap("test.sau");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                m_running = false;
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z", false, CanUndo())) {
                Undo();
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, CanRedo())) {
                Redo();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete", "Del")) {
                DeleteSelectedBrushes();
                DeleteSelectedEntities();
            }
            if (ImGui::MenuItem("Select All", "Ctrl+A")) {
                m_selection->SelectAll(m_editorBrushes);
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Reset Layout")) {
                m_firstFrame = true;  // Re-setup layout
            }
            ImGui::Separator();
            ImGui::MenuItem("Build Output", nullptr, &m_showBuildOutput);
            ImGui::MenuItem("ImGui Demo", nullptr, &m_showDemoWindow);
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Build")) {
            if (ImGui::MenuItem("Build Map", "F9")) {
                BuildMap();
            }
            if (ImGui::MenuItem("Build & Run", "F5")) {
                BuildMap();
                // TODO: Launch game
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About OracularV2")) {
                // TODO: About dialog
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMenuBar();
    }
}

// ============================================================================
// Toolbar
// ============================================================================

void EditorApp::RenderToolbar() {
    ImGui::Begin("Toolbar");
    
    ImGui::Text("Tools");
    ImGui::Separator();
    
    // Tool buttons with highlighting
    bool selectActive = (m_currentTool == EditorTool::Select);
    bool blockActive = (m_currentTool == EditorTool::Block);
    bool entityActive = (m_currentTool == EditorTool::Entity);
    
    if (selectActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    if (ImGui::Button("Select (S)", ImVec2(-1, 0))) {
        m_currentTool = EditorTool::Select;
        if (m_blockTool) m_blockTool->Cancel();
    }
    if (selectActive) ImGui::PopStyleColor();
    
    if (blockActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    if (ImGui::Button("Block (B)", ImVec2(-1, 0))) {
        m_currentTool = EditorTool::Block;
    }
    if (blockActive) ImGui::PopStyleColor();
    
    if (entityActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
    if (ImGui::Button("Entity (E)", ImVec2(-1, 0))) {
        m_currentTool = EditorTool::Entity;
        if (m_blockTool) m_blockTool->Cancel();
    }
    if (entityActive) ImGui::PopStyleColor();
    
    ImGui::Spacing();
    ImGui::Text("Grid");
    ImGui::Separator();
    
    if (m_grid) {
        float gridSize = m_grid->GetSnapSize();
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##GridSize", (std::to_string((int)gridSize) + " units").c_str())) {
            float sizes[] = {1, 2, 4, 8, 16, 32, 64};
            for (float s : sizes) {
                if (ImGui::Selectable((std::to_string((int)s) + " units").c_str(), gridSize == s)) {
                    m_grid->SetSnapSize(s);
                }
            }
            ImGui::EndCombo();
        }
        
        if (ImGui::Button("[ Smaller", ImVec2(-1, 0))) {
            m_grid->DecreaseGridSize();
        }
        if (ImGui::Button("] Larger", ImVec2(-1, 0))) {
            m_grid->IncreaseGridSize();
        }
    }
    
    ImGui::End();
}

// ============================================================================
// Entity Palette
// ============================================================================

void EditorApp::RenderEntityPalette() {
    ImGui::Begin("Entity Palette");
    
    if (m_currentTool != EditorTool::Entity) {
        ImGui::TextDisabled("Select Entity tool to place entities");
    } else {
        ImGui::Text("Click in viewport to place:");
        ImGui::Separator();
        
        bool lightSel = (m_entityPaletteType == EntityPaletteType::Light);
        bool spawnSel = (m_entityPaletteType == EntityPaletteType::PlayerStart);
        bool triggerSel = (m_entityPaletteType == EntityPaletteType::Trigger);
        
        if (lightSel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("Light", ImVec2(-1, 0))) {
            m_entityPaletteType = EntityPaletteType::Light;
        }
        if (lightSel) ImGui::PopStyleColor();
        
        if (spawnSel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 1.0f));
        if (ImGui::Button("Player Start", ImVec2(-1, 0))) {
            m_entityPaletteType = EntityPaletteType::PlayerStart;
        }
        if (spawnSel) ImGui::PopStyleColor();
        
        if (triggerSel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.8f, 1.0f));
        if (ImGui::Button("Trigger", ImVec2(-1, 0))) {
            m_entityPaletteType = EntityPaletteType::Trigger;
        }
        if (triggerSel) ImGui::PopStyleColor();
    }
    
    ImGui::End();
}

// ============================================================================
// Properties Panel
// ============================================================================

void EditorApp::RenderPropertiesPanel() {
    ImGui::Begin("Properties");
    
    if (m_selection && m_selection->HasSelection()) {
        // Show brush properties
        auto& selectedBrushes = m_selection->GetSelectedBrushes();
        if (!selectedBrushes.empty()) {
            ImGui::Text("Brush (%zu selected)", selectedBrushes.size());
            ImGui::Separator();
            
            EditorBrush* brush = selectedBrushes[0];
            if (brush) {
                // Position
                Genesis::Vec3 min = brush->brush.position;
                Genesis::Vec3 max = brush->Max();
                
                bool changed = false;
                
                ImGui::Text("Position");
                if (ImGui::DragFloat3("Min##Pos", &min.x, m_grid->GetSnapSize())) {
                    min = m_grid->Snap(min);
                    brush->brush.position = min;
                    brush->brush.size = max - min;
                    changed = true;
                }
                if (ImGui::DragFloat3("Max##Pos", &max.x, m_grid->GetSnapSize())) {
                    max = m_grid->Snap(max);
                    brush->brush.size = max - brush->brush.position;
                    changed = true;
                }
                
                ImGui::Separator();
                ImGui::Text("Material");
                char matBuf[128];
                strncpy(matBuf, brush->brush.materialName.c_str(), sizeof(matBuf) - 1);
                matBuf[sizeof(matBuf) - 1] = '\0';
                if (ImGui::InputText("##Material", matBuf, sizeof(matBuf))) {
                    brush->brush.materialName = matBuf;
                    changed = true;
                }
                
                ImGui::Separator();
                ImGui::Text("Flags");
                bool castShadow = Genesis::HasFlag(brush->brush.flags, Genesis::BrushFlags::CastShadow);
                bool noCollision = Genesis::HasFlag(brush->brush.flags, Genesis::BrushFlags::NoCollision);
                
                if (ImGui::Checkbox("Cast Shadow", &castShadow)) {
                    if (castShadow)
                        brush->brush.flags = brush->brush.flags | Genesis::BrushFlags::CastShadow;
                    else
                        brush->brush.flags = static_cast<Genesis::BrushFlags>(
                            static_cast<uint32_t>(brush->brush.flags) & ~static_cast<uint32_t>(Genesis::BrushFlags::CastShadow));
                    changed = true;
                }
                if (ImGui::Checkbox("No Collision", &noCollision)) {
                    if (noCollision)
                        brush->brush.flags = brush->brush.flags | Genesis::BrushFlags::NoCollision;
                    else
                        brush->brush.flags = static_cast<Genesis::BrushFlags>(
                            static_cast<uint32_t>(brush->brush.flags) & ~static_cast<uint32_t>(Genesis::BrushFlags::NoCollision));
                    changed = true;
                }
                
                if (changed) m_mapModified = true;
                
                ImGui::Separator();
                
                // ====================================================================
                // Material / Texture Selection
                // ====================================================================
                ImGui::Text("Texture");
                
                // Thumbnail preview
                if (m_textureBrowser) {
                    // Try to show current texture thumbnail
                    // In a real engine, we'd use the texture system. Here we use the browser's simplified cache or placeholder
                    // For now, just a button with the name
                    if (ImGui::Button(brush->brush.materialName.c_str(), ImVec2(-1, 0))) {
                        // Open texture browser for base material
                        m_textureBrowserForMaterial = true;
                        m_textureBrowser->Open(brush->brush.materialName);
                        
                        // Set callback
                        m_textureBrowser->SetSelectCallback([this, brush](const std::string& texture) {
                            brush->brush.materialName = texture;
                            
                            // Auto-set normal map if it exists
                            // Assume standard naming like "glass/window01.png" -> "glass/window01_normal.png"
                            std::string base = texture.substr(0, texture.find_last_of('.'));
                            std::string ext = texture.substr(texture.find_last_of('.'));
                            std::string normalPath = base + "_normal" + ext;
                            
                            // TODO: Verify file existence before setting?
                            // For now just set the path suggestion
                            brush->brush.normalMapPath = normalPath;
                            
                            m_mapModified = true;
                        });
                    }
                } else {
                    char matBuf[128];
                    strncpy(matBuf, brush->brush.materialName.c_str(), sizeof(matBuf) - 1);
                    matBuf[sizeof(matBuf) - 1] = '\0';
                    if (ImGui::InputText("##Material", matBuf, sizeof(matBuf))) {
                        brush->brush.materialName = matBuf;
                        changed = true;
                    }
                }
                
                ImGui::Spacing();
                
                // ====================================================================
                // Shader Selection
                // ====================================================================
                ImGui::Text("Shader");
                // Shader selection
        const char* shaderTypeStr = Genesis::ShaderTypeToString(brush->brush.shaderType);
        if (ImGui::BeginCombo("Shader Type", shaderTypeStr)) {
            for (int i = 0; i <= (int)Genesis::ShaderType::Unlit; i++) {
                auto type = (Genesis::ShaderType)i;
                bool isSelected = (brush->brush.shaderType == type);
                if (ImGui::Selectable(Genesis::ShaderTypeToString(type), isSelected)) {
                    brush->brush.shaderType = type;
                    m_mapModified = true;
                }
                if (isSelected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        // Shader-specific properties
        if (brush->brush.shaderType == Genesis::ShaderType::Glass) {
                    ImGui::Indent();
                    ImGui::TextDisabled("Glass Properties");
                    
                    if (ImGui::SliderFloat("Transparency", &brush->brush.transparency, 0.0f, 1.0f)) changed = true;
                    if (ImGui::SliderFloat("Fresnel Power", &brush->brush.fresnelPower, 0.1f, 10.0f)) changed = true;
                    if (ImGui::SliderFloat("Roughness", &brush->brush.roughness, 0.0f, 1.0f)) changed = true;
                    
                    // Normal map button
                    std::string btnText = brush->brush.normalMapPath.empty() ? "Select Normal Map" : "Normal Map: " + brush->brush.normalMapPath;
                    if (ImGui::Button(btnText.c_str())) {
                         m_textureBrowserForMaterial = false;
                         // ... logic ...
                    }
                    ImGui::Unindent();
                    }
                else if (brush->brush.shaderType == Genesis::ShaderType::Metal) {
                    ImGui::Indent();
                    ImGui::TextDisabled("Metal Properties");
                    if (ImGui::SliderFloat("Roughness", &brush->brush.roughness, 0.0f, 1.0f)) changed = true;
                    if (ImGui::SliderFloat("Metallic", &brush->brush.metallic, 0.0f, 1.0f)) changed = true;
                    ImGui::Unindent();
                }
                
                ImGui::Separator();
                if (ImGui::Button("Delete Brush", ImVec2(-1, 0))) {
                    DeleteSelectedBrushes();
                    // Don't access brush after this!
                } else {
                    // Safe to render browser modal here if needed, or in main loop
                    // The browser renders itself as a popup so it needs to be called in the UI hierarchy
                    if (m_textureBrowser) m_textureBrowser->Render();
                }
            }
        }
        
        // Show entity properties
        auto& selectedEntities = m_selection->GetSelectedEntities();
        if (!selectedEntities.empty()) {
            ImGui::Spacing();
            ImGui::Text("Entity (%zu selected)", selectedEntities.size());
            ImGui::Separator();
            
            EditorEntity* entity = selectedEntities[0];
            if (entity) {
                // Classname
                char classBuf[64];
                strncpy(classBuf, entity->entity.classname.c_str(), sizeof(classBuf) - 1);
                classBuf[sizeof(classBuf) - 1] = '\0';
                ImGui::Text("Classname");
                if (ImGui::InputText("##Classname", classBuf, sizeof(classBuf))) {
                    entity->entity.classname = classBuf;
                    entity->SetClassname(classBuf); // Helper to update visual type
                    m_mapModified = true;
                }
                
                // Position
                ImGui::Text("Position");
                if (ImGui::DragFloat3("##EntPos", &entity->entity.position.x, m_grid->GetSnapSize())) {
                    entity->entity.position = m_grid->Snap(entity->entity.position);
                    m_mapModified = true;
                }
                
                // Light-specific properties
                if (entity->entity.classname == "light") {
                    ImGui::Separator();
                    ImGui::Text("Light Properties");
                    
                    // Color (Format: "R G B")
                    float color[3] = {1, 1, 1};
                    auto itColor = entity->entity.properties.find("color");
                    if (itColor != entity->entity.properties.end()) {
                        std::istringstream ss(itColor->second);
                        int r = 255, g = 255, b = 255;
                        ss >> r >> g >> b;
                        color[0] = r / 255.0f;
                        color[1] = g / 255.0f;
                        color[2] = b / 255.0f;
                    }
                    
                    if (ImGui::ColorEdit3("Color", color)) {
                        std::stringstream ss;
                        ss << (int)(color[0] * 255) << " " << (int)(color[1] * 255) << " " << (int)(color[2] * 255);
                        entity->entity.properties["color"] = ss.str();
                        m_mapModified = true;
                    }
                    
                    // Intensity
                    float intensity = 500.0f;
                    auto itI = entity->entity.properties.find("intensity");
                    if (itI != entity->entity.properties.end()) {
                        try { intensity = std::stof(itI->second); } catch(...) {}
                    }
                    if (ImGui::DragFloat("Intensity", &intensity, 10.0f, 0.0f, 10000.0f)) {
                        entity->entity.properties["intensity"] = std::to_string(intensity);
                        m_mapModified = true;
                    }
                    
                    // Radius
                    float radius = 200.0f;
                    auto itRad = entity->entity.properties.find("radius");
                    if (itRad != entity->entity.properties.end()) {
                        try { radius = std::stof(itRad->second); } catch(...) {}
                    }
                    if (ImGui::DragFloat("Radius", &radius, 5.0f, 0.0f, 2000.0f)) {
                        entity->entity.properties["radius"] = std::to_string(radius);
                        m_mapModified = true;
                    }
                    
                    // Advanced Settings
                    ImGui::Separator();
                    ImGui::Text("Advanced Settings");

                    // Falloff
                    const char* falloffTypes[] = { "Linear", "Quadratic", "Constant" };
                    int currentFalloff = 0; // Default Linear
                    auto itFalloff = entity->entity.properties.find("falloff");
                    if (itFalloff != entity->entity.properties.end()) {
                        if (itFalloff->second == "quadratic") currentFalloff = 1;
                        else if (itFalloff->second == "constant") currentFalloff = 2;
                    }
                    
                    if (ImGui::Combo("Falloff", &currentFalloff, falloffTypes, IM_ARRAYSIZE(falloffTypes))) {
                        if (currentFalloff == 0) entity->entity.properties["falloff"] = "linear";
                        else if (currentFalloff == 1) entity->entity.properties["falloff"] = "quadratic";
                        else if (currentFalloff == 2) entity->entity.properties["falloff"] = "constant";
                        m_mapModified = true;
                    }
                    
                    // Cast Shadows
                    bool castShadows = true;
                    auto itShadow = entity->entity.properties.find("cast_shadows");
                    if (itShadow != entity->entity.properties.end()) {
                        castShadows = (itShadow->second == "true" || itShadow->second == "1");
                    }
                    
                    if (ImGui::Checkbox("Cast Shadows", &castShadows)) {
                        entity->entity.properties["cast_shadows"] = castShadows ? "true" : "false";
                        m_mapModified = true;
                    }
                }
                
                // Generic key-value properties
                ImGui::Separator();
                ImGui::Text("Custom Properties");
                for (auto& [key, value] : entity->entity.properties) {
                    // Skip properties that have dedicated UI controls
                    if (entity->entity.classname == "light") {
                        if (key == "color" || key == "intensity" || key == "radius" || 
                            key == "falloff" || key == "cast_shadows") continue;
                    }
                    
                    char valBuf[256];
                    strncpy(valBuf, value.c_str(), sizeof(valBuf) - 1);
                    valBuf[sizeof(valBuf) - 1] = '\0';
                    if (ImGui::InputText(key.c_str(), valBuf, sizeof(valBuf))) {
                        value = valBuf;
                        m_mapModified = true;
                    }
                }
                
                if (ImGui::Button("Delete Entity", ImVec2(-1, 0))) {
                    DeleteSelectedEntities();
                }
            }
        }
    } else {
        ImGui::TextDisabled("No selection");
        ImGui::Spacing();
        ImGui::TextWrapped("Use Select tool to click on brushes/entities, or use Block/Entity tools to create new objects.");
    }
    
    ImGui::End();
}

// ============================================================================
// Viewports
// ============================================================================

void EditorApp::RenderViewports() {
    if (m_viewportLayout) {
        m_viewportLayout->Render(m_grid.get(), m_currentMap.get(), 
                                  &m_editorBrushes, &m_editorEntities,
                                  m_selection.get(), 
                                  m_gizmo.get(), m_blockTool.get(),
                                  m_currentTool, m_entityPaletteType);
    }
}

// ============================================================================
// Status Bar
// ============================================================================

void EditorApp::RenderStatusBar() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, 
        viewport->WorkPos.y + viewport->WorkSize.y - m_statusBarHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, m_statusBarHeight));
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
    ImGui::Begin("##StatusBar", nullptr, 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoDocking);
    
    // Current tool
    const char* toolNames[] = {"Select", "Block", "Entity"};
    ImGui::Text("Tool: %s", toolNames[(int)m_currentTool]);
    ImGui::SameLine(120);
    
    // Grid info
    if (m_grid) {
        ImGui::Text("Grid: %.0f", m_grid->GetSnapSize());
    }
    
    ImGui::SameLine(220);
    
    // Object counts
    ImGui::Text("Brushes: %zu  Entities: %zu", m_editorBrushes.size(), m_editorEntities.size());
    
    ImGui::SameLine(420);
    
    // Selection info
    if (m_selection && m_selection->HasSelection()) {
        ImGui::Text("Selected: %zu", m_selection->GetSelectedCount());
    }
    
    ImGui::SameLine(550);
    
    // Status Message Overlay (Red, disappearing)
    if (m_statusMessageTime > 0.0f) {
        m_statusMessageTime -= ImGui::GetIO().DeltaTime;
        
        // Draw directly on screen (Bottom Left area, above status bar)
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        ImVec2 pos(20.0f, viewport->WorkPos.y + viewport->WorkSize.y - 60.0f);
        drawList->AddText(ImGui::GetFont(), 24.0f, pos, IM_COL32(255, 0, 0, 255), m_statusMessage.c_str());
    }


    
    // Modified indicator
    if (m_mapModified) {
        ImGui::TextColored(ImVec4(1, 0.7f, 0, 1), "[Modified]");
    }
    
    // Map path
    if (!m_currentMapPath.empty()) {
        ImGui::SameLine(ImGui::GetWindowWidth() - 200);
        ImGui::TextDisabled("%s", m_currentMapPath.c_str());
    }
    
    ImGui::PopStyleVar();
    ImGui::End();
}

// ============================================================================
// Build Output
// ============================================================================

void EditorApp::RenderBuildOutput() {
    ImGui::Begin("Build Output", &m_showBuildOutput);
    
    if (ImGui::Button("Clear")) {
        m_buildLog.clear();
    }
    ImGui::SameLine();
    if (ImGui::Button("Build Now")) {
        BuildMap();
    }
    
    ImGui::Separator();
    
    ImGui::BeginChild("BuildLog", ImVec2(0, 0), true);
    ImGui::TextUnformatted(m_buildLog.c_str());
    ImGui::EndChild();
    
    ImGui::End();
}

// ============================================================================
// Input Processing
// ============================================================================

void EditorApp::ProcessInput() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard) return;
    
    // Escape to deselect or exit
    static bool escPressed = false;
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (!escPressed) {
            if (m_blockTool && m_blockTool->IsActive()) {
                m_blockTool->Cancel();
            } else if (m_selection && m_selection->HasSelection()) {
                m_selection->ClearSelection();
            }
            escPressed = true;
        }
    } else escPressed = false;
    
    // Delete key
    static bool deletePressed = false;
    if (glfwGetKey(m_window, GLFW_KEY_DELETE) == GLFW_PRESS) {
        if (!deletePressed) {
            DeleteSelectedBrushes();
            DeleteSelectedEntities();
            deletePressed = true;
        }
    } else deletePressed = false;
    
    // Tool shortcuts
    static bool keyS = false, keyB = false, keyE = false;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS && !keyS && !io.WantTextInput) {
        m_currentTool = EditorTool::Select;
        if (m_blockTool) m_blockTool->Cancel();
        keyS = true;
    } else if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_RELEASE) keyS = false;
    
    if (glfwGetKey(m_window, GLFW_KEY_B) == GLFW_PRESS && !keyB && !io.WantTextInput) {
        m_currentTool = EditorTool::Block;
        keyB = true;
    } else if (glfwGetKey(m_window, GLFW_KEY_B) == GLFW_RELEASE) keyB = false;
    
    if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS && !keyE && !io.WantTextInput) {
        m_currentTool = EditorTool::Entity;
        if (m_blockTool) m_blockTool->Cancel();
        keyE = true;
    } else if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_RELEASE) keyE = false;
    
    // Grid size shortcuts
    static bool keyLB = false, keyRB = false;
    if (glfwGetKey(m_window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS && !keyLB) {
        if (m_grid) m_grid->DecreaseGridSize();
        keyLB = true;
    } else if (glfwGetKey(m_window, GLFW_KEY_LEFT_BRACKET) == GLFW_RELEASE) keyLB = false;
    
    if (glfwGetKey(m_window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS && !keyRB) {
        if (m_grid) m_grid->IncreaseGridSize();
        keyRB = true;
    } else if (glfwGetKey(m_window, GLFW_KEY_RIGHT_BRACKET) == GLFW_RELEASE) keyRB = false;
    
    // Build shortcut
    static bool keyF9 = false;
    if (glfwGetKey(m_window, GLFW_KEY_F9) == GLFW_PRESS && !keyF9) {
        BuildMap();
        keyF9 = true;
    } else if (glfwGetKey(m_window, GLFW_KEY_F9) == GLFW_RELEASE) keyF9 = false;
    
    // Save shortcut
    static bool keySave = false;
    if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS && 
        (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || 
         glfwGetKey(m_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) && !keySave) {
        if (m_currentMapPath.empty()) {
#ifdef ASSETS_DIR
            m_currentMapPath = std::string(ASSETS_DIR) + "/maps/moody_demo.sau";
#else
            m_currentMapPath = "assets/maps/current.json";
#endif
        }
        SaveMap(m_currentMapPath);
        keySave = true;
    } else if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_RELEASE) keySave = false;
    
    // Undo shortcut (Ctrl+Z)
    static bool keyUndo = false;
    if (glfwGetKey(m_window, GLFW_KEY_Z) == GLFW_PRESS && 
        (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || 
         glfwGetKey(m_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) && !keyUndo) {
        Undo();
        keyUndo = true;
    } else if (glfwGetKey(m_window, GLFW_KEY_Z) == GLFW_RELEASE) keyUndo = false;
    
    // Redo shortcut (Ctrl+Y or Ctrl+Shift+Z)
    static bool keyRedo = false;
    if (((glfwGetKey(m_window, GLFW_KEY_Y) == GLFW_PRESS) ||
         (glfwGetKey(m_window, GLFW_KEY_Z) == GLFW_PRESS && 
          (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
           glfwGetKey(m_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS))) &&
        (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || 
         glfwGetKey(m_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) && !keyRedo) {
        Redo();
        keyRedo = true;
    } else if (glfwGetKey(m_window, GLFW_KEY_Y) == GLFW_RELEASE && 
               glfwGetKey(m_window, GLFW_KEY_Z) == GLFW_RELEASE) keyRedo = false;
}

// ============================================================================
// Editor Brush Management
// ============================================================================

void EditorApp::AddEditorBrush(const EditorBrush& brush) {
    SaveUndoState("Add Brush");
    EditorBrush newBrush = brush;
    newBrush.editorId = m_nextBrushId++;
    m_editorBrushes.push_back(newBrush);
    m_mapModified = true;
    
    m_buildLog += "Added brush #" + std::to_string(newBrush.editorId) + "\n";
}

void EditorApp::DeleteSelectedBrushes() {
    if (!m_selection || m_selection->GetSelectedBrushes().empty()) return;
    
    SaveUndoState("Delete Brushes");
    auto selected = m_selection->GetSelectedBrushes();
    
    for (auto* brush : selected) {
        auto it = std::find_if(m_editorBrushes.begin(), m_editorBrushes.end(),
            [brush](const EditorBrush& eb) { return &eb == brush; });
        if (it != m_editorBrushes.end()) {
            m_buildLog += "Deleted brush #" + std::to_string(it->editorId) + "\n";
            m_editorBrushes.erase(it);
        }
    }
    
    m_selection->ClearSelection();
    m_mapModified = true;
}

// ============================================================================
// Editor Entity Management
// ============================================================================

void EditorApp::AddEditorEntity(const EditorEntity& entity) {
    SaveUndoState("Add Entity");
    EditorEntity newEntity = entity;
    newEntity.editorId = m_nextEntityId++;
    m_editorEntities.push_back(newEntity);
    m_mapModified = true;
    
    m_buildLog += "Added entity: " + newEntity.entity.classname + " #" + std::to_string(newEntity.editorId) + "\n";
}

void EditorApp::DeleteSelectedEntities() {
    if (!m_selection || m_selection->GetSelectedEntities().empty()) return;
    
    SaveUndoState("Delete Entity");
    
    // Copy the list of selected entities to delete (because clearing selection clears the original list)
    std::vector<uint32_t> idsToDelete;
    for (auto* ent : m_selection->GetSelectedEntities()) {
        if (ent) idsToDelete.push_back(ent->editorId);
    }
    
    // Clear selection first to avoiding dangling pointers in SelectionManager
    m_selection->ClearSelection();
    
    // Remove from main list
    m_editorEntities.erase(
        std::remove_if(m_editorEntities.begin(), m_editorEntities.end(),
            [&](const EditorEntity& ent) {
                return std::find(idsToDelete.begin(), idsToDelete.end(), ent.editorId) != idsToDelete.end();
            }),
        m_editorEntities.end()
    );
    
    m_mapModified = true;
    m_buildLog += "Deleted " + std::to_string(idsToDelete.size()) + " entities\n";
}

// ============================================================================
// Map Management
// ============================================================================

void EditorApp::NewMap() {
    m_currentMap = std::make_unique<Genesis::Map>("Untitled");
    m_currentMapPath.clear();
    m_mapModified = false;
    m_editorBrushes.clear();
    m_editorEntities.clear();
    m_nextBrushId = 1;
    m_nextEntityId = 1;
    
    if (m_selection) {
        m_selection->ClearSelection();
    }
    if (m_blockTool) {
        m_blockTool->Cancel();
    }
    
    m_buildLog = "New map created\n";
}

bool EditorApp::LoadMap(const std::string& path) {
    // Use engine's MapLoader which auto-detects format (JSON, SAU, etc)
    // skipBuild=true since editor doesn't need game meshes/materials
    auto& loader = Genesis::MapLoader::Instance();
    loader.SetBasePath("");  // Use absolute paths
    auto map = loader.Load(path, true);  // skipBuild = true
    if (map) {
        m_currentMap = std::move(map);
        m_currentMapPath = path;
        m_mapModified = false;
        
        // Convert Map brushes to EditorBrushes
        m_editorBrushes.clear();
        m_nextBrushId = 1;
        for (const auto& brush : m_currentMap->GetBrushes()) {
            EditorBrush eb;
            eb.brush = brush;
            eb.editorId = m_nextBrushId++;
            m_editorBrushes.push_back(eb);
        }
        
        // Convert Map entities to EditorEntities
        m_editorEntities.clear();
        m_nextEntityId = 1;
        for (const auto& entity : m_currentMap->GetEntities()) {
            EditorEntity ee;
            ee.entity = entity;
            ee.editorId = m_nextEntityId++;
            m_editorEntities.push_back(ee);
        }
        
        if (m_selection) {
            m_selection->ClearSelection();
        }
        
        m_buildLog += "Loaded map: " + path + "\n";
        m_buildLog += "  Brushes: " + std::to_string(m_editorBrushes.size()) + "\n";
        m_buildLog += "  Entities: " + std::to_string(m_editorEntities.size()) + "\n";
        return true;
    }
    
    m_buildLog += "Failed to load: " + path + "\n";
    return false;
}

bool EditorApp::SaveMap(const std::string& path) {
    if (!m_currentMap) return false;
    
    // Sync editor brushes to Map
    m_currentMap->ClearBrushes();
    for (const auto& eb : m_editorBrushes) {
        m_currentMap->AddBrush(eb.brush);
    }
    
    // Sync editor entities to Map
    m_currentMap->ClearEntities();
    for (const auto& ee : m_editorEntities) {
        m_currentMap->AddEntity(ee.entity);
    }
    
    // Detect format by file extension
    bool success = false;
    size_t dotPos = path.rfind('.');
    std::string ext = (dotPos != std::string::npos) ? path.substr(dotPos) : "";
    
    if (ext == ".json") {
        // Save as JSON (game compatible)
        auto& loader = Genesis::MapLoader::Instance();
        loader.SetBasePath("");  // Use absolute paths
        success = loader.SaveJSON(*m_currentMap, path);
    } else {
        // Default (SAU or others)
        success = SAU::Save(*m_currentMap, path);
    }
    
    if (success) {
        m_currentMapPath = path;
        m_mapModified = false;
        m_buildLog += "Saved map to: " + path + "\n";
        ShowStatusMessage("Map Saved");
    } else {
        m_buildLog += "Failed to save: " + path + "\n";
        ShowStatusMessage("Save Failed!");
    }
    
    return success;
}

// ============================================================================
// Build Pipeline
// ============================================================================

void EditorApp::BuildMap() {
    m_showBuildOutput = true;
    m_buildLog += "\n========================================\n";
    m_buildLog += "  Starting Build...\n";
    m_buildLog += "========================================\n";
    
    // First save to temp file
    std::string tempPath = "temp_build.sau";
    if (!SaveMap(tempPath)) {
        m_buildLog += "ERROR: Failed to save temp file\n";
        return;
    }
    
    // Build using Build module
    Build::Options options;
    options.verbose = true;
    options.runLightBake = true;
    options.runGame = false;
    
    m_buildLog += "Compiling BSP tree...\n";
    
    // Redirect cout to our log
    std::stringstream logStream;
    std::streambuf* oldCout = std::cout.rdbuf(logStream.rdbuf());
    
    bool success = Build::BuildMap(tempPath, "output.bsp", options);
    
    // Restore cout
    std::cout.rdbuf(oldCout);
    m_buildLog += logStream.str();
    
    if (success) {
        m_buildLog += "\nBuild completed successfully!\n";
    } else {
        m_buildLog += "\nBuild failed!\n";
    }
    
    m_buildLog += "========================================\n";
}

// ============================================================================
// Undo/Redo
// ============================================================================

void EditorApp::SaveUndoState(const std::string& description) {
    m_undoManager.PushState(m_editorBrushes, m_editorEntities, description);
}

void EditorApp::Undo() {
    if (m_undoManager.Undo(m_editorBrushes, m_editorEntities)) {
        if (m_selection) {
            m_selection->ClearSelection();
        }
        m_mapModified = true;
        m_buildLog += "Undo\n";
    }
}

void EditorApp::Redo() {
    if (m_undoManager.Redo(m_editorBrushes, m_editorEntities)) {
        if (m_selection) {
            m_selection->ClearSelection();
        }
        m_mapModified = true;
        m_buildLog += "Redo\n";
    }
}

void EditorApp::ShowStatusMessage(const std::string& message, float duration) {
    m_statusMessage = message;
    m_statusMessageTime = duration;
}
