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

#include "LookAtEntitiesGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace villager {

using namespace constants;

// ============================================================================
// LookAtEntitiesGoal - 村民看向实体目标
// ============================================================================

LookAtEntitiesGoal::LookAtEntitiesGoal(VillagerEntity* villager)
    : m_villager(villager)
    , m_lookTargetId(0)
    , m_lookTime(0)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Look});
}

bool LookAtEntitiesGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 概率检查
    math::Random& rng = m_villager->getRandom();
    if (rng.nextFloat() >= LOOK_CHANCE) return false;

    // 随机选择目标类型
    _selectTargetType();

    // 查找对应类型的实体
    LivingEntity* target = nullptr;
    switch (m_targetType) {
        case TargetType::Villager:
            target = EntityUtils::findClosestEntity<VillagerEntity>(
                m_villager->world(), m_villager->position(), LOOK_RANGE, m_villager, [](LivingEntity* entity) {
                    return entity && entity->isAlive();
                });
            break;
        case TargetType::Player:
        case TargetType::Cat:
        case TargetType::Creature:
            target = EntityUtils::findClosestEntity<LivingEntity>(
                m_villager->world(), m_villager->position(), LOOK_RANGE, m_villager, [](LivingEntity* entity) {
                    return entity && entity->isAlive();
                });
            break;
    }

    if (target) {
        m_lookTargetId = target->id();
        // 设置看向时间
        math::Random& rng2 = m_villager->getRandom();
        m_lookTime = LOOK_MIN_TIME + rng2.nextInt(LOOK_MAX_TIME - LOOK_MIN_TIME);
        return true;
    }

    return false;
}

bool LookAtEntitiesGoal::shouldContinueExecuting()
{
    if (!m_villager || m_lookTargetId == 0) return false;

    // 获取目标实体
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_lookTargetId) : nullptr;
    if (!entity) {
        m_lookTargetId = 0;
        return false;
    }

    LivingEntity* target = dynamic_cast<LivingEntity*>(entity);
    if (!target || !target->isAlive()) {
        m_lookTargetId = 0;
        return false;
    }

    // 检查距离
    f32 distSq = m_villager->distanceSqTo(*target);
    if (distSq > LOOK_RANGE * LOOK_RANGE) return false;

    return m_lookTime > 0;
}

void LookAtEntitiesGoal::startExecuting()
{
    // 开始看向目标
}

void LookAtEntitiesGoal::resetTask()
{
    m_lookTargetId = 0;
    m_lookTime = 0;
}

void LookAtEntitiesGoal::tick()
{
    if (!m_villager || m_lookTargetId == 0) return;

    m_lookTime--;

    // 获取目标实体
    Entity* entity = m_villager->world() ? m_villager->world()->getEntity(m_lookTargetId) : nullptr;
    if (!entity) {
        m_lookTargetId = 0;
        return;
    }

    LivingEntity* target = dynamic_cast<LivingEntity*>(entity);
    if (!target || !target->isAlive()) {
        m_lookTargetId = 0;
        return;
    }

    // 使用 LookController 看向目标
    if (auto* lookCtrl = m_villager->lookController()) {
        lookCtrl->setLookPosition(target->x(), target->y() + target->eyeHeight(), target->z());
    }
}

void LookAtEntitiesGoal::_selectTargetType()
{
    // 猫: 8, 村民: 2, 玩家: 2, 生物: 1
    math::Random& rng = m_villager->getRandom();
    i32 rand = rng.nextInt(13); // 8 + 2 + 2 + 1 = 13

    if (rand < 8) {
        m_targetType = TargetType::Cat;
    } else if (rand < 10) {
        m_targetType = TargetType::Villager;
    } else if (rand < 12) {
        m_targetType = TargetType::Player;
    } else {
        m_targetType = TargetType::Creature;
    }
}

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
