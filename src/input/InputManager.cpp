#include "InputManager.h"
#include "gui/Console.h"
#include <iostream>

namespace Genesis {

void InputManager::SetBackend(std::unique_ptr<IInputBackend> backend) {
    m_backend = std::move(backend);
}

bool InputManager::Initialize(void* windowHandle) {
    if (!m_backend) {
        return false;
    }
    return m_backend->Initialize(windowHandle);
}

void InputManager::Shutdown() {
    if (m_backend) {
        m_backend->Shutdown();
    }
    m_bindings.clear();
}

void InputManager::Update() {
    // Check for input actions BEFORE updating state
    // (because Update copies current to previous, which would clear "pressed" detection)
    HandleMouseLocking();
    LogWASDKeys();

    if (m_backend) {
        m_backend->Update();
    }
}

// ============================================================================
// Action Bindings
// ============================================================================
void InputManager::BindAction(GameAction action, InputBinding binding) {
    m_bindings[action].push_back(binding);
}

void InputManager::UnbindAction(GameAction action) {
    m_bindings.erase(action);
}

void InputManager::ClearBindings() {
    m_bindings.clear();
}

bool InputManager::CheckBinding(const InputBinding& binding,
                                 bool (IInputBackend::*keyCheck)(KeyCode) const,
                                 bool (IInputBackend::*mouseCheck)(MouseButton) const) const {
    if (!m_backend) return false;

    if (binding.type == InputBinding::Type::Key) {
        return (m_backend.get()->*keyCheck)(binding.key);
    } else {
        return (m_backend.get()->*mouseCheck)(binding.mouseButton);
    }
}

bool InputManager::IsActionDown(GameAction action) const {
    auto it = m_bindings.find(action);
    if (it == m_bindings.end()) return false;

    for (const auto& binding : it->second) {
        if (CheckBinding(binding, &IInputBackend::IsKeyDown, &IInputBackend::IsMouseButtonDown)) {
            return true;
        }
    }
    return false;
}

bool InputManager::IsActionPressed(GameAction action) const {
    auto it = m_bindings.find(action);
    if (it == m_bindings.end()) return false;

    for (const auto& binding : it->second) {
        if (CheckBinding(binding, &IInputBackend::IsKeyPressed, &IInputBackend::IsMouseButtonPressed)) {
            return true;
        }
    }
    return false;
}

bool InputManager::IsActionReleased(GameAction action) const {
    auto it = m_bindings.find(action);
    if (it == m_bindings.end()) return false;

    for (const auto& binding : it->second) {
        if (CheckBinding(binding, &IInputBackend::IsKeyReleased, &IInputBackend::IsMouseButtonReleased)) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// Direct Input Access
// ============================================================================
bool InputManager::IsKeyDown(KeyCode key) const {
    return m_backend && m_backend->IsKeyDown(key);
}

bool InputManager::IsKeyPressed(KeyCode key) const {
    return m_backend && m_backend->IsKeyPressed(key);
}

bool InputManager::IsKeyReleased(KeyCode key) const {
    return m_backend && m_backend->IsKeyReleased(key);
}

bool InputManager::IsMouseButtonDown(MouseButton button) const {
    return m_backend && m_backend->IsMouseButtonDown(button);
}

bool InputManager::IsMouseButtonPressed(MouseButton button) const {
    return m_backend && m_backend->IsMouseButtonPressed(button);
}

bool InputManager::IsMouseButtonReleased(MouseButton button) const {
    return m_backend && m_backend->IsMouseButtonReleased(button);
}

void InputManager::GetMousePosition(double& x, double& y) const {
    if (m_backend) {
        m_backend->GetMousePosition(x, y);
    } else {
        x = y = 0.0;
    }
}

void InputManager::GetMouseDelta(double& dx, double& dy) const {
    if (m_backend) {
        m_backend->GetMouseDelta(dx, dy);
    } else {
        dx = dy = 0.0;
    }
}

double InputManager::GetScrollDelta() const {
    return m_backend ? m_backend->GetScrollDelta() : 0.0;
}

void InputManager::SetCursorLocked(bool locked) {
    if (m_backend) {
        m_backend->SetCursorMode(locked);
    }
}

bool InputManager::IsCursorLocked() const {
    return m_backend && m_backend->IsCursorLocked();
}

void InputManager::HandleMouseLocking() {
    // Don't capture cursor if console is open
    if (GUI::Console::Instance().IsOpen()) {
        return;
    }

    if (IsMouseButtonPressed(MouseButton::Left)) {
        SetCursorLocked(true);
    }
}

void InputManager::LogWASDKeys() {
    // Disabled to prevent console spam
    // Key input logging can be enabled via ge_debug_input convar if needed
}

// ============================================================================
// Default Bindings
// ============================================================================
void InputManager::SetupDefaultBindings() {
    ClearBindings();

    // Movement - WASD
    BindAction(GameAction::MoveForward, InputBinding::FromKey(KeyCode::W));
    BindAction(GameAction::MoveBackward, InputBinding::FromKey(KeyCode::S));
    BindAction(GameAction::MoveLeft, InputBinding::FromKey(KeyCode::A));
    BindAction(GameAction::MoveRight, InputBinding::FromKey(KeyCode::D));

    // Movement - Arrow keys (alternative)
    BindAction(GameAction::MoveForward, InputBinding::FromKey(KeyCode::Up));
    BindAction(GameAction::MoveBackward, InputBinding::FromKey(KeyCode::Down));
    BindAction(GameAction::MoveLeft, InputBinding::FromKey(KeyCode::Left));
    BindAction(GameAction::MoveRight, InputBinding::FromKey(KeyCode::Right));

    // Jump, Crouch, Sprint
    BindAction(GameAction::Jump, InputBinding::FromKey(KeyCode::Space));
    BindAction(GameAction::Crouch, InputBinding::FromKey(KeyCode::LeftControl));
    BindAction(GameAction::Sprint, InputBinding::FromKey(KeyCode::LeftShift));

    // Actions
    BindAction(GameAction::PrimaryAction, InputBinding::FromMouseButton(MouseButton::Left));
    BindAction(GameAction::SecondaryAction, InputBinding::FromMouseButton(MouseButton::Right));
    BindAction(GameAction::Interact, InputBinding::FromKey(KeyCode::E));
    BindAction(GameAction::Reload, InputBinding::FromKey(KeyCode::R));

    // UI
    BindAction(GameAction::Pause, InputBinding::FromKey(KeyCode::Escape));
    BindAction(GameAction::Inventory, InputBinding::FromKey(KeyCode::Tab));
    BindAction(GameAction::Menu, InputBinding::FromKey(KeyCode::M));

    // Debug
    BindAction(GameAction::DebugConsole, InputBinding::FromKey(KeyCode::GraveAccent));
    BindAction(GameAction::DebugOverlay, InputBinding::FromKey(KeyCode::F3));
}

} // namespace Genesis

