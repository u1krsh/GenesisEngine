#include "Camera.h"
#include <iostream>
#include <cmath>

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
    m_pitch = Clamp(m_pitch, m_minPitch, m_maxPitch);

    // Keep yaw in reasonable range (optional, prevents float overflow over long sessions)
    while (m_yaw > 360.0f) m_yaw -= 360.0f;
    while (m_yaw < -360.0f) m_yaw += 360.0f;

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
    Vec3 forwardXZ = {m_forward.x, 0.0f, m_forward.z};
    forwardXZ.Normalize();

    Vec3 rightXZ = {m_right.x, 0.0f, m_right.z};
    rightXZ.Normalize();

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
    m_pitch = Clamp(pitch, m_minPitch, m_maxPitch);
    UpdateVectors();
}

void FPSCamera::SetPitchConstraints(float minPitch, float maxPitch) {
    m_minPitch = minPitch;
    m_maxPitch = maxPitch;
    // Re-clamp current pitch
    m_pitch = Clamp(m_pitch, m_minPitch, m_maxPitch);
    UpdateVectors();
}

Mat4 FPSCamera::GetViewMatrix() const {
    Vec3 target = m_position + m_forward;
    return Mat4::LookAt(m_position, target, m_worldUp);
}

Mat4 FPSCamera::GetProjectionMatrix() const {
    return Mat4::Perspective(DegreesToRadians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);
}

void FPSCamera::UpdateVectors() {
    // Calculate new forward vector from yaw and pitch
    float yawRad = DegreesToRadians(m_yaw);
    float pitchRad = DegreesToRadians(m_pitch);

    Vec3 forward;
    forward.x = std::cos(yawRad) * std::cos(pitchRad);
    forward.y = std::sin(pitchRad);
    forward.z = std::sin(yawRad) * std::cos(pitchRad);
    m_forward = forward.Normalized();

    // Recalculate right and up vectors
    m_right = Vec3::Cross(m_forward, m_worldUp).Normalized();
    m_up = Vec3::Cross(m_right, m_forward).Normalized();
}

float FPSCamera::Clamp(float value, float min, float max) const {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void FPSCamera::PrintDebugInfo() const {
    std::cout << "=== FPS Camera Debug ===" << std::endl;
    std::cout << "Position: (" << m_position.x << ", " << m_position.y << ", " << m_position.z << ")" << std::endl;
    std::cout << "Forward:  (" << m_forward.x << ", " << m_forward.y << ", " << m_forward.z << ")" << std::endl;
    std::cout << "Right:    (" << m_right.x << ", " << m_right.y << ", " << m_right.z << ")" << std::endl;
    std::cout << "Up:       (" << m_up.x << ", " << m_up.y << ", " << m_up.z << ")" << std::endl;
    std::cout << "Yaw: " << m_yaw << "° | Pitch: " << m_pitch << "°" << std::endl;
    std::cout << "Speed: " << m_moveSpeed << " | Sprint: " << m_sprintMultiplier << "x" << std::endl;
    std::cout << "FOV: " << m_fov << "° | Aspect: " << m_aspectRatio << std::endl;
    std::cout << "========================" << std::endl;
}

} // namespace Genesis

