**A modern 3D Game Engine built from scratch in C++ and OpenGL.**

![Genesis Engine Lighting Demo](assets/screenshots/lighting_showcase.png)

## Overview

Genesis Engine is a custom-built game engine focusing on high-performance rendering and retro-style aesthetics modernized with advanced lighting techniques. It implements a full BSP (Binary Space Partitioning) pipeline similar to the Quake/Source engines but enhanced with modern lighting capabilities.

## Key Features

### Lighting & Rendering
- **BSP-Based Rendering**: Efficient visibility culling and collision detection.
- **Offline Light Baking**: High-performance static lighting with zero runtime cost.
- **Soft Shadows & Ambient Occlusion**:
  - Source-Engine style radiosity approximation.
  - Lightmap smoothing (3x3 Blur Pass) for clean, artifact-free shadows.
  - Edge dilation prevents texture bleeding artifacts.
- **Colored Lighting**: Support for multi-colored point and directional lights (Orange, Blue, Purple, Warm Sun).

### Architecture
- **Core**: C++17 with minimal dependencies (GLAD, GLFW, GLM).
- **Physics**: AABB-based collision detection against BSP trees (Quake-style trace).
- **Asset Pipeline**: Custom JSON map format with OBJ fallback.

## Building and Running

### Prerequisites
- CMake 3.10+
- C++17 Compiler (GCC/Clang/MSVC)
- OpenGL 4.5+ capable GPU

### Build Instructions
```bash
mkdir build && cd build
cmake ..
make -j4
./GenesisEngine
```
