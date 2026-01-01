// ============================================================================
// Genesis Engine - Game Main Entry Point
// ============================================================================

#include "core/Engine.h"
#include "core/Logger.h"
#include "renderer/DebugRenderer.h"
#include "renderer/shader/Shader.h"
#include "Player.h"

using namespace Genesis;

// ============================================================================
// Game State
// ============================================================================
static DebugRenderer g_debugRenderer;
static std::shared_ptr<Shader> g_debugShader;
static Game::Player g_player;

// ============================================================================
// Game Initialization
// ============================================================================
bool OnInit() {
    LOG_INFO("Game", "Initializing game...");

    // Load shaders
    auto& shaderLib = ShaderLibrary::Instance();

    g_debugShader = shaderLib.Load("debug", "debug.vert", "debug.frag");
    if (!g_debugShader) {
        LOG_ERROR("Game", "Failed to load debug shader");
        return false;
    }

    // Initialize debug renderer
    if (!g_debugRenderer.Initialize()) {
        LOG_ERROR("Game", "Failed to initialize debug renderer");
        return false;
    }

    // Initialize player
    g_player.Initialize();

    LOG_INFO("Game", "Game initialized successfully");
    LOG_INFO("Game", "Controls: WASD=Move, Mouse=Look, Shift=Sprint, Space=Up, Ctrl=Down");
    LOG_INFO("Game", "          Left Click=Capture Mouse, Right Click=Release, ESC=Quit");

    return true;
}

// ============================================================================
// Game Shutdown
// ============================================================================
void OnShutdown() {
    LOG_INFO("Game", "Shutting down game...");
    g_debugRenderer.Shutdown();
}

// ============================================================================
// Game Update (Fixed Timestep)
// ============================================================================
void OnUpdate(double deltaTime) {
    g_player.Update(deltaTime);
}

// ============================================================================
// Game Render (Variable Framerate)
// ============================================================================
void OnRender(double interpolation) {
    auto& engine = Engine::Instance();
    auto& camera = engine.GetCamera();

    if (g_debugShader && g_debugShader->IsValid()) {
        g_debugShader->Bind();
        g_debugShader->SetMat4("u_View", camera.GetViewMatrix());
        g_debugShader->SetMat4("u_Proj", camera.GetProjectionMatrix());

        g_debugRenderer.BeginFrame();

        // Draw grid
        g_debugRenderer.DrawGrid(50.0f, 1.0f, 0.25f, 0.25f, 0.25f);

        // Draw axes
        g_debugRenderer.DrawAxes(5.0f);

        // Draw floor
        g_debugRenderer.DrawFloor(20.0f, -0.01f, 0.15f, 0.15f, 0.2f);

        // Draw reference cubes
        g_debugRenderer.DrawCube(0.0f, 0.5f, 0.0f, 1.0f, 0.8f, 0.8f, 0.8f);      // Origin
        g_debugRenderer.DrawCube(5.0f, 0.5f, 0.0f, 1.0f, 1.0f, 0.3f, 0.3f);      // +X Red
        g_debugRenderer.DrawCube(0.0f, 5.0f, 0.0f, 1.0f, 0.3f, 1.0f, 0.3f);      // +Y Green
        g_debugRenderer.DrawCube(0.0f, 0.5f, 5.0f, 1.0f, 0.3f, 0.3f, 1.0f);      // +Z Blue
        g_debugRenderer.DrawCube(10.0f, 0.5f, 0.0f, 1.0f, 0.7f, 0.2f, 0.2f);     // +10X
        g_debugRenderer.DrawCube(0.0f, 0.5f, 10.0f, 1.0f, 0.2f, 0.2f, 0.7f);     // +10Z

        // Wire cubes at negative positions
        g_debugRenderer.DrawWireCube(-5.0f, 0.5f, 0.0f, 1.0f, 0.5f, 0.2f, 0.2f);
        g_debugRenderer.DrawWireCube(0.0f, 0.5f, -5.0f, 1.0f, 0.2f, 0.2f, 0.5f);

        // Scattered cubes
        g_debugRenderer.DrawCube(3.0f, 0.5f, 3.0f, 1.0f, 0.6f, 0.4f, 0.8f);
        g_debugRenderer.DrawCube(-3.0f, 0.5f, 4.0f, 1.0f, 0.8f, 0.6f, 0.4f);
        g_debugRenderer.DrawCube(7.0f, 0.5f, -3.0f, 1.0f, 0.4f, 0.8f, 0.6f);

        // Stack of cubes
        g_debugRenderer.DrawCube(-5.0f, 1.5f, -5.0f, 1.0f, 0.5f, 0.5f, 0.5f);
        g_debugRenderer.DrawCube(-5.0f, 2.5f, -5.0f, 0.8f, 0.6f, 0.6f, 0.6f);
        g_debugRenderer.DrawCube(-5.0f, 3.3f, -5.0f, 0.6f, 0.7f, 0.7f, 0.7f);

        // Render
        g_debugRenderer.RenderTriangles();
        g_debugRenderer.RenderLines();
        g_debugRenderer.EndFrame();

        g_debugShader->Unbind();
    }
}

// ============================================================================
// Main Entry Point
// ============================================================================
int main() {
    // Configure engine
    EngineConfig config;
    config.windowTitle = "Genesis Engine";
    config.windowWidth = 1920;
    config.windowHeight = 1080;
    config.fullscreen = false;  // Launch in fullscreen mode
    config.vsync = false;
    config.fixedTimestep = 1.0 / 66.0;

    // Get engine instance
    auto& engine = Engine::Instance();

    // Set callbacks
    engine.SetOnInit(OnInit);
    engine.SetOnShutdown(OnShutdown);
    engine.SetOnUpdate(OnUpdate);
    engine.SetOnRender(OnRender);

    // Initialize and run
    if (!engine.Initialize(config)) {
        LOG_FATAL("Game", "Failed to initialize engine");
        return -1;
    }

    engine.Run();
    engine.Shutdown();

    return 0;
}

