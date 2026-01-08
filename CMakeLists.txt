cmake_minimum_required(VERSION 3.20)
project(GenesisEngine)

set(CMAKE_CXX_STANDARD 20)

# ============================================================================
# External Dependencies
# ============================================================================

include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# Fetch GLM (header-only math library)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# Fetch ImGui (for OracularV2 editor)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# Build ImGui as a library with GLFW + OpenGL3 backends
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

# GLAD (local)
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# Find OpenGL
find_package(OpenGL REQUIRED)

# ============================================================================
# Engine Library
# ============================================================================

set(ENGINE_SOURCES
    # Core
    src/core/Engine.cpp

    # Input
    src/input/GLFWInputBackend.cpp
    src/input/InputManager.cpp

    # Camera
    src/camera/Camera.cpp

    # Player
    src/player/PlayerController.cpp

    # Renderer
    src/renderer/shader/Shader.cpp
    src/renderer/DebugRenderer.cpp
    src/renderer/mesh/Mesh.cpp
    src/renderer/mesh/MeshPrimitives.cpp
    src/renderer/material/Material.cpp
    src/renderer/material/MaterialLibrary.cpp
    src/renderer/Renderer.cpp
    src/renderer/world/StaticWorldRenderer.cpp

    # GUI
    src/gui/GUIRenderer.cpp
    src/gui/Console.cpp
    src/gui/DebugOverlay.cpp

    # Map System
    src/map/MapLoader.cpp
    src/map/MapRenderer.cpp

    # BSP System
    src/bsp/BSPTree.cpp
    src/bsp/BSPCompiler.cpp
    src/bsp/BSPRenderer.cpp
    src/bsp/BSPCollision.cpp
    src/bsp/BSPPVS.cpp
    src/bsp/BSPBuildVisualizer.cpp
    src/bsp/LightmapAtlas.cpp
    src/bsp/LightBaker.cpp
)

set(ENGINE_HEADERS
    # Core
    src/core/Engine.h
    src/core/Time.h
    src/core/Logger.h

    # Math
    src/math/Math.h

    # Input
    src/input/Input.h
    src/input/InputTypes.h
    src/input/InputManager.h
    src/input/IInputBackend.h
    src/input/GLFWInputBackend.h

    # Camera
    src/camera/Camera.h

    # Player
    src/player/PlayerController.h

    # Renderer
    src/renderer/shader/Shader.h
    src/renderer/DebugRenderer.h
    src/renderer/mesh/VertexLayout.h
    src/renderer/mesh/Mesh.h
    src/renderer/mesh/MeshBuilder.h
    src/renderer/mesh/MeshPrimitives.h
    src/renderer/material/MaterialProperty.h
    src/renderer/material/Material.h
    src/renderer/material/MaterialLibrary.h
    src/renderer/Renderer.h
    src/renderer/Material.h
    src/renderer/Mesh.h
    src/renderer/world/StaticWorldRenderer.h

    # GUI
    src/gui/GUITypes.h
    src/gui/GUIRenderer.h
    src/gui/Console.h
    src/gui/DebugOverlay.h
    src/gui/widgets/Widgets.h
    src/gui/widgets/Widget.h
    src/gui/widgets/Button.h
    src/gui/widgets/Panel.h
    src/gui/widgets/Label.h
    src/gui/widgets/Checkbox.h
    src/gui/widgets/Slider.h
    src/gui/widgets/TextField.h

    # Map System
    src/Map.h
    src/map/Brush.h
    src/map/Map.h
    src/map/MeshLibrary.h
    src/map/MapLoader.h
    src/map/MapRenderer.h

    # BSP System
    src/bsp/BSPTypes.h
    src/bsp/BSPTree.h
    src/bsp/BSPCompiler.h
    src/bsp/BSPRenderer.h
    src/bsp/BSPCollision.h
    src/bsp/BSPPVS.h
    src/bsp/BSPBuildVisualizer.h
    src/bsp/LightmapAtlas.h
    src/bsp/LightBaker.h
)

add_library(GenesisEngineLib STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})
target_include_directories(GenesisEngineLib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(GenesisEngineLib PUBLIC glfw glad OpenGL::GL glm::glm)

# ============================================================================
# Game Executable
# ============================================================================

set(GAME_SOURCES
    game/main.cpp
    game/Player.cpp
)

set(GAME_HEADERS
    game/Player.h
)

add_executable(GenesisEngine ${GAME_SOURCES} ${GAME_HEADERS})
target_include_directories(GenesisEngine PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/game)
target_link_libraries(GenesisEngine PRIVATE GenesisEngineLib)

# ============================================================================
# Copy assets to build directory (for running from build folder)
# ============================================================================

add_custom_command(TARGET GenesisEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:GenesisEngine>/assets
)

# ============================================================================
# OracularV2 Map Editor
# ============================================================================

set(ORACULAR_SOURCES
    tools/OracularV2/main.cpp
    tools/OracularV2/EditorApp.cpp
    tools/OracularV2/Grid.cpp
    tools/OracularV2/Viewport.cpp
    tools/OracularV2/ViewportLayout.cpp
    tools/OracularV2/SelectionManager.cpp
    tools/OracularV2/Gizmo.cpp
    tools/OracularV2/SAUFormat.cpp
    tools/OracularV2/BuildMap.cpp
    tools/OracularV2/tools/BlockTool.cpp
)

set(ORACULAR_HEADERS
    tools/OracularV2/EditorApp.h
    tools/OracularV2/Grid.h
    tools/OracularV2/Viewport.h
    tools/OracularV2/ViewportLayout.h
    tools/OracularV2/EditorBrush.h
    tools/OracularV2/SelectionManager.h
    tools/OracularV2/Gizmo.h
    tools/OracularV2/SAUFormat.h
    tools/OracularV2/BuildMap.h
    tools/OracularV2/tools/BlockTool.h
)

add_executable(OracularV2 ${ORACULAR_SOURCES} ${ORACULAR_HEADERS})
target_include_directories(OracularV2 PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tools/OracularV2
)

# Find X11 for ImGui multi-viewport support on Linux
find_package(X11 REQUIRED)
target_link_libraries(OracularV2 PRIVATE GenesisEngineLib imgui ${X11_LIBRARIES})

# Copy assets for OracularV2 too
add_custom_command(TARGET OracularV2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:OracularV2>/assets
)

# Set working directory for CLion/VS debugging
set_target_properties(OracularV2 PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
)
