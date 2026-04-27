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

    // MC 1.16.5: isInWater() && getFluidHeight(FluidTags.WATER) > getEyeHeight()
    // 或 isInLava()
    if (m_mob->isInWater()) {
        // MC 1.16.5: func_233571_b_ 获取流体高度，func_233579_cu_ 获取眼睛高度
        // 需要检查水位是否超过眼睛高度
        f32 fluidHeight = m_mob->getFluidHeight();  // 水位高度
        f32 eyeHeight = m_mob->eyeHeight();         // 眼睛高度
        return fluidHeight > eyeHeight;
    }
    return m_mob->isInLava();
}

void SwimGoal::tick() {
    if (!m_mob) return;

    // MC 1.16.5: 以 80% 概率跳跃
    math::Random rng = m_mob->getRandom();
    if (rng.nextFloat() < 0.8f) {
        // 触发跳跃
        if (auto* jumpCtrl = m_mob->jumpController()) {
            jumpCtrl->setJumping();
        }
    }
}

} // namespace mc::entity::ai::goal
