#pragma once

// ============================================================================
// OracularV2 Editor Camera
// Simple camera supporting both perspective and orthographic projections
// ============================================================================

#include "math/Math.h"

// ============================================================================
// EditorCamera - Camera for viewport rendering
// ============================================================================
class EditorCamera {
public:
    EditorCamera() = default;
    
    // ========================================================================
    // Projection setup
    // ========================================================================
    
    void SetPerspective(float fovDegrees, float aspect, float nearPlane, float farPlane) {
        m_isPerspective = true;
        m_fov = fovDegrees;
        m_aspect = aspect;
        m_near = nearPlane;
        m_far = farPlane;
        UpdateProjection();
    }
    
    void SetOrthographic(float left, float right, float bottom, float top, 
                         float nearPlane, float farPlane) {
        m_isPerspective = false;
        m_orthoLeft = left;
        m_orthoRight = right;
        m_orthoBottom = bottom;
        m_orthoTop = top;
        m_near = nearPlane;
        m_far = farPlane;
        UpdateProjection();
    }
    
    void SetProjection(float fov, float aspect, float nearPlane, float farPlane) {
        SetPerspective(fov, aspect, nearPlane, farPlane);
    }
    
    // ========================================================================
    // Position/orientation
    // ========================================================================
    
    void SetPosition(const Genesis::Vec3& pos) {
        m_position = pos;
        UpdateViewMatrix();
    }
    
    void LookAt(const Genesis::Vec3& target, const Genesis::Vec3& up = Genesis::Vec3(0, 1, 0)) {
        m_target = target;
        m_up = up;
        UpdateViewMatrix();
    }
    
    const Genesis::Vec3& GetPosition() const { return m_position; }
    const Genesis::Vec3& GetTarget() const { return m_target; }
    const Genesis::Vec3& GetUp() const { return m_up; }
    
    Genesis::Vec3 GetForward() const { 
        return glm::normalize(m_target - m_position); 
    }
    
    // ========================================================================
    // Matrices
    // ========================================================================
    
    Genesis::Mat4 GetViewMatrix() const { return m_viewMatrix; }
    Genesis::Mat4 GetProjectionMatrix() const { return m_projMatrix; }
    Genesis::Mat4 GetViewProjectionMatrix() const { return m_projMatrix * m_viewMatrix; }
    
    // ========================================================================
    // Queries
    // ========================================================================
    
    bool IsPerspective() const { return m_isPerspective; }
    
private:
    void UpdateViewMatrix() {
        m_viewMatrix = glm::lookAt(m_position, m_target, m_up);
    }
    
    void UpdateProjection() {
        if (m_isPerspective) {
            m_projMatrix = glm::perspective(glm::radians(m_fov), m_aspect, m_near, m_far);
        } else {
            m_projMatrix = glm::ortho(m_orthoLeft, m_orthoRight, m_orthoBottom, m_orthoTop, m_near, m_far);
        }
    }

private:
    Genesis::Vec3 m_position{0, 0, 10};
    Genesis::Vec3 m_target{0, 0, 0};
    Genesis::Vec3 m_up{0, 1, 0};
    
    Genesis::Mat4 m_viewMatrix{1.0f};
    Genesis::Mat4 m_projMatrix{1.0f};
    
    bool m_isPerspective = true;
    
    // Perspective params
    float m_fov = 60.0f;
    float m_aspect = 16.0f / 9.0f;
    
    // Ortho params
    float m_orthoLeft = -100.0f;
    float m_orthoRight = 100.0f;
    float m_orthoBottom = -100.0f;
    float m_orthoTop = 100.0f;
    
    // Common
    float m_near = 0.1f;
    float m_far = 10000.0f;
};
