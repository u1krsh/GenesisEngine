#pragma once

#include "Widget.h"
#include <functional>

namespace Genesis {
namespace GUI {

// ============================================================================
// Checkbox Widget - Windows 7 style toggle with red checkmark
// ============================================================================
class Checkbox : public Widget {
public:
    using ChangeCallback = std::function<void(bool)>;

    Checkbox() {
        m_rect = Rect(0, 0, 150, 20);
    }

    Checkbox(const std::string& label, float x, float y, bool checked = false)
        : m_label(label), m_checked(checked) {
        m_rect = Rect(x, y, 150, 20);
    }

    void SetLabel(const std::string& label) { m_label = label; }
    const std::string& GetLabel() const { return m_label; }

    void SetChecked(bool checked) { m_checked = checked; }
    bool IsChecked() const { return m_checked; }

    void SetOnChange(ChangeCallback callback) { m_onChange = callback; }

    void Render(GUIRenderer& renderer) override {
        if (!m_visible) return;

        float boxSize = 16;
        Rect boxRect(m_rect.x, m_rect.y + 2, boxSize, boxSize);

        // Checkbox background
        Vec4 bgColor = m_enabled ? Colors::CheckboxBg : Colors::ButtonDisabled;
        renderer.DrawRect(boxRect, bgColor);

        // 3D border (sunken)
        renderer.DrawBorder3D(boxRect, false);

        // Red focus border when hovered
        if (m_state == WidgetState::Hovered || m_state == WidgetState::Pressed) {
            renderer.DrawRectOutline(boxRect.Inset(-1), Colors::Accent, 1);
        }

        // Checkmark (red)
        if (m_checked) {
            Rect checkRect = boxRect.Inset(3);
            renderer.DrawRect(checkRect, Colors::CheckboxMark);
        }

        // Label text
        Vec4 textColor = m_enabled ? Colors::Text : Colors::TextDisabled;
        renderer.DrawText(m_label, m_rect.x + boxSize + 6, m_rect.y + 3, textColor, 1.0f);
    }

    bool OnMouseUp(float x, float y, int button) override {
        if (!m_visible || !m_enabled) return false;

        if (Widget::OnMouseUp(x, y, button) && ContainsPoint(x, y)) {
            m_checked = !m_checked;
            if (m_onChange) {
                m_onChange(m_checked);
            }
            return true;
        }
        return false;
    }

private:
    std::string m_label;
    bool m_checked = false;
    ChangeCallback m_onChange;
};

} // namespace GUI
} // namespace Genesis

