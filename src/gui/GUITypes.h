#pragma once

#include "math/Math.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace Genesis {
namespace GUI {

// ============================================================================
// GUI Colors - Windows 7 / Retro style with dark violet/purple and red accents
// ============================================================================
namespace Colors {
    // Base colors - Dark violet/purple theme (Windows 7 inspired)
    const Vec4 Background       = Vec4(0.12f, 0.10f, 0.18f, 0.95f);   // Dark violet
    const Vec4 BackgroundDark   = Vec4(0.08f, 0.06f, 0.12f, 0.98f);   // Darker violet
    const Vec4 BackgroundLight  = Vec4(0.20f, 0.16f, 0.26f, 0.90f);   // Lighter violet

    // Title bar - Windows 7 Aero style
    const Vec4 TitleBar         = Vec4(0.22f, 0.12f, 0.32f, 1.0f);    // Deep purple title bar
    const Vec4 TitleBarInactive = Vec4(0.15f, 0.12f, 0.20f, 1.0f);    // Inactive title
    const Vec4 TitleBarGradientTop    = Vec4(0.28f, 0.16f, 0.38f, 1.0f);
    const Vec4 TitleBarGradientBottom = Vec4(0.18f, 0.10f, 0.26f, 1.0f);

    // Borders - 3D effect like Windows 7
    const Vec4 Border           = Vec4(0.32f, 0.22f, 0.42f, 1.0f);    // Violet border
    const Vec4 BorderLight      = Vec4(0.48f, 0.38f, 0.58f, 1.0f);    // Light border (highlight)
    const Vec4 BorderDark       = Vec4(0.10f, 0.06f, 0.14f, 1.0f);    // Dark border (shadow)
    const Vec4 BorderFocus      = Vec4(0.75f, 0.25f, 0.30f, 1.0f);    // Red focus border

    // Text
    const Vec4 Text             = Vec4(0.88f, 0.84f, 0.92f, 1.0f);    // Light lavender text
    const Vec4 TextDim          = Vec4(0.55f, 0.50f, 0.60f, 1.0f);    // Dimmed text
    const Vec4 TextHighlight    = Vec4(1.0f, 0.98f, 1.0f, 1.0f);      // Bright text
    const Vec4 TextDisabled     = Vec4(0.40f, 0.38f, 0.42f, 1.0f);    // Disabled text

    // Buttons - Windows 7 style with red accent
    const Vec4 Button           = Vec4(0.26f, 0.20f, 0.34f, 1.0f);    // Button normal
    const Vec4 ButtonHover      = Vec4(0.35f, 0.28f, 0.44f, 1.0f);    // Button hover
    const Vec4 ButtonPressed    = Vec4(0.18f, 0.14f, 0.26f, 1.0f);    // Button pressed
    const Vec4 ButtonDisabled   = Vec4(0.20f, 0.18f, 0.24f, 0.8f);    // Button disabled

    // Red accent colors (for active/important elements)
    const Vec4 Accent           = Vec4(0.75f, 0.20f, 0.25f, 1.0f);    // Deep red accent
    const Vec4 AccentHover      = Vec4(0.85f, 0.28f, 0.32f, 1.0f);    // Red hover
    const Vec4 AccentPressed    = Vec4(0.60f, 0.15f, 0.20f, 1.0f);    // Red pressed
    const Vec4 AccentLight      = Vec4(0.90f, 0.40f, 0.45f, 1.0f);    // Light red

    // Input fields
    const Vec4 InputBackground  = Vec4(0.06f, 0.05f, 0.10f, 1.0f);    // Dark input bg
    const Vec4 InputBorder      = Vec4(0.28f, 0.20f, 0.36f, 1.0f);    // Input border
    const Vec4 InputBorderFocus = Vec4(0.70f, 0.25f, 0.30f, 1.0f);    // Red focus border

    // Scrollbar - Windows 7 style
    const Vec4 ScrollbarBg      = Vec4(0.10f, 0.08f, 0.14f, 1.0f);
    const Vec4 ScrollbarThumb   = Vec4(0.32f, 0.26f, 0.42f, 1.0f);
    const Vec4 ScrollbarThumbHover = Vec4(0.42f, 0.34f, 0.52f, 1.0f);
    const Vec4 ScrollbarThumbPressed = Vec4(0.25f, 0.20f, 0.35f, 1.0f);

    // Checkbox/Radio
    const Vec4 CheckboxBg       = Vec4(0.08f, 0.06f, 0.12f, 1.0f);
    const Vec4 CheckboxMark     = Vec4(0.80f, 0.25f, 0.30f, 1.0f);    // Red checkmark

    // Slider
    const Vec4 SliderTrack      = Vec4(0.15f, 0.12f, 0.20f, 1.0f);
    const Vec4 SliderFill       = Vec4(0.70f, 0.22f, 0.28f, 1.0f);    // Red fill
    const Vec4 SliderThumb      = Vec4(0.85f, 0.30f, 0.35f, 1.0f);    // Red thumb

    // Console specific
    const Vec4 ConsoleOutput    = Vec4(0.78f, 0.74f, 0.88f, 1.0f);    // Console text
    const Vec4 ConsoleError     = Vec4(0.95f, 0.35f, 0.40f, 1.0f);    // Error text (red)
    const Vec4 ConsoleWarning   = Vec4(0.95f, 0.82f, 0.35f, 1.0f);    // Warning text
    const Vec4 ConsoleSuccess   = Vec4(0.40f, 0.90f, 0.50f, 1.0f);    // Success text
    const Vec4 ConsoleCommand   = Vec4(0.55f, 0.78f, 1.0f, 1.0f);     // Command echo

    // Panel/Window specific
    const Vec4 PanelBackground  = Vec4(0.14f, 0.11f, 0.20f, 0.95f);
    const Vec4 PanelHeader      = Vec4(0.20f, 0.14f, 0.28f, 1.0f);

    // Selection
    const Vec4 Selection        = Vec4(0.65f, 0.20f, 0.25f, 0.6f);    // Red selection
    const Vec4 SelectionBorder  = Vec4(0.80f, 0.25f, 0.30f, 1.0f);
}

// ============================================================================
// GUI Rect - Simple rectangle
// ============================================================================
struct Rect {
    float x = 0, y = 0, width = 0, height = 0;

    Rect() = default;
    Rect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}

    bool Contains(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }

    Rect Inset(float amount) const {
        return Rect(x + amount, y + amount, width - amount * 2, height - amount * 2);
    }
};

// ============================================================================
// GUI Vertex - For rendering
// ============================================================================
struct GUIVertex {
    float x, y;         // Position
    float u, v;         // Texture coords
    float r, g, b, a;   // Color

    GUIVertex() = default;
    GUIVertex(float x, float y, float u, float v, const Vec4& color)
        : x(x), y(y), u(u), v(v), r(color.x), g(color.y), b(color.z), a(color.w) {}
};

// ============================================================================
// Font Character Info (for basic bitmap font)
// ============================================================================
struct CharInfo {
    float u0, v0, u1, v1;  // Texture coordinates
    float width, height;
    float xOffset, yOffset;
    float xAdvance;
};

} // namespace GUI
} // namespace Genesis

