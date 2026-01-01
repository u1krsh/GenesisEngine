#include "Camera.h"
#include <iostream>

namespace Genesis {

FPSCamera::FPSCamera() {
    UpdateVectors();
}

void FPSCamera::ProcessMouseLook(float deltaX, float deltaY) {
    // Apply sensitivity
    deltaX *= m_mouseSensitivity;
    deltaY *= m_mouseSensitivity;

    // Update yaw and pitch
    m_yaw += deltaX;
    m_pitch -= deltaY;  // Inverted: moving mouse up should look up (negative Y)

    // Clamp pitch to prevent camera flip
    m_pitch = glm::clamp(m_pitch, m_minPitch, m_maxPitch);

    // Keep yaw in reasonable range (optional, prevents float overflow over long sessions)
    if (m_yaw > 360.0f) m_yaw -= 360.0f;
    if (m_yaw < -360.0f) m_yaw += 360.0f;

    // Update direction vectors
    UpdateVectors();
}

void FPSCamera::ProcessMovement(bool forward, bool backward, bool left, bool right,
                                 bool up, bool down, float deltaTime) {
    // Calculate effective speed
    float speed = m_moveSpeed;
    if (m_isSprinting) {
        speed *= m_sprintMultiplier;
    }

    float velocity = speed * deltaTime;

    // Calculate movement direction on XZ plane (true FPS style - no flying)
    Vec3 forwardXZ = Math::Normalize(Vec3(m_forward.x, 0.0f, m_forward.z));
    Vec3 rightXZ = Math::Normalize(Vec3(m_right.x, 0.0f, m_right.z));

    // Apply movement
    if (forward) {
        m_position += forwardXZ * velocity;
    }
    if (backward) {
        m_position -= forwardXZ * velocity;
    }
    if (left) {
        m_position -= rightXZ * velocity;
    }
    if (right) {
        m_position += rightXZ * velocity;
    }

    // Vertical movement (for noclip/flying mode - can be disabled for true FPS)
    if (up) {
        m_position += m_worldUp * velocity;
    }
    if (down) {
        m_position -= m_worldUp * velocity;
    }
}

void FPSCamera::Update() {
    // Reserved for any per-frame updates (interpolation, effects, etc.)
    // Currently, vectors are updated immediately in ProcessMouseLook
}

void FPSCamera::SetPitch(float pitch) {
    m_pitch = glm::clamp(pitch, m_minPitch, m_maxPitch);
    UpdateVectors();
}

void FPSCamera::SetPitchConstraints(float minPitch, float maxPitch) {
    m_minPitch = minPitch;
    m_maxPitch = maxPitch;
    // Re-clamp current pitch
    m_pitch = glm::clamp(m_pitch, m_minPitch, m_maxPitch);
    UpdateVectors();
}

Mat4 FPSCamera::GetViewMatrix() const {
    return Math::LookAt(m_position, m_position + m_forward, m_worldUp);
}

Mat4 FPSCamera::GetProjectionMatrix() const {
    return Math::Perspective(Math::Radians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);
}

void FPSCamera::UpdateVectors() {
    // Calculate new forward vector from yaw and pitch
    float yawRad = Math::Radians(m_yaw);
    float pitchRad = Math::Radians(m_pitch);

    Vec3 forward;
    forward.x = cos(yawRad) * cos(pitchRad);
    forward.y = sin(pitchRad);
    forward.z = sin(yawRad) * cos(pitchRad);
    m_forward = Math::Normalize(forward);

    // Recalculate right and up vectors
    m_right = Math::Normalize(Math::Cross(m_forward, m_worldUp));
    m_up = Math::Normalize(Math::Cross(m_right, m_forward));
}

void FPSCamera::PrintDebugInfo() const {
    std::cout << "=== FPS Camera Debug ===" << std::endl;
    std::cout << "Position: " << Math::ToString(m_position) << std::endl;
    std::cout << "Forward:  " << Math::ToString(m_forward) << std::endl;
    std::cout << "Right:    " << Math::ToString(m_right) << std::endl;
    std::cout << "Up:       " << Math::ToString(m_up) << std::endl;
    std::cout << "Yaw: " << m_yaw << "° | Pitch: " << m_pitch << "°" << std::endl;
    std::cout << "Speed: " << m_moveSpeed << " | Sprint: " << m_sprintMultiplier << "x" << std::endl;
    std::cout << "FOV: " << m_fov << "° | Aspect: " << m_aspectRatio << std::endl;
    std::cout << "========================" << std::endl;
}

} // namespace Genesis

