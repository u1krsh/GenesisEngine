#pragma once

#include "core/Time.h"
#include "core/Logger.h"
#include "input/InputManager.h"
#include "renderer/shader/Shader.h"
#include "camera/Camera.h"

#include <string>
#include <functional>

struct GLFWwindow;

namespace Genesis {

// ============================================================================
// Engine Configuration
// ============================================================================
struct EngineConfig {
    std::string windowTitle = "Genesis Engine";
    int windowWidth = 1920;
    int windowHeight = 1080;
    bool vsync = false;
    bool fullscreen = true;   // Default to fullscreen
    double fixedTimestep = 1.0 / 66.0;  // 66 Hz physics
    int maxFrameSkip = 5;
};

// ============================================================================
// Engine - Main engine class that manages the game loop
// ============================================================================
class Engine {
public:
    static Engine& Instance() {
        static Engine instance;
        return instance;
    }

    // ========================================================================
    // Lifecycle
    // ========================================================================

    bool Initialize(const EngineConfig& config = EngineConfig());
    void Shutdown();
    void Run();
    void Stop() { m_running = false; }

    // ========================================================================
    // Callbacks - Set these to hook into the engine loop
    // ========================================================================

    using UpdateCallback = std::function<void(double deltaTime)>;
    using RenderCallback = std::function<void(double interpolation)>;
    using InitCallback = std::function<bool()>;
    using ShutdownCallback = std::function<void()>;
    using InputCallback = std::function<void(double deltaTime)>;

    void SetOnInit(InitCallback callback) { m_onInit = callback; }
    void SetOnShutdown(ShutdownCallback callback) { m_onShutdown = callback; }
    void SetOnUpdate(UpdateCallback callback) { m_onUpdate = callback; }
    void SetOnRender(RenderCallback callback) { m_onRender = callback; }
    void SetOnInput(InputCallback callback) { m_onInput = callback; }  // Called once per frame

    // ========================================================================
    // Accessors
    // ========================================================================

    GLFWwindow* GetWindow() { return m_window; }
    const EngineConfig& GetConfig() const { return m_config; }
    bool IsRunning() const { return m_running; }

    int GetScreenWidth() const { return m_screenWidth; }
    int GetScreenHeight() const { return m_screenHeight; }

    FPSCamera& GetCamera() { return m_camera; }
    const FPSCamera& GetCamera() const { return m_camera; }

private:
    Engine() = default;
    ~Engine() = default;
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool InitializeWindow();
    bool InitializeGraphics();
    bool InitializeInput();
    bool InitializeShaders();
    bool InitializeGUI();

    void ProcessInput();
    void Update(double deltaTime);
    void Render(double interpolation);
    void RenderGUI();

    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void CharCallback(GLFWwindow* window, unsigned int codepoint);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);

private:
    GLFWwindow* m_window = nullptr;
    EngineConfig m_config;
    bool m_running = false;
    bool m_initialized = false;

    int m_screenWidth = 1920;
    int m_screenHeight = 1080;

    // Core systems
    FPSCamera m_camera;

    // Callbacks
    InitCallback m_onInit;
    ShutdownCallback m_onShutdown;
    UpdateCallback m_onUpdate;
    RenderCallback m_onRender;
    InputCallback m_onInput;

    // Fixed timestep accumulator
    double m_accumulator = 0.0;
};

} // namespace Genesis

