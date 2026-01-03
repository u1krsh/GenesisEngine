#pragma once

// ============================================================================
// Genesis Engine Map System
// Include this single header for the complete map functionality
//
// Source Engine-like design:
// - Map = Collection of Brushes (world geometry)
// - Brush = Shape + Material + Collider
// - MapLoader = Parses map files (JSON or simple text)
// - MapRenderer = Syncs map to rendering and collision systems
// - MeshLibrary = Caches primitive meshes
//
// Future extensions:
// - BSP compilation for visibility/collision optimization
// - Lightmap baking
// - Area portals
// - Displacement surfaces
//
// Usage:
//   // Load a map from file
//   auto& mapRenderer = MapRenderer::Instance();
//   if (mapRenderer.LoadMap("testmap.json")) {
//       Vec3 spawn = mapRenderer.GetSpawnPosition();
//       player.SetPosition(spawn);
//   }
//
//   // Or build a map programmatically
//   auto map = std::make_shared<Map>("MyMap");
//
//   Brush floor;
//   floor.shape = BrushShape::Cube;
//   floor.position = Vec3(0, -0.5f, 0);
//   floor.size = Vec3(20, 1, 20);
//   floor.materialName = "floor";
//   map->AddBrush(floor);
//
//   mapRenderer.SetActiveMap(map);
//
// Map file format (JSON):
//   {
//     "name": "Test Map",
//     "author": "Developer",
//     "spawn_position": [0, 1, 0],
//     "brushes": [
//       {
//         "shape": "cube",
//         "position": [0, 0.5, 0],
//         "size": [10, 1, 10],
//         "material": "floor"
//       },
//       {
//         "shape": "cube",
//         "position": [5, 1.5, 0],
//         "size": [1, 3, 10],
//         "material": "wall"
//       }
//     ]
//   }
//
// Simple text format (.map):
//   @name "Test Map"
//   @spawn 0 1 0
//
//   # shape posX posY posZ sizeX sizeY sizeZ material [flags]
//   cube 0 0.5 0 10 1 10 floor
//   cube 5 1.5 0 1 3 10 wall
//   cube 0 0.25 5 2 0.5 1 floor stair
//
// ============================================================================

#include "map/Brush.h"
#include "map/Map.h"
#include "map/MeshLibrary.h"
#include "map/MapLoader.h"
#include "map/MapRenderer.h"

