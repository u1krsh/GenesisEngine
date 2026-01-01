#pragma once

#include "core/Engine.h"
#include "renderer/DebugRenderer.h"

namespace Game {

// ============================================================================
// Player - Simple player controller
// ============================================================================
class Player {
public:
    Player() = default;
    ~Player() = default;

    void Initialize();
    void Update(double deltaTime);
    void Render();

    Genesis::Vec3 GetPosition() const;
    void SetPosition(const Genesis::Vec3& pos);

private:
    // Player uses the engine's camera for now
};

} // namespace Game

