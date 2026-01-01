#pragma once

#include "GUITypes.h"
#include "renderer/shader/Shader.h"
#include <vector>
#include <string>

namespace Genesis {
namespace GUI {

// ============================================================================
// GUIRenderer - Low-level GUI rendering backend
// ============================================================================
class GUIRenderer {
public:
    static GUIRenderer& Instance() {
        static GUIRenderer instance;
        return instance;
    }

    bool Initialize();
    void Shutdown();

    // Begin/End frame
    void BeginFrame(int screenWidth, int screenHeight);
    void EndFrame();

    // ========================================================================
    // Primitive Drawing
    // ========================================================================

    void DrawRect(const Rect& rect, const Vec4& color);
    void DrawRectOutline(const Rect& rect, const Vec4& color, float thickness = 1.0f);
    void DrawRectGradientV(const Rect& rect, const Vec4& topColor, const Vec4& bottomColor);
    void DrawRectGradientH(const Rect& rect, const Vec4& leftColor, const Vec4& rightColor);

    // Windows 7 style 3D border (raised or sunken)
    void DrawBorder3D(const Rect& rect, bool raised = true);

    // Text drawing (simple bitmap font)
    void DrawText(const std::string& text, float x, float y, const Vec4& color, float scale = 1.0f);
    void DrawTextCentered(const std::string& text, const Rect& rect, const Vec4& color, float scale = 1.0f);

    // Get text dimensions
    Vec2 MeasureText(const std::string& text, float scale = 1.0f) const;
    float GetFontHeight(float scale = 1.0f) const;

    // ========================================================================
    // Scissor/Clipping
    // ========================================================================

    void PushClipRect(const Rect& rect);
    void PopClipRect();

private:
    GUIRenderer() = default;

    void Flush();
    void AddVertex(float x, float y, float u, float v, const Vec4& color);
    void CreateFontTexture();

private:
    std::shared_ptr<Shader> m_shader;
    unsigned int m_vao = 0;
    unsigned int m_vbo = 0;
    unsigned int m_fontTexture = 0;

    std::vector<GUIVertex> m_vertices;
    std::vector<Rect> m_clipStack;

    int m_screenWidth = 1280;
    int m_screenHeight = 720;

    bool m_initialized = false;

    // Simple 8x8 bitmap font data
    static constexpr int FONT_CHAR_WIDTH = 8;
    static constexpr int FONT_CHAR_HEIGHT = 12;
    static constexpr int FONT_TEXTURE_SIZE = 128;
};

} // namespace GUI
} // namespace Genesis

