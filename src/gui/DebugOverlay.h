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

private:
    DebugOverlay() = default;
    bool m_forceVisible = false;
};

} // namespace GUI
} // namespace Genesis

