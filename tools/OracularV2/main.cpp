// ============================================================================
// OracularV2 Map Editor - Entry Point
// ============================================================================

#include "EditorApp.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "  OracularV2 Map Editor\n";
    std::cout << "  Hammer-style editor for GenesisEngine\n";
    std::cout << "========================================\n\n";
    
    EditorApp app;
    
    if (!app.Initialize(1600, 900, "OracularV2 Map Editor")) {
        std::cerr << "Failed to initialize editor\n";
        return 1;
    }
    
    app.Run();
    app.Shutdown();
    
    return 0;
}
