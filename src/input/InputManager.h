#pragma once

#include "IInputBackend.h"
#include "InputTypes.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace Genesis {

// ============================================================================
// Input Binding - Maps a key or mouse button to an action
// ============================================================================
struct InputBinding {
    enum class Type { Key, MouseButton };

    Type type;
    KeyCode key = KeyCode::Unknown;
    MouseButton mouseButton = MouseButton::Unknown;

    static InputBinding FromKey(KeyCode k) {
        InputBinding b;
        b.type = Type::Key;
        b.key = k;
        return b;
    }

    static InputBinding FromMouseButton(MouseButton mb) {
        InputBinding b;
        b.type = Type::MouseButton;
        b.mouseButton = mb;
        return b;
    }
};

// ============================================================================
// Input Manager - High level input with action bindings
// ============================================================================
class InputManager {
public:
    static InputManager& Instance() {
        static InputManager instance;
        return instance;
    }

    // Set the backend (call before Initialize)
    void SetBackend(std::unique_ptr<IInputBackend> backend);

    // Initialize with window handle
    bool Initialize(void* windowHandle);
    void Shutdown();

    // Call once per frame before checking input
    void Update();

    // ========================================================================
    // Action-based input (bindable)
    // ========================================================================
    void BindAction(GameAction action, InputBinding binding);
    void UnbindAction(GameAction action);
    void ClearBindings();

    bool IsActionDown(GameAction action) const;
    bool IsActionPressed(GameAction action) const;
    bool IsActionReleased(GameAction action) const;

    // ========================================================================
    // Direct input access (for when you need it)
    // ========================================================================
    bool IsKeyDown(KeyCode key) const;
    bool IsKeyPressed(KeyCode key) const;
    bool IsKeyReleased(KeyCode key) const;

    bool IsMouseButtonDown(MouseButton button) const;
    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonReleased(MouseButton button) const;

    void GetMousePosition(double& x, double& y) const;
    void GetMouseDelta(double& dx, double& dy) const;
    double GetScrollDelta() const;

    void SetCursorLocked(bool locked);
    bool IsCursorLocked() const;

    // ========================================================================
    // Default bindings setup
    // ========================================================================
    void SetupDefaultBindings();

    // Get backend (for advanced use)
    IInputBackend* GetBackend() { return m_backend.get(); }

private:
    InputManager() = default;
    ~InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    bool CheckBinding(const InputBinding& binding, bool (IInputBackend::*keyCheck)(KeyCode) const,
                      bool (IInputBackend::*mouseCheck)(MouseButton) const) const;

    // Internal helpers for Update()
    void HandleMouseLocking();
    void LogWASDKeys();

private:
    std::unique_ptr<IInputBackend> m_backend;
    std::unordered_map<GameAction, std::vector<InputBinding>> m_bindings;
};

} // namespace Genesis

