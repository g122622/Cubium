#include "JumpController.hpp"
#include "../../core/MobEntity.hpp"

namespace mc::entity::ai::controller {

JumpController::JumpController(MobEntity* mob)
    : m_mob(mob)
{}

void JumpController::setJumping() {
    m_isJumping = true;
}

void JumpController::tick() {
    // MC 1.16.5: 总是调用 setJumping(isJumping)，即使 isJumping 为 false
    // 这确保实体的跳跃状态被正确重置
    if (m_mob) {
        m_mob->setJumping(m_isJumping);
    }
    m_isJumping = false;
}

} // namespace mc::entity::ai::controller
