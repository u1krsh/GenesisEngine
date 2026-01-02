#pragma once

// ============================================================================
// Genesis Engine Material System
// Include this single header to use the full material abstraction
//
// Source Engine-like design:
// - Shader: Low-level GPU program (defines "type" of surface)
// - Material: Surface properties (defines "instance" - colors, textures, etc.)
// - Mesh: Geometry data (just vertices/indices, no rendering logic)
// - Renderer: Handles global state (camera, lighting) and draw calls
//
// One shader → many materials → zero duplicate logic
//
// Usage:
//   // Setup (once)
//   auto& matLib = MaterialLibrary::Instance();
//   auto redMetal = matLib.CreateSolidColor("RedMetal", Vec3(0.8f, 0.2f, 0.2f));
//   redMetal->SetMetallic(0.9f);
//   redMetal->SetRoughness(0.3f);
//
//   // Render loop
//   auto& renderer = Renderer::Instance();
//   renderer.BeginFrame(camera);
//   renderer.SetDirectionalLight(sunLight);
//
//   // Option 1: Immediate mode
//   renderer.Draw(cubeMesh, redMetal, cubeTransform);
//
//   // Option 2: Deferred (sorted) mode
//   renderer.Submit(cubeMesh, redMetal, cubeTransform);
//
//   renderer.EndFrame();
// ============================================================================

#include "renderer/material/MaterialProperty.h"
#include "renderer/material/Material.h"
#include "renderer/material/MaterialLibrary.h"
#include "renderer/Renderer.h"

