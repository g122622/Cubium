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

#include "IronGolemGoals.hpp"
#include "util/math/random/Random.hpp"
#include "world/IWorld.hpp"
#include "entity/core/EntityUtils.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/entities/passive/golem/IronGolemEntity.hpp"
#include "entity/entities/villager/VillagerEntity.hpp"
#include "entity/ai/controller/LookController.hpp"
#include <cmath>

namespace mc::entity::ai::goal {

// ==================== ShowVillagerFlowerGoal ====================

ShowVillagerFlowerGoal::ShowVillagerFlowerGoal(IronGolemEntity* ironGolem)
    : m_ironGolem(ironGolem)
{
    // MC 1.16.5: this.setMutexFlags(EnumSet.of(Goal.Flag.MOVE, Goal.Flag.LOOK));
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool ShowVillagerFlowerGoal::shouldExecute()
{
    if (!m_ironGolem) return false;

    // MC 1.16.5: 只在白天执行
    IWorld* world = m_ironGolem->world();
    if (!world) return false;

    // 检查是否是白天
    // MC 1.16.5: !this.ironGolem.world.isDaytime()
    if (!world->isDaytime()) {
        return false;
    }

    // MC 1.16.5: 概率检查 1/8000
    math::Random rng = m_ironGolem->getRandom();
    if (rng.nextInt(CHANCE) != 0) {
        return false;
    }

    // MC 1.16.5: 在 6 格范围内搜索村民
    // 使用 EntityPredicate 检查距离和可见性
    Vector3 pos = m_ironGolem->position();
    AxisAlignedBB searchBox = m_ironGolem->boundingBox().expand(SEARCH_RANGE, SEARCH_HEIGHT, SEARCH_RANGE);

    entity::VillagerEntity* nearestVillager = EntityUtils::findClosestEntity<entity::VillagerEntity>(
        world, pos, SEARCH_RANGE, m_ironGolem,
        [](entity::VillagerEntity* villager) -> bool {
            if (!villager) return false;
            return villager->isAlive();
        });

    if (nearestVillager) {
        m_villager = nearestVillager;
        return true;
    }

    return false;
}

bool ShowVillagerFlowerGoal::shouldContinueExecuting()
{
    // MC 1.16.5: return this.lookTime > 0;
    return m_lookTime > 0;
}

void ShowVillagerFlowerGoal::startExecuting()
{
    // MC 1.16.5: this.lookTime = 400;
    m_lookTime = LOOK_DURATION;

    // MC 1.16.5: this.ironGolem.setHoldingRose(true);
    m_ironGolem->setHoldingRose(true);
}

void ShowVillagerFlowerGoal::resetTask()
{
    // MC 1.16.5: this.ironGolem.setHoldingRose(false);
    if (m_ironGolem) {
        m_ironGolem->setHoldingRose(false);
    }
    m_villager = nullptr;
}

void ShowVillagerFlowerGoal::tick()
{
    if (!m_ironGolem || !m_villager) return;

    // MC 1.16.5: this.ironGolem.getLookController().setLookPositionWithEntity(this.villager, 30.0F, 30.0F);
    auto* lookController = m_ironGolem->lookController();
    if (lookController) {
        lookController->setLookPositionWithEntity(*m_villager, 30.0f, 30.0f);
    }

    // 递减看向时间
    m_lookTime--;
}

} // namespace mc::entity::ai::goal
