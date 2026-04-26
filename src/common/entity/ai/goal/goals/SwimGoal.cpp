#include "SwimGoal.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../core/Entity.hpp"
#include "../../../ai/controller/JumpController.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../../../../util/math/random/Random.hpp"

namespace mc::entity::ai::goal {

SwimGoal::SwimGoal(MobEntity* mob)
    : m_mob(mob)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Jump});
    // MC 1.16.5: 设置导航器可以游泳
    if (m_mob) {
        if (auto* nav = m_mob->navigator()) {
            nav->setCanSwim(true);
        }
    }
}

bool SwimGoal::shouldExecute() {
    if (!m_mob) return false;

    // MC 1.16.5: 检查是否在水中且水位超过眼睛高度，或者在岩浆中
    if (m_mob->isInWater()) {
        // 需要水位超过眼睛高度才游泳
        f32 fluidHeight = m_mob->getFluidHeight();  // 水位高度
        f32 eyeHeight = m_mob->eyeHeight();         // 眼睛高度
        return fluidHeight > eyeHeight;
    }
    return m_mob->isInLava();
}

bool SwimGoal::shouldContinueExecuting() {
    if (!m_mob) return false;

    // MC 1.16.5: 继续执行直到不在流体中
    return m_mob->isInWater() || m_mob->isInLava();
}

void SwimGoal::tick() {
    if (!m_mob) return;

    // MC 1.16.5: 在水中或岩浆中时以80%概率跳跃
    math::Random rng = m_mob->getRandom();
    if (rng.nextFloat() < 0.8f) {
        // 触发跳跃
        if (auto* jumpCtrl = m_mob->jumpController()) {
            jumpCtrl->setJumping();
        }
    }
}

} // namespace mc::entity::ai::goal
