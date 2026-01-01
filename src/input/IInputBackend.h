#pragma once

#include "InputTypes.h"
#include <functional>

namespace Genesis {

// ============================================================================
// Input Interface - Abstract base for all input implementations
// ============================================================================
class IInputBackend {
public:
    virtual ~IInputBackend() = default;

    // Initialization
    virtual bool Initialize(void* windowHandle) = 0;
    virtual void Shutdown() = 0;

    // Per-frame update (call before processing input)
    virtual void Update() = 0;

    // Keyboard
    virtual bool IsKeyDown(KeyCode key) const = 0;
    virtual bool IsKeyPressed(KeyCode key) const = 0;   // Just pressed this frame
    virtual bool IsKeyReleased(KeyCode key) const = 0;  // Just released this frame

    // Mouse buttons
    virtual bool IsMouseButtonDown(MouseButton button) const = 0;
    virtual bool IsMouseButtonPressed(MouseButton button) const = 0;
    virtual bool IsMouseButtonReleased(MouseButton button) const = 0;

    // Mouse position
    virtual void GetMousePosition(double& x, double& y) const = 0;
    virtual void GetMouseDelta(double& dx, double& dy) const = 0;

    // Mouse scroll
    virtual double GetScrollDelta() const = 0;

    // Mouse control
    virtual void SetCursorMode(bool locked) = 0;
    virtual bool IsCursorLocked() const = 0;

    // Gamepad (future)
    virtual bool IsGamepadConnected(int gamepadId) const { return false; }
    virtual float GetGamepadAxis(int gamepadId, int axis) const { return 0.0f; }
    virtual bool IsGamepadButtonDown(int gamepadId, int button) const { return false; }
};

} // namespace Genesis

