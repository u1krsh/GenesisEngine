#include "Console.h"
#include "core/Time.h"
#include "core/Engine.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace Genesis {
namespace GUI {

void Console::Initialize() {
    if (m_initialized) return;

    RegisterBuiltInCommands();

    Print("Genesis Engine Console", MessageType::Success);
    Print("Type 'help' for a list of commands", MessageType::Normal);
    Print("", MessageType::Normal);

    m_initialized = true;
}

void Console::Shutdown() {
    m_commands.clear();
    m_convars.clear();
    m_messages.clear();
    m_initialized = false;
}

void Console::RegisterBuiltInCommands() {
    // Help command
    RegisterCommand("help", [this](const std::vector<std::string>& args) {
        if (args.size() > 1) {
            // Help for specific command
            auto it = m_commands.find(args[1]);
            if (it != m_commands.end()) {
                Print(it->second.name + ": " + it->second.help, MessageType::Normal);
            } else {
                auto cvar = FindConVar(args[1]);
                if (cvar) {
                    Print(cvar->GetName() + " = \"" + cvar->GetString() + "\"", MessageType::Normal);
                    Print("  " + cvar->GetHelp(), MessageType::Normal);
                } else {
                    PrintError("Unknown command: " + args[1]);
                }
            }
        } else {
            Print("=== Commands ===", MessageType::Success);
            for (const auto& [name, cmd] : m_commands) {
                Print("  " + name + " - " + cmd.help, MessageType::Normal);
            }
            Print("", MessageType::Normal);
            Print("=== Variables ===", MessageType::Success);
            for (const auto& [name, cvar] : m_convars) {
                Print("  " + name + " = \"" + cvar->GetString() + "\"", MessageType::Normal);
            }
        }
    }, "Show help for commands");

    // Clear command
    RegisterCommand("clear", [this](const std::vector<std::string>&) {
        Clear();
    }, "Clear console output");

    // Echo command
    RegisterCommand("echo", [this](const std::vector<std::string>& args) {
        std::string message;
        for (size_t i = 1; i < args.size(); i++) {
            if (i > 1) message += " ";
            message += args[i];
        }
        Print(message, MessageType::Normal);
    }, "Print a message to the console");

    // Quit command
    RegisterCommand("quit", [](const std::vector<std::string>&) {
        Engine::Instance().Stop();
    }, "Quit the game");

    RegisterCommand("exit", [](const std::vector<std::string>&) {
        Engine::Instance().Stop();
    }, "Quit the game");

    // List commands
    RegisterCommand("cmdlist", [this](const std::vector<std::string>&) {
        Print("=== Registered Commands ===", MessageType::Success);
        std::vector<std::string> names;
        for (const auto& [name, cmd] : m_commands) {
            names.push_back(name);
        }
        std::sort(names.begin(), names.end());
        for (const auto& name : names) {
            Print("  " + name, MessageType::Normal);
        }
        Print("Total: " + std::to_string(names.size()) + " commands", MessageType::Normal);
    }, "List all commands");

    // List convars
    RegisterCommand("cvarlist", [this](const std::vector<std::string>&) {
        Print("=== Registered Variables ===", MessageType::Success);
        std::vector<std::string> names;
        for (const auto& [name, cvar] : m_convars) {
            names.push_back(name);
        }
        std::sort(names.begin(), names.end());
        for (const auto& name : names) {
            auto* cvar = FindConVar(name);
            Print("  " + name + " = \"" + cvar->GetString() + "\"", MessageType::Normal);
        }
        Print("Total: " + std::to_string(names.size()) + " variables", MessageType::Normal);
    }, "List all variables");

    // Find command - searches for commands/convars matching a pattern
    RegisterCommand("find", [this](const std::vector<std::string>& args) {
        if (args.size() < 2) {
            PrintError("Usage: find <pattern>");
            return;
        }
        const std::string& pattern = args[1];
        Print("=== Searching for '" + pattern + "' ===", MessageType::Success);

        int found = 0;
        for (const auto& [name, cmd] : m_commands) {
            if (name.find(pattern) != std::string::npos || cmd.help.find(pattern) != std::string::npos) {
                Print("  [CMD] " + name + " - " + cmd.help, MessageType::Normal);
                found++;
            }
        }
        for (const auto& [name, cvar] : m_convars) {
            if (name.find(pattern) != std::string::npos || cvar->GetHelp().find(pattern) != std::string::npos) {
                Print("  [VAR] " + name + " = \"" + cvar->GetString() + "\" - " + cvar->GetHelp(), MessageType::Normal);
                found++;
            }
        }
        Print("Found " + std::to_string(found) + " matches", MessageType::Normal);
    }, "Search for commands and variables");

    // Reset command - reset a convar to default
    RegisterCommand("reset", [this](const std::vector<std::string>& args) {
        if (args.size() < 2) {
            PrintError("Usage: reset <convar_name>");
            return;
        }
        auto* cvar = FindConVar(args[1]);
        if (cvar) {
            cvar->Reset();
            Print(args[1] + " reset to \"" + cvar->GetString() + "\"", MessageType::Success);
        } else {
            PrintError("Unknown variable: " + args[1]);
        }
    }, "Reset a variable to its default value");

    // Toggle command - toggle a boolean convar
    RegisterCommand("toggle", [this](const std::vector<std::string>& args) {
        if (args.size() < 2) {
            PrintError("Usage: toggle <convar_name>");
            return;
        }
        auto* cvar = FindConVar(args[1]);
        if (cvar) {
            bool current = cvar->GetBool();
            cvar->SetBool(!current);
            Print(args[1] + " = \"" + cvar->GetString() + "\"", MessageType::Success);
        } else {
            PrintError("Unknown variable: " + args[1]);
        }
    }, "Toggle a boolean variable (0/1)");

    // Version command
    RegisterCommand("version", [this](const std::vector<std::string>&) {
        Print("Genesis Engine v0.1.0", MessageType::Success);
        Print("Built with OpenGL 3.3 Core", MessageType::Normal);
        Print("(c) 2025 Genesis Engine Team", MessageType::Normal);
    }, "Show engine version information");

    // ge_showinfo - Show debug info on screen
    RegisterConVar("ge_showinfo", "0", "Show debug info on screen (FPS, position, etc.) - 0 or 1");

    // ge_console_mode - Console display mode (0 = docked at top, 1 = floating window)
    RegisterConVar("ge_console_mode", "0", "Console mode: 0 = docked at top, 1 = floating window");

    // Other useful convars
    RegisterConVar("ge_timescale", "1.0", "Time scale multiplier");
    RegisterConVar("ge_fov", "70", "Field of view in degrees");
    RegisterConVar("ge_sensitivity", "0.35", "Mouse sensitivity");
}

void Console::Update(float deltaTime) {
    // Animate console opening/closing
    if (m_isOpen) {
        m_openAmount = Math::Lerp(m_openAmount, m_targetOpenAmount, deltaTime * m_animationSpeed);
    } else {
        m_openAmount = Math::Lerp(m_openAmount, 0.0f, deltaTime * m_animationSpeed);
    }

    // Cursor blink
    m_cursorBlinkTimer += deltaTime;
    if (m_cursorBlinkTimer >= 0.5f) {
        m_cursorBlinkTimer = 0.0f;
        m_cursorVisible = !m_cursorVisible;
    }

    // Apply convars
    auto* timescale = FindConVar("ge_timescale");
    if (timescale) {
        Time::Instance().SetTimeScale(timescale->GetFloat());
    }

    auto* fov = FindConVar("ge_fov");
    if (fov) {
        Engine::Instance().GetCamera().SetFOV(fov->GetFloat());
    }

    auto* sensitivity = FindConVar("ge_sensitivity");
    if (sensitivity) {
        Engine::Instance().GetCamera().SetMouseSensitivity(sensitivity->GetFloat());
    }
}

void Console::Render(int screenWidth, int screenHeight) {
    auto* consoleMode = FindConVar("ge_console_mode");
    int mode = consoleMode ? consoleMode->GetInt() : 0;

    if (mode == 1) {
        // Floating mode: show/hide immediately, no animation
        if (!m_isOpen) return;
        RenderFloating(screenWidth, screenHeight);
    } else {
        // Docked mode: uses animation
        if (m_openAmount < 0.01f) return;
        RenderDocked(screenWidth, screenHeight);
    }
}

void Console::RenderDocked(int screenWidth, int screenHeight) {
    auto& renderer = GUIRenderer::Instance();

    float consoleHeight = screenHeight * m_openAmount;
    Rect consoleRect(0, 0, static_cast<float>(screenWidth), consoleHeight);

    // Background with gradient (Windows 7 style)
    renderer.DrawRectGradientV(consoleRect, Colors::BackgroundDark, Colors::Background);

    // Title bar with gradient
    Rect titleRect(0, 0, static_cast<float>(screenWidth), 26);
    renderer.DrawRectGradientV(titleRect, Colors::TitleBarGradientTop, Colors::TitleBarGradientBottom);

    // Red accent line at top (Windows 7 style)
    renderer.DrawRect(Rect(0, 0, static_cast<float>(screenWidth), 2), Colors::Accent);

    renderer.DrawText("Genesis Engine Console", 8, 7, Colors::TextHighlight, 1.0f);

    // Version info on right side
    std::string version = "v0.1.0 | Press ~ to close";
    Vec2 versionSize = renderer.MeasureText(version, 1.0f);
    renderer.DrawText(version, screenWidth - versionSize.x - 8, 7, Colors::TextDim, 1.0f);

    // 3D border effect
    renderer.DrawBorder3D(consoleRect, false);

    // Messages area
    float inputHeight = 28;
    float messagesTop = 30;
    float messagesBottom = consoleHeight - inputHeight - 4;
    float messagesHeight = messagesBottom - messagesTop;

    Rect messagesRect(4, messagesTop, static_cast<float>(screenWidth) - 8, messagesHeight);
    renderer.DrawRect(messagesRect, Vec4(0.04f, 0.03f, 0.07f, 0.95f));
    renderer.DrawBorder3D(messagesRect, false);

    // Render messages
    float lineHeight = renderer.GetFontHeight(1.0f) + 2;
    float y = messagesBottom - lineHeight - 4;

    int visibleLines = static_cast<int>(messagesHeight / lineHeight);
    int startIndex = std::max(0, static_cast<int>(m_messages.size()) - visibleLines - static_cast<int>(m_scrollOffset));

    for (int i = static_cast<int>(m_messages.size()) - 1; i >= startIndex && y > messagesTop; i--) {
        const auto& msg = m_messages[i];
        renderer.DrawText(msg.text, 8, y, GetMessageColor(msg.type), 1.0f);
        y -= lineHeight;
    }

    // Input area with Windows 7 style
    Rect inputRect(4, consoleHeight - inputHeight - 2, static_cast<float>(screenWidth) - 8, inputHeight);
    renderer.DrawRect(inputRect, Colors::InputBackground);
    renderer.DrawBorder3D(inputRect, false);

    // Red focus indicator on input when console is open
    if (m_isOpen) {
        renderer.DrawRect(Rect(4, consoleHeight - inputHeight - 2, 3, inputHeight), Colors::Accent);
    }

    // Prompt with red accent
    renderer.DrawText("]", 12, consoleHeight - inputHeight + 5, Colors::Accent, 1.0f);

    // Input text
    renderer.DrawText(m_inputBuffer, 24, consoleHeight - inputHeight + 5, Colors::Text, 1.0f);

    // Cursor
    if (m_cursorVisible && m_isOpen) {
        float cursorX = 24 + m_cursorPos * 8;
        renderer.DrawRect(Rect(cursorX, consoleHeight - inputHeight + 5, 8, 14),
                         Vec4(Colors::Accent.x, Colors::Accent.y, Colors::Accent.z, 0.7f));
    }

    // Bottom border with red accent
    renderer.DrawRect(Rect(0, consoleHeight - 3, static_cast<float>(screenWidth), 3), Colors::BorderDark);
    renderer.DrawRect(Rect(0, consoleHeight - 2, static_cast<float>(screenWidth), 2), Colors::Accent);
}

void Console::RenderFloating(int screenWidth, int screenHeight) {
    auto& renderer = GUIRenderer::Instance();

    // Clamp window position to screen bounds
    m_windowX = std::max(0.0f, std::min(m_windowX, static_cast<float>(screenWidth) - m_windowWidth));
    m_windowY = std::max(0.0f, std::min(m_windowY, static_cast<float>(screenHeight) - m_windowHeight));

    // Window shadow
    Rect shadowRect(m_windowX + 4, m_windowY + 4, m_windowWidth, m_windowHeight);
    renderer.DrawRect(shadowRect, Vec4(0.0f, 0.0f, 0.0f, 0.4f));

    // Main window background
    Rect windowRect(m_windowX, m_windowY, m_windowWidth, m_windowHeight);
    renderer.DrawRect(windowRect, Colors::BackgroundDark);

    // Windows 7 style outer border (light on top-left, dark on bottom-right)
    float bx = m_windowX, by = m_windowY, bw = m_windowWidth, bh = m_windowHeight;
    renderer.DrawRect(Rect(bx, by, bw, 1), Colors::BorderLight);        // Top
    renderer.DrawRect(Rect(bx, by, 1, bh), Colors::BorderLight);        // Left
    renderer.DrawRect(Rect(bx, by + bh - 1, bw, 1), Colors::BorderDark); // Bottom
    renderer.DrawRect(Rect(bx + bw - 1, by, 1, bh), Colors::BorderDark); // Right

    // Inner border
    renderer.DrawRect(Rect(bx + 1, by + 1, bw - 2, 1), Colors::Border);
    renderer.DrawRect(Rect(bx + 1, by + 1, 1, bh - 2), Colors::Border);
    renderer.DrawRect(Rect(bx + 1, by + bh - 2, bw - 2, 1), Colors::BorderDark);
    renderer.DrawRect(Rect(bx + bw - 2, by + 1, 1, bh - 2), Colors::BorderDark);

    // Title bar with gradient (Windows 7 Aero style)
    float titleBarHeight = 28.0f;
    Rect titleBarRect(m_windowX + 2, m_windowY + 2, m_windowWidth - 4, titleBarHeight);
    renderer.DrawRectGradientV(titleBarRect, Colors::TitleBarGradientTop, Colors::TitleBarGradientBottom);

    // Red accent line at top of title bar
    renderer.DrawRect(Rect(m_windowX + 2, m_windowY + 2, m_windowWidth - 4, 2), Colors::Accent);

    // Title text
    renderer.DrawText("Genesis Console", m_windowX + 10, m_windowY + 9, Colors::TextHighlight, 1.0f);

    // Close button (X) - Windows 7 style
    float closeButtonSize = 20.0f;
    float closeButtonX = m_windowX + m_windowWidth - closeButtonSize - 8;
    float closeButtonY = m_windowY + 6;
    Rect closeButtonRect(closeButtonX, closeButtonY, closeButtonSize, closeButtonSize);
    renderer.DrawRect(closeButtonRect, Colors::Accent);
    renderer.DrawBorder3D(closeButtonRect, true);
    renderer.DrawTextCentered("X", closeButtonRect, Colors::TextHighlight, 1.0f);

    // Messages area
    float inputHeight = 28.0f;
    float messagesTop = m_windowY + titleBarHeight + 6;
    float messagesBottom = m_windowY + m_windowHeight - inputHeight - 10;
    float messagesHeight = messagesBottom - messagesTop;

    Rect messagesRect(m_windowX + 6, messagesTop, m_windowWidth - 12, messagesHeight);
    renderer.DrawRect(messagesRect, Vec4(0.04f, 0.03f, 0.07f, 0.95f));
    renderer.DrawBorder3D(messagesRect, false);

    // Render messages
    float lineHeight = renderer.GetFontHeight(1.0f) + 2;
    float y = messagesBottom - lineHeight - 4;

    int visibleLines = static_cast<int>(messagesHeight / lineHeight);
    int startIndex = std::max(0, static_cast<int>(m_messages.size()) - visibleLines - static_cast<int>(m_scrollOffset));

    for (int i = static_cast<int>(m_messages.size()) - 1; i >= startIndex && y > messagesTop; i--) {
        const auto& msg = m_messages[i];
        renderer.DrawText(msg.text, m_windowX + 10, y, GetMessageColor(msg.type), 1.0f);
        y -= lineHeight;
    }

    // Input area with Windows 7 style
    float inputY = m_windowY + m_windowHeight - inputHeight - 6;
    Rect inputRect(m_windowX + 6, inputY, m_windowWidth - 12, inputHeight);
    renderer.DrawRect(inputRect, Colors::InputBackground);
    renderer.DrawBorder3D(inputRect, false);

    // Red focus indicator
    if (m_isOpen) {
        renderer.DrawRect(Rect(m_windowX + 6, inputY, 3, inputHeight), Colors::Accent);
    }

    // Prompt
    renderer.DrawText("]", m_windowX + 14, inputY + 7, Colors::Accent, 1.0f);

    // Input text
    renderer.DrawText(m_inputBuffer, m_windowX + 26, inputY + 7, Colors::Text, 1.0f);

    // Cursor
    if (m_cursorVisible && m_isOpen) {
        float cursorX = m_windowX + 26 + m_cursorPos * 8;
        renderer.DrawRect(Rect(cursorX, inputY + 7, 8, 14),
                         Vec4(Colors::Accent.x, Colors::Accent.y, Colors::Accent.z, 0.7f));
    }

    // Resize grip (bottom-right corner) - Windows 7 style
    float gripSize = 16.0f;
    float gripX = m_windowX + m_windowWidth - gripSize;
    float gripY = m_windowY + m_windowHeight - gripSize;

    // Draw diagonal lines for resize grip
    for (int i = 0; i < 3; i++) {
        float offset = i * 4.0f;
        renderer.DrawRect(Rect(gripX + 6 + offset, gripY + gripSize - 4 - offset, 2, 2), Colors::BorderLight);
        renderer.DrawRect(Rect(gripX + 8 + offset, gripY + gripSize - 2 - offset, 2, 2), Colors::BorderDark);
    }
}

void Console::Toggle() {
    m_isOpen = !m_isOpen;
    if (m_isOpen) {
        m_cursorBlinkTimer = 0.0f;
        m_cursorVisible = true;
    }
}

void Console::Open() {
    m_isOpen = true;
    m_cursorBlinkTimer = 0.0f;
    m_cursorVisible = true;
}

void Console::Close() {
    m_isOpen = false;
}

void Console::OnKeyPressed(int key) {
    if (!m_isOpen) return;

    switch (key) {
        case GLFW_KEY_ENTER:
            if (!m_inputBuffer.empty()) {
                Print("] " + m_inputBuffer, MessageType::Command);
                ExecuteCommand(m_inputBuffer);
                AddToHistory(m_inputBuffer);
                m_inputBuffer.clear();
                m_cursorPos = 0;
                m_historyIndex = -1;
            }
            break;

        case GLFW_KEY_BACKSPACE:
            if (m_cursorPos > 0) {
                m_inputBuffer.erase(m_cursorPos - 1, 1);
                m_cursorPos--;
            }
            break;

        case GLFW_KEY_DELETE:
            if (m_cursorPos < static_cast<int>(m_inputBuffer.length())) {
                m_inputBuffer.erase(m_cursorPos, 1);
            }
            break;

        case GLFW_KEY_LEFT:
            if (m_cursorPos > 0) m_cursorPos--;
            break;

        case GLFW_KEY_RIGHT:
            if (m_cursorPos < static_cast<int>(m_inputBuffer.length())) m_cursorPos++;
            break;

        case GLFW_KEY_HOME:
            m_cursorPos = 0;
            break;

        case GLFW_KEY_END:
            m_cursorPos = static_cast<int>(m_inputBuffer.length());
            break;

        case GLFW_KEY_UP:
            if (!m_commandHistory.empty()) {
                if (m_historyIndex < static_cast<int>(m_commandHistory.size()) - 1) {
                    m_historyIndex++;
                    m_inputBuffer = m_commandHistory[m_commandHistory.size() - 1 - m_historyIndex];
                    m_cursorPos = static_cast<int>(m_inputBuffer.length());
                }
            }
            break;

        case GLFW_KEY_DOWN:
            if (m_historyIndex > 0) {
                m_historyIndex--;
                m_inputBuffer = m_commandHistory[m_commandHistory.size() - 1 - m_historyIndex];
                m_cursorPos = static_cast<int>(m_inputBuffer.length());
            } else if (m_historyIndex == 0) {
                m_historyIndex = -1;
                m_inputBuffer.clear();
                m_cursorPos = 0;
            }
            break;

        case GLFW_KEY_TAB:
            AutoComplete();
            break;

        case GLFW_KEY_PAGE_UP:
            m_scrollOffset += 5;
            m_scrollOffset = std::min(m_scrollOffset, static_cast<float>(m_messages.size()));
            break;

        case GLFW_KEY_PAGE_DOWN:
            m_scrollOffset -= 5;
            m_scrollOffset = std::max(m_scrollOffset, 0.0f);
            break;
    }

    m_cursorBlinkTimer = 0.0f;
    m_cursorVisible = true;
}

void Console::OnCharInput(unsigned int codepoint) {
    if (!m_isOpen) return;
    if (codepoint < 32 || codepoint > 126) return;  // Only printable ASCII

    char c = static_cast<char>(codepoint);
    m_inputBuffer.insert(m_cursorPos, 1, c);
    m_cursorPos++;

    m_cursorBlinkTimer = 0.0f;
    m_cursorVisible = true;
}

void Console::OnMouseMove(float x, float y) {
    if (!m_isOpen) return;

    auto* consoleMode = FindConVar("ge_console_mode");
    if (!consoleMode || consoleMode->GetInt() != 1) return; // Only for floating mode

    if (m_isDragging) {
        m_windowX = x - m_dragOffsetX;
        m_windowY = y - m_dragOffsetY;
    }

    if (m_isResizing) {
        float newWidth = m_resizeStartWidth + (x - m_resizeStartX);
        float newHeight = m_resizeStartHeight + (y - m_resizeStartY);

        // Minimum window size
        m_windowWidth = std::max(300.0f, newWidth);
        m_windowHeight = std::max(200.0f, newHeight);
    }
}

void Console::OnMouseDown(float x, float y, int button) {
    if (!m_isOpen) return;
    if (button != 0) return; // Left button only

    auto* consoleMode = FindConVar("ge_console_mode");
    if (!consoleMode || consoleMode->GetInt() != 1) return; // Only for floating mode

    float titleBarHeight = 28.0f;
    float gripSize = 16.0f;

    // Check if clicking on title bar (for dragging)
    Rect titleBarRect(m_windowX + 2, m_windowY + 2, m_windowWidth - 4 - 24, titleBarHeight); // Leave space for close button
    if (titleBarRect.Contains(x, y)) {
        m_isDragging = true;
        m_dragOffsetX = x - m_windowX;
        m_dragOffsetY = y - m_windowY;
        return;
    }

    // Check if clicking on close button
    float closeButtonSize = 20.0f;
    float closeButtonX = m_windowX + m_windowWidth - closeButtonSize - 8;
    float closeButtonY = m_windowY + 6;
    Rect closeButtonRect(closeButtonX, closeButtonY, closeButtonSize, closeButtonSize);
    if (closeButtonRect.Contains(x, y)) {
        Close();
        return;
    }

    // Check if clicking on resize grip (bottom-right corner)
    Rect resizeGripRect(m_windowX + m_windowWidth - gripSize, m_windowY + m_windowHeight - gripSize, gripSize, gripSize);
    if (resizeGripRect.Contains(x, y)) {
        m_isResizing = true;
        m_resizeStartX = x;
        m_resizeStartY = y;
        m_resizeStartWidth = m_windowWidth;
        m_resizeStartHeight = m_windowHeight;
        return;
    }
}

void Console::OnMouseUp(float x, float y, int button) {
    (void)x; (void)y;
    if (button != 0) return; // Left button only

    m_isDragging = false;
    m_isResizing = false;
}

void Console::Print(const std::string& message, MessageType type) {
    m_messages.emplace_back(message, type);
    if (m_messages.size() > MAX_MESSAGES) {
        m_messages.pop_front();
    }
    m_scrollOffset = 0;  // Auto-scroll to bottom
}

void Console::Printf(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Print(buffer, MessageType::Normal);
}

void Console::PrintWarning(const std::string& message) {
    Print("[WARNING] " + message, MessageType::Warning);
}

void Console::PrintError(const std::string& message) {
    Print("[ERROR] " + message, MessageType::Error);
}

void Console::PrintSuccess(const std::string& message) {
    Print(message, MessageType::Success);
}

void Console::Clear() {
    m_messages.clear();
    m_scrollOffset = 0;
}

void Console::RegisterCommand(const std::string& name, CommandCallback callback, const std::string& help) {
    m_commands[name] = {name, help, callback};
}

void Console::UnregisterCommand(const std::string& name) {
    m_commands.erase(name);
}

ConVar* Console::RegisterConVar(const std::string& name, const std::string& defaultValue, const std::string& help) {
    auto cvar = std::make_unique<ConVar>(name, defaultValue, help);
    ConVar* ptr = cvar.get();
    m_convars[name] = std::move(cvar);
    return ptr;
}

ConVar* Console::FindConVar(const std::string& name) {
    auto it = m_convars.find(name);
    if (it != m_convars.end()) {
        return it->second.get();
    }
    return nullptr;
}

void Console::ExecuteCommand(const std::string& commandLine) {
    auto args = ParseCommandLine(commandLine);
    if (args.empty()) return;

    const std::string& cmdName = args[0];

    // Check if it's a command
    auto cmdIt = m_commands.find(cmdName);
    if (cmdIt != m_commands.end()) {
        cmdIt->second.callback(args);
        return;
    }

    // Check if it's a convar
    auto* cvar = FindConVar(cmdName);
    if (cvar) {
        if (args.size() > 1) {
            // Set value
            cvar->SetString(args[1]);
            Print(cmdName + " = \"" + cvar->GetString() + "\"", MessageType::Normal);
        } else {
            // Print current value
            Print(cmdName + " = \"" + cvar->GetString() + "\"", MessageType::Normal);
            if (!cvar->GetHelp().empty()) {
                Print("  " + cvar->GetHelp(), MessageType::Normal);
            }
        }
        return;
    }

    PrintError("Unknown command: " + cmdName);
}

std::vector<std::string> Console::ParseCommandLine(const std::string& line) {
    std::vector<std::string> args;
    std::string current;
    bool inQuotes = false;

    for (char c : line) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ' ' && !inQuotes) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        args.push_back(current);
    }

    return args;
}

void Console::AddToHistory(const std::string& command) {
    // Don't add duplicates
    if (!m_commandHistory.empty() && m_commandHistory.back() == command) {
        return;
    }
    m_commandHistory.push_back(command);

    // Limit history size
    if (m_commandHistory.size() > 100) {
        m_commandHistory.erase(m_commandHistory.begin());
    }
}

void Console::AutoComplete() {
    if (m_inputBuffer.empty()) return;

    std::vector<std::string> matches;

    // Match commands
    for (const auto& [name, cmd] : m_commands) {
        if (name.find(m_inputBuffer) == 0) {
            matches.push_back(name);
        }
    }

    // Match convars
    for (const auto& [name, cvar] : m_convars) {
        if (name.find(m_inputBuffer) == 0) {
            matches.push_back(name);
        }
    }

    if (matches.empty()) return;

    if (matches.size() == 1) {
        m_inputBuffer = matches[0] + " ";
        m_cursorPos = static_cast<int>(m_inputBuffer.length());
    } else {
        // Print all matches
        Print("", MessageType::Normal);
        for (const auto& match : matches) {
            Print("  " + match, MessageType::Normal);
        }

        // Find common prefix
        std::string prefix = matches[0];
        for (const auto& match : matches) {
            size_t i = 0;
            while (i < prefix.length() && i < match.length() && prefix[i] == match[i]) {
                i++;
            }
            prefix = prefix.substr(0, i);
        }

        if (prefix.length() > m_inputBuffer.length()) {
            m_inputBuffer = prefix;
            m_cursorPos = static_cast<int>(m_inputBuffer.length());
        }
    }
}

Vec4 Console::GetMessageColor(MessageType type) const {
    switch (type) {
        case MessageType::Command: return Colors::ConsoleCommand;
        case MessageType::Warning: return Colors::ConsoleWarning;
        case MessageType::Error:   return Colors::ConsoleError;
        case MessageType::Success: return Colors::ConsoleSuccess;
        default:                   return Colors::ConsoleOutput;
    }
}

} // namespace GUI
} // namespace Genesis

