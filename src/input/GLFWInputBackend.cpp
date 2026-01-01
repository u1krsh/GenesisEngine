#include "GLFWInputBackend.h"
#include <cstring>

namespace Genesis {

// Static instance for callbacks
GLFWInputBackend* GLFWInputBackend::s_instance = nullptr;

GLFWInputBackend::~GLFWInputBackend() {
    Shutdown();
}

bool GLFWInputBackend::Initialize(void* windowHandle) {
    m_window = static_cast<GLFWwindow*>(windowHandle);
    if (!m_window) {
        return false;
    }

    s_instance = this;

    // Set up callbacks
    glfwSetKeyCallback(m_window, KeyCallback);
    glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
    glfwSetCursorPosCallback(m_window, CursorPosCallback);
    glfwSetScrollCallback(m_window, ScrollCallback);

    // Initialize mouse position
    glfwGetCursorPos(m_window, &m_mouseX, &m_mouseY);
    m_lastMouseX = m_mouseX;
    m_lastMouseY = m_mouseY;
    m_firstMouse = true;

    return true;
}

void GLFWInputBackend::Shutdown() {
    if (m_window) {
        glfwSetKeyCallback(m_window, nullptr);
        glfwSetMouseButtonCallback(m_window, nullptr);
        glfwSetCursorPosCallback(m_window, nullptr);
        glfwSetScrollCallback(m_window, nullptr);
    }
    m_window = nullptr;
    s_instance = nullptr;
}

void GLFWInputBackend::Update() {
    // Copy current state to previous
    m_keysPrevious = m_keysCurrent;
    m_mouseButtonsPrevious = m_mouseButtonsCurrent;

    // Transfer accumulated mouse delta and reset accumulator
    m_mouseDeltaX = m_accumulatedDeltaX;
    m_mouseDeltaY = m_accumulatedDeltaY;
    m_accumulatedDeltaX = 0.0;
    m_accumulatedDeltaY = 0.0;

    // Handle first mouse frame (avoid jump when cursor is first captured)
    if (m_firstMouse) {
        m_mouseDeltaX = 0.0;
        m_mouseDeltaY = 0.0;
        m_firstMouse = false;
    }

    // Transfer scroll accumulator and reset
    m_scrollDelta = m_scrollAccumulator;
    m_scrollAccumulator = 0.0;
}

// ============================================================================
// Keyboard
// ============================================================================
bool GLFWInputBackend::IsKeyDown(KeyCode key) const {
    size_t index = static_cast<size_t>(key);
    if (index >= m_keysCurrent.size()) return false;
    return m_keysCurrent[index];
}

bool GLFWInputBackend::IsKeyPressed(KeyCode key) const {
    size_t index = static_cast<size_t>(key);
    if (index >= m_keysCurrent.size()) return false;
    return m_keysCurrent[index] && !m_keysPrevious[index];
}

bool GLFWInputBackend::IsKeyReleased(KeyCode key) const {
    size_t index = static_cast<size_t>(key);
    if (index >= m_keysCurrent.size()) return false;
    return !m_keysCurrent[index] && m_keysPrevious[index];
}

// ============================================================================
// Mouse Buttons
// ============================================================================
bool GLFWInputBackend::IsMouseButtonDown(MouseButton button) const {
    size_t index = static_cast<size_t>(button);
    if (index >= m_mouseButtonsCurrent.size()) return false;
    return m_mouseButtonsCurrent[index];
}

bool GLFWInputBackend::IsMouseButtonPressed(MouseButton button) const {
    size_t index = static_cast<size_t>(button);
    if (index >= m_mouseButtonsCurrent.size()) return false;
    return m_mouseButtonsCurrent[index] && !m_mouseButtonsPrevious[index];
}

bool GLFWInputBackend::IsMouseButtonReleased(MouseButton button) const {
    size_t index = static_cast<size_t>(button);
    if (index >= m_mouseButtonsCurrent.size()) return false;
    return !m_mouseButtonsCurrent[index] && m_mouseButtonsPrevious[index];
}

// ============================================================================
// Mouse Position & Scroll
// ============================================================================
void GLFWInputBackend::GetMousePosition(double& x, double& y) const {
    x = m_mouseX;
    y = m_mouseY;
}

void GLFWInputBackend::GetMouseDelta(double& dx, double& dy) const {
    dx = m_mouseDeltaX;
    dy = m_mouseDeltaY;
}

double GLFWInputBackend::GetScrollDelta() const {
    return m_scrollDelta;
}

void GLFWInputBackend::SetCursorMode(bool locked) {
    m_cursorLocked = locked;
    glfwSetInputMode(m_window, GLFW_CURSOR,
                     locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    if (locked) {
        m_firstMouse = true; // Reset to avoid jump when locking
    }
}

bool GLFWInputBackend::IsCursorLocked() const {
    return m_cursorLocked;
}

// ============================================================================
// GLFW Key Conversion
// ============================================================================
int GLFWInputBackend::ToGLFWKey(KeyCode key) {
    switch (key) {
        // Letters
        case KeyCode::A: return GLFW_KEY_A;
        case KeyCode::B: return GLFW_KEY_B;
        case KeyCode::C: return GLFW_KEY_C;
        case KeyCode::D: return GLFW_KEY_D;
        case KeyCode::E: return GLFW_KEY_E;
        case KeyCode::F: return GLFW_KEY_F;
        case KeyCode::G: return GLFW_KEY_G;
        case KeyCode::H: return GLFW_KEY_H;
        case KeyCode::I: return GLFW_KEY_I;
        case KeyCode::J: return GLFW_KEY_J;
        case KeyCode::K: return GLFW_KEY_K;
        case KeyCode::L: return GLFW_KEY_L;
        case KeyCode::M: return GLFW_KEY_M;
        case KeyCode::N: return GLFW_KEY_N;
        case KeyCode::O: return GLFW_KEY_O;
        case KeyCode::P: return GLFW_KEY_P;
        case KeyCode::Q: return GLFW_KEY_Q;
        case KeyCode::R: return GLFW_KEY_R;
        case KeyCode::S: return GLFW_KEY_S;
        case KeyCode::T: return GLFW_KEY_T;
        case KeyCode::U: return GLFW_KEY_U;
        case KeyCode::V: return GLFW_KEY_V;
        case KeyCode::W: return GLFW_KEY_W;
        case KeyCode::X: return GLFW_KEY_X;
        case KeyCode::Y: return GLFW_KEY_Y;
        case KeyCode::Z: return GLFW_KEY_Z;

        // Numbers
        case KeyCode::Num0: return GLFW_KEY_0;
        case KeyCode::Num1: return GLFW_KEY_1;
        case KeyCode::Num2: return GLFW_KEY_2;
        case KeyCode::Num3: return GLFW_KEY_3;
        case KeyCode::Num4: return GLFW_KEY_4;
        case KeyCode::Num5: return GLFW_KEY_5;
        case KeyCode::Num6: return GLFW_KEY_6;
        case KeyCode::Num7: return GLFW_KEY_7;
        case KeyCode::Num8: return GLFW_KEY_8;
        case KeyCode::Num9: return GLFW_KEY_9;

        // Function keys
        case KeyCode::F1: return GLFW_KEY_F1;
        case KeyCode::F2: return GLFW_KEY_F2;
        case KeyCode::F3: return GLFW_KEY_F3;
        case KeyCode::F4: return GLFW_KEY_F4;
        case KeyCode::F5: return GLFW_KEY_F5;
        case KeyCode::F6: return GLFW_KEY_F6;
        case KeyCode::F7: return GLFW_KEY_F7;
        case KeyCode::F8: return GLFW_KEY_F8;
        case KeyCode::F9: return GLFW_KEY_F9;
        case KeyCode::F10: return GLFW_KEY_F10;
        case KeyCode::F11: return GLFW_KEY_F11;
        case KeyCode::F12: return GLFW_KEY_F12;

        // Arrow keys
        case KeyCode::Up: return GLFW_KEY_UP;
        case KeyCode::Down: return GLFW_KEY_DOWN;
        case KeyCode::Left: return GLFW_KEY_LEFT;
        case KeyCode::Right: return GLFW_KEY_RIGHT;

        // Special keys
        case KeyCode::Space: return GLFW_KEY_SPACE;
        case KeyCode::Enter: return GLFW_KEY_ENTER;
        case KeyCode::Escape: return GLFW_KEY_ESCAPE;
        case KeyCode::Tab: return GLFW_KEY_TAB;
        case KeyCode::Backspace: return GLFW_KEY_BACKSPACE;
        case KeyCode::Delete: return GLFW_KEY_DELETE;
        case KeyCode::Insert: return GLFW_KEY_INSERT;
        case KeyCode::Home: return GLFW_KEY_HOME;
        case KeyCode::End: return GLFW_KEY_END;
        case KeyCode::PageUp: return GLFW_KEY_PAGE_UP;
        case KeyCode::PageDown: return GLFW_KEY_PAGE_DOWN;

        // Modifiers
        case KeyCode::LeftShift: return GLFW_KEY_LEFT_SHIFT;
        case KeyCode::RightShift: return GLFW_KEY_RIGHT_SHIFT;
        case KeyCode::LeftControl: return GLFW_KEY_LEFT_CONTROL;
        case KeyCode::RightControl: return GLFW_KEY_RIGHT_CONTROL;
        case KeyCode::LeftAlt: return GLFW_KEY_LEFT_ALT;
        case KeyCode::RightAlt: return GLFW_KEY_RIGHT_ALT;
        case KeyCode::LeftSuper: return GLFW_KEY_LEFT_SUPER;
        case KeyCode::RightSuper: return GLFW_KEY_RIGHT_SUPER;

        // Misc
        case KeyCode::CapsLock: return GLFW_KEY_CAPS_LOCK;
        case KeyCode::NumLock: return GLFW_KEY_NUM_LOCK;
        case KeyCode::ScrollLock: return GLFW_KEY_SCROLL_LOCK;
        case KeyCode::GraveAccent: return GLFW_KEY_GRAVE_ACCENT;
        case KeyCode::Minus: return GLFW_KEY_MINUS;
        case KeyCode::Equal: return GLFW_KEY_EQUAL;
        case KeyCode::LeftBracket: return GLFW_KEY_LEFT_BRACKET;
        case KeyCode::RightBracket: return GLFW_KEY_RIGHT_BRACKET;
        case KeyCode::Backslash: return GLFW_KEY_BACKSLASH;
        case KeyCode::Semicolon: return GLFW_KEY_SEMICOLON;
        case KeyCode::Apostrophe: return GLFW_KEY_APOSTROPHE;
        case KeyCode::Comma: return GLFW_KEY_COMMA;
        case KeyCode::Period: return GLFW_KEY_PERIOD;
        case KeyCode::Slash: return GLFW_KEY_SLASH;

        default: return GLFW_KEY_UNKNOWN;
    }
}

int GLFWInputBackend::ToGLFWMouseButton(MouseButton button) {
    switch (button) {
        case MouseButton::Left: return GLFW_MOUSE_BUTTON_LEFT;
        case MouseButton::Right: return GLFW_MOUSE_BUTTON_RIGHT;
        case MouseButton::Middle: return GLFW_MOUSE_BUTTON_MIDDLE;
        case MouseButton::Button4: return GLFW_MOUSE_BUTTON_4;
        case MouseButton::Button5: return GLFW_MOUSE_BUTTON_5;
        case MouseButton::Button6: return GLFW_MOUSE_BUTTON_6;
        case MouseButton::Button7: return GLFW_MOUSE_BUTTON_7;
        case MouseButton::Button8: return GLFW_MOUSE_BUTTON_8;
        default: return -1;
    }
}

// ============================================================================
// Reverse GLFW to KeyCode conversion
// ============================================================================
static KeyCode FromGLFWKey(int glfwKey) {
    switch (glfwKey) {
        case GLFW_KEY_A: return KeyCode::A;
        case GLFW_KEY_B: return KeyCode::B;
        case GLFW_KEY_C: return KeyCode::C;
        case GLFW_KEY_D: return KeyCode::D;
        case GLFW_KEY_E: return KeyCode::E;
        case GLFW_KEY_F: return KeyCode::F;
        case GLFW_KEY_G: return KeyCode::G;
        case GLFW_KEY_H: return KeyCode::H;
        case GLFW_KEY_I: return KeyCode::I;
        case GLFW_KEY_J: return KeyCode::J;
        case GLFW_KEY_K: return KeyCode::K;
        case GLFW_KEY_L: return KeyCode::L;
        case GLFW_KEY_M: return KeyCode::M;
        case GLFW_KEY_N: return KeyCode::N;
        case GLFW_KEY_O: return KeyCode::O;
        case GLFW_KEY_P: return KeyCode::P;
        case GLFW_KEY_Q: return KeyCode::Q;
        case GLFW_KEY_R: return KeyCode::R;
        case GLFW_KEY_S: return KeyCode::S;
        case GLFW_KEY_T: return KeyCode::T;
        case GLFW_KEY_U: return KeyCode::U;
        case GLFW_KEY_V: return KeyCode::V;
        case GLFW_KEY_W: return KeyCode::W;
        case GLFW_KEY_X: return KeyCode::X;
        case GLFW_KEY_Y: return KeyCode::Y;
        case GLFW_KEY_Z: return KeyCode::Z;
        case GLFW_KEY_0: return KeyCode::Num0;
        case GLFW_KEY_1: return KeyCode::Num1;
        case GLFW_KEY_2: return KeyCode::Num2;
        case GLFW_KEY_3: return KeyCode::Num3;
        case GLFW_KEY_4: return KeyCode::Num4;
        case GLFW_KEY_5: return KeyCode::Num5;
        case GLFW_KEY_6: return KeyCode::Num6;
        case GLFW_KEY_7: return KeyCode::Num7;
        case GLFW_KEY_8: return KeyCode::Num8;
        case GLFW_KEY_9: return KeyCode::Num9;
        case GLFW_KEY_SPACE: return KeyCode::Space;
        case GLFW_KEY_ENTER: return KeyCode::Enter;
        case GLFW_KEY_ESCAPE: return KeyCode::Escape;
        case GLFW_KEY_TAB: return KeyCode::Tab;
        case GLFW_KEY_BACKSPACE: return KeyCode::Backspace;
        case GLFW_KEY_UP: return KeyCode::Up;
        case GLFW_KEY_DOWN: return KeyCode::Down;
        case GLFW_KEY_LEFT: return KeyCode::Left;
        case GLFW_KEY_RIGHT: return KeyCode::Right;
        case GLFW_KEY_LEFT_SHIFT: return KeyCode::LeftShift;
        case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RightShift;
        case GLFW_KEY_LEFT_CONTROL: return KeyCode::LeftControl;
        case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RightControl;
        case GLFW_KEY_LEFT_ALT: return KeyCode::LeftAlt;
        case GLFW_KEY_RIGHT_ALT: return KeyCode::RightAlt;
        case GLFW_KEY_F1: return KeyCode::F1;
        case GLFW_KEY_F2: return KeyCode::F2;
        case GLFW_KEY_F3: return KeyCode::F3;
        case GLFW_KEY_F4: return KeyCode::F4;
        case GLFW_KEY_F5: return KeyCode::F5;
        case GLFW_KEY_F6: return KeyCode::F6;
        case GLFW_KEY_F7: return KeyCode::F7;
        case GLFW_KEY_F8: return KeyCode::F8;
        case GLFW_KEY_F9: return KeyCode::F9;
        case GLFW_KEY_F10: return KeyCode::F10;
        case GLFW_KEY_F11: return KeyCode::F11;
        case GLFW_KEY_F12: return KeyCode::F12;
        default: return KeyCode::Unknown;
    }
}

static MouseButton FromGLFWMouseButton(int glfwButton) {
    switch (glfwButton) {
        case GLFW_MOUSE_BUTTON_LEFT: return MouseButton::Left;
        case GLFW_MOUSE_BUTTON_RIGHT: return MouseButton::Right;
        case GLFW_MOUSE_BUTTON_MIDDLE: return MouseButton::Middle;
        case GLFW_MOUSE_BUTTON_4: return MouseButton::Button4;
        case GLFW_MOUSE_BUTTON_5: return MouseButton::Button5;
        case GLFW_MOUSE_BUTTON_6: return MouseButton::Button6;
        case GLFW_MOUSE_BUTTON_7: return MouseButton::Button7;
        case GLFW_MOUSE_BUTTON_8: return MouseButton::Button8;
        default: return MouseButton::Unknown;
    }
}

// ============================================================================
// Forward Key Events (used by Engine when it intercepts callbacks)
// ============================================================================
void GLFWInputBackend::ForwardKeyEvent(int glfwKey, int action) {
    KeyCode keyCode = FromGLFWKey(glfwKey);
    if (keyCode == KeyCode::Unknown) return;

    size_t index = static_cast<size_t>(keyCode);
    if (index < m_keysCurrent.size()) {
        m_keysCurrent[index] = (action != GLFW_RELEASE);
    }
}

void GLFWInputBackend::ForwardMouseButtonEvent(int glfwButton, int action) {
    MouseButton mb = FromGLFWMouseButton(glfwButton);
    if (mb == MouseButton::Unknown) return;

    size_t index = static_cast<size_t>(mb);
    if (index < m_mouseButtonsCurrent.size()) {
        m_mouseButtonsCurrent[index] = (action != GLFW_RELEASE);
    }
}

void GLFWInputBackend::ForwardCursorPosEvent(double xpos, double ypos) {
    // Accumulate delta from cursor movement
    if (!m_firstMouse) {
        m_accumulatedDeltaX += xpos - m_mouseX;
        m_accumulatedDeltaY += ypos - m_mouseY;
    }

    m_mouseX = xpos;
    m_mouseY = ypos;
}

// ============================================================================
// GLFW Callbacks
// ============================================================================
void GLFWInputBackend::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (!s_instance) return;

    KeyCode keyCode = FromGLFWKey(key);
    if (keyCode == KeyCode::Unknown) return;

    size_t index = static_cast<size_t>(keyCode);
    if (index < s_instance->m_keysCurrent.size()) {
        s_instance->m_keysCurrent[index] = (action != GLFW_RELEASE);
    }
}

void GLFWInputBackend::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (!s_instance) return;

    MouseButton mb = FromGLFWMouseButton(button);
    if (mb == MouseButton::Unknown) return;

    size_t index = static_cast<size_t>(mb);
    if (index < s_instance->m_mouseButtonsCurrent.size()) {
        s_instance->m_mouseButtonsCurrent[index] = (action != GLFW_RELEASE);
    }
}

void GLFWInputBackend::CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!s_instance) return;

    // Accumulate delta from cursor movement
    if (!s_instance->m_firstMouse) {
        s_instance->m_accumulatedDeltaX += xpos - s_instance->m_mouseX;
        s_instance->m_accumulatedDeltaY += ypos - s_instance->m_mouseY;
    }

    s_instance->m_mouseX = xpos;
    s_instance->m_mouseY = ypos;
}

void GLFWInputBackend::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (!s_instance) return;
    s_instance->m_scrollAccumulator += yoffset;
}

} // namespace Genesis

