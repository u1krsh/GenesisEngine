#pragma once

#include <cstdint>

namespace Genesis {

// ============================================================================
// Platform-agnostic key codes
// ============================================================================
enum class KeyCode : uint16_t {
    // Letters
    A = 0, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

    // Numbers
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

    // Arrow keys
    Up, Down, Left, Right,

    // Special keys
    Space, Enter, Escape, Tab, Backspace, Delete, Insert,
    Home, End, PageUp, PageDown,

    // Modifiers
    LeftShift, RightShift, LeftControl, RightControl,
    LeftAlt, RightAlt, LeftSuper, RightSuper,

    // Numpad
    Numpad0, Numpad1, Numpad2, Numpad3, Numpad4,
    Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,
    NumpadAdd, NumpadSubtract, NumpadMultiply, NumpadDivide,
    NumpadEnter, NumpadDecimal,

    // Misc
    CapsLock, NumLock, ScrollLock, PrintScreen, Pause,
    GraveAccent, Minus, Equal, LeftBracket, RightBracket,
    Backslash, Semicolon, Apostrophe, Comma, Period, Slash,

    Count,
    Unknown = 0xFFFF
};

// ============================================================================
// Mouse buttons
// ============================================================================
enum class MouseButton : uint8_t {
    Left = 0,
    Right,
    Middle,
    Button4,
    Button5,
    Button6,
    Button7,
    Button8,

    Count,
    Unknown = 0xFF
};

// ============================================================================
// Key/Button states
// ============================================================================
enum class InputState : uint8_t {
    Released = 0,   // Not pressed
    Pressed,        // Just pressed this frame
    Held,           // Held down
    JustReleased    // Just released this frame
};

// ============================================================================
// Game actions (bindable)
// ============================================================================
enum class GameAction : uint16_t {
    // Movement
    MoveForward = 0,
    MoveBackward,
    MoveLeft,
    MoveRight,
    Jump,
    Crouch,
    Sprint,

    // Camera
    LookUp,
    LookDown,
    LookLeft,
    LookRight,

    // Interactions
    PrimaryAction,
    SecondaryAction,
    Interact,
    Reload,

    // UI
    Pause,
    Inventory,
    Menu,

    // Debug
    DebugConsole,
    DebugOverlay,

    Count,
    None = 0xFFFF
};

} // namespace Genesis

