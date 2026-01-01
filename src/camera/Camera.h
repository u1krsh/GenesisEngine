#pragma once

#include <cmath>

namespace Genesis {

// ============================================================================
// Simple 3D Vector (minimal, no external dependencies)
// ============================================================================
struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vec3 operator-(const Vec3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vec3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    Vec3 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }

    Vec3& operator+=(const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }
    Vec3& operator-=(const Vec3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
    Vec3& operator*=(float scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }

    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float LengthSquared() const { return x * x + y * y + z * z; }

    Vec3 Normalized() const {
        float len = Length();
        if (len > 0.0001f) return *this / len;
        return {0.0f, 0.0f, 0.0f};
    }

    void Normalize() {
        float len = Length();
        if (len > 0.0001f) {
            x /= len; y /= len; z /= len;
        }
    }

    static float Dot(const Vec3& a, const Vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vec3 Cross(const Vec3& a, const Vec3& b) {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }
};

// ============================================================================
// 4x4 Matrix (column-major, OpenGL compatible)
// ============================================================================
struct Mat4 {
    float m[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    Mat4() = default;

    // Access element at row r, column c
    float& At(int r, int c) { return m[c * 4 + r]; }
    float At(int r, int c) const { return m[c * 4 + r]; }

    // Get pointer to data (for OpenGL)
    const float* Data() const { return m; }
    float* Data() { return m; }

    static Mat4 Identity() {
        return Mat4();
    }

    static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& worldUp) {
        Vec3 forward = (target - eye).Normalized();
        Vec3 right = Vec3::Cross(forward, worldUp).Normalized();
        Vec3 up = Vec3::Cross(right, forward);

        Mat4 result;
        result.At(0, 0) = right.x;
        result.At(0, 1) = right.y;
        result.At(0, 2) = right.z;
        result.At(0, 3) = -Vec3::Dot(right, eye);

        result.At(1, 0) = up.x;
        result.At(1, 1) = up.y;
        result.At(1, 2) = up.z;
        result.At(1, 3) = -Vec3::Dot(up, eye);

        result.At(2, 0) = -forward.x;
        result.At(2, 1) = -forward.y;
        result.At(2, 2) = -forward.z;
        result.At(2, 3) = Vec3::Dot(forward, eye);

        result.At(3, 0) = 0.0f;
        result.At(3, 1) = 0.0f;
        result.At(3, 2) = 0.0f;
        result.At(3, 3) = 1.0f;

        return result;
    }

    static Mat4 Perspective(float fovRadians, float aspect, float nearPlane, float farPlane) {
        float tanHalfFov = std::tan(fovRadians / 2.0f);

        Mat4 result;
        for (int i = 0; i < 16; i++) result.m[i] = 0.0f;

        result.At(0, 0) = 1.0f / (aspect * tanHalfFov);
        result.At(1, 1) = 1.0f / tanHalfFov;
        result.At(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
        result.At(2, 3) = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        result.At(3, 2) = -1.0f;

        return result;
    }
};

// ============================================================================
// FPS Camera - First Person Shooter style camera
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
    void SetPosition(float x, float y, float z) { m_position = {x, y, z}; }

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
    float DegreesToRadians(float degrees) const { return degrees * 3.14159265359f / 180.0f; }
    float Clamp(float value, float min, float max) const;

private:
    // Position
    Vec3 m_position{0.0f, 0.0f, 0.0f};

    // Direction vectors
    Vec3 m_forward{0.0f, 0.0f, -1.0f};
    Vec3 m_right{1.0f, 0.0f, 0.0f};
    Vec3 m_up{0.0f, 1.0f, 0.0f};

    // World up (for calculations)
    Vec3 m_worldUp{0.0f, 1.0f, 0.0f};

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

