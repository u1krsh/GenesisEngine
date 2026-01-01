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

private:
    DebugOverlay() = default;
};

} // namespace GUI
} // namespace Genesis

