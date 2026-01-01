#include "Player.h"

namespace Game {

void Player::Initialize() {
    // Player initialization
}

void Player::Update(double deltaTime) {
    // Player update logic
    // Camera movement is handled by the engine
}

void Player::Render() {
    // Player rendering (if any player-specific rendering)
}

Genesis::Vec3 Player::GetPosition() const {
    return Genesis::Engine::Instance().GetCamera().GetPosition();
}

void Player::SetPosition(const Genesis::Vec3& pos) {
    Genesis::Engine::Instance().GetCamera().SetPosition(pos);
}

} // namespace Game

