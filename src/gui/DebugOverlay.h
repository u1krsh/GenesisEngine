#pragma once

#include "GUIRenderer.h"
#include "Console.h"

namespace Genesis {
namespace GUI {

// ============================================================================
// DebugOverlay - Shows debug information on screen (controlled by ge_showinfo)
// ============================================================================
class DebugOverlay {
public:
    static DebugOverlay& Instance() {
        static DebugOverlay instance;
        return instance;
    }

    void Render(int screenWidth, int screenHeight);

    // Direct control - bypasses convar
    void SetVisible(bool visible) { m_forceVisible = visible; }
    bool IsVisible() const { return m_forceVisible; }
    void Toggle() { m_forceVisible = !m_forceVisible; }
    
    // Detailed stats (F3) - shows BSP, PVS, collision, minimap
    void SetShowDetailed(bool show) { m_showDetailed = show; }
    bool GetShowDetailed() const { return m_showDetailed; }
    void ToggleDetailed() { m_showDetailed = !m_showDetailed; }

private:
    DebugOverlay() = default;
    bool m_forceVisible = false;
    bool m_showDetailed = false;  // F3 for detailed stats
};

} // namespace GUI
} // namespace Genesis

