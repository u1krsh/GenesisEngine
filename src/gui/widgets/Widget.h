#pragma once

#include "../GUITypes.h"
#include "../GUIRenderer.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Genesis {
namespace GUI {

// ============================================================================
// Widget State
// ============================================================================
enum class WidgetState {
    Normal,
    Hovered,
    Pressed,
    Focused,
    Disabled
};

// ============================================================================
// Widget Base Class - Windows 7 style widgets
// ============================================================================
class Widget {
public:
    Widget() = default;
    virtual ~Widget() = default;

    // ========================================================================
    // Core widget interface
    // ========================================================================
    virtual void Update(float deltaTime) {}
    virtual void Render(GUIRenderer& renderer) = 0;

    // ========================================================================
    // Position and Size
    // ========================================================================
    void SetPosition(float x, float y) { m_rect.x = x; m_rect.y = y; }
    void SetSize(float width, float height) { m_rect.width = width; m_rect.height = height; }
    void SetRect(const Rect& rect) { m_rect = rect; }
    const Rect& GetRect() const { return m_rect; }

    float GetX() const { return m_rect.x; }
    float GetY() const { return m_rect.y; }
    float GetWidth() const { return m_rect.width; }
    float GetHeight() const { return m_rect.height; }

    // ========================================================================
    // State
    // ========================================================================
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void SetFocused(bool focused) { m_focused = focused; }
    bool IsFocused() const { return m_focused; }

    WidgetState GetState() const { return m_state; }

    // ========================================================================
    // Input handling
    // ========================================================================
    virtual bool OnMouseMove(float x, float y);
    virtual bool OnMouseDown(float x, float y, int button);
    virtual bool OnMouseUp(float x, float y, int button);
    virtual bool OnKeyDown(int key);
    virtual bool OnKeyUp(int key);
    virtual bool OnCharInput(unsigned int codepoint);

    // ========================================================================
    // Identification
    // ========================================================================
    void SetId(const std::string& id) { m_id = id; }
    const std::string& GetId() const { return m_id; }

    void SetTooltip(const std::string& tooltip) { m_tooltip = tooltip; }
    const std::string& GetTooltip() const { return m_tooltip; }

protected:
    bool ContainsPoint(float x, float y) const {
        return m_rect.Contains(x, y);
    }

protected:
    Rect m_rect;
    std::string m_id;
    std::string m_tooltip;

    bool m_visible = true;
    bool m_enabled = true;
    bool m_focused = false;
    bool m_hovered = false;

    WidgetState m_state = WidgetState::Normal;
};

// ============================================================================
// Widget Implementation
// ============================================================================
inline bool Widget::OnMouseMove(float x, float y) {
    if (!m_visible || !m_enabled) return false;

    bool wasHovered = m_hovered;
    m_hovered = ContainsPoint(x, y);

    if (m_hovered && m_state != WidgetState::Pressed) {
        m_state = WidgetState::Hovered;
    } else if (!m_hovered && m_state == WidgetState::Hovered) {
        m_state = WidgetState::Normal;
    }

    return m_hovered;
}

inline bool Widget::OnMouseDown(float x, float y, int button) {
    if (!m_visible || !m_enabled) return false;

    if (ContainsPoint(x, y)) {
        m_state = WidgetState::Pressed;
        m_focused = true;
        return true;
    }
    return false;
}

inline bool Widget::OnMouseUp(float x, float y, int button) {
    if (!m_visible || !m_enabled) return false;

    bool wasPressed = (m_state == WidgetState::Pressed);
    if (m_hovered) {
        m_state = WidgetState::Hovered;
    } else {
        m_state = WidgetState::Normal;
    }

    return wasPressed && ContainsPoint(x, y);
}

inline bool Widget::OnKeyDown(int key) {
    return false;
}

inline bool Widget::OnKeyUp(int key) {
    return false;
}

inline bool Widget::OnCharInput(unsigned int codepoint) {
    return false;
}

} // namespace GUI
} // namespace Genesis
ll