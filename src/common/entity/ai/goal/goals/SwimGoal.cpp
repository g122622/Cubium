/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SwimGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/controller/JumpController.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::entity::ai::goal {

SwimGoal::SwimGoal(MobEntity* mob)
    : m_mob(mob)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Jump});
    // 设置导航器可以游泳
    if (m_mob) {
        if (auto* nav = m_mob->navigator()) {
            nav->setCanSwim(true);
        }
    }
}

bool SwimGoal::shouldExecute()
{
    if (!m_mob) return false;

    // 检查是否在水中且水位超过眼睛高度，或者在岩浆中
    if (m_mob->isInWater()) {
        f32 fluidHeight = m_mob->getFluidHeight();
        f32 eyeHeight = m_mob->eyeHeight();
        return fluidHeight > eyeHeight;
    }
    return m_mob->isInLava();
}

void SwimGoal::tick()
{
    if (!m_mob) return;

    // 以 80% 概率跳跃
    math::Random& rng = m_mob->getRandom();
    if (rng.nextFloat() < 0.8f) {
        if (auto* jumpCtrl = m_mob->jumpController()) {
            jumpCtrl->setJumping();
        }
    }
}

} // namespace mc::entity::ai::goal
