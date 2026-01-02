#pragma once

// ============================================================================
// Genesis Engine Math Library
// Wrapper around GLM for convenient math operations
// ============================================================================

// GLM Configuration
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  // Vulkan-style depth (also works with GL)
#define GLM_ENABLE_EXPERIMENTAL

// GLM Core
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/string_cast.hpp>

namespace Genesis {

// ============================================================================
// Type Aliases - Use these throughout the engine
// ============================================================================

// Vectors
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;

// Integer vectors
using IVec2 = glm::ivec2;
using IVec3 = glm::ivec3;
using IVec4 = glm::ivec4;

// Unsigned integer vectors
using UVec2 = glm::uvec2;
using UVec3 = glm::uvec3;
using UVec4 = glm::uvec4;

// Matrices
using Mat2 = glm::mat2;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

// Quaternion
using Quat = glm::quat;

// ============================================================================
// AABB (Axis-Aligned Bounding Box) Structure
// ============================================================================
struct AABB {
    Vec3 min;
    Vec3 max;

    AABB() : min(0.0f), max(0.0f) {}
    AABB(const Vec3& minPoint, const Vec3& maxPoint) : min(minPoint), max(maxPoint) {}

    // Create AABB from center and half-extents
    static AABB FromCenterExtents(const Vec3& center, const Vec3& halfExtents) {
        return AABB(center - halfExtents, center + halfExtents);
    }

    Vec3 GetCenter() const { return (min + max) * 0.5f; }
    Vec3 GetExtents() const { return (max - min) * 0.5f; }
    Vec3 GetSize() const { return max - min; }

    bool Contains(const Vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }

    bool Intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }
};

// ============================================================================
// Math Constants
// ============================================================================
namespace Math {
    constexpr float PI = 3.14159265358979323846f;
    constexpr float TWO_PI = PI * 2.0f;
    constexpr float HALF_PI = PI / 2.0f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    constexpr float EPSILON = 1e-6f;

    // ========================================================================
    // Conversion Functions
    // ========================================================================

    inline float Radians(float degrees) {
        return glm::radians(degrees);
    }

    inline float Degrees(float radians) {
        return glm::degrees(radians);
    }

    // ========================================================================
    // Vector Operations
    // ========================================================================

    // Dot product
    inline float Dot(const Vec2& a, const Vec2& b) { return glm::dot(a, b); }
    inline float Dot(const Vec3& a, const Vec3& b) { return glm::dot(a, b); }
    inline float Dot(const Vec4& a, const Vec4& b) { return glm::dot(a, b); }

    // Cross product
    inline Vec3 Cross(const Vec3& a, const Vec3& b) { return glm::cross(a, b); }

    // Length
    inline float Length(const Vec2& v) { return glm::length(v); }
    inline float Length(const Vec3& v) { return glm::length(v); }
    inline float Length(const Vec4& v) { return glm::length(v); }

    // Length squared (faster, no sqrt)
    inline float LengthSquared(const Vec2& v) { return glm::length2(v); }
    inline float LengthSquared(const Vec3& v) { return glm::length2(v); }
    inline float LengthSquared(const Vec4& v) { return glm::length2(v); }

    // Normalize
    inline Vec2 Normalize(const Vec2& v) { return glm::normalize(v); }
    inline Vec3 Normalize(const Vec3& v) { return glm::normalize(v); }
    inline Vec4 Normalize(const Vec4& v) { return glm::normalize(v); }

    // Safe normalize (returns zero vector if input is zero)
    inline Vec3 SafeNormalize(const Vec3& v) {
        float len = glm::length(v);
        if (len > EPSILON) return v / len;
        return Vec3(0.0f);
    }

    // Distance
    inline float Distance(const Vec2& a, const Vec2& b) { return glm::distance(a, b); }
    inline float Distance(const Vec3& a, const Vec3& b) { return glm::distance(a, b); }

    // Lerp (linear interpolation)
    inline float Lerp(float a, float b, float t) { return glm::mix(a, b, t); }
    inline Vec2 Lerp(const Vec2& a, const Vec2& b, float t) { return glm::mix(a, b, t); }
    inline Vec3 Lerp(const Vec3& a, const Vec3& b, float t) { return glm::mix(a, b, t); }
    inline Vec4 Lerp(const Vec4& a, const Vec4& b, float t) { return glm::mix(a, b, t); }

    // Clamp
    inline float Clamp(float value, float min, float max) { return glm::clamp(value, min, max); }
    inline Vec3 Clamp(const Vec3& value, const Vec3& min, const Vec3& max) { return glm::clamp(value, min, max); }

    // Min/Max
    inline float Min(float a, float b) { return glm::min(a, b); }
    inline float Max(float a, float b) { return glm::max(a, b); }
    inline Vec3 Min(const Vec3& a, const Vec3& b) { return glm::min(a, b); }
    inline Vec3 Max(const Vec3& a, const Vec3& b) { return glm::max(a, b); }

    // Reflect
    inline Vec3 Reflect(const Vec3& incident, const Vec3& normal) {
        return glm::reflect(incident, normal);
    }

    // ========================================================================
    // Matrix Operations
    // ========================================================================

    // Identity matrix
    inline Mat4 Identity() { return glm::mat4(1.0f); }

    // Translation
    inline Mat4 Translate(const Vec3& translation) {
        return glm::translate(glm::mat4(1.0f), translation);
    }
    inline Mat4 Translate(const Mat4& matrix, const Vec3& translation) {
        return glm::translate(matrix, translation);
    }

    // Rotation (angle in radians)
    inline Mat4 Rotate(float angleRadians, const Vec3& axis) {
        return glm::rotate(glm::mat4(1.0f), angleRadians, axis);
    }
    inline Mat4 Rotate(const Mat4& matrix, float angleRadians, const Vec3& axis) {
        return glm::rotate(matrix, angleRadians, axis);
    }

    // Scale
    inline Mat4 Scale(const Vec3& scale) {
        return glm::scale(glm::mat4(1.0f), scale);
    }
    inline Mat4 Scale(const Mat4& matrix, const Vec3& scale) {
        return glm::scale(matrix, scale);
    }

    // LookAt matrix (for cameras)
    inline Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        return glm::lookAt(eye, target, up);
    }

    // Perspective projection
    inline Mat4 Perspective(float fovRadians, float aspectRatio, float nearPlane, float farPlane) {
        return glm::perspective(fovRadians, aspectRatio, nearPlane, farPlane);
    }

    // Orthographic projection
    inline Mat4 Ortho(float left, float right, float bottom, float top, float nearPlane, float farPlane) {
        return glm::ortho(left, right, bottom, top, nearPlane, farPlane);
    }

    // Inverse
    inline Mat4 Inverse(const Mat4& matrix) { return glm::inverse(matrix); }
    inline Mat3 Inverse(const Mat3& matrix) { return glm::inverse(matrix); }

    // Transpose
    inline Mat4 Transpose(const Mat4& matrix) { return glm::transpose(matrix); }
    inline Mat3 Transpose(const Mat3& matrix) { return glm::transpose(matrix); }

    // Get pointer to matrix data (for OpenGL)
    inline const float* ValuePtr(const Mat4& matrix) { return glm::value_ptr(matrix); }
    inline const float* ValuePtr(const Mat3& matrix) { return glm::value_ptr(matrix); }
    inline const float* ValuePtr(const Vec3& vector) { return glm::value_ptr(vector); }
    inline const float* ValuePtr(const Vec4& vector) { return glm::value_ptr(vector); }

    // ========================================================================
    // Quaternion Operations
    // ========================================================================

    // Create quaternion from axis-angle (angle in radians)
    inline Quat QuatFromAxisAngle(const Vec3& axis, float angleRadians) {
        return glm::angleAxis(angleRadians, axis);
    }

    // Create quaternion from euler angles (in radians)
    inline Quat QuatFromEuler(float pitch, float yaw, float roll) {
        return glm::quat(Vec3(pitch, yaw, roll));
    }

    // Convert quaternion to matrix
    inline Mat4 QuatToMat4(const Quat& q) {
        return glm::toMat4(q);
    }

    // Quaternion slerp (spherical linear interpolation)
    inline Quat Slerp(const Quat& a, const Quat& b, float t) {
        return glm::slerp(a, b, t);
    }

    // Normalize quaternion
    inline Quat Normalize(const Quat& q) {
        return glm::normalize(q);
    }

    // ========================================================================
    // Utility Functions
    // ========================================================================

    // Smoothstep
    inline float Smoothstep(float edge0, float edge1, float x) {
        return glm::smoothstep(edge0, edge1, x);
    }

    // Abs
    inline float Abs(float x) { return glm::abs(x); }
    inline Vec3 Abs(const Vec3& v) { return glm::abs(v); }

    // Floor/Ceil
    inline float Floor(float x) { return glm::floor(x); }
    inline float Ceil(float x) { return glm::ceil(x); }

    // Mod
    inline float Mod(float x, float y) { return glm::mod(x, y); }

    // Fract
    inline float Fract(float x) { return glm::fract(x); }

    // Sign
    inline float Sign(float x) { return glm::sign(x); }

    // ========================================================================
    // String Conversion (for debugging)
    // ========================================================================
    inline std::string ToString(const Vec2& v) { return glm::to_string(v); }
    inline std::string ToString(const Vec3& v) { return glm::to_string(v); }
    inline std::string ToString(const Vec4& v) { return glm::to_string(v); }
    inline std::string ToString(const Mat4& m) { return glm::to_string(m); }
    inline std::string ToString(const Quat& q) { return glm::to_string(q); }

} // namespace Math

// ============================================================================
// Common Vector Constants
// ============================================================================
namespace Vectors {
    const Vec3 Zero    = Vec3(0.0f, 0.0f, 0.0f);
    const Vec3 One     = Vec3(1.0f, 1.0f, 1.0f);
    const Vec3 Up      = Vec3(0.0f, 1.0f, 0.0f);
    const Vec3 Down    = Vec3(0.0f, -1.0f, 0.0f);
    const Vec3 Left    = Vec3(-1.0f, 0.0f, 0.0f);
    const Vec3 Right   = Vec3(1.0f, 0.0f, 0.0f);
    const Vec3 Forward = Vec3(0.0f, 0.0f, -1.0f);  // OpenGL convention: -Z is forward
    const Vec3 Back    = Vec3(0.0f, 0.0f, 1.0f);
} // namespace Vectors

} // namespace Genesis

