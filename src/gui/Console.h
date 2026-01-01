#pragma once

#include "GUIRenderer.h"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <functional>
#include <sstream>

namespace Genesis {
namespace GUI {

// ============================================================================
// Console Message Types
// ============================================================================
enum class MessageType {
    Normal,
    Command,
    Warning,
    Error,
    Success
};

struct ConsoleMessage {
    std::string text;
    MessageType type;

    ConsoleMessage(const std::string& t, MessageType mt = MessageType::Normal)
        : text(t), type(mt) {}
};

// ============================================================================
// Console Variable (ConVar)
// ============================================================================
class ConVar {
public:
    ConVar(const std::string& name, const std::string& defaultValue, const std::string& help = "")
        : m_name(name), m_value(defaultValue), m_defaultValue(defaultValue), m_help(help) {}

    const std::string& GetName() const { return m_name; }
    const std::string& GetString() const { return m_value; }
    const std::string& GetHelp() const { return m_help; }

    int GetInt() const { return std::stoi(m_value); }
    float GetFloat() const { return std::stof(m_value); }
    bool GetBool() const { return m_value == "1" || m_value == "true"; }

    void SetString(const std::string& value) { m_value = value; }
    void SetInt(int value) { m_value = std::to_string(value); }
    void SetFloat(float value) { m_value = std::to_string(value); }
    void SetBool(bool value) { m_value = value ? "1" : "0"; }

    void Reset() { m_value = m_defaultValue; }

private:
    std::string m_name;
    std::string m_value;
    std::string m_defaultValue;
    std::string m_help;
};

// ============================================================================
// Console Command Callback
// ============================================================================
using CommandCallback = std::function<void(const std::vector<std::string>& args)>;

struct ConsoleCommand {
    std::string name;
    std::string help;
    CommandCallback callback;
};

// ============================================================================
// Developer Console - Source Engine Style
// ============================================================================
class Console {
public:
    static Console& Instance() {
        static Console instance;
        return instance;
    }

    // ========================================================================
    // Lifecycle
    // ========================================================================

    void Initialize();
    void Shutdown();
    void Update(float deltaTime);
    void Render(int screenWidth, int screenHeight);

    // ========================================================================
    // Console Control
    // ========================================================================

    void Toggle();
    void Open();
    void Close();
    bool IsOpen() const { return m_isOpen; }

    // Set how far down the console slides (0.0 = hidden, 1.0 = full screen)
    void SetOpenAmount(float amount) { m_targetOpenAmount = amount; }

    // ========================================================================
    // Input Handling
    // ========================================================================

    void OnKeyPressed(int key);
    void OnCharInput(unsigned int codepoint);
    void OnMouseMove(float x, float y);
    void OnMouseDown(float x, float y, int button);
    void OnMouseUp(float x, float y, int button);

    // ========================================================================
    // Output
    // ========================================================================

    void Print(const std::string& message, MessageType type = MessageType::Normal);
    void Printf(const char* format, ...);
    void PrintWarning(const std::string& message);
    void PrintError(const std::string& message);
    void PrintSuccess(const std::string& message);
    void Clear();

    // ========================================================================
    // Commands & Variables
    // ========================================================================

    void RegisterCommand(const std::string& name, CommandCallback callback, const std::string& help = "");
    void UnregisterCommand(const std::string& name);

    ConVar* RegisterConVar(const std::string& name, const std::string& defaultValue, const std::string& help = "");
    ConVar* FindConVar(const std::string& name);

    void ExecuteCommand(const std::string& commandLine);

    // ========================================================================
    // History
    // ========================================================================

    const std::deque<ConsoleMessage>& GetMessages() const { return m_messages; }

private:
    Console() = default;

    void RegisterBuiltInCommands();
    std::vector<std::string> ParseCommandLine(const std::string& line);
    void AddToHistory(const std::string& command);
    void AutoComplete();
    Vec4 GetMessageColor(MessageType type) const;

    // Render helpers for different modes
    void RenderDocked(int screenWidth, int screenHeight);
    void RenderFloating(int screenWidth, int screenHeight);

private:
    bool m_isOpen = false;
    float m_openAmount = 0.0f;        // Current open amount (animated)
    float m_targetOpenAmount = 0.5f;  // Target open amount (50% of screen)
    float m_animationSpeed = 8.0f;    // How fast console opens/closes

    // Floating window state
    float m_windowX = 100.0f;
    float m_windowY = 100.0f;
    float m_windowWidth = 800.0f;
    float m_windowHeight = 500.0f;
    bool m_isDragging = false;
    float m_dragOffsetX = 0.0f;
    float m_dragOffsetY = 0.0f;
    bool m_isResizing = false;
    float m_resizeStartX = 0.0f;
    float m_resizeStartY = 0.0f;
    float m_resizeStartWidth = 0.0f;
    float m_resizeStartHeight = 0.0f;

    // Input
    std::string m_inputBuffer;
    int m_cursorPos = 0;
    float m_cursorBlinkTimer = 0.0f;
    bool m_cursorVisible = true;

    // History
    std::vector<std::string> m_commandHistory;
    int m_historyIndex = -1;

    // Messages
    std::deque<ConsoleMessage> m_messages;
    static constexpr size_t MAX_MESSAGES = 1000;
    float m_scrollOffset = 0.0f;

    // Commands and variables
    std::unordered_map<std::string, ConsoleCommand> m_commands;
    std::unordered_map<std::string, std::unique_ptr<ConVar>> m_convars;

    // Auto-complete
    std::vector<std::string> m_autoCompleteOptions;
    int m_autoCompleteIndex = 0;

    bool m_initialized = false;
};

} // namespace GUI
} // namespace Genesis

