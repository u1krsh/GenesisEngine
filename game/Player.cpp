#include "Player.h"
#include "input/InputManager.h"
#include <cmath>

namespace Game {

void Player::Initialize(const PlayerConfig& config) {
    m_config = config;
    m_mouseSensitivity = config.mouseSensitivity;

    // Initialize the controller with configuration
    m_controller.Initialize(config.controllerConfig);

    // Set spawn position
    m_controller.SetPosition(config.spawnPosition);

    // Initialize look direction to match engine camera
    auto& camera = Genesis::Engine::Instance().GetCamera();
    m_controller.SetLookDirection(camera.GetYaw(), camera.GetPitch());

    // Sync camera to player position
    SyncCamera();
}

void Player::Update(double deltaTime) {
    // Process player input
    ProcessInput();

    // Update controller physics
    m_controller.Update(static_cast<float>(deltaTime));

    // Sync camera to follow player
    SyncCamera();
}

void Player::ProcessInput() {
    auto& input = Genesis::InputManager::Instance();

    // Build movement input vector
    Genesis::Vec3 moveInput(0.0f);

    if (input.IsActionDown(Genesis::GameAction::MoveForward)) {
        moveInput.z += 1.0f;
    }
    if (input.IsActionDown(Genesis::GameAction::MoveBackward)) {
        moveInput.z -= 1.0f;
    }
    if (input.IsActionDown(Genesis::GameAction::MoveLeft)) {
        moveInput.x -= 1.0f;
    }
    if (input.IsActionDown(Genesis::GameAction::MoveRight)) {
        moveInput.x += 1.0f;
    }

    m_controller.SetMoveInput(moveInput);

    // Sprint
    m_controller.SetSprinting(input.IsActionDown(Genesis::GameAction::Sprint));

    // Crouch
    m_controller.SetCrouching(input.IsActionDown(Genesis::GameAction::Crouch));

    // Note: Jump is handled in OnInput (per-frame) to properly detect key press
}

void Player::ProcessMouseLook(float deltaX, float deltaY) {
    // Apply mouse sensitivity
    float newYaw = m_controller.GetYaw() + deltaX * m_mouseSensitivity;
    float newPitch = m_controller.GetPitch() - deltaY * m_mouseSensitivity;

    // Keep yaw in reasonable range
    if (newYaw > 360.0f) newYaw -= 360.0f;
    if (newYaw < -360.0f) newYaw += 360.0f;

    // Clamp pitch
    newPitch = glm::clamp(newPitch, -89.0f, 89.0f);

    m_controller.SetLookDirection(newYaw, newPitch);
}

void Player::Render(Genesis::DebugRenderer* debugRenderer) {
    if (debugRenderer) {
        DrawDebugInfo(debugRenderer);
    }
}

Genesis::Vec3 Player::GetPosition() const {
    return m_controller.GetPosition();
}

void Player::SetPosition(const Genesis::Vec3& pos) {
    m_controller.SetPosition(pos);
    SyncCamera();
}

void Player::Teleport(const Genesis::Vec3& pos) {
    m_controller.Teleport(pos);
    SyncCamera();
}

Genesis::Vec3 Player::GetEyePosition() const {
    return m_controller.GetEyePosition();
}

Genesis::Vec3 Player::GetVelocity() const {
    return m_controller.GetVelocity();
}

void Player::SyncCamera() {
    auto& camera = Genesis::Engine::Instance().GetCamera();

    // First person: camera at eye position, looking where player looks
    camera.SetPosition(m_controller.GetEyePosition());
    camera.SetYaw(m_controller.GetYaw());
    camera.SetPitch(m_controller.GetPitch());
}

void Player::DrawDebugInfo(Genesis::DebugRenderer* renderer) const {
    if (!renderer) return;

    const auto& config = m_controller.GetConfig();
    Genesis::Vec3 pos = m_controller.GetPosition();

    // Draw player capsule/AABB outline
    if (config.colliderType == Genesis::PlayerColliderType::Capsule) {
        float radius = config.capsuleRadius;
        float height = m_controller.IsCrouching() ? config.capsuleHeight * 0.5f : config.capsuleHeight;

        const int segments = 16;
        for (int i = 0; i < segments; i++) {
            float angle1 = (float)i / segments * Genesis::Math::TWO_PI;
            float angle2 = (float)(i + 1) / segments * Genesis::Math::TWO_PI;

            Genesis::Vec3 p1(pos.x + cosf(angle1) * radius, pos.y, pos.z + sinf(angle1) * radius);
            Genesis::Vec3 p2(pos.x + cosf(angle2) * radius, pos.y, pos.z + sinf(angle2) * radius);
            Genesis::Vec3 p3(pos.x + cosf(angle1) * radius, pos.y + height, pos.z + sinf(angle1) * radius);
            Genesis::Vec3 p4(pos.x + cosf(angle2) * radius, pos.y + height, pos.z + sinf(angle2) * radius);

            renderer->DrawLine(p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, 0.0f, 1.0f, 0.0f);
            renderer->DrawLine(p3.x, p3.y, p3.z, p4.x, p4.y, p4.z, 0.0f, 1.0f, 0.0f);
            if (i % 4 == 0) {
                renderer->DrawLine(p1.x, p1.y, p1.z, p3.x, p3.y, p3.z, 0.0f, 1.0f, 0.0f);
            }
        }
    } else {
        Genesis::AABB bounds = m_controller.GetAABB();
        renderer->DrawWireCube(
            (bounds.min.x + bounds.max.x) * 0.5f,
            (bounds.min.y + bounds.max.y) * 0.5f,
            (bounds.min.z + bounds.max.z) * 0.5f,
            bounds.max.x - bounds.min.x,
            0.0f, 1.0f, 0.0f
        );
    }

    // Draw velocity vector
    Genesis::Vec3 vel = m_controller.GetVelocity();
    renderer->DrawLine(
        pos.x, pos.y + 1.0f, pos.z,
        pos.x + vel.x * 0.1f, pos.y + 1.0f + vel.y * 0.1f, pos.z + vel.z * 0.1f,
        1.0f, 1.0f, 0.0f
    );

    // Draw forward direction
    Genesis::Vec3 forward = m_controller.GetForwardXZ();
    renderer->DrawLine(
        pos.x, pos.y + 0.5f, pos.z,
        pos.x + forward.x, pos.y + 0.5f, pos.z + forward.z,
        0.0f, 0.0f, 1.0f
    );

    // Draw ground point if grounded
    if (m_controller.IsGrounded()) {
        const auto& groundInfo = m_controller.GetGroundInfo();
        renderer->DrawLine(
            pos.x, pos.y, pos.z,
            groundInfo.groundPoint.x, groundInfo.groundPoint.y, groundInfo.groundPoint.z,
            0.0f, 1.0f, 1.0f
        );
    }
}

} // namespace Game

