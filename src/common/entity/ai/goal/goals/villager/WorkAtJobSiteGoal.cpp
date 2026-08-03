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

#include "WorkAtJobSiteGoal.hpp"

#include "VillagerGoalUtils.hpp"
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace villager {

using namespace constants;

WorkAtJobSiteGoal::WorkAtJobSiteGoal(VillagerEntity* villager)
    : m_villager(villager)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look});
}

bool WorkAtJobSiteGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 傻子村民不工作
    if (m_villager->isNitwit()) return false;

    // 检查是否是工作时间
    if (!_isWorkTime()) return false;

    // 检查是否有工作站点
    return _hasJobSite();
}

bool WorkAtJobSiteGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 继续工作的条件
    if (!_isWorkTime()) return false;
    if (!_hasJobSite()) return false;

    // 限制工作时间
    return m_workTicks < WORK_TICKS_MAX;
}

void WorkAtJobSiteGoal::startExecuting()
{
    m_workTicks = 0;
    m_atJobSite = false;
    _moveToJobSite();
}

void WorkAtJobSiteGoal::resetTask()
{
    m_workTicks = 0;
    m_atJobSite = false;

    if (m_villager) {
        m_villager->clearNavigation();
        // 重置工作状态
        m_villager->rest();
    }
}

void WorkAtJobSiteGoal::tick()
{
    if (!m_villager) return;

    m_workTicks++;

    // 检查是否在工作站点附近
    BlockPos workPos = m_villager->workStation();

    if (isWithinDistance(m_villager, workPos, 2.0f)) {
        m_atJobSite = true;
        _doWork();

        // 在工作站点时检查补货，有300tick冷却
        // shouldRestock() 内部也包含跨天检测和每日2次补货限制
        IWorld* world = m_villager->world();
        if (world) {
            i64 currentTime = static_cast<i64>(world->currentTick());
            if (currentTime >= m_lastRestockCheckTime + RESTOCK_CHECK_COOLDOWN) {
                if (_needsRestock()) {
                    _restock();
                }
                m_lastRestockCheckTime = currentTime;
            }
        }
    } else {
        m_atJobSite = false;
        _moveToJobSite();
    }
}

bool WorkAtJobSiteGoal::_isWorkTime() const
{
    if (!m_villager) return false;
    return m_villager->isWorkTime();
}

bool WorkAtJobSiteGoal::_hasJobSite() const
{
    if (!m_villager) return false;
    return m_villager->workStation() != BlockPos::zero();
}

void WorkAtJobSiteGoal::_moveToJobSite()
{
    if (!m_villager) return;

    BlockPos workPos = m_villager->workStation();
    m_villager->tryMoveTo(workPos.x + 0.5, workPos.y, workPos.z + 0.5, 0.4);
}

void WorkAtJobSiteGoal::_doWork()
{
    if (!m_villager) return;

    // 设置工作状态
    m_villager->work();

    // 每隔一段时间增加经验
    if (m_workTicks % 100 == 0) {
        m_villager->addVillagerExperience(1);
    }
}

bool WorkAtJobSiteGoal::_needsRestock() const
{
    if (!m_villager) return false;

    // 补货检查逻辑：
    // 委托给 VillagerEntity::shouldRestock()，包含完整逻辑：
    // - 跨天检测（12000tick 或新游戏日）与每日补货次数重置
    // - 允许补货检查（每日最多2次，第二次需间隔2400tick）
    // - 需要补货检查（任意交易被使用过）
    return m_villager->shouldRestock();
}

void WorkAtJobSiteGoal::_restock()
{
    if (!m_villager) return;

    // 委托给 VillagerEntity::restock()，执行完整的补货流程：
    // 1. 更新所有交易的需求值
    // 2. 重置所有交易的使用次数
    // 3. 记录补货时间
    // 4. 增加今日补货次数
    m_villager->restock();
}

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
