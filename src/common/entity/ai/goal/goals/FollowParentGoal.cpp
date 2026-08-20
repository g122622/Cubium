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

#include "FollowParentGoal.hpp"
#include "../../../core/EntityUtils.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../entities/passive/basic/AnimalEntity.hpp"
#include "../../pathfinding/PathNavigator.hpp"
#include "../GoalConstants.hpp"
#include "common/entity/core/EntitySize.hpp"

namespace mc::entity::ai::goal {

using namespace constants;

FollowParentGoal::FollowParentGoal(AnimalEntity* animal, f64 speed)
    : m_childAnimal(animal)
    , m_speed(speed)
{
    // 不设置任何 mutex flags，这意味着它可以与其他 Move goals 同时运行
}

bool FollowParentGoal::shouldExecute()
{
    if (!m_childAnimal) return false;

    // 检查是否是幼体
    if (m_childAnimal->getGrowingAge() >= 0) {
        return false; // 已成年，不需要跟随父母
    }

    // 在 8x4x8 范围内寻找成年动物
    m_parentAnimal = findParent();
    if (!m_parentAnimal) {
        return false;
    }

    // 只有距离 >= 9.0D 时才开始跟随
    f64 distSq = m_childAnimal->distanceSqTo(*m_parentAnimal);
    return distSq >= FOLLOW_PARENT_MIN_DISTANCE_SQ;
}

bool FollowParentGoal::shouldContinueExecuting()
{
    if (!m_childAnimal || !m_parentAnimal) return false;

    // 检查是否成年
    if (m_childAnimal->getGrowingAge() >= 0) {
        return false;
    }

    // 检查父/母是否存活
    if (!m_parentAnimal->isAlive()) {
        return false;
    }

    // 检查距离 - 只有在 [9.0D, 256.0D] 范围内才继续
    f64 distSq = m_childAnimal->distanceSqTo(*m_parentAnimal);
    return distSq >= FOLLOW_PARENT_MIN_DISTANCE_SQ && distSq <= FOLLOW_PARENT_MAX_DISTANCE_SQ;
}

void FollowParentGoal::startExecuting()
{
    m_delayCounter = 0;
}

void FollowParentGoal::resetTask()
{
    m_parentAnimal = nullptr;
    if (m_childAnimal) {
        m_childAnimal->clearNavigation();
    }
}

void FollowParentGoal::tick()
{
    if (!m_childAnimal || !m_parentAnimal) return;

    // tick 方法只有路径更新逻辑，没有 lookAt
    // 等待延迟计数器。对齐 vanilla FollowParentGoal.tick：
    // timeToRecalcPath = adjustedTickDelay(10)，减半补偿半 tick 评估。
    if (--m_delayCounter <= 0) {
        m_delayCounter = adjustedTickDelay(FOLLOW_DELAY_INTERVAL);

        // 尝试移动到父母实体位置
        if (auto* nav = m_childAnimal->navigator()) {
            static_cast<void>(nav->moveTo(*m_parentAnimal, m_speed));
        }
    }
}

AnimalEntity* FollowParentGoal::findParent()
{
    if (!m_childAnimal || !m_childAnimal->world()) return nullptr;

    // 在 8x4x8 范围内搜索，找最近的成年同类。
    // 对齐 vanilla FollowParentGoal.getEntitiesOfClass(this.animal.getClass(), ...)：只搜索与自身同类
    // （同 entityType()）的成年实体作为父母。此前 predicate 仅检查成年未检查同类，导致小牛会跟随
    // 成年羊/猪等异种动物（与 vanilla 不符）。
    const EntityType* childType = m_childAnimal->entityType();
    return EntityUtils::findClosestEntity<AnimalEntity>(m_childAnimal->world(),
        m_childAnimal->position(),
        FOLLOW_PARENT_SEARCH_RANGE, // 8.0f
        m_childAnimal,
        [childType](AnimalEntity* animal) {
            // 必须是成年动物
            if (animal->getGrowingAge() < 0) {
                return false;
            }
            // 必须是同类（对齐 vanilla getClass() 比较，与 AnimalEntity::canMateWith 同范式）
            return childType == animal->entityType();
        });
}

} // namespace mc::entity::ai::goal
