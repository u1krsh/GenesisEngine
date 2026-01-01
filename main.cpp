#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>

#include "input/Input.h"
#include "camera/Camera.h"
#include "renderer/shader/Shader.h"

// ============================================================================
// Constants
// ============================================================================
constexpr double TICK_RATE = 1.0 / 66.0;  // 66 ticks per second
constexpr int MAX_FRAMESKIP = 5;           // Max updates per frame to prevent spiral of death
constexpr float HOT_RELOAD_INTERVAL = 1.0f; // Check for shader changes every second

// ============================================================================
// Global state (will be refactored into classes later)
// ============================================================================
GLFWwindow* g_window = nullptr;
Genesis::FPSCamera g_camera;
std::shared_ptr<Genesis::Shader> g_basicShader;
float g_hotReloadTimer = 0.0f;

// ============================================================================
// Callbacks
// ============================================================================
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (height > 0) {
        g_camera.SetAspectRatio(static_cast<float>(width) / static_cast<float>(height));
    }
}

// ============================================================================
// Update - Fixed timestep logic (physics, entities, game state)
// ============================================================================
void Update(double deltaTime) {
    auto& input = Genesis::InputManager::Instance();

    // Handle pause/quit
    if (input.IsActionPressed(Genesis::GameAction::Pause)) {
        glfwSetWindowShouldClose(g_window, true);
    }

    // Camera mouse look (only when cursor is locked)
    if (input.IsCursorLocked()) {
        double dx, dy;
        input.GetMouseDelta(dx, dy);
        if (dx != 0.0 || dy != 0.0) {
            g_camera.ProcessMouseLook(static_cast<float>(dx), static_cast<float>(dy));
        }
    }

    // Camera movement
    bool forward = input.IsActionDown(Genesis::GameAction::MoveForward);
    bool backward = input.IsActionDown(Genesis::GameAction::MoveBackward);
    bool left = input.IsActionDown(Genesis::GameAction::MoveLeft);
    bool right = input.IsActionDown(Genesis::GameAction::MoveRight);
    bool up = input.IsActionDown(Genesis::GameAction::Jump);
    bool down = input.IsActionDown(Genesis::GameAction::Crouch);

    // Sprint
    g_camera.SetSprinting(input.IsActionDown(Genesis::GameAction::Sprint));

    // Process movement
    g_camera.ProcessMovement(forward, backward, left, right, up, down, static_cast<float>(deltaTime));

    // Update camera
    g_camera.Update();

    // TODO: Update physics
    // TODO: Update entities
    // TODO: Update game logic
}

// ============================================================================
// Render - Variable framerate rendering
// ============================================================================
void Render(double interpolation) {
    // interpolation: 0.0 to 1.0, represents how far we are between ticks
    // Use this for smooth rendering between physics states

    // Clear with a nice color
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use the basic shader
    if (g_basicShader && g_basicShader->IsValid()) {
        g_basicShader->Bind();

        // Set camera matrices
        g_basicShader->SetMat4("u_View", g_camera.GetViewMatrix());
        g_basicShader->SetMat4("u_Proj", g_camera.GetProjectionMatrix());

        // Set model matrix (identity for now)
        g_basicShader->SetMat4("u_Model", Genesis::Mat4::Identity());

        // Set color
        g_basicShader->SetVec3("u_Color", 1.0f, 0.5f, 0.2f);

        // TODO: Draw meshes here

        g_basicShader->Unbind();
    }

    // TODO: Render world
    // TODO: Render entities (use interpolation for smooth movement)
    // TODO: Render UI

    glfwSwapBuffers(g_window);
}

// ============================================================================
// Main Entry Point
// ============================================================================
int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW for OpenGL 3.3 Core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    g_window = glfwCreateWindow(800, 600, "Genesis Engine", nullptr, nullptr);
    if (!g_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(g_window);
    glfwSetFramebufferSizeCallback(g_window, framebuffer_size_callback);

    // Disable VSync for uncapped framerate (optional - comment out to enable VSync)
    glfwSwapInterval(0);

    // Load OpenGL functions with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // ========================================================================
    // Initialize Input System
    // ========================================================================
    auto& input = Genesis::InputManager::Instance();
    input.SetBackend(std::make_unique<Genesis::GLFWInputBackend>());
    if (!input.Initialize(g_window)) {
        std::cerr << "Failed to initialize input system" << std::endl;
        return -1;
    }
    input.SetupDefaultBindings();

    // ========================================================================
    // Initialize Camera
    // ========================================================================
    g_camera.SetPosition(0.0f, 2.0f, 5.0f);  // Start position
    g_camera.SetMoveSpeed(5.0f);              // Units per second
    g_camera.SetSprintMultiplier(2.0f);       // 2x speed when sprinting
    g_camera.SetMouseSensitivity(0.1f);       // Degrees per pixel
    g_camera.SetFOV(70.0f);                   // Field of view
    g_camera.SetAspectRatio(800.0f / 600.0f); // Initial aspect ratio
    g_camera.SetPitchConstraints(-89.0f, 89.0f); // Clamp pitch

    // ========================================================================
    // Initialize Shader System
    // ========================================================================
    auto& shaderLib = Genesis::ShaderLibrary::Instance();
    shaderLib.SetShaderBasePath("../assets/shaders/");  // Relative to build directory
    shaderLib.SetHotReloadEnabled(true);

    // Load basic shader
    g_basicShader = shaderLib.Load("basic", "basic.vert", "basic.frag");
    if (!g_basicShader) {
        std::cerr << "Failed to load basic shader!" << std::endl;
        return -1;
    }
    std::cout << "[Main] Basic shader loaded with ID: " << g_basicShader->GetProgramId() << std::endl;

    // Also load grid shader for later use
    auto gridShader = shaderLib.Load("grid", "grid.vert", "grid.frag");

    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "Genesis Engine started successfully!" << std::endl;
    std::cout << "Tick Rate: 66 ticks/sec (fixed)" << std::endl;
    std::cout << "Render: Variable framerate" << std::endl;
    std::cout << "Input System: Initialized with GLFW backend" << std::endl;
    std::cout << "Camera: FPS Camera initialized" << std::endl;
    std::cout << "Shader System: Initialized with hot reload" << std::endl;
    std::cout << "Controls: WASD=Move, Mouse=Look, Shift=Sprint, Space=Up, Ctrl=Down" << std::endl;
    std::cout << "          Left Click=Capture Mouse, Right Click=Release Mouse, ESC=Quit" << std::endl;
    std::cout.flush();

    // ========================================================================
    // Game Loop - Fixed timestep with variable rendering
    // ========================================================================
    double accumulator = 0.0;
    double lastTime = glfwGetTime();

    // Performance tracking
    double fpsTimer = 0.0;
    int frameCount = 0;
    int tickCount = 0;

    while (!glfwWindowShouldClose(g_window)) {
        // Calculate frame time
        double currentTime = glfwGetTime();
        double frameTime = currentTime - lastTime;
        lastTime = currentTime;

        // Clamp frame time to prevent spiral of death
        if (frameTime > MAX_FRAMESKIP * TICK_RATE) {
            frameTime = MAX_FRAMESKIP * TICK_RATE;
        }

        accumulator += frameTime;

        // Fixed timestep updates
        int updatesThisFrame = 0;
        while (accumulator >= TICK_RATE && updatesThisFrame < MAX_FRAMESKIP) {
            Update(TICK_RATE);
            accumulator -= TICK_RATE;
            tickCount++;
            updatesThisFrame++;
        }

        // Calculate interpolation for smooth rendering
        double interpolation = accumulator / TICK_RATE;

        // Variable framerate rendering
        Render(interpolation);
        frameCount++;

        // Poll events and update input
        glfwPollEvents();
        input.Update();

        // Shader hot reload check
        g_hotReloadTimer += static_cast<float>(frameTime);
        if (g_hotReloadTimer >= HOT_RELOAD_INTERVAL) {
            shaderLib.CheckForReloads();
            g_hotReloadTimer = 0.0f;
        }

        // FPS/TPS display (every second)
        fpsTimer += frameTime;
        if (fpsTimer >= 1.0) {
            const auto& pos = g_camera.GetPosition();
            std::cout << "FPS: " << frameCount << " | TPS: " << tickCount
                      << " | Pos: (" << pos.x << ", " << pos.y << ", " << pos.z << ")"
                      << " | Yaw: " << g_camera.GetYaw() << " Pitch: " << g_camera.GetPitch()
                      << std::endl;
            frameCount = 0;
            tickCount = 0;
            fpsTimer = 0.0;
        }
    }

    // Cleanup
    shaderLib.Clear();
    glfwTerminate();
    return 0;
}