#include "PlayerController.h"
#include <cmath>
#include <algorithm>

namespace Genesis {

PlayerController::PlayerController() {
    m_currentHeight = m_config.capsuleHeight;
}

void PlayerController::Initialize(const PlayerControllerConfig& config) {
    m_config = config;
    m_currentHeight = config.capsuleHeight;
    UpdateCapsule();
    Reset();
}

void PlayerController::Reset() {
    m_velocity = Vec3(0.0f);
    m_moveInput = Vec3(0.0f);
    m_isSprinting = false;
    m_isCrouching = false;
    m_isJumping = false;
    m_isMoving = false;
    m_wantsToJump = false;
    m_jumpCooldownTimer = 0.0f;
    m_airJumpsRemaining = m_config.maxAirJumps;
    m_groundInfo = GroundInfo();
}

void PlayerController::SetConfig(const PlayerControllerConfig& config) {
    m_config = config;
    UpdateCapsule();
}

void PlayerController::Update(float deltaTime) {
    // Update jump cooldown
    if (m_jumpCooldownTimer > 0.0f) {
        m_jumpCooldownTimer -= deltaTime;
    }

    // Handle jump request BEFORE ground check so we can leave the ground
    if (m_wantsToJump) {
        if (m_jumpCooldownTimer <= 0.0f) {
            bool canJump = false;

            if (m_groundInfo.isGrounded) {
                canJump = true;
            } else if (m_airJumpsRemaining > 0) {
                canJump = true;
                m_airJumpsRemaining--;
            }

            if (canJump) {
                m_velocity.y = m_config.jumpForce;
                m_isJumping = true;
                m_jumpCooldownTimer = m_config.jumpCooldown;
                m_groundInfo.isGrounded = false;
            }
        }
        m_wantsToJump = false;
    }

    // Apply gravity only if not grounded or jumping up
    if (!m_groundInfo.isGrounded || m_isJumping) {
        m_velocity.y -= m_config.gravity * deltaTime;
        m_velocity.y = std::max(m_velocity.y, -m_config.maxFallSpeed);
    }

    // Apply horizontal movement
    ApplyMovement(deltaTime);

    // Calculate desired new position
    Vec3 newPosition = m_position + m_velocity * deltaTime;

    // === AUTO STAIR CLIMBING ===
    // Check for tagged stairs and auto-climb them
    if (m_autoClimbStairs && m_stairClimbCallback && m_groundInfo.isGrounded && m_isMoving) {
        // Get movement direction
        Vec3 moveDir = Vec3(0.0f);
        float horizSpeed = std::sqrt(m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z);
        if (horizSpeed > 0.1f) {
            moveDir = Vec3(m_velocity.x / horizSpeed, 0.0f, m_velocity.z / horizSpeed);
        }

        if (Math::Length(moveDir) > 0.01f) {
            float stairTop = m_stairClimbCallback(
                newPosition.x, newPosition.z, m_position.y,
                m_config.capsuleRadius, m_config.autoClimbStairHeight, moveDir
            );

            if (stairTop > 0.0f && stairTop > m_position.y) {
                // Found a stair to climb
                float heightDiff = stairTop - m_position.y;
                float climbAmount = m_config.stairClimbSpeed * deltaTime;

                if (climbAmount >= heightDiff) {
                    // Snap to stair top
                    newPosition.y = stairTop;
                } else {
                    // Smooth climb
                    newPosition.y = m_position.y + climbAmount;
                }

                // Keep grounded during stair climb
                m_groundInfo.isGrounded = true;
                m_velocity.y = 0.0f;
            }
        }
    }

    // === HORIZONTAL COLLISION (sides of cubes) ===
    // Use a collision check that ignores the bottom part of the player
    // This prevents getting stuck on edges of blocks we're standing on
    if (m_collisionCallback) {
        // Create AABB that's raised slightly off the ground to avoid false collisions
        float stepOffset = m_config.stepHeight + 0.15f;  // Increased offset
        float checkHeight = m_currentHeight - stepOffset;

        if (checkHeight > 0.1f) {
            Vec3 checkCenter = Vec3(newPosition.x, m_position.y + stepOffset + checkHeight * 0.5f, newPosition.z);
            Vec3 checkExtents = Vec3(m_config.capsuleRadius * 0.95f, checkHeight * 0.5f, m_config.capsuleRadius * 0.95f);  // Slightly smaller
            AABB horizontalBounds = AABB::FromCenterExtents(checkCenter, checkExtents);

            bool blocked = m_collisionCallback(newPosition, horizontalBounds);

            if (blocked) {
                // Use separate-axis collision resolution
                // This prevents getting stuck on corners/edges

                // First, try the full movement but with a very small AABB to detect which direction is blocked
                const float epsilon = 0.001f;

                // Test X movement independently
                Vec3 xTestCenter = Vec3(newPosition.x, m_position.y + stepOffset + checkHeight * 0.5f, m_position.z);
                AABB xTestBounds = AABB::FromCenterExtents(xTestCenter, checkExtents);
                bool xBlocked = m_collisionCallback(Vec3(newPosition.x, m_position.y, m_position.z), xTestBounds);

                // Test Z movement independently
                Vec3 zTestCenter = Vec3(m_position.x, m_position.y + stepOffset + checkHeight * 0.5f, newPosition.z);
                AABB zTestBounds = AABB::FromCenterExtents(zTestCenter, checkExtents);
                bool zBlocked = m_collisionCallback(Vec3(m_position.x, m_position.y, newPosition.z), zTestBounds);

                // Apply sliding resolution
                if (xBlocked && zBlocked) {
                    // Both axes blocked - don't move horizontally
                    newPosition.x = m_position.x;
                    newPosition.z = m_position.z;
                    m_velocity.x = 0.0f;
                    m_velocity.z = 0.0f;
                } else if (xBlocked) {
                    // Only X blocked, slide along Z
                    newPosition.x = m_position.x;
                    m_velocity.x = 0.0f;
                } else if (zBlocked) {
                    // Only Z blocked, slide along X
                    newPosition.z = m_position.z;
                    m_velocity.z = 0.0f;
                } else {
                    // Neither axis blocked individually but combined is blocked
                    // This is a corner case - allow the axis with more movement
                    float xMove = std::abs(newPosition.x - m_position.x);
                    float zMove = std::abs(newPosition.z - m_position.z);

                    if (xMove > zMove) {
                        // Keep X movement, cancel Z
                        newPosition.z = m_position.z;
                        m_velocity.z = 0.0f;
                    } else {
                        // Keep Z movement, cancel X
                        newPosition.x = m_position.x;
                        m_velocity.x = 0.0f;
                    }

                    // Verify the chosen direction is actually clear
                    Vec3 finalCenter = Vec3(newPosition.x, m_position.y + stepOffset + checkHeight * 0.5f, newPosition.z);
                    AABB finalBounds = AABB::FromCenterExtents(finalCenter, checkExtents);
                    if (m_collisionCallback(newPosition, finalBounds)) {
                        // Still blocked, cancel all movement
                        newPosition.x = m_position.x;
                        newPosition.z = m_position.z;
                        m_velocity.x = 0.0f;
                        m_velocity.z = 0.0f;
                    }
                }
            }
        }
    }

    // === VERTICAL COLLISION (ground/top of cubes) ===
    // Get ground height at the new XZ position, using current Y to find valid ground
    float groundHeight = GetGroundHeight(newPosition.x, newPosition.z, m_position.y);

    // Ground collision: if we would go below ground, stop at ground level
    if (newPosition.y <= groundHeight) {
        newPosition.y = groundHeight;

        // Stop falling
        if (m_velocity.y < 0.0f) {
            m_velocity.y = 0.0f;
        }

        // We've landed
        m_groundInfo.isGrounded = true;
        m_groundInfo.groundPoint = Vec3(newPosition.x, groundHeight, newPosition.z);
        m_groundInfo.groundNormal = Vec3(0.0f, 1.0f, 0.0f);
        m_groundInfo.groundDistance = 0.0f;
        m_isJumping = false;
        m_airJumpsRemaining = m_config.maxAirJumps;
    } else {
        // We're in the air
        float distToGround = newPosition.y - groundHeight;
        if (distToGround > m_config.groundCheckDistance) {
            m_groundInfo.isGrounded = false;
        }
    }

    // Apply the new position
    m_position = newPosition;

    // === DEPENETRATION (safety net for stuck situations) ===
    if (m_depenetrationCallback) {
        Vec3 pushOut;
        AABB currentBounds = GetAABB();
        if (m_depenetrationCallback(currentBounds, pushOut)) {
            // Apply pushout
            m_position += pushOut;
            // Also adjust velocity to prevent re-entering the collision
            if (pushOut.x != 0.0f) m_velocity.x = 0.0f;
            if (pushOut.z != 0.0f) m_velocity.z = 0.0f;
        }
    }

    // Update capsule position
    UpdateCapsule();
}

void PlayerController::SetMoveInput(const Vec3& input) {
    m_moveInput = input;
    // Clamp input magnitude
    float length = Math::Length(Vec3(input.x, 0.0f, input.z));
    if (length > 1.0f) {
        m_moveInput.x /= length;
        m_moveInput.z /= length;
    }
    m_isMoving = length > 0.01f;
}

void PlayerController::SetLookDirection(float yaw, float pitch) {
    m_yaw = yaw;
    m_pitch = glm::clamp(pitch, -89.0f, 89.0f);
}

void PlayerController::Jump() {
    m_wantsToJump = true;
}

void PlayerController::SetSprinting(bool sprinting) {
    m_isSprinting = sprinting && !m_isCrouching;
}

void PlayerController::SetCrouching(bool crouching) {
    // TODO: Add crouch height transition and uncrouch collision check
    m_isCrouching = crouching;
    if (crouching) {
        m_isSprinting = false;
    }
}

void PlayerController::SetPosition(const Vec3& position) {
    m_position = position;
    UpdateCapsule();
}

void PlayerController::Teleport(const Vec3& position) {
    m_position = position;
    m_velocity = Vec3(0.0f);
    UpdateCapsule();
    CheckGroundCollision();
}

Vec3 PlayerController::GetEyePosition() const {
    float eyeHeight = m_isCrouching ? m_config.crouchEyeHeight : m_config.eyeHeight;
    return m_position + Vec3(0.0f, eyeHeight, 0.0f);
}

Vec3 PlayerController::GetForwardXZ() const {
    float yawRad = Math::Radians(m_yaw);
    return Math::Normalize(Vec3(cos(yawRad), 0.0f, sin(yawRad)));
}

Vec3 PlayerController::GetRightXZ() const {
    float yawRad = Math::Radians(m_yaw);
    return Math::Normalize(Vec3(-sin(yawRad), 0.0f, cos(yawRad)));
}

AABB PlayerController::GetAABB() const {
    Vec3 halfExtents = m_config.aabbHalfExtents;
    if (m_config.colliderType == PlayerColliderType::Capsule) {
        halfExtents = Vec3(m_config.capsuleRadius, m_currentHeight * 0.5f, m_config.capsuleRadius);
    }
    return AABB::FromCenterExtents(m_position + Vec3(0.0f, m_currentHeight * 0.5f, 0.0f), halfExtents);
}

void PlayerController::ApplyGravity(float deltaTime) {
    if (!m_groundInfo.isGrounded) {
        m_velocity.y -= m_config.gravity * deltaTime;
        // Clamp fall speed
        m_velocity.y = std::max(m_velocity.y, -m_config.maxFallSpeed);
    } else if (m_velocity.y < 0.0f) {
        // Small downward velocity to keep grounded
        m_velocity.y = -0.5f;
    }
}

void PlayerController::ApplyMovement(float deltaTime) {
    // Get max speed based on state
    float maxSpeed = m_config.walkSpeed;
    if (m_isSprinting && m_isMoving) {
        maxSpeed = m_config.sprintSpeed;
    } else if (m_isCrouching) {
        maxSpeed = m_config.crouchSpeed;
    }

    // Calculate wish direction and speed in world space
    Vec3 forward = GetForwardXZ();
    Vec3 right = GetRightXZ();

    Vec3 wishDir = Vec3(0.0f);
    wishDir += forward * m_moveInput.z;  // Forward/back
    wishDir += right * m_moveInput.x;    // Left/right

    float wishSpeed = Math::Length(wishDir);
    if (wishSpeed > 0.0001f) {
        wishDir = Math::Normalize(wishDir);
        wishSpeed = std::min(wishSpeed, 1.0f) * maxSpeed;
    } else {
        wishDir = Vec3(0.0f);
        wishSpeed = 0.0f;
    }

    if (m_groundInfo.isGrounded) {
        // === GROUND MOVEMENT (Source-style) ===
        ApplyFriction(deltaTime);
        ApplyGroundAcceleration(wishDir, wishSpeed, deltaTime);
    } else {
        // === AIR MOVEMENT (Source-style) ===
        ApplyAirAcceleration(wishDir, wishSpeed, deltaTime);
    }
}

// ============================================================================
// Source-Style Friction
// ============================================================================
void PlayerController::ApplyFriction(float deltaTime) {
    Vec3 vel = Vec3(m_velocity.x, 0.0f, m_velocity.z);
    float speed = Math::Length(vel);

    if (speed < 0.1f) {
        // Below threshold, just stop
        m_velocity.x = 0.0f;
        m_velocity.z = 0.0f;
        return;
    }

    // Apply much stronger friction when not providing input
    float friction = m_config.groundFriction;
    if (!m_isMoving) {
        // Apply extra stopping friction when no input
        friction *= 2.5f;
    }

    // Calculate friction drop
    // Source uses: drop = speed * friction * deltaTime
    // But if speed < stopspeed, use stopspeed for the calculation
    float control = (speed < m_config.stopSpeed) ? m_config.stopSpeed : speed;
    float drop = control * friction * deltaTime;

    // Scale the velocity
    float newSpeed = speed - drop;
    if (newSpeed < 0.0f) {
        newSpeed = 0.0f;
    }

    if (speed > 0.0f) {
        float scale = newSpeed / speed;
        m_velocity.x *= scale;
        m_velocity.z *= scale;
    }
}

// ============================================================================
// Source-Style Ground Acceleration
// ============================================================================
void PlayerController::ApplyGroundAcceleration(const Vec3& wishDir, float wishSpeed, float deltaTime) {
    // Current speed in wish direction
    float currentSpeed = Math::Dot(Vec3(m_velocity.x, 0.0f, m_velocity.z), wishDir);

    // How much we need to add
    float addSpeed = wishSpeed - currentSpeed;

    if (addSpeed <= 0.0f) {
        return;  // Already going fast enough in this direction
    }

    // Acceleration: accelspeed = accel * deltaTime * wishspeed
    float accelSpeed = m_config.groundAccelerate * deltaTime * wishSpeed;

    // Cap the acceleration
    if (accelSpeed > addSpeed) {
        accelSpeed = addSpeed;
    }

    // Add to velocity
    m_velocity.x += accelSpeed * wishDir.x;
    m_velocity.z += accelSpeed * wishDir.z;
}

// ============================================================================
// Source-Style Air Acceleration (enables strafing/bhop)
// ============================================================================
void PlayerController::ApplyAirAcceleration(const Vec3& wishDir, float wishSpeed, float deltaTime) {
    // Cap wish speed for air movement (this is key for strafe jumping)
    float wishSpeedCapped = std::min(wishSpeed, m_config.airSpeedCap * m_config.walkSpeed);

    // Current speed in wish direction
    float currentSpeed = Math::Dot(Vec3(m_velocity.x, 0.0f, m_velocity.z), wishDir);

    // How much we need to add
    float addSpeed = wishSpeedCapped - currentSpeed;

    if (addSpeed <= 0.0f) {
        return;
    }

    // Air acceleration
    float accelSpeed = m_config.airAccelerate * deltaTime * wishSpeed;

    // Cap it
    if (accelSpeed > addSpeed) {
        accelSpeed = addSpeed;
    }

    // Add to velocity
    m_velocity.x += accelSpeed * wishDir.x;
    m_velocity.z += accelSpeed * wishDir.z;

    // Optional: Apply air friction (usually 0 in Source)
    if (m_config.airFriction > 0.0f) {
        Vec3 vel = Vec3(m_velocity.x, 0.0f, m_velocity.z);
        float speed = Math::Length(vel);
        if (speed > 0.1f) {
            float drop = speed * m_config.airFriction * deltaTime;
            float newSpeed = std::max(0.0f, speed - drop);
            float scale = newSpeed / speed;
            m_velocity.x *= scale;
            m_velocity.z *= scale;
        }
    }
}

void PlayerController::CheckGroundCollision() {
    // Reset ground info
    m_groundInfo = GroundInfo();

    // Don't check for ground if we're jumping upward
    if (m_isJumping && m_velocity.y > 0.0f) {
        return;
    }

    // Get ground height at current position
    float groundHeight = GetGroundHeight(m_position.x, m_position.z, m_position.y);

    // Calculate distance to ground (positive = above, negative = below/inside)
    float distanceToGround = m_position.y - groundHeight;

    // Check if we're on, near, or below the ground
    // Use a larger check distance when falling to catch fast falls
    float checkDist = m_config.groundCheckDistance;
    if (m_velocity.y < 0.0f) {
        // When falling, extend check distance based on fall speed and frame time
        // This prevents tunneling through thin platforms
        checkDist = std::max(checkDist, std::abs(m_velocity.y) * 0.02f);
    }

    if (distanceToGround <= checkDist) {
        m_groundInfo.isGrounded = true;
        m_groundInfo.groundDistance = distanceToGround;
        m_groundInfo.groundPoint = Vec3(m_position.x, groundHeight, m_position.z);
        m_groundInfo.groundNormal = Vec3(0.0f, 1.0f, 0.0f);

        // Snap to ground surface if we're at or below it
        if (distanceToGround <= 0.0f) {
            m_position.y = groundHeight;
            if (m_velocity.y < 0.0f) {
                m_velocity.y = 0.0f;
            }
        }
    }
}

void PlayerController::HandleStepUp(float deltaTime) {
    if (!m_groundInfo.isGrounded || !m_isMoving) {
        return;
    }

    // Get movement direction
    Vec3 moveDir = Vec3(m_velocity.x, 0.0f, m_velocity.z);
    float moveSpeed = Math::Length(moveDir);

    if (moveSpeed < 0.01f) {
        return;
    }

    moveDir = Math::Normalize(moveDir);

    // Check for obstacle in front
    Vec3 checkPos = m_position + moveDir * m_config.stepSearchDistance;
    float frontGroundHeight = GetGroundHeight(checkPos.x, checkPos.z, m_position.y);

    // Calculate height difference
    float heightDiff = frontGroundHeight - m_position.y;

    // If there's a step-able obstacle
    if (heightDiff > 0.01f && heightDiff <= m_config.stepHeight) {
        // Check if we can stand on top
        Vec3 stepTopPos = Vec3(checkPos.x, frontGroundHeight, checkPos.z);

        if (!CheckCollision(stepTopPos)) {
            // Smoothly step up
            float stepSpeed = m_config.walkSpeed * 2.0f;  // Step up faster than walk
            float step = stepSpeed * deltaTime;

            if (step > heightDiff) {
                m_position.y = frontGroundHeight;
            } else {
                m_position.y += step;
            }
        }
    }
}

void PlayerController::HandleSlopes() {
    if (!m_groundInfo.isGrounded) {
        return;
    }

    // Calculate slope angle from ground normal
    float dotUp = Math::Dot(m_groundInfo.groundNormal, Vec3(0.0f, 1.0f, 0.0f));
    float slopeAngle = Math::Degrees(std::acos(dotUp));

    m_groundInfo.slopeAngle = slopeAngle;
    m_groundInfo.isOnSlope = slopeAngle > 1.0f;  // More than 1 degree

    // If on too steep a slope, slide down
    if (slopeAngle > m_config.maxSlopeAngle) {
        // Calculate slide direction (down the slope)
        Vec3 slideDir = m_groundInfo.groundNormal;
        slideDir.y = 0.0f;

        if (Math::Length(slideDir) > 0.01f) {
            slideDir = Math::Normalize(slideDir);
            float slideSpeed = m_config.gravity * 0.5f;
            m_velocity += slideDir * slideSpeed;
            m_groundInfo.isGrounded = false;  // Can't stand on steep slopes
        }
    }
}

void PlayerController::UpdateCapsule() {
    m_capsule.base = m_position;
    m_capsule.radius = m_config.capsuleRadius;
    m_capsule.height = m_isCrouching ? m_config.capsuleHeight * 0.5f : m_config.capsuleHeight;
    m_currentHeight = m_capsule.height;
}

bool PlayerController::CheckCollision(const Vec3& position) const {
    if (!m_collisionCallback) {
        return false;
    }

    // Create AABB at position
    Vec3 halfExtents = m_config.colliderType == PlayerColliderType::Capsule
        ? Vec3(m_config.capsuleRadius, m_currentHeight * 0.5f, m_config.capsuleRadius)
        : m_config.aabbHalfExtents;

    AABB bounds = AABB::FromCenterExtents(position + Vec3(0.0f, m_currentHeight * 0.5f, 0.0f), halfExtents);
    return m_collisionCallback(position, bounds);
}

float PlayerController::GetGroundHeight(float x, float z, float playerY) const {
    if (m_groundHeightCallback) {
        return m_groundHeightCallback(x, z, playerY);
    }
    // Default: flat ground at y = 0
    return 0.0f;
}

Vec3 PlayerController::ResolveCollision(const Vec3& desiredPosition) {
    Vec3 resolvedPos = desiredPosition;

    // When falling, use the higher of current or desired Y to find ground
    // This prevents tunneling through platforms
    float checkY = std::max(m_position.y, desiredPosition.y);

    // Get ground height - use checkY to properly detect surfaces we might pass through
    float groundHeight = GetGroundHeight(desiredPosition.x, desiredPosition.z, checkY);

    // If we're at or below ground level, snap to ground immediately
    if (resolvedPos.y <= groundHeight) {
        resolvedPos.y = groundHeight;
        if (m_velocity.y < 0.0f) {
            m_velocity.y = 0.0f;
        }
        // Also mark as grounded immediately to prevent further falling
        m_groundInfo.isGrounded = true;
        m_groundInfo.groundPoint = Vec3(resolvedPos.x, groundHeight, resolvedPos.z);
        m_isJumping = false;
    }

    // Check collision callback for world geometry
    if (m_collisionCallback) {
        // Try to resolve by sliding along obstacles
        if (CheckCollision(resolvedPos)) {
            // Try moving only horizontally
            Vec3 horizontalPos = Vec3(desiredPosition.x, m_position.y, desiredPosition.z);
            if (!CheckCollision(horizontalPos)) {
                resolvedPos = horizontalPos;
            } else {
                // Try moving only on X axis
                Vec3 xOnlyPos = Vec3(desiredPosition.x, m_position.y, m_position.z);
                if (!CheckCollision(xOnlyPos)) {
                    resolvedPos = xOnlyPos;
                    m_velocity.z = 0.0f;
                } else {
                    // Try moving only on Z axis
                    Vec3 zOnlyPos = Vec3(m_position.x, m_position.y, desiredPosition.z);
                    if (!CheckCollision(zOnlyPos)) {
                        resolvedPos = zOnlyPos;
                        m_velocity.x = 0.0f;
                    } else {
                        // Can't move at all
                        resolvedPos = m_position;
                        m_velocity.x = 0.0f;
                        m_velocity.z = 0.0f;
                    }
                }
            }
        }
    }

    return resolvedPos;
}

} // namespace Genesis

