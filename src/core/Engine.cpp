#include "Engine.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "input/GLFWInputBackend.h"
#include "gui/GUIRenderer.h"
#include "gui/Console.h"
#include "gui/DebugOverlay.h"

namespace Genesis {

bool Engine::Initialize(const EngineConfig& config) {
    if (m_initialized) {
        LOG_WARNING("Engine", "Engine already initialized");
        return true;
    }

    m_config = config;
    LOG_INFO("Engine", "Initializing Genesis Engine...");

    // Initialize subsystems
    if (!InitializeWindow()) return false;
    if (!InitializeGraphics()) return false;
    if (!InitializeInput()) return false;
    if (!InitializeShaders()) return false;
    if (!InitializeGUI()) return false;

    // Initialize time
    Time::Instance().Initialize();
    Time::Instance().SetFixedDeltaTime(m_config.fixedTimestep);

    // Initialize camera
    m_camera.SetPosition(0.0f, 2.0f, 5.0f);
    m_camera.SetMoveSpeed(5.0f);
    m_camera.SetSprintMultiplier(2.0f);
    m_camera.SetMouseSensitivity(0.25f);
    m_camera.SetFOV(70.0f);
    m_camera.SetAspectRatio(static_cast<float>(m_screenWidth) /
                            static_cast<float>(m_screenHeight));
    m_camera.SetPitchConstraints(-89.0f, 89.0f);

    // Call user init callback
    if (m_onInit) {
        if (!m_onInit()) {
            LOG_ERROR("Engine", "User initialization failed");
            return false;
        }
    }

    m_initialized = true;
    LOG_INFO("Engine", "Genesis Engine initialized successfully");
    LOG_INFO("Engine", "Press ~ (tilde) to open console");

    // Lock cursor for gameplay on startup
    InputManager::Instance().SetCursorLocked(true);

    return true;
}

void Engine::Shutdown() {
    if (!m_initialized) return;

    LOG_INFO("Engine", "Shutting down Genesis Engine...");

    // Call user shutdown callback
    if (m_onShutdown) {
        m_onShutdown();
    }

    // Shutdown subsystems
    ShaderLibrary::Instance().Clear();
    InputManager::Instance().Shutdown();

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();

    m_initialized = false;
    LOG_INFO("Engine", "Genesis Engine shut down");
}

void Engine::Run() {
    if (!m_initialized) {
        LOG_ERROR("Engine", "Engine not initialized, cannot run");
        return;
    }

    m_running = true;
    m_accumulator = 0.0;

    LOG_INFO("Engine", "Starting game loop...");

    while (m_running && !glfwWindowShouldClose(m_window)) {
        // Update time
        Time::Instance().Update();
        double deltaTime = Time::Instance().GetDeltaTime();

        // Clamp frame time to prevent spiral of death
        if (deltaTime > m_config.maxFrameSkip * m_config.fixedTimestep) {
            deltaTime = m_config.maxFrameSkip * m_config.fixedTimestep;
        }

        // Poll events and update input FIRST
        glfwPollEvents();
        InputManager::Instance().Update();

        // Check if console is open - pause game when open
        bool consolePaused = GUI::Console::Instance().IsOpen();

        // Handle cursor visibility only on console state change
        static bool wasConsolePaused = false;
        if (consolePaused != wasConsolePaused) {
            auto& input = InputManager::Instance();
            if (consolePaused) {
                // Show cursor when console opens
                input.SetCursorLocked(false);
            } else {
                // Lock cursor when console closes
                input.SetCursorLocked(true);
            }
            wasConsolePaused = consolePaused;
        }

        // Process input (mouse look happens per-frame, but not when console is open)
        if (!consolePaused) {
            ProcessInput();
        }

        // Fixed timestep updates - SKIP when console is open (pause the game)
        if (!consolePaused) {
            m_accumulator += deltaTime;
            int updatesThisFrame = 0;
            while (m_accumulator >= m_config.fixedTimestep &&
                   updatesThisFrame < m_config.maxFrameSkip) {
                Update(m_config.fixedTimestep);
                m_accumulator -= m_config.fixedTimestep;
                updatesThisFrame++;
            }
        }

        // Calculate interpolation for smooth rendering
        double interpolation = m_accumulator / m_config.fixedTimestep;

        // Render (always render even when paused)
        Render(interpolation);

        // Swap buffers
        glfwSwapBuffers(m_window);

        // Check for shader hot reload
        static float hotReloadTimer = 0.0f;
        hotReloadTimer += static_cast<float>(deltaTime);
        if (hotReloadTimer >= 1.0f) {
            ShaderLibrary::Instance().CheckForReloads();
            hotReloadTimer = 0.0f;
        }
    }

    m_running = false;
    LOG_INFO("Engine", "Game loop ended");
}

bool Engine::InitializeWindow() {
    LOG_INFO("Engine", "Initializing window...");

    if (!glfwInit()) {
        LOG_FATAL("Engine", "Failed to initialize GLFW");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWmonitor* monitor = m_config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    m_window = glfwCreateWindow(m_config.windowWidth, m_config.windowHeight,
                                 m_config.windowTitle.c_str(), monitor, nullptr);

    if (!m_window) {
        LOG_FATAL("Engine", "Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetFramebufferSizeCallback(m_window, FramebufferSizeCallback);
    glfwSwapInterval(m_config.vsync ? 1 : 0);

    LOG_INFO("Engine", "Window created: " + std::to_string(m_config.windowWidth) +
             "x" + std::to_string(m_config.windowHeight));
    return true;
}

bool Engine::InitializeGraphics() {
    LOG_INFO("Engine", "Initializing graphics...");

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        LOG_FATAL("Engine", "Failed to initialize GLAD");
        return false;
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Enable line smoothing
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.5f);

    LOG_INFO("Engine", "OpenGL Version: " + std::string((char*)glGetString(GL_VERSION)));
    LOG_INFO("Engine", "Renderer: " + std::string((char*)glGetString(GL_RENDERER)));

    return true;
}

bool Engine::InitializeInput() {
    LOG_INFO("Engine", "Initializing input...");

    auto& input = InputManager::Instance();
    input.SetBackend(std::make_unique<GLFWInputBackend>());

    if (!input.Initialize(m_window)) {
        LOG_FATAL("Engine", "Failed to initialize input system");
        return false;
    }

    input.SetupDefaultBindings();
    LOG_INFO("Engine", "Input system initialized");
    return true;
}

bool Engine::InitializeShaders() {
    LOG_INFO("Engine", "Initializing shader system...");

    auto& shaderLib = ShaderLibrary::Instance();
    shaderLib.SetShaderBasePath("../assets/shaders/");
    shaderLib.SetHotReloadEnabled(true);

    LOG_INFO("Engine", "Shader system initialized");
    return true;
}

bool Engine::InitializeGUI() {
    LOG_INFO("Engine", "Initializing GUI system...");

    // Initialize GUI renderer
    if (!GUI::GUIRenderer::Instance().Initialize()) {
        LOG_ERROR("Engine", "Failed to initialize GUI renderer");
        return false;
    }

    // Initialize console
    GUI::Console::Instance().Initialize();

    // Hook up logger to console - forward log messages to in-game console
    Logger::Instance().SetConsoleCallback([](LogLevel level, const std::string& category, const std::string& message) {
        auto& console = GUI::Console::Instance();
        std::string formattedMsg = "[" + category + "] " + message;

        switch (level) {
            case LogLevel::Warning:
                console.PrintWarning(formattedMsg);
                break;
            case LogLevel::Error:
            case LogLevel::Fatal:
                console.PrintError(formattedMsg);
                break;
            default:
                console.Print(formattedMsg, GUI::MessageType::Normal);
                break;
        }
    });

    // Set up key callbacks for console
    glfwSetKeyCallback(m_window, KeyCallback);
    glfwSetCharCallback(m_window, CharCallback);
    glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
    glfwSetCursorPosCallback(m_window, CursorPosCallback);

    LOG_INFO("Engine", "GUI system initialized");
    return true;
}

void Engine::RenderGUI() {
    int width, height;
    glfwGetFramebufferSize(m_window, &width, &height);
    m_screenWidth = width;
    m_screenHeight = height;

    auto& guiRenderer = GUI::GUIRenderer::Instance();
    auto& console = GUI::Console::Instance();
    auto& debugOverlay = GUI::DebugOverlay::Instance();

    // Begin GUI rendering
    guiRenderer.BeginFrame(width, height);

    // Update and render console
    console.Update(Time::Instance().GetDeltaTime());
    console.Render(width, height);

    // Render debug overlay (controlled by ge_showinfo)
    debugOverlay.Render(width, height);

    // End GUI rendering
    guiRenderer.EndFrame();
}

void Engine::ProcessInput() {
    auto& input = InputManager::Instance();

    // Handle quit
    if (input.IsActionPressed(GameAction::Pause)) {
        Stop();
    }

    // Mouse look - always process when this function is called
    // (console state is checked before calling ProcessInput)
    double dx, dy;
    input.GetMouseDelta(dx, dy);
    if (dx != 0.0 || dy != 0.0) {
        m_camera.ProcessMouseLook(static_cast<float>(dx), static_cast<float>(dy));
    }
}

void Engine::Update(double deltaTime) {
    auto& input = InputManager::Instance();

    // Camera movement
    bool forward = input.IsActionDown(GameAction::MoveForward);
    bool backward = input.IsActionDown(GameAction::MoveBackward);
    bool left = input.IsActionDown(GameAction::MoveLeft);
    bool right = input.IsActionDown(GameAction::MoveRight);
    bool up = input.IsActionDown(GameAction::Jump);
    bool down = input.IsActionDown(GameAction::Crouch);

    m_camera.SetSprinting(input.IsActionDown(GameAction::Sprint));
    m_camera.ProcessMovement(forward, backward, left, right, up, down,
                             static_cast<float>(deltaTime));
    m_camera.Update();

    // Call user update callback
    if (m_onUpdate) {
        m_onUpdate(deltaTime);
    }
}

void Engine::Render(double interpolation) {
    // Clear
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Call user render callback
    if (m_onRender) {
        m_onRender(interpolation);
    }

    // Render GUI overlay (console, debug info, etc.)
    RenderGUI();
}

void Engine::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (height > 0) {
        Engine::Instance().m_camera.SetAspectRatio(
            static_cast<float>(width) / static_cast<float>(height));
    }
}

void Engine::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto& console = GUI::Console::Instance();
    auto& input = InputManager::Instance();

    // Always forward key events to the input backend first
    // This allows the input system to track key states
    auto* backend = dynamic_cast<GLFWInputBackend*>(input.GetBackend());
    if (backend) {
        // Manually update key state in backend
        backend->ForwardKeyEvent(key, action);
    }

    // Toggle console with tilde/grave key
    if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS) {
        console.Toggle();
        return;
    }

    // If console is open, send keys to it (don't process game input)
    if (console.IsOpen()) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            console.OnKeyPressed(key);
        }
    }
}

void Engine::CharCallback(GLFWwindow* window, unsigned int codepoint) {
    auto& console = GUI::Console::Instance();

    // Don't process the tilde character that opened the console
    if (codepoint == '`' || codepoint == '~') {
        return;
    }

    // If console is open, send characters to it
    if (console.IsOpen()) {
        console.OnCharInput(codepoint);
    }
}

void Engine::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    auto& console = GUI::Console::Instance();
    auto& input = InputManager::Instance();

    // Always forward to backend for input state tracking
    auto* backend = dynamic_cast<GLFWInputBackend*>(input.GetBackend());
    if (backend) {
        backend->ForwardMouseButtonEvent(button, action);
    }

    // If console is open, forward mouse events to it
    if (console.IsOpen()) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        if (action == GLFW_PRESS) {
            console.OnMouseDown(static_cast<float>(xpos), static_cast<float>(ypos), button);
        } else if (action == GLFW_RELEASE) {
            console.OnMouseUp(static_cast<float>(xpos), static_cast<float>(ypos), button);
        }
    }
}

void Engine::CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto& console = GUI::Console::Instance();
    auto& input = InputManager::Instance();

    // Always forward to backend for mouse delta calculation
    auto* backend = dynamic_cast<GLFWInputBackend*>(input.GetBackend());
    if (backend) {
        backend->ForwardCursorPosEvent(xpos, ypos);
    }

    // If console is open, forward mouse movement to it
    if (console.IsOpen()) {
        console.OnMouseMove(static_cast<float>(xpos), static_cast<float>(ypos));
    }
}

} // namespace Genesis

