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

#include "LookForJobSiteGoal.hpp"

#include "VillagerGoalUtils.hpp"
#include "common/entity/ai/goal/GoalConstants.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/network/packet/EntityPackets.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace villager {

using namespace constants;

LookForJobSiteGoal::LookForJobSiteGoal(VillagerEntity* villager)
    : m_villager(villager)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool LookForJobSiteGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 已有工作站点的不需要寻找
    if (m_villager->workStation() != BlockPos::zero()) return false;

    // 傻子村民不找工作
    if (m_villager->isNitwit()) return false;

    // 冷却时间
    if (m_searchCooldown > 0) return false;

    return true;
}

bool LookForJobSiteGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    // 找到工作站点或超时
    return !m_targetSite.has_value() && m_searchCooldown < SEARCH_COOLDOWN;
}

void LookForJobSiteGoal::startExecuting()
{
    m_targetSite = std::nullopt;
    m_searchCooldown = 0;
    _searchForJobSite();
}

void LookForJobSiteGoal::resetTask()
{
    m_targetSite = std::nullopt;
    m_searchCooldown = SEARCH_COOLDOWN;

    if (m_villager) {
        m_villager->clearNavigation();
    }
}

void LookForJobSiteGoal::tick()
{
    if (!m_villager) return;

    m_searchCooldown++;

    if (m_targetSite.has_value()) {
        // 移动到目标工作站点
        BlockPos pos = m_targetSite.value();
        m_villager->tryMoveTo(pos.x + 0.5, pos.y, pos.z + 0.5, 0.4);

        // 检查是否到达
        if (isWithinDistance(m_villager, pos, 2.0f)) {
            // 绑定工作站点
            m_villager->setWorkStation(pos);

            // MC原版 AssignProfessionFromJobSite：绑定工作站后播放开心村民粒子
            if (m_villager->world() != nullptr) {
                m_villager->world()->broadcastEntityStatus(
                    m_villager->id(), static_cast<u8>(network::EntityStatusPacket::Status::VillagerHappy));
            }

            m_targetSite = std::nullopt;
        }
    }
}

void LookForJobSiteGoal::_searchForJobSite()
{
    if (!m_villager || !m_villager->world()) return;

    // TODO: 集成POI系统搜索工作站点
    // 根据村民职业搜索对应的工作站点类型
    // 目前不实现，等待POI系统
}

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
