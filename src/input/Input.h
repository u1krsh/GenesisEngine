#pragma once

// Genesis Engine Input System
// Include this header to use the input abstraction layer

#include "InputTypes.h"
#include "IInputBackend.h"
#include "GLFWInputBackend.h"
#include "InputManager.h"

// ============================================================================
// Quick Reference:
// ============================================================================
//
// Setup:
//   auto& input = Genesis::InputManager::Instance();
//   input.SetBackend(std::make_unique<Genesis::GLFWInputBackend>());
//   input.Initialize(windowHandle);
//   input.SetupDefaultBindings();
//
// In game loop (call once per frame before checking input):
//   input.Update();
//
// Action-based input (recommended):
//   if (input.IsActionPressed(Genesis::GameAction::Jump)) { ... }
//   if (input.IsActionDown(Genesis::GameAction::MoveForward)) { ... }
//
// Direct input (when needed):
//   if (input.IsKeyPressed(Genesis::KeyCode::Space)) { ... }
//   if (input.IsMouseButtonDown(Genesis::MouseButton::Left)) { ... }
//
// Mouse:
//   double mx, my;
//   input.GetMousePosition(mx, my);
//   input.GetMouseDelta(dx, dy);
//   input.SetCursorLocked(true);
//
// Custom bindings:
//   input.BindAction(GameAction::Jump, InputBinding::FromKey(KeyCode::Space));
//   input.BindAction(GameAction::Jump, InputBinding::FromKey(KeyCode::W)); // multiple bindings
//
// ============================================================================

