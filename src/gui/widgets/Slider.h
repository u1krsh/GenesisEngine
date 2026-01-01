#pragma once

#include "Widget.h"
#include <functional>
#include <algorithm>

namespace Genesis {
namespace GUI {

// ============================================================================
// Slider Widget - Windows 7 style slider with red fill
// ============================================================================
class Slider : public Widget {
public:
    using ChangeCallback = std::function<void(float)>;

    Slider() {
        m_rect = Rect(0, 0, 200, 24);
    }

    Slider(float x, float y, float width, float minVal = 0.0f, float maxVal = 1.0f, float value = 0.5f)
        : m_minValue(minVal), m_maxValue(maxVal), m_value(value) {
        m_rect = Rect(x, y, width, 24);
    }

    void SetRange(float minVal, float maxVal) {
        m_minValue = minVal;
        m_maxValue = maxVal;
        m_value = std::clamp(m_value, m_minValue, m_maxValue);
    }

    void SetValue(float value) {
        m_value = std::clamp(value, m_minValue, m_maxValue);
    }
    float GetValue() const { return m_value; }

    void SetLabel(const std::string& label) { m_label = label; }
    const std::string& GetLabel() const { return m_label; }

    void SetShowValue(bool show) { m_showValue = show; }

    void SetOnChange(ChangeCallback callback) { m_onChange = callback; }

    void Render(GUIRenderer& renderer) override {
        if (!m_visible) return;

        float labelWidth = m_label.empty() ? 0 : 80;
        float valueWidth = m_showValue ? 50 : 0;
        float sliderX = m_rect.x + labelWidth;
        float sliderWidth = m_rect.width - labelWidth - valueWidth;
        float sliderY = m_rect.y + 8;
        float sliderHeight = 8;

        // Label
        if (!m_label.empty()) {
            Vec4 textColor = m_enabled ? Colors::Text : Colors::TextDisabled;
            renderer.DrawText(m_label, m_rect.x, m_rect.y + 5, textColor, 1.0f);
        }

        // Track background
        Rect trackRect(sliderX, sliderY, sliderWidth, sliderHeight);
        renderer.DrawRect(trackRect, Colors::SliderTrack);
        renderer.DrawBorder3D(trackRect, false);

        // Fill (red)
        float normalizedValue = (m_value - m_minValue) / (m_maxValue - m_minValue);
        float fillWidth = sliderWidth * normalizedValue;
        if (fillWidth > 2) {
            Rect fillRect(sliderX + 1, sliderY + 1, fillWidth - 2, sliderHeight - 2);
            renderer.DrawRect(fillRect, Colors::SliderFill);
        }

        // Thumb
        float thumbWidth = 12;
        float thumbHeight = 20;
        float thumbX = sliderX + normalizedValue * (sliderWidth - thumbWidth);
        float thumbY = m_rect.y + 2;
        Rect thumbRect(thumbX, thumbY, thumbWidth, thumbHeight);

        Vec4 thumbColor = Colors::SliderThumb;
        if (m_state == WidgetState::Pressed || m_dragging) {
            thumbColor = Colors::AccentPressed;
        } else if (m_state == WidgetState::Hovered) {
            thumbColor = Colors::AccentHover;
        }

        renderer.DrawRectGradientV(thumbRect,
            Vec4(thumbColor.x + 0.1f, thumbColor.y + 0.1f, thumbColor.z + 0.1f, thumbColor.w),
            thumbColor);
        renderer.DrawBorder3D(thumbRect, !m_dragging);

        // Value display
        if (m_showValue) {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.2f", m_value);
            Vec4 textColor = m_enabled ? Colors::Text : Colors::TextDisabled;
            renderer.DrawText(buf, sliderX + sliderWidth + 8, m_rect.y + 5, textColor, 1.0f);
        }
    }

    bool OnMouseDown(float x, float y, int button) override {
        if (!m_visible || !m_enabled) return false;

        if (ContainsPoint(x, y)) {
            m_dragging = true;
            UpdateValueFromMouse(x);
            return true;
        }
        return false;
    }

    bool OnMouseMove(float x, float y) override {
        Widget::OnMouseMove(x, y);

        if (m_dragging) {
            UpdateValueFromMouse(x);
            return true;
        }
        return m_hovered;
    }

    bool OnMouseUp(float x, float y, int button) override {
        m_dragging = false;
        return Widget::OnMouseUp(x, y, button);
    }

private:
    void UpdateValueFromMouse(float mouseX) {
        float labelWidth = m_label.empty() ? 0 : 80;
        float valueWidth = m_showValue ? 50 : 0;
        float sliderX = m_rect.x + labelWidth;
        float sliderWidth = m_rect.width - labelWidth - valueWidth;

        float normalized = (mouseX - sliderX) / sliderWidth;
        normalized = std::clamp(normalized, 0.0f, 1.0f);
        float newValue = m_minValue + normalized * (m_maxValue - m_minValue);

        if (newValue != m_value) {
            m_value = newValue;
            if (m_onChange) {
                m_onChange(m_value);
            }
        }
    }

private:
    std::string m_label;
    float m_minValue = 0.0f;
    float m_maxValue = 1.0f;
    float m_value = 0.5f;
    bool m_showValue = true;
    bool m_dragging = false;
    ChangeCallback m_onChange;
};

} // namespace GUI
} // namespace Genesis

