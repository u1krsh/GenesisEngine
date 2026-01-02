// ============================================================================
// Genesis Engine - Game Main Entry Point
// ============================================================================

#include "core/Engine.h"
#include "core/Logger.h"
#include "renderer/DebugRenderer.h"
#include "renderer/shader/Shader.h"
#include "world/WorldCollision.h"
#include "Player.h"

using namespace Genesis;

// ============================================================================
// Game State
// ============================================================================
static DebugRenderer g_debugRenderer;
static std::shared_ptr<Shader> g_debugShader;
static Game::Player g_player;
static bool g_showCollisionDebug = false;

// ============================================================================
// Setup World Collision Geometry
// ============================================================================
void SetupWorldCollision() {
    auto& world = WorldCollision::Instance();
    world.Clear();
    world.SetFloorHeight(0.0f);

    // ========================================================================
    // MAIN SPAWN AREA - Open courtyard
    // ========================================================================
    // Floor is at y=0 (handled by SetFloorHeight)

    // ========================================================================
    // STAIR TESTING AREA (Southeast) - Various step heights
    // ========================================================================
    // Standard staircase (0.25 unit steps) going up +X direction - AUTO CLIMB
    for (int i = 0; i < 8; i++) {
        float x = 8.0f + i * 1.0f;
        float y = 0.125f + i * 0.25f;  // Each step 0.25 units high
        world.AddStair(x, y, 8.0f, 1.0f, 0.25f + i * 0.5f, 2.0f);  // Wide stairs (tagged as Stair)
    }

    // Landing platform at top of stairs
    world.AddBox(16.5f, 2.0f, 8.0f, 2.0f, 4.0f, 4.0f);

    // Steep staircase (0.4 unit steps) - tests step height limits - AUTO CLIMB
    for (int i = 0; i < 5; i++) {
        float x = 8.0f + i * 0.8f;
        float y = 0.2f + i * 0.4f;
        world.AddStair(x, y, 12.0f, 0.8f, 0.4f + i * 0.8f, 2.0f);  // Tagged as Stair
    }

    // ========================================================================
    // PLATFORMING AREA (Northeast) - Jump testing
    // ========================================================================
    // Series of platforms at different heights
    world.AddCube(-8.0f, 0.5f, 8.0f, 2.0f);      // Ground level platform
    world.AddCube(-8.0f, 1.5f, 12.0f, 2.0f);     // 1 unit up
    world.AddCube(-8.0f, 2.5f, 16.0f, 2.0f);     // 2 units up
    world.AddCube(-12.0f, 2.5f, 16.0f, 2.0f);    // Side platform
    world.AddCube(-12.0f, 3.5f, 12.0f, 2.0f);    // Higher side
    world.AddCube(-12.0f, 1.0f, 8.0f, 2.0f);     // Lower side

    // Gap jump test - platforms with gaps
    world.AddCube(-16.0f, 0.5f, 8.0f, 2.0f);     // Start
    world.AddCube(-16.0f, 0.5f, 12.0f, 2.0f);    // 2 unit gap
    world.AddCube(-16.0f, 1.0f, 17.0f, 2.0f);    // 3 unit gap + height

    // ========================================================================
    // HALLWAY/CORRIDOR AREA (West)
    // ========================================================================
    // Long narrow corridor with walls
    float corridorZ = -8.0f;

    // Left wall
    world.AddBox(-15.0f, 1.5f, corridorZ, 20.0f, 3.0f, 0.5f);
    // Right wall
    world.AddBox(-15.0f, 1.5f, corridorZ + 3.0f, 20.0f, 3.0f, 0.5f);
    // End wall (creates a room)
    world.AddBox(-25.5f, 1.5f, corridorZ + 1.5f, 0.5f, 3.0f, 3.5f);

    // Obstacles in corridor
    world.AddCube(-10.0f, 0.5f, corridorZ + 1.5f, 1.0f);  // Box to jump over
    world.AddCube(-18.0f, 0.25f, corridorZ + 1.5f, 0.5f); // Small step

    // ========================================================================
    // ROOM AREA (Southwest) - Enclosed space
    // ========================================================================
    float roomX = -20.0f;
    float roomZ = -20.0f;
    float roomW = 10.0f;
    float roomD = 10.0f;

    // Room walls (3 units high, 0.5 units thick)
    world.AddBox(roomX, 1.5f, roomZ - roomD/2 + 0.25f, roomW, 3.0f, 0.5f);           // North wall
    world.AddBox(roomX, 1.5f, roomZ + roomD/2 - 0.25f, roomW, 3.0f, 0.5f);           // South wall
    world.AddBox(roomX - roomW/2 + 0.25f, 1.5f, roomZ, 0.5f, 3.0f, roomD - 1.0f);   // West wall
    world.AddBox(roomX + roomW/2 - 0.25f, 1.5f, roomZ - 2.0f, 0.5f, 3.0f, roomD - 5.0f); // East wall (with doorway)

    // Furniture in room
    world.AddCube(roomX - 3.0f, 0.5f, roomZ - 3.0f, 1.0f);  // Table
    world.AddCube(roomX + 2.0f, 0.25f, roomZ + 2.0f, 0.5f); // Small object
    world.AddBox(roomX - 2.0f, 0.4f, roomZ + 3.0f, 3.0f, 0.8f, 1.0f); // Couch

    // ========================================================================
    // RAMP TESTING AREA (South)
    // ========================================================================
    // Gradual slope made of thin steps - AUTO CLIMB
    for (int i = 0; i < 16; i++) {
        float z = -15.0f - i * 0.5f;
        float y = 0.0625f + i * 0.125f;
        world.AddStair(5.0f, y, z, 4.0f, 0.125f + i * 0.25f, 0.5f);  // Tagged as Stair
    }

    // Platform at top of ramp
    world.AddBox(5.0f, 2.0f, -24.0f, 4.0f, 4.0f, 2.0f);

    // ========================================================================
    // VERTICAL CLIMBING AREA (Southeast corner)
    // ========================================================================
    // Stack of blocks to climb
    world.AddCube(15.0f, 0.5f, -10.0f, 2.0f);
    world.AddCube(15.0f, 1.5f, -10.0f, 1.8f);
    world.AddCube(15.0f, 2.5f, -10.0f, 1.6f);
    world.AddCube(15.0f, 3.5f, -10.0f, 1.4f);
    world.AddCube(15.0f, 4.5f, -10.0f, 1.2f);

    // Adjacent pillar
    world.AddBox(18.0f, 2.5f, -10.0f, 1.0f, 5.0f, 1.0f);

    // ========================================================================
    // OBSTACLE COURSE (North)
    // ========================================================================
    // Low obstacles to jump over
    world.AddBox(0.0f, 0.25f, 15.0f, 4.0f, 0.5f, 1.0f);
    world.AddBox(0.0f, 0.5f, 18.0f, 4.0f, 1.0f, 1.0f);
    world.AddBox(0.0f, 0.75f, 21.0f, 4.0f, 1.5f, 1.0f);

    // Pillars to navigate around
    world.AddBox(5.0f, 1.5f, 15.0f, 1.0f, 3.0f, 1.0f);
    world.AddBox(7.0f, 1.5f, 18.0f, 1.0f, 3.0f, 1.0f);
    world.AddBox(5.0f, 1.5f, 21.0f, 1.0f, 3.0f, 1.0f);

    // ========================================================================
    // EDGE/CORNER TEST AREA (center-east)
    // ========================================================================
    // Cubes placed to test corner collision
    world.AddCube(8.0f, 0.5f, 0.0f, 1.0f);
    world.AddCube(9.0f, 0.5f, 1.0f, 1.0f);
    world.AddCube(10.0f, 0.5f, 0.0f, 1.0f);
    world.AddCube(9.0f, 0.5f, -1.0f, 1.0f);

    // L-shaped obstacle
    world.AddBox(12.0f, 0.5f, 3.0f, 3.0f, 1.0f, 1.0f);
    world.AddBox(13.5f, 0.5f, 5.0f, 1.0f, 1.0f, 3.0f);

    // ========================================================================
    // SPAWN AREA REFERENCE MARKERS
    // ========================================================================
    // Keep some reference cubes at origin for orientation
    world.AddCube(0.0f, 0.5f, 0.0f, 1.0f);       // Origin cube

    LOG_INFO("Game", "World collision setup with " + std::to_string(world.GetBoxes().size()) + " boxes");
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

    // Initialize debug renderer
    if (!g_debugRenderer.Initialize()) {
        LOG_ERROR("Game", "Failed to initialize debug renderer");
        return false;
    }

    // Setup world collision geometry
    SetupWorldCollision();

    // Configure and initialize player
    Game::PlayerConfig playerConfig;
    playerConfig.spawnPosition = Vec3(0.0f, 0.0f, 5.0f);
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
    LOG_INFO("Game", "          F1=Toggle Collision Debug");

    return true;
}

// ============================================================================
// Draw Collision Debug Visualization
// ============================================================================
void DrawCollisionDebug() {
    auto& world = WorldCollision::Instance();
    const auto& boxes = world.GetBoxes();

    // Draw each collision box as a wire cube
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

    // F1 - Toggle collision debug view
    if (input.IsKeyPressed(KeyCode::F1)) {
        g_showCollisionDebug = !g_showCollisionDebug;
        LOG_INFO("Debug", g_showCollisionDebug ? "Collision debug ON" : "Collision debug OFF");
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

    if (g_debugShader && g_debugShader->IsValid()) {
        g_debugShader->Bind();
        g_debugShader->SetMat4("u_View", camera.GetViewMatrix());
        g_debugShader->SetMat4("u_Proj", camera.GetProjectionMatrix());

        g_debugRenderer.BeginFrame();

        // Draw grid
        g_debugRenderer.DrawGrid(60.0f, 1.0f, 0.2f, 0.2f, 0.2f);

        // Draw axes at origin
        g_debugRenderer.DrawAxes(3.0f);

        // Draw floor
        g_debugRenderer.DrawFloor(60.0f, -0.01f, 0.12f, 0.12f, 0.15f);

        // ====================================================================
        // SPAWN AREA - Origin marker
        // ====================================================================
        g_debugRenderer.DrawCube(0.0f, 0.5f, 0.0f, 1.0f, 0.8f, 0.8f, 0.8f);

        // ====================================================================
        // STAIR TESTING AREA (Southeast)
        // ====================================================================
        // Standard staircase
        for (int i = 0; i < 8; i++) {
            float x = 8.0f + i * 1.0f;
            float y = (0.25f + i * 0.5f) / 2.0f;
            float h = 0.25f + i * 0.5f;
            g_debugRenderer.DrawWireBox(x, y, 8.0f, 1.0f, h, 2.0f, 0.6f, 0.4f, 0.2f);
        }
        // Landing platform
        g_debugRenderer.DrawCube(16.5f, 2.0f, 8.0f, 2.0f, 0.7f, 0.5f, 0.3f);

        // Steep staircase
        for (int i = 0; i < 5; i++) {
            float x = 8.0f + i * 0.8f;
            float y = (0.4f + i * 0.8f) / 2.0f;
            float h = 0.4f + i * 0.8f;
            g_debugRenderer.DrawWireBox(x, y, 12.0f, 0.8f, h, 2.0f, 0.8f, 0.4f, 0.2f);
        }

        // ====================================================================
        // PLATFORMING AREA (Northeast)
        // ====================================================================
        g_debugRenderer.DrawCube(-8.0f, 0.5f, 8.0f, 2.0f, 0.3f, 0.6f, 0.3f);
        g_debugRenderer.DrawCube(-8.0f, 1.5f, 12.0f, 2.0f, 0.35f, 0.65f, 0.35f);
        g_debugRenderer.DrawCube(-8.0f, 2.5f, 16.0f, 2.0f, 0.4f, 0.7f, 0.4f);
        g_debugRenderer.DrawCube(-12.0f, 2.5f, 16.0f, 2.0f, 0.4f, 0.7f, 0.4f);
        g_debugRenderer.DrawCube(-12.0f, 3.5f, 12.0f, 2.0f, 0.45f, 0.75f, 0.45f);
        g_debugRenderer.DrawCube(-12.0f, 1.0f, 8.0f, 2.0f, 0.35f, 0.65f, 0.35f);

        // Gap jump platforms
        g_debugRenderer.DrawCube(-16.0f, 0.5f, 8.0f, 2.0f, 0.2f, 0.5f, 0.7f);
        g_debugRenderer.DrawCube(-16.0f, 0.5f, 12.0f, 2.0f, 0.2f, 0.5f, 0.7f);
        g_debugRenderer.DrawCube(-16.0f, 1.0f, 17.0f, 2.0f, 0.25f, 0.55f, 0.75f);

        // ====================================================================
        // HALLWAY/CORRIDOR AREA (West)
        // ====================================================================
        float corridorZ = -8.0f;
        // Walls
        g_debugRenderer.DrawWireBox(-15.0f, 1.5f, corridorZ, 20.0f, 3.0f, 0.5f, 0.5f, 0.5f, 0.6f);
        g_debugRenderer.DrawWireBox(-15.0f, 1.5f, corridorZ + 3.0f, 20.0f, 3.0f, 0.5f, 0.5f, 0.5f, 0.6f);
        g_debugRenderer.DrawWireBox(-25.5f, 1.5f, corridorZ + 1.5f, 0.5f, 3.0f, 3.5f, 0.5f, 0.5f, 0.6f);

        // Corridor obstacles
        g_debugRenderer.DrawCube(-10.0f, 0.5f, corridorZ + 1.5f, 1.0f, 0.7f, 0.3f, 0.3f);
        g_debugRenderer.DrawCube(-18.0f, 0.25f, corridorZ + 1.5f, 0.5f, 0.6f, 0.3f, 0.3f);

        // ====================================================================
        // ROOM AREA (Southwest)
        // ====================================================================
        float roomX = -20.0f;
        float roomZ = -20.0f;
        float roomW = 10.0f;
        float roomD = 10.0f;

        // Room walls
        g_debugRenderer.DrawWireBox(roomX, 1.5f, roomZ - roomD/2 + 0.25f, roomW, 3.0f, 0.5f, 0.6f, 0.6f, 0.7f);
        g_debugRenderer.DrawWireBox(roomX, 1.5f, roomZ + roomD/2 - 0.25f, roomW, 3.0f, 0.5f, 0.6f, 0.6f, 0.7f);
        g_debugRenderer.DrawWireBox(roomX - roomW/2 + 0.25f, 1.5f, roomZ, 0.5f, 3.0f, roomD - 1.0f, 0.6f, 0.6f, 0.7f);
        g_debugRenderer.DrawWireBox(roomX + roomW/2 - 0.25f, 1.5f, roomZ - 2.0f, 0.5f, 3.0f, roomD - 5.0f, 0.6f, 0.6f, 0.7f);

        // Room furniture
        g_debugRenderer.DrawCube(roomX - 3.0f, 0.5f, roomZ - 3.0f, 1.0f, 0.5f, 0.35f, 0.2f);
        g_debugRenderer.DrawCube(roomX + 2.0f, 0.25f, roomZ + 2.0f, 0.5f, 0.6f, 0.4f, 0.25f);
        g_debugRenderer.DrawWireBox(roomX - 2.0f, 0.4f, roomZ + 3.0f, 3.0f, 0.8f, 1.0f, 0.4f, 0.3f, 0.5f);

        // ====================================================================
        // RAMP AREA (South)
        // ====================================================================
        for (int i = 0; i < 16; i++) {
            float z = -15.0f - i * 0.5f;
            float h = 0.125f + i * 0.25f;
            float y = h / 2.0f;
            g_debugRenderer.DrawWireBox(5.0f, y, z, 4.0f, h, 0.5f, 0.4f, 0.6f, 0.4f);
        }
        // Platform at top
        g_debugRenderer.DrawCube(5.0f, 2.0f, -24.0f, 2.0f, 0.5f, 0.7f, 0.5f);

        // ====================================================================
        // VERTICAL CLIMBING AREA (Southeast corner)
        // ====================================================================
        g_debugRenderer.DrawCube(15.0f, 0.5f, -10.0f, 2.0f, 0.6f, 0.6f, 0.8f);
        g_debugRenderer.DrawCube(15.0f, 1.5f, -10.0f, 1.8f, 0.62f, 0.62f, 0.82f);
        g_debugRenderer.DrawCube(15.0f, 2.5f, -10.0f, 1.6f, 0.64f, 0.64f, 0.84f);
        g_debugRenderer.DrawCube(15.0f, 3.5f, -10.0f, 1.4f, 0.66f, 0.66f, 0.86f);
        g_debugRenderer.DrawCube(15.0f, 4.5f, -10.0f, 1.2f, 0.68f, 0.68f, 0.88f);

        // Pillar
        g_debugRenderer.DrawWireBox(18.0f, 2.5f, -10.0f, 1.0f, 5.0f, 1.0f, 0.7f, 0.5f, 0.7f);

        // ====================================================================
        // OBSTACLE COURSE (North)
        // ====================================================================
        g_debugRenderer.DrawWireBox(0.0f, 0.25f, 15.0f, 4.0f, 0.5f, 1.0f, 0.8f, 0.6f, 0.2f);
        g_debugRenderer.DrawWireBox(0.0f, 0.5f, 18.0f, 4.0f, 1.0f, 1.0f, 0.8f, 0.5f, 0.2f);
        g_debugRenderer.DrawWireBox(0.0f, 0.75f, 21.0f, 4.0f, 1.5f, 1.0f, 0.8f, 0.4f, 0.2f);

        // Pillars
        g_debugRenderer.DrawWireBox(5.0f, 1.5f, 15.0f, 1.0f, 3.0f, 1.0f, 0.5f, 0.5f, 0.5f);
        g_debugRenderer.DrawWireBox(7.0f, 1.5f, 18.0f, 1.0f, 3.0f, 1.0f, 0.5f, 0.5f, 0.5f);
        g_debugRenderer.DrawWireBox(5.0f, 1.5f, 21.0f, 1.0f, 3.0f, 1.0f, 0.5f, 0.5f, 0.5f);

        // ====================================================================
        // EDGE/CORNER TEST AREA
        // ====================================================================
        g_debugRenderer.DrawCube(8.0f, 0.5f, 0.0f, 1.0f, 0.9f, 0.3f, 0.3f);
        g_debugRenderer.DrawCube(9.0f, 0.5f, 1.0f, 1.0f, 0.9f, 0.3f, 0.3f);
        g_debugRenderer.DrawCube(10.0f, 0.5f, 0.0f, 1.0f, 0.9f, 0.3f, 0.3f);
        g_debugRenderer.DrawCube(9.0f, 0.5f, -1.0f, 1.0f, 0.9f, 0.3f, 0.3f);

        // L-shaped obstacle
        g_debugRenderer.DrawWireBox(12.0f, 0.5f, 3.0f, 3.0f, 1.0f, 1.0f, 0.3f, 0.9f, 0.3f);
        g_debugRenderer.DrawWireBox(13.5f, 0.5f, 5.0f, 1.0f, 1.0f, 3.0f, 0.3f, 0.9f, 0.3f);

        // Render player debug visualization
        g_player.Render(&g_debugRenderer);

        // Draw collision debug if enabled (F1 to toggle)
        if (g_showCollisionDebug) {
            DrawCollisionDebug();
        }

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

