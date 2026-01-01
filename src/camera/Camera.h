#pragma once

#include "math/Math.h"

namespace Genesis {

// ============================================================================
// FPS Camera - First Person Shooter style camera using GLM
// ============================================================================
class FPSCamera {
public:
    FPSCamera();
    ~FPSCamera() = default;

    // ========================================================================
    // Core Update
    // ========================================================================

    // Process mouse input for looking around
    void ProcessMouseLook(float deltaX, float deltaY);

    // Process keyboard input for movement (call with deltaTime)
    void ProcessMovement(bool forward, bool backward, bool left, bool right,
                         bool up, bool down, float deltaTime);

    // Update internal state (call once per frame after input processing)
    void Update();

    // ========================================================================
    // Getters
    // ========================================================================

    const Vec3& GetPosition() const { return m_position; }
    const Vec3& GetForward() const { return m_forward; }
    const Vec3& GetRight() const { return m_right; }
    const Vec3& GetUp() const { return m_up; }

    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }

    // Get the view matrix (for rendering)
    Mat4 GetViewMatrix() const;

    // Get projection matrix
    Mat4 GetProjectionMatrix() const;

    // ========================================================================
    // Setters
    // ========================================================================

    void SetPosition(const Vec3& position) { m_position = position; }
    void SetPosition(float x, float y, float z) { m_position = Vec3(x, y, z); }

    void SetYaw(float yaw) { m_yaw = yaw; UpdateVectors(); }
    void SetPitch(float pitch);

    // Movement speed (units per second)
    void SetMoveSpeed(float speed) { m_moveSpeed = speed; }
    float GetMoveSpeed() const { return m_moveSpeed; }

    // Sprint multiplier
    void SetSprintMultiplier(float mult) { m_sprintMultiplier = mult; }
    float GetSprintMultiplier() const { return m_sprintMultiplier; }
    void SetSprinting(bool sprinting) { m_isSprinting = sprinting; }
    bool IsSprinting() const { return m_isSprinting; }

    // Mouse sensitivity
    void SetMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }
    float GetMouseSensitivity() const { return m_mouseSensitivity; }

    // Pitch constraints
    void SetPitchConstraints(float minPitch, float maxPitch);
    float GetMinPitch() const { return m_minPitch; }
    float GetMaxPitch() const { return m_maxPitch; }

    // Field of view (in degrees)
    void SetFOV(float fovDegrees) { m_fov = fovDegrees; }
    float GetFOV() const { return m_fov; }

    // Aspect ratio
    void SetAspectRatio(float aspect) { m_aspectRatio = aspect; }
    float GetAspectRatio() const { return m_aspectRatio; }

    // Near/Far planes
    void SetNearPlane(float nearPlane) { m_nearPlane = nearPlane; }
    void SetFarPlane(float farPlane) { m_farPlane = farPlane; }
    float GetNearPlane() const { return m_nearPlane; }
    float GetFarPlane() const { return m_farPlane; }

    // ========================================================================
    // Debug
    // ========================================================================
    void PrintDebugInfo() const;

private:
    void UpdateVectors();

private:
    // Position
    Vec3 m_position = Vec3(0.0f, 0.0f, 0.0f);

    // Direction vectors
    Vec3 m_forward = Vec3(0.0f, 0.0f, -1.0f);
    Vec3 m_right = Vec3(1.0f, 0.0f, 0.0f);
    Vec3 m_up = Vec3(0.0f, 1.0f, 0.0f);

    // World up (for calculations)
    Vec3 m_worldUp = Vec3(0.0f, 1.0f, 0.0f);

    // Euler angles (in degrees)
    float m_yaw = -90.0f;    // Yaw: rotation around Y axis (look left/right)
    float m_pitch = 0.0f;    // Pitch: rotation around X axis (look up/down)

    // Pitch constraints (in degrees)
    float m_minPitch = -89.0f;
    float m_maxPitch = 89.0f;

    // Movement
    float m_moveSpeed = 5.0f;          // Units per second
    float m_sprintMultiplier = 2.0f;   // Speed multiplier when sprinting
    bool m_isSprinting = false;

    // Mouse look
    float m_mouseSensitivity = 0.1f;   // Degrees per pixel

    // Projection
    float m_fov = 70.0f;               // Field of view in degrees
    float m_aspectRatio = 16.0f / 9.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 1000.0f;
};

} // namespace Genesis

