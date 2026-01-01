#pragma once

#include "Widget.h"

namespace Genesis {
namespace GUI {

// ============================================================================
// Label Widget - Simple text display
// ============================================================================
class Label : public Widget {
public:
    Label() {
        m_rect = Rect(0, 0, 100, 16);
    }

    Label(const std::string& text, float x, float y, float scale = 1.0f)
        : m_text(text), m_scale(scale) {
        m_rect = Rect(x, y, 100, 16);
    }

    void SetText(const std::string& text) { m_text = text; }
    const std::string& GetText() const { return m_text; }

    void SetColor(const Vec4& color) { m_color = color; }
    const Vec4& GetColor() const { return m_color; }

    void SetScale(float scale) { m_scale = scale; }
    float GetScale() const { return m_scale; }

    void SetAlignment(int align) { m_alignment = align; } // 0=left, 1=center, 2=right

    void Render(GUIRenderer& renderer) override {
        if (!m_visible) return;

        Vec4 color = m_enabled ? m_color : Colors::TextDisabled;

        if (m_alignment == 1) {  // Center
            renderer.DrawTextCentered(m_text, m_rect, color, m_scale);
        } else if (m_alignment == 2) {  // Right
            Vec2 textSize = renderer.MeasureText(m_text, m_scale);
            float x = m_rect.x + m_rect.width - textSize.x;
            renderer.DrawText(m_text, x, m_rect.y, color, m_scale);
        } else {  // Left (default)
            renderer.DrawText(m_text, m_rect.x, m_rect.y, color, m_scale);
        }
    }

private:
    std::string m_text;
    Vec4 m_color = Colors::Text;
    float m_scale = 1.0f;
    int m_alignment = 0;  // 0=left, 1=center, 2=right
};

} // namespace GUI
} // namespace Genesis

