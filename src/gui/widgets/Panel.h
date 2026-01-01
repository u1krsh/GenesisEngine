#pragma once

#include "Widget.h"
#include <vector>
#include <memory>

namespace Genesis {
namespace GUI {

// ============================================================================
// Panel Widget - Windows 7 style container with title bar
// ============================================================================
class Panel : public Widget {
public:
    Panel() {
        m_rect = Rect(0, 0, 300, 200);
    }

    Panel(const std::string& title, float x, float y, float width = 300, float height = 200)
        : m_title(title) {
        m_rect = Rect(x, y, width, height);
    }

    void SetTitle(const std::string& title) { m_title = title; }
    const std::string& GetTitle() const { return m_title; }

    void SetDraggable(bool draggable) { m_draggable = draggable; }
    bool IsDraggable() const { return m_draggable; }

    void SetCloseable(bool closeable) { m_closeable = closeable; }
    bool IsCloseable() const { return m_closeable; }

    // Child widget management
    void AddChild(std::shared_ptr<Widget> child) {
        m_children.push_back(child);
    }

    void RemoveChild(const std::string& id) {
        m_children.erase(
            std::remove_if(m_children.begin(), m_children.end(),
                [&id](const auto& w) { return w->GetId() == id; }),
            m_children.end()
        );
    }

    void ClearChildren() { m_children.clear(); }

    void Update(float deltaTime) override {
        for (auto& child : m_children) {
            child->Update(deltaTime);
        }
    }

    void Render(GUIRenderer& renderer) override {
        if (!m_visible) return;

        // Panel background with gradient
        renderer.DrawRectGradientV(m_rect, Colors::PanelHeader, Colors::PanelBackground);

        // 3D border
        renderer.DrawBorder3D(m_rect, true);

        // Title bar
        float titleBarHeight = 24;
        Rect titleRect(m_rect.x, m_rect.y, m_rect.width, titleBarHeight);
        renderer.DrawRectGradientV(titleRect, Colors::TitleBarGradientTop, Colors::TitleBarGradientBottom);

        // Red accent line at top
        renderer.DrawRect(Rect(m_rect.x, m_rect.y, m_rect.width, 2), Colors::Accent);

        // Title text
        renderer.DrawText(m_title, m_rect.x + 8, m_rect.y + 6, Colors::TextHighlight, 1.0f);

        // Close button (if closeable)
        if (m_closeable) {
            float closeSize = 16;
            Rect closeRect(m_rect.x + m_rect.width - closeSize - 4, m_rect.y + 4, closeSize, closeSize);

            Vec4 closeColor = m_closeHovered ? Colors::AccentHover : Colors::Accent;
            renderer.DrawRect(closeRect, closeColor);
            renderer.DrawBorder3D(closeRect, !m_closePressed);

            // X mark
            renderer.DrawText("X", closeRect.x + 4, closeRect.y + 2, Colors::TextHighlight, 1.0f);
        }

        // Render children (offset by title bar)
        for (auto& child : m_children) {
            // Children positions are relative to panel content area
            child->Render(renderer);
        }
    }

    bool OnMouseMove(float x, float y) override {
        if (!m_visible || !m_enabled) return false;

        // Check close button hover
        if (m_closeable) {
            float closeSize = 16;
            Rect closeRect(m_rect.x + m_rect.width - closeSize - 4, m_rect.y + 4, closeSize, closeSize);
            m_closeHovered = closeRect.Contains(x, y);
        }

        // Handle dragging
        if (m_dragging) {
            m_rect.x = x - m_dragOffsetX;
            m_rect.y = y - m_dragOffsetY;
            return true;
        }

        // Pass to children
        for (auto& child : m_children) {
            if (child->OnMouseMove(x, y)) return true;
        }

        return Widget::OnMouseMove(x, y);
    }

    bool OnMouseDown(float x, float y, int button) override {
        if (!m_visible || !m_enabled) return false;

        // Check close button
        if (m_closeable) {
            float closeSize = 16;
            Rect closeRect(m_rect.x + m_rect.width - closeSize - 4, m_rect.y + 4, closeSize, closeSize);
            if (closeRect.Contains(x, y)) {
                m_closePressed = true;
                return true;
            }
        }

        // Pass to children first
        for (auto& child : m_children) {
            if (child->OnMouseDown(x, y, button)) return true;
        }

        // Start dragging if clicking title bar
        if (m_draggable) {
            float titleBarHeight = 24;
            Rect titleRect(m_rect.x, m_rect.y, m_rect.width, titleBarHeight);
            if (titleRect.Contains(x, y)) {
                m_dragging = true;
                m_dragOffsetX = x - m_rect.x;
                m_dragOffsetY = y - m_rect.y;
                return true;
            }
        }

        return Widget::OnMouseDown(x, y, button);
    }

    bool OnMouseUp(float x, float y, int button) override {
        if (!m_visible || !m_enabled) return false;

        // Check close button click
        if (m_closeable && m_closePressed) {
            m_closePressed = false;
            float closeSize = 16;
            Rect closeRect(m_rect.x + m_rect.width - closeSize - 4, m_rect.y + 4, closeSize, closeSize);
            if (closeRect.Contains(x, y)) {
                m_visible = false;  // Close the panel
                return true;
            }
        }

        // Stop dragging
        m_dragging = false;

        // Pass to children
        for (auto& child : m_children) {
            if (child->OnMouseUp(x, y, button)) return true;
        }

        return Widget::OnMouseUp(x, y, button);
    }

    // Get content area (excluding title bar)
    Rect GetContentRect() const {
        float titleBarHeight = 24;
        return Rect(m_rect.x + 4, m_rect.y + titleBarHeight + 4,
                   m_rect.width - 8, m_rect.height - titleBarHeight - 8);
    }

private:
    std::string m_title;
    std::vector<std::shared_ptr<Widget>> m_children;

    bool m_draggable = true;
    bool m_closeable = true;
    bool m_dragging = false;
    float m_dragOffsetX = 0;
    float m_dragOffsetY = 0;

    bool m_closeHovered = false;
    bool m_closePressed = false;
};

} // namespace GUI
} // namespace Genesis

