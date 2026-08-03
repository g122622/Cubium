/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software be
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

#include "RestrictSunGoal.hpp"

#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/Goal.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/CreatureEntity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"

namespace mc::entity::ai::goal {

RestrictSunGoal::RestrictSunGoal(CreatureEntity* creature)
    : Goal(EnumSet<GoalFlag>{})
    , m_creature(creature)
{
    MC_ASSERT_RELEASE(creature != nullptr);
}

bool RestrictSunGoal::shouldExecute()
{
    IWorld* worldPtr = m_creature->world();
    if (worldPtr == nullptr) {
        return false;
    }

    // MC原版使用 isBrightOutside()，考虑天气（雷暴时白天也会变暗）
    if (!worldPtr->isBrightOutside()) {
        return false;
    }

    // 头部有装备时（如戴头盔），不限制阳光
    // MC原版逻辑：如果头部有物品，则不会因阳光而限制移动
    const ItemStack& headItem = m_creature->getEquipment(EquipmentSlot::Head);
    if (!headItem.isEmpty()) {
        return false;
    }

    return true;
}

void RestrictSunGoal::startExecuting()
{
    // 设置路径导航器避开阳光路径
    // 当 avoidSun 为 true 时，PathNavigator 在路径计算完成后会截断
    // 暴露在阳光下的路径部分，使实体只在阴影区域移动
    auto* nav = m_creature->navigator();
    if (nav != nullptr) {
        nav->setAvoidSunPathing(true);
    }
}

void RestrictSunGoal::resetTask()
{
    // 恢复路径导航器正常路径规划
    auto* nav = m_creature->navigator();
    if (nav != nullptr) {
        nav->setAvoidSunPathing(false);
    }
}

} // namespace mc::entity::ai::goal
