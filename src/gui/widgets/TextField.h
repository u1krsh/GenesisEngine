#pragma once

#include "Widget.h"
#include <functional>
#include <GLFW/glfw3.h>

namespace Genesis {
namespace GUI {

// ============================================================================
// TextField Widget - Text input field with Windows 7 style
// ============================================================================
class TextField : public Widget {
public:
    using ChangeCallback = std::function<void(const std::string&)>;
    using SubmitCallback = std::function<void(const std::string&)>;

    TextField() {
        m_rect = Rect(0, 0, 200, 24);
    }

    TextField(float x, float y, float width, const std::string& text = "")
        : m_text(text) {
        m_rect = Rect(x, y, width, 24);
        m_cursorPos = static_cast<int>(text.length());
    }

    void SetText(const std::string& text) {
        m_text = text;
        m_cursorPos = static_cast<int>(text.length());
    }
    const std::string& GetText() const { return m_text; }

    void SetPlaceholder(const std::string& placeholder) { m_placeholder = placeholder; }

    void SetMaxLength(size_t maxLen) { m_maxLength = maxLen; }

    void SetOnChange(ChangeCallback callback) { m_onChange = callback; }
    void SetOnSubmit(SubmitCallback callback) { m_onSubmit = callback; }

    void Update(float deltaTime) override {
        // Cursor blink
        m_cursorBlinkTimer += deltaTime;
        if (m_cursorBlinkTimer >= 0.5f) {
            m_cursorBlinkTimer = 0.0f;
            m_cursorVisible = !m_cursorVisible;
        }
    }

    void Render(GUIRenderer& renderer) override {
        if (!m_visible) return;

        // Background
        Vec4 bgColor = m_enabled ? Colors::InputBackground : Colors::ButtonDisabled;
        renderer.DrawRect(m_rect, bgColor);

        // 3D border (sunken)
        renderer.DrawBorder3D(m_rect, false);

        // Focus border (red)
        if (m_focused) {
            renderer.DrawRectOutline(m_rect.Inset(-1), Colors::InputBorderFocus, 1);
        }

        // Text or placeholder
        float textX = m_rect.x + 4;
        float textY = m_rect.y + 6;

        if (m_text.empty() && !m_placeholder.empty() && !m_focused) {
            renderer.DrawText(m_placeholder, textX, textY, Colors::TextDim, 1.0f);
        } else {
            Vec4 textColor = m_enabled ? Colors::Text : Colors::TextDisabled;
            renderer.DrawText(m_text, textX, textY, textColor, 1.0f);
        }

        // Cursor (when focused)
        if (m_focused && m_cursorVisible) {
            float cursorX = textX + m_cursorPos * 8;
            renderer.DrawRect(Rect(cursorX, textY, 2, 12), Colors::Accent);
        }
    }

    bool OnMouseDown(float x, float y, int button) override {
        bool result = Widget::OnMouseDown(x, y, button);
        if (result) {
            // Calculate cursor position from click
            float textX = m_rect.x + 4;
            int clickPos = static_cast<int>((x - textX) / 8);
            m_cursorPos = std::clamp(clickPos, 0, static_cast<int>(m_text.length()));
            m_cursorBlinkTimer = 0;
            m_cursorVisible = true;
        }
        return result;
    }

    bool OnKeyDown(int key) override {
        if (!m_focused || !m_enabled) return false;

        switch (key) {
            case GLFW_KEY_BACKSPACE:
                if (m_cursorPos > 0) {
                    m_text.erase(m_cursorPos - 1, 1);
                    m_cursorPos--;
                    if (m_onChange) m_onChange(m_text);
                }
                break;

            case GLFW_KEY_DELETE:
                if (m_cursorPos < static_cast<int>(m_text.length())) {
                    m_text.erase(m_cursorPos, 1);
                    if (m_onChange) m_onChange(m_text);
                }
                break;

            case GLFW_KEY_LEFT:
                if (m_cursorPos > 0) m_cursorPos--;
                break;

            case GLFW_KEY_RIGHT:
                if (m_cursorPos < static_cast<int>(m_text.length())) m_cursorPos++;
                break;

            case GLFW_KEY_HOME:
                m_cursorPos = 0;
                break;

            case GLFW_KEY_END:
                m_cursorPos = static_cast<int>(m_text.length());
                break;

            case GLFW_KEY_ENTER:
                if (m_onSubmit) m_onSubmit(m_text);
                break;

            default:
                return false;
        }

        m_cursorBlinkTimer = 0;
        m_cursorVisible = true;
        return true;
    }

    bool OnCharInput(unsigned int codepoint) override {
        if (!m_focused || !m_enabled) return false;
        if (codepoint < 32 || codepoint > 126) return false;

        if (m_text.length() < m_maxLength) {
            char c = static_cast<char>(codepoint);
            m_text.insert(m_cursorPos, 1, c);
            m_cursorPos++;
            if (m_onChange) m_onChange(m_text);
        }

        m_cursorBlinkTimer = 0;
        m_cursorVisible = true;
        return true;
    }

private:
    std::string m_text;
    std::string m_placeholder;
    size_t m_maxLength = 256;

    int m_cursorPos = 0;
    float m_cursorBlinkTimer = 0.0f;
    bool m_cursorVisible = true;

    ChangeCallback m_onChange;
    SubmitCallback m_onSubmit;
};

} // namespace GUI
} // namespace Genesis

