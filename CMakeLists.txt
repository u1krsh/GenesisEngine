cmake_minimum_required(VERSION 3.20)
project(GenesisEngine)

set(CMAKE_CXX_STANDARD 20)

# Fetch GLFW
include(FetchContent)
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

# GLAD
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Source files
set(GENESIS_SOURCES
    main.cpp
    src/input/GLFWInputBackend.cpp
    src/input/InputManager.cpp
    src/camera/Camera.cpp
    src/renderer/shader/Shader.cpp
)

# Main executable
add_executable(GenesisEngine ${GENESIS_SOURCES})
target_include_directories(GenesisEngine PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(GenesisEngine PRIVATE glfw glad OpenGL::GL)
