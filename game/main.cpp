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
#include "bsp/BSPRenderer.h"
#include "bsp/BSPTree.h"
#include "bsp/BSPBuildVisualizer.h"
#include "bsp/LightBaker.h"
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
static bool g_showPVSDebug = false;       // F6 - Show PVS visualization
static bool g_useBSPRendering = false;  // Standard rendering by default (faster), press B to toggle BSP

// Debug visualization meshes (grid, axes)
static MeshPtr g_gridMesh;
static MeshPtr g_axesMesh;
static MaterialPtr g_matDebug;

// ============================================================================
// Setup World Collision Geometry - Now handled by MapRenderer
// ============================================================================
// Forward declarations
void LoadAndCompileMap();

// The world collision is now set up automatically when loading a map via MapRenderer.
// This function is kept as a fallback for when no map is loaded.
void SetupWorldCollision() {
    auto& world = WorldCollision::Instance();
    world.Clear();
    world.SetFloorHeight(0.0f);
    LOG_INFO("Game", "World collision cleared - map will provide geometry");
}

// ============================================================================
// Internal Helper: Load Map & Recompile
// ============================================================================
void LoadAndCompileMap(bool openConsole) {
    auto& mapRenderer = MapRenderer::Instance();
    auto& bspRenderer = BSPRenderer::Instance();
    auto& staticWorld = StaticWorldRenderer::Instance();
    
    // Clear previous state
    bspRenderer.SetRenderingActive(false);
    staticWorld.Clear();
    
    // Set "Moody" base lighting to allow point lights to shine
    staticWorld.SetAmbientLight(Vec3(0.02f, 0.02f, 0.05f), 1.0f);
    staticWorld.SetDirectionalLight(Vec3(0.5f, 1.0f, 0.3f), Vec3(0.1f, 0.1f, 0.15f), 0.2f);
    
    // Load the test map - SAU first (has lights from editor), then JSON
    bool mapLoaded = false;
#ifdef ASSETS_DIR
    std::string mapPath = std::string(ASSETS_DIR) + "/maps/moody_demo.sau";
    LOG_INFO("Game", "Attempting to load: " + mapPath);
    mapLoaded = mapRenderer.LoadMap(mapPath);
#else
    LOG_INFO("Game", "Attempting to load: moody_demo.sau (Relative)");
    mapLoaded = mapRenderer.LoadMap("moody_demo.sau");
#endif

    if (!mapLoaded) {
        if (!mapRenderer.LoadMap("moody_demo.sau") && !mapRenderer.LoadMap("moody_demo.json")) {
            if (!mapRenderer.LoadMap("large_culling_demo.json")) {
                LOG_WARNING("Game", "Failed to load large_culling_demo.json, trying bsp_demo.json...");
                if (!mapRenderer.LoadMap("bsp_demo.json")) {
                    LOG_WARNING("Game", "Failed to load bsp_demo.json, trying testmap.json...");
                    if (!mapRenderer.LoadMap("testmap.json")) {
                        LOG_ERROR("Game", "Failed to load any map file!");
                        // Fall back to a simple floor
                        staticWorld.Clear();
                        auto groundPlane = MeshPrimitives::CreatePlane(60.0f, 60.0f, 30, 30, "GroundPlane");
                        auto matFloor = MaterialLibrary::Instance().CreateSolidColor("Floor", Vec3(0.15f, 0.15f, 0.18f));
                        staticWorld.AddFloor(groundPlane, matFloor, glm::translate(Mat4(1.0f), Vec3(0.0f, 0.0f, 0.0f)));
                        staticWorld.SetDirectionalLight(Vec3(0.5f, 1.0f, 0.3f), Vec3(1.0f, 0.98f, 0.95f), 1.0f);
                        staticWorld.SetAmbientLight(Vec3(0.15f, 0.15f, 0.2f), 1.0f);
                        staticWorld.RebuildBatches();
                        SetupWorldCollision();
                        return;
                    }
                }
            }
        }
    }

    LOG_INFO("Game", "Map loaded with " + std::to_string(mapRenderer.GetBrushCount()) + " brushes");
    
    // Always print success message if map loaded (verified by HasMap)
    if (mapRenderer.HasMap()) {
        std::string mapName = mapRenderer.GetActiveMap()->GetName();
        std::string msg = "LOADED MAP: [" + mapName + "]";
        
        // 1. In-game console (Cyan-Green)
        GUI::Console::Instance().PrintSuccess(msg);
        if (openConsole) {
            GUI::Console::Instance().Open(); // Force open so user sees it
        }
        
        // 2. Standard Output (ANSI Cyan)
        std::cout << "\033[36m" << "CONSOLE: " << msg << "\033[0m" << std::endl;
    } else {
        LOG_ERROR("Game", "Map load failed or no map active");
    }

    // ========================================================================
    // BSP Compilation - Compile map to BSP tree for alternative rendering
    // ========================================================================
    if (mapRenderer.HasMap()) {
        LOG_INFO("Game", "Compiling map to BSP...");
        bspRenderer.InitializeShaders();

        if (bspRenderer.CompileMap(mapRenderer.GetActiveMap())) {
            LOG_INFO("Game", "BSP compilation successful! Map: " + mapRenderer.GetActiveMap()->GetName() + 
                     ", Brushes: " + std::to_string(mapRenderer.GetBrushCount()));
            
            // ================================================================
            // Phase 4: Add Map Lights and Bake
            // ================================================================
            auto bsp = bspRenderer.GetBSP();
            if (bsp) {
                // Clear any existing lights
                bsp->ClearLights();
                
                // NOTE: No automatic directional light - lighting is entirely from map entities
                
                // 2. Add lights from Map Entities
                const auto& entities = mapRenderer.GetActiveMap()->GetEntities();
                int mapLightsAdded = 0;
                
                for (const auto& ent : entities) {
                    if (ent.classname == "light") {
                        // Position
                        Vec3 pos = ent.position;
                        
                        // Color
                        Vec3 color(1.0f);
                        if (ent.properties.find("color") != ent.properties.end()) {
                            std::stringstream ss(ent.properties.at("color"));
                            float r, g, b;
                            ss >> r >> g >> b;
                            if (r > 1.0f || g > 1.0f || b > 1.0f) {
                                color = Vec3(r/255.0f, g/255.0f, b/255.0f);
                            } else {
                                color = Vec3(r, g, b);
                            }
                        }
                        
                        // Intensity
                        float intensity = 300.0f;
                        if (ent.properties.find("intensity") != ent.properties.end()) {
                            try { intensity = std::stof(ent.properties.at("intensity")); } catch (...) {}
                        }
                        float bspIntensity = intensity / 200.0f;
                        
                        // Radius
                        float radius = 30.0f;
                        if (ent.properties.find("radius") != ent.properties.end()) {
                            try { radius = std::stof(ent.properties.at("radius")); } catch (...) {}
                        }
                        
                        bsp->AddLight(StaticLight::CreatePoint(pos, color, bspIntensity, radius));
                        mapLightsAdded++;
                    }
                }
                LOG_INFO("Game", "Added " + std::to_string(mapLightsAdded) + " lights from map entities");
                
                // Bake the lighting
                LOG_INFO("Game", "Baking lightmaps (this may take a moment)...");
                LightBaker baker;
                LightBaker::Options bakeOptions;
                bakeOptions.texelsPerUnit = 2.0f;       
                bakeOptions.maxLightmapSize = 32;       
                bakeOptions.minLightmapSize = 4;
                bakeOptions.ambientLight = 0.01f;  // Very low ambient for moody lighting       
                bakeOptions.shadowBias = 0.05f;        
                bakeOptions.numSamples = 1;            
                bakeOptions.verbose = true;
                
                baker.BakeWithSceneLights(*bsp, bakeOptions);
                
                auto stats = baker.GetLastStats();
                LOG_INFO("Game", "Lightmap baking complete! " + 
                         std::to_string(stats.numTexels) + " texels, " +
                         std::to_string(stats.numShadowRays) + " shadow rays, " +
                         std::to_string(stats.bakeTimeSeconds) + "s");
            }

            // Enable BSP rendering by default
            g_useBSPRendering = true;
            bspRenderer.SetRenderingActive(true);
        } else {
            LOG_ERROR("Game", "BSP compilation failed");
            // Fallback to static mesh rendering
            auto& map = *mapRenderer.GetActiveMap();
            staticWorld.Clear();
            for (const auto& brush : map.GetBrushes()) {
                if (!HasFlag(brush.flags, BrushFlags::NoRender)) {
                    staticWorld.Add(brush.mesh, brush.material, brush.transform);
                }
            }
            staticWorld.RebuildBatches();
            SetupWorldCollision();
        }
    }

    // ========================================================================
    // Player Spawn Reset
    // ========================================================================
    Vec3 spawnPos(0, 5, 0);  // Default
    float spawnYaw = 0.0f;
    
    if (mapRenderer.HasMap()) {
        const auto& meta = mapRenderer.GetActiveMap()->GetMetadata();
        spawnPos = meta.spawnPosition;
        
        // Also check for info_player_start entity
        for (const auto& ent : mapRenderer.GetActiveMap()->GetEntities()) {
            if (ent.classname == "info_player_start") {
                spawnPos = ent.position;
                spawnYaw = ent.rotation.y;
                break;
            }
        }
        
        LOG_INFO("Game", "Player spawn from map: " + std::to_string(spawnPos.x) + ", " + 
                 std::to_string(spawnPos.y) + ", " + std::to_string(spawnPos.z));
    }

    // Reset player state
    g_player.SetPosition(spawnPos);
    g_player.GetController().SetLookDirection(spawnYaw, 0.0f);
    g_player.GetController().SetVelocity(Vec3(0.0f));
}

// ============================================================================
// Game Initialization
// ============================================================================
bool OnInit() {
    LOG_INFO("Game", "Initializing game...");

    // Initialize GUI
    if (!GUI::GUIRenderer::Instance().Initialize()) {
        LOG_ERROR("Game", "Failed to initialize GUI Renderer");
        return false;
    }
    GUI::Console::Instance().Initialize();

    // Load shaders
    auto& shaderLib = ShaderLibrary::Instance();
#ifdef ASSETS_DIR
    std::string shaderPath = std::string(ASSETS_DIR) + "/shaders/";
    shaderLib.SetShaderBasePath(shaderPath);
#else
    shaderLib.SetShaderBasePath("assets/shaders/");
#endif
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
    matLib.CreateSolidColor("stone", Vec3(0.2f, 0.2f, 0.22f));  // Dark stone
    matLib.CreateSolidColor("concrete", Vec3(0.5f, 0.5f, 0.52f));
    matLib.CreateSolidColor("brick", Vec3(0.55f, 0.3f, 0.25f)); // Reddish brick
    matLib.CreateSolidColor("metal", Vec3(0.5f, 0.5f, 0.55f));
    matLib.CreateSolidColor("wood", Vec3(0.45f, 0.3f, 0.2f));

    LOG_INFO("Game", "Materials created!");


    // ========================================================================
    // Player Setup
    // ========================================================================
    // Configure and initialize player
    Game::PlayerConfig playerConfig;

    // Load Default Map (moody_demo.sau via LoadAndCompileMap)
    LoadAndCompileMap(false); // Don't force open console on startup

    // Get spawn position from map, or use default
    auto& mapRenderer = MapRenderer::Instance();
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

    // ========================================================================
    // BSP Collision Setup (Phase 2) - Connect BSP collision to player
    // ========================================================================
    if (BSPRenderer::Instance().HasBSP()) {
        LOG_INFO("Game", "Setting up BSP collision for player...");
        
        // Get pointer to BSP tree (it lives as long as BSPRenderer)
        auto bspTree = BSPRenderer::Instance().GetBSP();
        
        // Set BSP trace callback
        controller.SetBSPTraceCallback([bspTree](const Vec3& start, const Vec3& end, 
                                                   float radius, float halfHeight) {
            return bspTree->TraceCapsule(start, end, radius, halfHeight);
        });
        
        // Set BSP slide move callback
        controller.SetBSPSlideMoveCallback([bspTree](const Vec3& start, const Vec3& velocity, float deltaTime,
                                                       float radius, float halfHeight, Vec3& outVelocity) {
            return bspTree->SlideMove(start, velocity, deltaTime, radius, halfHeight, outVelocity);
        });
        
        // Enable BSP collision by default if BSP is compiled
        controller.SetUseBSPCollision(true);
        LOG_INFO("Game", "BSP collision ENABLED for player");
    }

    LOG_INFO("Game", "Game initialized successfully");
    LOG_INFO("Game", "Controls: WASD=Move, Mouse=Look, Shift=Sprint, Space=Jump, Ctrl=Crouch");
    LOG_INFO("Game", "          Left Click=Capture Mouse, Right Click=Release, ESC=Quit");
    LOG_INFO("Game", "          F1=Collision Debug, F2=Console, F3=Debug Overlay, F4=BSP Toggle");

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
    auto& staticWorld = StaticWorldRenderer::Instance();
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
// Draw Lights Debug Visualization
// Shows static lights as colored spheres with radius indicators
// ============================================================================
void DrawLightsDebug() {
    auto& bspRenderer = BSPRenderer::Instance();
    if (!bspRenderer.HasBSP()) return;
    
    auto bsp = bspRenderer.GetBSP();
    if (!bsp) return;
    
    const auto& lights = bsp->GetLights();
    
    for (const auto& light : lights) {
        if (light.type == StaticLightType::Point) {
            // Draw point light as a colored sphere
            Vec3 pos = light.position;
            Vec3 col = light.color;
            
            // Inner sphere (bright, shows position)
            g_debugRenderer.DrawWireSphere(pos.x, pos.y, pos.z, 0.3f, col.r, col.g, col.b);
            
            // Outer sphere (dimmer, shows radius)
            g_debugRenderer.DrawWireSphere(pos.x, pos.y, pos.z, light.radius, 
                                            col.r * 0.3f, col.g * 0.3f, col.b * 0.3f);
            
            // Vertical line to floor (helps see light position)
            // Approximate with a thin box
            g_debugRenderer.DrawWireBox(pos.x, pos.y - light.radius * 0.5f, pos.z,
                                         0.1f, light.radius, 0.1f,
                                         col.r * 0.5f, col.g * 0.5f, col.b * 0.5f);
        }
        else if (light.type == StaticLightType::Directional) {
            // Draw directional light as an arrow in the sky
            Vec3 dir = light.direction;
            Vec3 col = light.color;
            
            // Draw arrow at multiple positions across the map to show direction
            for (float x = -50; x <= 50; x += 25.0f) {
                for (float z = -50; z <= 50; z += 25.0f) {
                    Vec3 start(x, 20.0f, z);
                    Vec3 end = start + dir * 5.0f;
                    
                    // Arrow line (approximated with thin box)
                    Vec3 mid = (start + end) * 0.5f;
                    Vec3 size = glm::abs(end - start) + Vec3(0.1f);
                    g_debugRenderer.DrawWireBox(mid.x, mid.y, mid.z,
                                                 size.x, size.y, size.z,
                                                 col.r, col.g, col.b);
                }
            }
        }
    }
}

// ============================================================================
// Draw PVS Debug Visualization (Top-Down View)
// Shows which leafs are visible from camera position
// ============================================================================
void DrawPVSDebug() {
    auto& bspRenderer = BSPRenderer::Instance();
    if (!bspRenderer.HasBSP() || !bspRenderer.HasPVS()) return;
    
    auto bsp = bspRenderer.GetBSP();
    if (!bsp) return;
    
    const auto& leafs = bsp->GetLeafs();
    const auto& pvs = bsp->GetPVS();
    
    // Find which leaf the camera is in
    Vec3 camPos = g_player.GetController().GetPosition();
    int32_t cameraLeaf = bsp->FindLeaf(camPos);
    
    // Get visible leafs from PVS
    std::vector<bool> isVisible(leafs.size(), false);
    if (cameraLeaf >= 0 && pvs.IsBuilt()) {
        const auto& visibleLeafs = pvs.GetVisibleLeafs(static_cast<uint32_t>(cameraLeaf));
        for (uint32_t leafIdx : visibleLeafs) {
            if (leafIdx < isVisible.size()) {
                isVisible[leafIdx] = true;
            }
        }
    }
    
    // Draw each leaf bounds (projected to XZ plane at Y=0.1 for top-down view)
    float drawY = 0.1f;  // Slightly above ground to be visible
    
    for (size_t i = 0; i < leafs.size(); ++i) {
        const auto& leaf = leafs[i];
        
        // Skip leafs with no geometry
        if (leaf.numFaces == 0) continue;
        
        // Calculate center and size
        Vec3 center = (leaf.boundsMin + leaf.boundsMax) * 0.5f;
        Vec3 size = leaf.boundsMax - leaf.boundsMin;
        
        // Color based on visibility
        float r, g, b;
        if (static_cast<int32_t>(i) == cameraLeaf) {
            // Current leaf - bright green
            r = 0.0f; g = 1.0f; b = 0.0f;
        } else if (isVisible[i]) {
            // Visible leaf - cyan/blue
            r = 0.0f; g = 0.8f; b = 1.0f;
        } else {
            // Culled leaf - red
            r = 1.0f; g = 0.2f; b = 0.2f;
        }
        
        // Draw flat box at ground level (top-down view representation)
        g_debugRenderer.DrawWireBox(center.x, drawY, center.z,
                                     size.x, 0.05f, size.z,
                                     r, g, b);
        
        // Also draw the full 3D bounds with lower opacity (thinner lines)
        g_debugRenderer.DrawWireBox(center.x, center.y, center.z,
                                     size.x, size.y, size.z,
                                     r * 0.5f, g * 0.5f, b * 0.5f);
    }
    
    // Draw camera position marker
    g_debugRenderer.DrawWireBox(camPos.x, drawY, camPos.z,
                                 0.5f, 0.1f, 0.5f,
                                 1.0f, 1.0f, 0.0f);  // Yellow marker
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

    // F1 - Toggle debug overlay (text panel with stats)
    if (input.IsKeyPressed(KeyCode::F1)) {
        GUI::DebugOverlay::Instance().Toggle();
        LOG_INFO("Debug", GUI::DebugOverlay::Instance().IsVisible() ? "Debug overlay ON" : "Debug overlay OFF");
    }

    // F2 - Toggle console (alternative to ~ key)
    if (input.IsKeyPressed(KeyCode::F2)) {
        GUI::Console::Instance().Toggle();
        LOG_INFO("Debug", "Console toggled via F2");
    }

    // F3 - Toggle collision debug lines (3D wireframe boxes)
    if (input.IsKeyPressed(KeyCode::F3)) {
        g_showCollisionDebug = !g_showCollisionDebug;
        LOG_INFO("Debug", g_showCollisionDebug ? "Collision lines ON" : "Collision lines OFF");
    }

    // F4 - Toggle BSP rendering mode
    if (input.IsKeyPressed(KeyCode::F4)) {
        g_useBSPRendering = !g_useBSPRendering;
        if (g_useBSPRendering && !BSPRenderer::Instance().HasBSP()) {
            g_useBSPRendering = false;
            BSPRenderer::Instance().SetRenderingActive(false);
            LOG_WARNING("Debug", "No BSP compiled, cannot enable BSP rendering");
        } else {
            BSPRenderer::Instance().SetRenderingActive(g_useBSPRendering);
            LOG_INFO("Debug", g_useBSPRendering ? "BSP rendering ON (F4)" : "Standard rendering ON (F4)");
        }
    }

    // F5 - Hot-reload map (re-compile BSP and re-bake lighting)
    if (input.IsKeyPressed(KeyCode::F5)) {
        GUI::Console::Instance().Print("Reloading map...", GUI::MessageType::Warning);
        LoadAndCompileMap(false); // Don't force open console
    }

    // F8 - Toggle PVS (Potentially Visible Set) when in BSP mode
    if (input.IsKeyPressed(KeyCode::F8)) {
        if (BSPRenderer::Instance().HasPVS()) {
            bool usePVS = !BSPRenderer::Instance().GetUsePVS();
            BSPRenderer::Instance().SetUsePVS(usePVS);
            LOG_INFO("Debug", usePVS ? "PVS culling ON (F8)" : "PVS culling OFF (F8)");
        } else {
            LOG_WARNING("Debug", "No PVS available");
        }
    }

    // F6 - Toggle PVS debug visualization (shows visible/culled leafs)
    if (input.IsKeyPressed(KeyCode::F6)) {
        g_showPVSDebug = !g_showPVSDebug;
        LOG_INFO("Debug", g_showPVSDebug ? "PVS visualization ON (F6) - Green=current, Cyan=visible, Red=culled" 
                                         : "PVS visualization OFF (F6)");
    }

    // F7 - Play/restart BSP build visualization
    if (input.IsKeyPressed(KeyCode::F7)) {
        auto& visualizer = BSPBuildVisualizer::Instance();
        if (visualizer.GetTotalSteps() > 0) {
            visualizer.TogglePlayback();
            LOG_INFO("Debug", std::string("BSP build visualization ") + 
                             (visualizer.IsPlaying() ? "PLAYING" : "PAUSED") + 
                             " (" + std::to_string(visualizer.GetTotalSteps()) + " steps)");
        } else {
            LOG_WARNING("Debug", "No BSP build steps recorded");
        }
    }
}

// ============================================================================
// Game Update (Fixed Timestep)
// ============================================================================
void OnUpdate(double deltaTime) {
    // Update player with deltaTime (movement, physics)
    g_player.Update(deltaTime);

    // Update BSP build visualizer playback
    BSPBuildVisualizer::Instance().Update(static_cast<float>(deltaTime));

    // Update Console
    GUI::Console::Instance().Update(static_cast<float>(deltaTime));
}

// ============================================================================
// Game Render (Variable Framerate)
// ============================================================================
void OnRender(double interpolation) {
    auto& engine = Engine::Instance();
    auto& camera = engine.GetCamera();

    // ========================================================================
    // Render World - Either via BSP or StaticWorldRenderer
    // ========================================================================
    if (g_useBSPRendering && BSPRenderer::Instance().HasBSP()) {
        // BSP Tree Rendering - traverses tree using DrawNode(rootNode)
        BSPRenderer::Instance().Render(camera);
    } else {
        // Standard Rendering - iterates over all meshes
        auto& staticWorld = StaticWorldRenderer::Instance();
        staticWorld.Render(camera);
    }

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
            DrawLightsDebug();  // Also show lights in collision debug view
        }

        // F6 - Draw PVS debug visualization (visible/culled leafs)
        if (g_showPVSDebug) {
            DrawPVSDebug();
        }

        g_debugRenderer.RenderTriangles();
        g_debugRenderer.RenderLines();
        g_debugRenderer.EndFrame();

        g_debugShader->Unbind();
    }

    // ========================================================================
    // GUI Rendering
    // ========================================================================
    int width = engine.GetScreenWidth();
    int height = engine.GetScreenHeight();
    
    GUI::GUIRenderer::Instance().BeginFrame(width, height);
    GUI::Console::Instance().Render(width, height);
    GUI::GUIRenderer::Instance().EndFrame();
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

