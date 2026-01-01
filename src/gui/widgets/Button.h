#pragma once

#include "Widget.h"
#include <functional>

namespace Genesis {
namespace GUI {

// ============================================================================
// Button Widget - Windows 7 style button with 3D borders
// ============================================================================
class Button : public Widget {
public:
    using ClickCallback = std::function<void()>;

    Button() {
        m_rect = Rect(0, 0, 100, 28);
    }

    Button(const std::string& text, float x, float y, float width = 100, float height = 28)
        : m_text(text) {
        m_rect = Rect(x, y, width, height);
    }

    void SetText(const std::string& text) { m_text = text; }
    const std::string& GetText() const { return m_text; }

    void SetOnClick(ClickCallback callback) { m_onClick = callback; }

    void Render(GUIRenderer& renderer) override {
        if (!m_visible) return;

        Vec4 bgColor;
        Vec4 textColor = Colors::Text;

        if (!m_enabled) {
            bgColor = Colors::ButtonDisabled;
            textColor = Colors::TextDisabled;
        } else {
            switch (m_state) {
                case WidgetState::Pressed:
                    bgColor = Colors::ButtonPressed;
                    break;
                case WidgetState::Hovered:
                    bgColor = Colors::ButtonHover;
                    break;
                default:
                    bgColor = Colors::Button;
                    break;
            }
        }

        // Draw button background with gradient (Windows 7 style)
        Vec4 topColor = Vec4(bgColor.x + 0.05f, bgColor.y + 0.05f, bgColor.z + 0.05f, bgColor.w);
        Vec4 bottomColor = Vec4(bgColor.x - 0.03f, bgColor.y - 0.03f, bgColor.z - 0.03f, bgColor.w);
        renderer.DrawRectGradientV(m_rect, topColor, bottomColor);

        // 3D border (raised when not pressed, sunken when pressed)
        renderer.DrawBorder3D(m_rect, m_state != WidgetState::Pressed);

        // Red accent on hover
        if (m_state == WidgetState::Hovered || m_state == WidgetState::Pressed) {
            renderer.DrawRect(Rect(m_rect.x, m_rect.y, m_rect.width, 2), Colors::Accent);
        }

        // Text (offset down-right when pressed for 3D effect)
        float textOffsetX = (m_state == WidgetState::Pressed) ? 1.0f : 0.0f;
        float textOffsetY = (m_state == WidgetState::Pressed) ? 1.0f : 0.0f;

        Vec2 textSize = renderer.MeasureText(m_text, 1.0f);
        float textX = m_rect.x + (m_rect.width - textSize.x) / 2.0f + textOffsetX;
        float textY = m_rect.y + (m_rect.height - textSize.y) / 2.0f + textOffsetY;
        renderer.DrawText(m_text, textX, textY, textColor, 1.0f);
    }

    bool OnMouseUp(float x, float y, int button) override {
        bool clicked = Widget::OnMouseUp(x, y, button);
        if (clicked && m_onClick) {
            m_onClick();
        }
        return clicked;
    }

private:
    std::string m_text;
    ClickCallback m_onClick;
};

} // namespace GUI
} // namespace Genesis

