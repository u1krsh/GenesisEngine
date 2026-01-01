#include "DebugOverlay.h"
#include "core/Engine.h"
#include "core/Time.h"
#include <sstream>
#include <iomanip>

namespace Genesis {
namespace GUI {

void DebugOverlay::Render(int screenWidth, int screenHeight) {
    auto* showInfo = Console::Instance().FindConVar("ge_showinfo");
    if (!showInfo || !showInfo->GetBool()) return;

    auto& renderer = GUIRenderer::Instance();
    auto& engine = Engine::Instance();
    auto& camera = engine.GetCamera();
    auto& time = Time::Instance();

    float lineHeight = 14;
    float padding = 8;

    // Background panel - positioned at TOP LEFT
    float panelWidth = 280;
    float panelHeight = lineHeight * 13 + padding * 2;
    Rect panelRect(10, 10, panelWidth, panelHeight);

    // Windows 7 style panel with gradient
    renderer.DrawRectGradientV(panelRect, Colors::PanelHeader, Colors::PanelBackground);
    renderer.DrawBorder3D(panelRect, true);

    // Title bar accent line (red)
    renderer.DrawRect(Rect(10, 10, panelWidth, 2), Colors::Accent);

    float x = 10 + padding;
    float y = 10 + padding + 4;

    // Title with red accent
    renderer.DrawText("=== Debug Info ===", x, y, Colors::Accent, 1.0f);
    y += lineHeight + 6;

    // Separator line
    renderer.DrawRect(Rect(x, y - 2, panelWidth - padding * 2, 1), Colors::BorderDark);

    // FPS
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "FPS: " << time.GetFPS();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Frame time
    oss.str("");
    oss << "Frame Time: " << (time.GetDeltaTime() * 1000.0) << " ms";
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Total time
    oss.str("");
    oss << "Total Time: " << std::setprecision(1) << time.GetTotalTime() << " s";
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Frame count
    oss.str("");
    oss << "Frame: " << time.GetFrameCount();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight + 4;

    // Camera section header
    renderer.DrawText("-- Camera --", x, y, Colors::AccentLight, 1.0f);
    y += lineHeight;

    // Camera position
    const auto& pos = camera.GetPosition();
    oss.str("");
    oss << std::setprecision(2);
    oss << "Pos: (" << pos.x << ", " << pos.y << ", " << pos.z << ")";
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Camera angles
    oss.str("");
    oss << "Yaw: " << std::setprecision(1) << camera.GetYaw()
        << "  Pitch: " << camera.GetPitch();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Camera forward vector
    const auto& fwd = camera.GetForward();
    oss.str("");
    oss << std::setprecision(2);
    oss << "Fwd: (" << fwd.x << ", " << fwd.y << ", " << fwd.z << ")";
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight + 4;

    // Settings section header
    renderer.DrawText("-- Settings --", x, y, Colors::AccentLight, 1.0f);
    y += lineHeight;

    // FOV
    oss.str("");
    oss << "FOV: " << std::setprecision(0) << camera.GetFOV();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
    y += lineHeight;

    // Time scale
    oss.str("");
    oss << "Time Scale: " << std::setprecision(2) << time.GetTimeScale();
    renderer.DrawText(oss.str(), x, y, Colors::Text, 1.0f);
}

} // namespace GUI
} // namespace Genesis

