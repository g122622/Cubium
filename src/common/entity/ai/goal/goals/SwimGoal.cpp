#include "SwimGoal.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../ai/controller/JumpController.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc::entity::ai::goal {

SwimGoal::SwimGoal(MobEntity* mob)
    : m_mob(mob)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Jump});
}

bool SwimGoal::shouldExecute() {
    if (!m_mob) return false;

    // 检查实体是否在水中或岩浆中
    return m_mob->isInWater() || m_mob->isInLava();
}

bool SwimGoal::shouldContinueExecuting() {
    if (!m_mob) return false;

    // 继续执行直到不在水中
    return m_mob->isInWater() || m_mob->isInLava();
}

void SwimGoal::tick() {
    if (!m_mob) return;

    // 在水中或岩浆中时随机跳跃以游泳
    math::Random rng = m_mob->getRandom();
    if (rng.nextFloat() < 0.8f) {
        // 触发跳跃
        if (auto* jumpCtrl = m_mob->jumpController()) {
            jumpCtrl->setJumping();
        }
    }
}

} // namespace mc::entity::ai::goal
