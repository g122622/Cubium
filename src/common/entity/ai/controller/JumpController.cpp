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
    // 将跳跃状态应用到实体
    if (m_mob && m_isJumping) {
        m_mob->setJumping(true);
    }
    m_isJumping = false;
}

} // namespace mc::entity::ai::controller
