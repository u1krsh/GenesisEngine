// ============================================================================
// Genesis Engine - Game Main Entry Point
// ============================================================================

#include "core/Engine.h"
#include "core/Logger.h"
#include "renderer/DebugRenderer.h"
#include "renderer/shader/Shader.h"
#include "renderer/Mesh.h"
#include "renderer/Material.h"
#include "renderer/world/StaticWorldRenderer.h"
#include "gui/Console.h"
#include "gui/DebugOverlay.h"
#include "world/WorldCollision.h"
#include "map/MapRenderer.h"
#include "map/Brush.h"
#include "Player.h"

#include <algorithm>  // for std::max

using namespace Genesis;

// ============================================================================
// Game State
// ============================================================================
static DebugRenderer g_debugRenderer;
static std::shared_ptr<Shader> g_debugShader;
static std::shared_ptr<Shader> g_basicShader;
static Game::Player g_player;
static bool g_showCollisionDebug = false;

// Debug visualization meshes (grid, axes)
static MeshPtr g_gridMesh;
static MeshPtr g_axesMesh;
static MaterialPtr g_matDebug;

// ============================================================================
// Setup World Collision Geometry - Now handled by MapRenderer
// ============================================================================
// The world collision is now set up automatically when loading a map via MapRenderer.
// This function is kept as a fallback for when no map is loaded.
void SetupWorldCollision() {
    auto& world = WorldCollision::Instance();
    world.Clear();
    world.SetFloorHeight(0.0f);
    LOG_INFO("Game", "World collision cleared - map will provide geometry");
}

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

    g_basicShader = shaderLib.Load("mesh", "mesh.vert", "mesh.frag");
    if (!g_basicShader) {
        LOG_ERROR("Game", "Failed to load mesh shader");
        return false;
    }

    // Initialize debug renderer
    if (!g_debugRenderer.Initialize()) {
        LOG_ERROR("Game", "Failed to initialize debug renderer");
        return false;
    }

    // ========================================================================
    // Create debug visualization meshes
    // ========================================================================
    g_gridMesh = MeshPrimitives::CreateGrid(60.0f, 1.0f, Vec3(0.25f, 0.25f, 0.25f), "Grid");
    g_axesMesh = MeshPrimitives::CreateAxes(3.0f, "Axes");

    // ========================================================================
    // Setup Materials - Source Engine-style (one shader, many materials)
    // The MapLoader will auto-create materials based on brush material names
    // but we can pre-register some with specific colors if desired.
    // ========================================================================
    LOG_INFO("Game", "Creating materials...");
    auto& matLib = MaterialLibrary::Instance();

    // Debug material (unlit, for grid/axes)
    g_matDebug = matLib.Create("Debug", g_debugShader);
    g_matDebug->SetCullMode(CullMode::Off);

    // Pre-register common materials with specific colors
    // These will be used by MapLoader if brushes reference them
    matLib.CreateSolidColor("floor", Vec3(0.15f, 0.15f, 0.18f));
    matLib.CreateSolidColor("wall", Vec3(0.45f, 0.45f, 0.5f));
    matLib.CreateSolidColor("concrete", Vec3(0.5f, 0.5f, 0.52f));
    matLib.CreateSolidColor("brick", Vec3(0.55f, 0.3f, 0.25f));
    matLib.CreateSolidColor("metal", Vec3(0.5f, 0.5f, 0.55f));
    matLib.CreateSolidColor("wood", Vec3(0.45f, 0.3f, 0.2f));

    LOG_INFO("Game", "Materials created!");

    // ========================================================================
    // Load Map from file - World geometry is now data-driven!
    // ========================================================================
    LOG_INFO("Game", "Loading map...");

    auto& mapRenderer = MapRenderer::Instance();

    // Load the test map (you can switch to .json or .map format)
    if (!mapRenderer.LoadMap("testmap.json")) {
        LOG_WARNING("Game", "Failed to load testmap.json, trying testmap.map...");
        if (!mapRenderer.LoadMap("testmap.map")) {
            LOG_ERROR("Game", "Failed to load any map file!");
            // Fall back to a simple floor
            auto& staticWorld = StaticWorldRenderer::Instance();
            staticWorld.Clear();
            auto groundPlane = MeshPrimitives::CreatePlane(60.0f, 60.0f, 30, 30, "GroundPlane");
            auto matFloor = MaterialLibrary::Instance().CreateSolidColor("Floor", Vec3(0.15f, 0.15f, 0.18f));
            staticWorld.AddFloor(groundPlane, matFloor, glm::translate(Mat4(1.0f), Vec3(0.0f, 0.0f, 0.0f)));
            staticWorld.SetDirectionalLight(Vec3(0.5f, 1.0f, 0.3f), Vec3(1.0f, 0.98f, 0.95f), 1.0f);
            staticWorld.SetAmbientLight(Vec3(0.15f, 0.15f, 0.2f), 1.0f);
            staticWorld.RebuildBatches();
            SetupWorldCollision();
        }
    }

    LOG_INFO("Game", "Map loaded with " + std::to_string(mapRenderer.GetBrushCount()) + " brushes");

    // Setup lighting from map metadata (or use defaults)
    auto& staticWorld = StaticWorldRenderer::Instance();
    if (mapRenderer.HasMap()) {
        auto& meta = mapRenderer.GetActiveMap()->GetMetadata();
        staticWorld.SetDirectionalLight(meta.sunDirection, meta.sunColor, meta.sunIntensity);
        staticWorld.SetAmbientLight(meta.ambientColor, 1.0f);
    }

    // Configure and initialize player
    Game::PlayerConfig playerConfig;

    // Get spawn position from map, or use default
    if (mapRenderer.HasMap()) {
        playerConfig.spawnPosition = mapRenderer.GetSpawnPosition();
        LOG_INFO("Game", "Player spawn from map: " +
            std::to_string(playerConfig.spawnPosition.x) + ", " +
            std::to_string(playerConfig.spawnPosition.y) + ", " +
            std::to_string(playerConfig.spawnPosition.z));
    } else {
        playerConfig.spawnPosition = Vec3(0.0f, 1.0f, 5.0f);
    }
    playerConfig.mouseSensitivity = 0.1f;

    // Controller settings - Source-style movement
    playerConfig.controllerConfig.walkSpeed = 6.0f;     // Base movement speed
    playerConfig.controllerConfig.sprintSpeed = 9.0f;   // Sprint speed
    playerConfig.controllerConfig.crouchSpeed = 3.0f;   // Crouch speed

    // Source-style physics
    playerConfig.controllerConfig.groundAccelerate = 10.0f;  // Ground acceleration (Source default: 10)
    playerConfig.controllerConfig.groundFriction = 5.0f;     // Ground friction (Source default: 4-6)
    playerConfig.controllerConfig.stopSpeed = 1.5f;          // Speed below which friction applies fully
    playerConfig.controllerConfig.airAccelerate = 12.0f;     // Air acceleration (higher = better strafing)
    playerConfig.controllerConfig.airSpeedCap = 0.7f;        // Limits air speed gain
    playerConfig.controllerConfig.airFriction = 0.0f;        // No air friction for bhop potential

    // Jump and gravity
    playerConfig.controllerConfig.jumpForce = 8.0f;
    playerConfig.controllerConfig.gravity = 25.0f;           // Slightly stronger gravity
    playerConfig.controllerConfig.maxFallSpeed = 50.0f;      // Terminal velocity

    // Player dimensions
    playerConfig.controllerConfig.eyeHeight = 1.6f;
    playerConfig.controllerConfig.stepHeight = 0.35f;
    playerConfig.controllerConfig.capsuleRadius = 0.3f;
    playerConfig.controllerConfig.capsuleHeight = 1.8f;
    playerConfig.controllerConfig.groundCheckDistance = 0.15f;

    // Auto stair climbing
    playerConfig.controllerConfig.autoClimbStairHeight = 0.5f;  // Max height for auto-climb
    playerConfig.controllerConfig.stairClimbSpeed = 10.0f;      // Speed of climbing

    g_player.Initialize(playerConfig);

    // Connect player to world collision system
    auto& controller = g_player.GetController();

    // Set ground height callback
    controller.SetGroundHeightCallback([](float x, float z, float playerY) -> float {
        return WorldCollision::Instance().GetGroundHeight(x, z, 0.3f, playerY);
    });

    // Set collision callback (pass stair climb height to allow walking into climbable stairs)
    float stairClimbHeight = playerConfig.controllerConfig.autoClimbStairHeight;
    controller.SetCollisionCallback([stairClimbHeight](const Vec3& position, const AABB& bounds) -> bool {
        return WorldCollision::Instance().CheckCollision(position, bounds, stairClimbHeight);
    });

    // Set depenetration callback (pass stair climb height to not push out of climbable stairs)
    controller.SetDepenetrationCallback([stairClimbHeight](const AABB& bounds, Vec3& pushOut) -> bool {
        return WorldCollision::Instance().GetPenetration(bounds, pushOut, stairClimbHeight);
    });

    // Set stair climb callback for auto-climbing tagged stairs
    controller.SetStairClimbCallback([](float x, float z, float playerY, float radius, float maxHeight, const Vec3& moveDir) -> float {
        return WorldCollision::Instance().GetStairClimbHeight(x, z, playerY, radius, maxHeight, moveDir);
    });

    LOG_INFO("Game", "Game initialized successfully");
    LOG_INFO("Game", "Controls: WASD=Move, Mouse=Look, Shift=Sprint, Space=Jump, Ctrl=Crouch");
    LOG_INFO("Game", "          Left Click=Capture Mouse, Right Click=Release, ESC=Quit");
    LOG_INFO("Game", "          F1=Collision Debug, F2=Console, F3=Debug Overlay");

    return true;
}

// ============================================================================
// Draw Collision Debug Visualization
// ============================================================================
void DrawCollisionDebug() {
    auto& world = WorldCollision::Instance();
    const auto& boxes = world.GetBoxes();

    // Draw each collision box as a wire cube (yellow for normal, cyan for stairs)
    for (const auto& box : boxes) {
        AABB aabb = box.GetAABB();
        Vec3 center = (aabb.min + aabb.max) * 0.5f;
        Vec3 size = aabb.max - aabb.min;

        // Draw wire box - stairs in cyan, normal boxes in yellow
        if (box.IsStair()) {
            g_debugRenderer.DrawWireBox(center.x, center.y, center.z,
                                         size.x, size.y, size.z,
                                         0.0f, 1.0f, 1.0f);  // Cyan for stairs
        } else {
            g_debugRenderer.DrawWireBox(center.x, center.y, center.z,
                                         size.x, size.y, size.z,
                                         1.0f, 1.0f, 0.0f);  // Yellow for normal
        }
    }

    // Draw brush mesh wireframes (magenta/pink for visual geometry)
    auto& mapRenderer = MapRenderer::Instance();
    if (mapRenderer.HasMap()) {
        const auto& brushes = mapRenderer.GetActiveMap()->GetBrushes();
        for (const auto& brush : brushes) {
            Vec3 pos = brush.position;
            Vec3 size = brush.size;

            // Different colors for different shapes
            float r = 1.0f, g = 0.4f, b = 1.0f;  // Magenta for mesh wireframe

            switch (brush.shape) {
                case BrushShape::Cube:
                    g_debugRenderer.DrawWireBox(pos.x, pos.y, pos.z,
                                                 size.x, size.y, size.z,
                                                 r, g, b);
                    break;

                case BrushShape::Sphere: {
                    float radius = std::max({size.x, size.y, size.z}) * 0.5f;
                    g_debugRenderer.DrawWireSphere(pos.x, pos.y, pos.z, radius, r, g, b);
                    break;
                }

                case BrushShape::Cone:
                    g_debugRenderer.DrawWireCone(pos.x, pos.y, pos.z,
                                                  size.x * 0.5f, size.y,
                                                  r, g, b);
                    break;

                case BrushShape::Cylinder:
                    g_debugRenderer.DrawWireCylinder(pos.x, pos.y, pos.z,
                                                      size.x * 0.5f, size.y,
                                                      r, g, b);
                    break;

                default:
                    g_debugRenderer.DrawWireBox(pos.x, pos.y, pos.z,
                                                 size.x, size.y, size.z,
                                                 r, g, b);
                    break;
            }
        }
    }

    // Draw player collision bounds
    auto& controller = g_player.GetController();
    Vec3 playerPos = controller.GetPosition();
    float playerHeight = controller.GetConfig().capsuleHeight;
    float playerRadius = controller.GetConfig().capsuleRadius;

    // Draw player full AABB in green
    g_debugRenderer.DrawWireBox(playerPos.x, playerPos.y + playerHeight * 0.5f, playerPos.z,
                                 playerRadius * 2.0f, playerHeight, playerRadius * 2.0f,
                                 0.0f, 1.0f, 0.0f);

    // Draw the "raised" collision check area in cyan (the area used for horizontal collision)
    float stepOffset = controller.GetConfig().stepHeight + 0.05f;
    float checkHeight = playerHeight - stepOffset;
    if (checkHeight > 0.1f) {
        g_debugRenderer.DrawWireBox(playerPos.x, playerPos.y + stepOffset + checkHeight * 0.5f, playerPos.z,
                                     playerRadius * 2.0f, checkHeight, playerRadius * 2.0f,
                                     0.0f, 1.0f, 1.0f);
    }
}

// ============================================================================
// Game Shutdown
// ============================================================================
void OnShutdown() {
    LOG_INFO("Game", "Shutting down game...");
    g_debugRenderer.Shutdown();
}

// ============================================================================
// Per-Frame Input (called once per frame for mouse look and jump)
// ============================================================================
void OnInput(double deltaTime) {
    auto& input = InputManager::Instance();

    // Mouse look
    double dx, dy;
    input.GetMouseDelta(dx, dy);

    if (dx != 0.0 || dy != 0.0) {
        g_player.ProcessMouseLook(static_cast<float>(dx), static_cast<float>(dy));
    }

    // Jump - must be detected per-frame, not in fixed update
    if (input.IsActionPressed(GameAction::Jump)) {
        g_player.GetController().Jump();
    }

    // F1 - Toggle collision debug view (also enables debug overlay)
    if (input.IsKeyPressed(KeyCode::F1)) {
        g_showCollisionDebug = !g_showCollisionDebug;
        // Also toggle debug overlay directly
        GUI::DebugOverlay::Instance().SetVisible(g_showCollisionDebug);
        LOG_INFO("Debug", g_showCollisionDebug ? "Collision debug + overlay ON" : "Collision debug + overlay OFF");
    }

    // F2 - Toggle console (alternative to ~ key)
    if (input.IsKeyPressed(KeyCode::F2)) {
        GUI::Console::Instance().Toggle();
        LOG_INFO("Debug", "Console toggled via F2");
    }

    // F3 - Toggle debug overlay directly (without collision boxes)
    if (input.IsKeyPressed(KeyCode::F3)) {
        GUI::DebugOverlay::Instance().Toggle();
        LOG_INFO("Debug", GUI::DebugOverlay::Instance().IsVisible() ? "Debug overlay ON (F3)" : "Debug overlay OFF (F3)");
    }
}

// ============================================================================
// Game Update (Fixed Timestep)
// ============================================================================
void OnUpdate(double deltaTime) {
    // Update player with deltaTime (movement, physics)
    g_player.Update(deltaTime);
}

// ============================================================================
// Game Render (Variable Framerate)
// ============================================================================
void OnRender(double interpolation) {
    auto& engine = Engine::Instance();
    auto& camera = engine.GetCamera();

    // ========================================================================
    // Render Static World - All floors, walls, ceilings, props
    // ========================================================================
    auto& staticWorld = StaticWorldRenderer::Instance();
    staticWorld.Render(camera);

    // ========================================================================
    // Render Debug Visualization (Grid, Axes)
    // ========================================================================
    if (g_debugShader && g_debugShader->IsValid()) {
        g_debugShader->Bind();
        g_debugShader->SetMat4("u_View", camera.GetViewMatrix());
        g_debugShader->SetMat4("u_Proj", camera.GetProjectionMatrix());

        g_gridMesh->Draw();
        g_axesMesh->Draw();

        g_debugShader->Unbind();
    }

    // ========================================================================
    // Debug Renderer - Player visualization and collision debug
    // ========================================================================
    if (g_debugShader && g_debugShader->IsValid()) {
        g_debugShader->Bind();
        g_debugShader->SetMat4("u_View", camera.GetViewMatrix());
        g_debugShader->SetMat4("u_Proj", camera.GetProjectionMatrix());

        g_debugRenderer.BeginFrame();

        // Render player debug visualization
        g_player.Render(&g_debugRenderer);

        // F1 - Draw collision debug wireframes over real geometry
        if (g_showCollisionDebug) {
            DrawCollisionDebug();
        }

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
    engine.SetOnInput(OnInput);
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

