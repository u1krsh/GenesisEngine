#pragma once

#include "math/Math.h"
#include <functional>
#include <vector>

namespace Genesis {

// ============================================================================
// Collision Shape Types
// ============================================================================
// Player Collider Type (legacy - use physics/Collider.h for new code)
// ============================================================================
enum class PlayerColliderType {
    Capsule,
    Box
};

// AABB is now defined in math/Math.h

// ============================================================================
// Capsule Structure (for player collision)
// ============================================================================
struct Capsule {
    Vec3 base;      // Bottom center of capsule
    float radius;   // Capsule radius
    float height;   // Total height (includes both hemispheres)

    Capsule() : base(0.0f), radius(0.3f), height(1.8f) {}
    Capsule(const Vec3& basePos, float r, float h) : base(basePos), radius(r), height(h) {}

    // Get the center of the capsule
    Vec3 GetCenter() const { return base + Vec3(0.0f, height * 0.5f, 0.0f); }

    // Get the top point
    Vec3 GetTop() const { return base + Vec3(0.0f, height, 0.0f); }

    // Get AABB that contains this capsule
    AABB GetBoundingAABB() const {
        return AABB(
            base - Vec3(radius, 0.0f, radius),
            base + Vec3(radius, height, radius)
        );
    }
};

// ============================================================================
// Ground Detection Result
// ============================================================================
struct GroundInfo {
    bool isGrounded = false;
    Vec3 groundNormal = Vec3(0.0f, 1.0f, 0.0f);
    float groundDistance = 0.0f;
    Vec3 groundPoint = Vec3(0.0f);
    bool isOnSlope = false;
    float slopeAngle = 0.0f;
};

// ============================================================================
// Player Controller Configuration
// ============================================================================
struct PlayerControllerConfig {
    // Collider settings
    PlayerColliderType colliderType = PlayerColliderType::Capsule;
    float capsuleRadius = 0.3f;
    float capsuleHeight = 1.8f;  // Total height
    Vec3 aabbHalfExtents = Vec3(0.3f, 0.9f, 0.3f);

    // === Source-style Movement Settings ===
    // Max speeds
    float walkSpeed = 5.0f;       // cl_forwardspeed equivalent
    float sprintSpeed = 7.5f;     // Sprint multiplier
    float crouchSpeed = 2.5f;     // Crouch speed

    // Ground movement (Source-style)
    float groundAccelerate = 15.0f;   // sv_accelerate equivalent (higher = snappier)
    float groundFriction = 12.0f;     // sv_friction equivalent (higher = stops faster)
    float stopSpeed = 3.0f;           // sv_stopspeed - below this speed, friction applies fully

    // Air movement (Source-style)
    float airAccelerate = 10.0f;      // sv_airaccelerate (10-12 for good strafing)
    float airSpeedCap = 0.7f;         // Limits speed gain per strafe (lower = more bhop potential)
    float airFriction = 0.0f;         // Usually 0 in Source for bunny hopping

    // Legacy (for compatibility)
    float acceleration = 50.0f;
    float deceleration = 40.0f;
    float airControl = 0.3f;

    // Jump settings
    float jumpForce = 8.0f;
    float jumpCooldown = 0.1f;
    int maxAirJumps = 0;  // 0 = single jump, 1 = double jump, etc.

    // Gravity settings
    float gravity = 20.0f;
    float maxFallSpeed = 50.0f;

    // Ground detection
    float groundCheckDistance = 0.1f;
    float maxSlopeAngle = 45.0f;  // Maximum walkable slope in degrees

    // Step handling
    float stepHeight = 0.35f;    // Max height to automatically step up
    float stepSearchDistance = 0.5f;  // How far forward to check for steps
    float autoClimbStairHeight = 0.5f;  // Max stair height for auto-climb (stairs tagged as Stair)
    float stairClimbSpeed = 8.0f;  // Speed of auto stair climbing

    // Camera settings
    float eyeHeight = 1.6f;  // Height of camera from player base
    float crouchEyeHeight = 0.9f;
};

// ============================================================================
// Player Controller - Handles player physics and movement
// ============================================================================
class PlayerController {
public:
    PlayerController();
    ~PlayerController() = default;

    // ========================================================================
    // Initialization
    // ========================================================================
    void Initialize(const PlayerControllerConfig& config = PlayerControllerConfig());
    void Reset();

    // ========================================================================
    // Update
    // ========================================================================
    void Update(float deltaTime);

    // ========================================================================
    // Movement Input
    // ========================================================================
    void SetMoveInput(const Vec3& input);  // Normalized XZ input (-1 to 1)
    void SetLookDirection(float yaw, float pitch);
    void Jump();
    void SetSprinting(bool sprinting);
    void SetCrouching(bool crouching);

    // ========================================================================
    // Position/State
    // ========================================================================
    const Vec3& GetPosition() const { return m_position; }
    void SetPosition(const Vec3& position);
    void Teleport(const Vec3& position);

    const Vec3& GetVelocity() const { return m_velocity; }
    void SetVelocity(const Vec3& velocity) { m_velocity = velocity; }
    void AddVelocity(const Vec3& velocity) { m_velocity += velocity; }

    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }

    // Get camera position (eye position)
    Vec3 GetEyePosition() const;

    // Get forward direction on XZ plane
    Vec3 GetForwardXZ() const;
    Vec3 GetRightXZ() const;

    // ========================================================================
    // State Queries
    // ========================================================================
    bool IsGrounded() const { return m_groundInfo.isGrounded; }
    bool IsOnSlope() const { return m_groundInfo.isOnSlope; }
    bool IsSprinting() const { return m_isSprinting && m_isMoving; }
    bool IsCrouching() const { return m_isCrouching; }
    bool IsJumping() const { return m_isJumping; }
    bool IsFalling() const { return !m_groundInfo.isGrounded && m_velocity.y < 0; }

    const GroundInfo& GetGroundInfo() const { return m_groundInfo; }

    // ========================================================================
    // Collision
    // ========================================================================
    const Capsule& GetCapsule() const { return m_capsule; }
    AABB GetAABB() const;

    // Ground/World collision callback - return true if position is blocked
    using CollisionCallback = std::function<bool(const Vec3& position, const AABB& bounds)>;
    void SetCollisionCallback(CollisionCallback callback) { m_collisionCallback = callback; }

    // Ground height callback - returns ground height at XZ position, playerY is current player height
    using GroundHeightCallback = std::function<float(float x, float z, float playerY)>;
    void SetGroundHeightCallback(GroundHeightCallback callback) { m_groundHeightCallback = callback; }

    // Depenetration callback - returns push direction if stuck
    using DepenetrationCallback = std::function<bool(const AABB& bounds, Vec3& pushOut)>;
    void SetDepenetrationCallback(DepenetrationCallback callback) { m_depenetrationCallback = callback; }

    // Stair climb callback - returns stair top height to auto-climb, or -1 if no stair
    // Parameters: x, z, playerY, radius, maxStairHeight, moveDirection
    using StairClimbCallback = std::function<float(float x, float z, float playerY, float radius, float maxHeight, const Vec3& moveDir)>;
    void SetStairClimbCallback(StairClimbCallback callback) { m_stairClimbCallback = callback; }

    // Enable/disable auto stair climbing
    void SetAutoClimbStairs(bool enabled) { m_autoClimbStairs = enabled; }
    bool GetAutoClimbStairs() const { return m_autoClimbStairs; }

    // ========================================================================
    // Configuration
    // ========================================================================
    const PlayerControllerConfig& GetConfig() const { return m_config; }
    void SetConfig(const PlayerControllerConfig& config);

private:
    // Internal update methods
    void ApplyGravity(float deltaTime);
    void ApplyMovement(float deltaTime);
    void ApplyFriction(float deltaTime);
    void ApplyGroundAcceleration(const Vec3& wishDir, float wishSpeed, float deltaTime);
    void ApplyAirAcceleration(const Vec3& wishDir, float wishSpeed, float deltaTime);
    void CheckGroundCollision();
    void HandleStepUp(float deltaTime);
    void HandleSlopes();
    void UpdateCapsule();

    // Helper methods
    bool CheckCollision(const Vec3& position) const;
    float GetGroundHeight(float x, float z, float playerY) const;
    Vec3 ResolveCollision(const Vec3& desiredPosition);

private:
    // Configuration
    PlayerControllerConfig m_config;

    // Position and movement
    Vec3 m_position = Vec3(0.0f);
    Vec3 m_velocity = Vec3(0.0f);
    Vec3 m_moveInput = Vec3(0.0f);

    // Orientation (yaw and pitch in degrees)
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;

    // Collision shape
    Capsule m_capsule;

    // Ground detection
    GroundInfo m_groundInfo;

    // State flags
    bool m_isSprinting = false;
    bool m_isCrouching = false;
    bool m_isJumping = false;
    bool m_isMoving = false;
    bool m_wantsToJump = false;

    // Jump state
    float m_jumpCooldownTimer = 0.0f;
    int m_airJumpsRemaining = 0;

    // Crouch state
    float m_currentHeight = 0.0f;  // For smooth crouch transitions

    // Auto stair climbing
    bool m_autoClimbStairs = true;

    // Callbacks
    CollisionCallback m_collisionCallback;
    GroundHeightCallback m_groundHeightCallback;
    DepenetrationCallback m_depenetrationCallback;
    StairClimbCallback m_stairClimbCallback;
};

} // namespace Genesis

