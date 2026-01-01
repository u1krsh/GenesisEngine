#pragma once

#include "IInputBackend.h"
#include <GLFW/glfw3.h>
#include <array>

namespace Genesis {

// ============================================================================
// GLFW Input Backend Implementation
// ============================================================================
class GLFWInputBackend : public IInputBackend {
public:
    GLFWInputBackend() = default;
    ~GLFWInputBackend() override;

    // IInputBackend interface
    bool Initialize(void* windowHandle) override;
    void Shutdown() override;
    void Update() override;

    // Keyboard
    bool IsKeyDown(KeyCode key) const override;
    bool IsKeyPressed(KeyCode key) const override;
    bool IsKeyReleased(KeyCode key) const override;

    // Mouse buttons
    bool IsMouseButtonDown(MouseButton button) const override;
    bool IsMouseButtonPressed(MouseButton button) const override;
    bool IsMouseButtonReleased(MouseButton button) const override;

    // Mouse position
    void GetMousePosition(double& x, double& y) const override;
    void GetMouseDelta(double& dx, double& dy) const override;

    // Mouse scroll
    double GetScrollDelta() const override;

    // Mouse control
    void SetCursorMode(bool locked) override;
    bool IsCursorLocked() const override;

    // Forward key events from external callbacks (used by Engine)
    void ForwardKeyEvent(int glfwKey, int action);
    void ForwardMouseButtonEvent(int glfwButton, int action);
    void ForwardCursorPosEvent(double xpos, double ypos);

private:
    // Convert our KeyCode to GLFW key
    static int ToGLFWKey(KeyCode key);
    static int ToGLFWMouseButton(MouseButton button);

    // GLFW callbacks (static for C callback compatibility)
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
    GLFWwindow* m_window = nullptr;

    // Key states: current and previous frame
    std::array<bool, static_cast<size_t>(KeyCode::Count)> m_keysCurrent{};
    std::array<bool, static_cast<size_t>(KeyCode::Count)> m_keysPrevious{};

    // Mouse button states
    std::array<bool, static_cast<size_t>(MouseButton::Count)> m_mouseButtonsCurrent{};
    std::array<bool, static_cast<size_t>(MouseButton::Count)> m_mouseButtonsPrevious{};

    // Mouse position
    double m_mouseX = 0.0, m_mouseY = 0.0;
    double m_lastMouseX = 0.0, m_lastMouseY = 0.0;
    double m_mouseDeltaX = 0.0, m_mouseDeltaY = 0.0;
    double m_accumulatedDeltaX = 0.0, m_accumulatedDeltaY = 0.0;  // Accumulated between updates
    bool m_firstMouse = true;
    bool m_mouseDeltaConsumed = true;  // Track if delta was consumed

    // Scroll
    double m_scrollDelta = 0.0;
    double m_scrollAccumulator = 0.0;

    // Cursor mode
    bool m_cursorLocked = false;

    // Static instance for callbacks
    static GLFWInputBackend* s_instance;
};

} // namespace Genesis

