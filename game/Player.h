#pragma once

#include "core/Engine.h"
#include "player/PlayerController.h"
#include "renderer/DebugRenderer.h"

namespace Game {

// ============================================================================
// Player Configuration
// ============================================================================
struct PlayerConfig {
    // Starting position
    Genesis::Vec3 spawnPosition = Genesis::Vec3(0.0f, 0.0f, 5.0f);

    // Controller settings (passed to PlayerController)
    Genesis::PlayerControllerConfig controllerConfig;

    // Mouse sensitivity
    float mouseSensitivity = 0.1f;
};

// ============================================================================
// Player - Game player with custom movement controller
// ============================================================================
class Player {
public:
    Player() = default;
    ~Player() = default;

    // ========================================================================
    // Lifecycle
    // ========================================================================
    void Initialize(const PlayerConfig& config = PlayerConfig());
    void Update(double deltaTime);
    void Render(Genesis::DebugRenderer* debugRenderer = nullptr);

    // ========================================================================
    // Input Processing
    // ========================================================================
    void ProcessInput();
    void ProcessMouseLook(float deltaX, float deltaY);

    // ========================================================================
    // Position and State
    // ========================================================================
    Genesis::Vec3 GetPosition() const;
    void SetPosition(const Genesis::Vec3& pos);
    void Teleport(const Genesis::Vec3& pos);

    Genesis::Vec3 GetEyePosition() const;
    Genesis::Vec3 GetVelocity() const;

    float GetYaw() const { return m_controller.GetYaw(); }
    float GetPitch() const { return m_controller.GetPitch(); }

    bool IsGrounded() const { return m_controller.IsGrounded(); }
    bool IsSprinting() const { return m_controller.IsSprinting(); }
    bool IsCrouching() const { return m_controller.IsCrouching(); }
    bool IsFalling() const { return m_controller.IsFalling(); }

    // ========================================================================
    // Controller Access
    // ========================================================================
    Genesis::PlayerController& GetController() { return m_controller; }
    const Genesis::PlayerController& GetController() const { return m_controller; }

    // ========================================================================
    // Camera Sync
    // ========================================================================
    // Updates the engine camera to follow the player
    void SyncCamera();

    // ========================================================================
    // Debug
    // ========================================================================
    void DrawDebugInfo(Genesis::DebugRenderer* renderer) const;

private:
    Genesis::PlayerController m_controller;
    PlayerConfig m_config;
    float m_mouseSensitivity = 0.1f;
};

} // namespace Game

