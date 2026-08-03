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
#include "common/core/EnumSet.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/entities/villager/ProfessionMapping.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"
#include <optional>

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

            // 根据 POI 类型分配职业（参考 MC 原版 AssignProfessionFromJobSite）
            if (m_villager->profession() == VillagerProfession::None) {
                auto* villageMgr = m_villager->world()->villageManager();
                if (villageMgr) {
                    auto& poiStorage = villageMgr->getPOIStorage();
                    const auto* poi = poiStorage.getPOI(pos);
                    if (poi && world::village::poi::POITypeHelper::isWorkstation(poi->getType())) {
                        VillagerProfession profession =
                            entity::villager::ProfessionMapping::getProfessionFromPOI(poi->getType());
                        if (entity::villager::ProfessionMapping::isValidProfession(profession)) {
                            m_villager->setProfession(profession);
                        }
                    }
                }
            }

            // MC原版 AssignProfessionFromJobSite：绑定工作站后播放开心村民粒子
            if (m_villager->world() != nullptr) {
                m_villager->world()->broadcastEntityStatus(
                    m_villager->id(), static_cast<u8>(network::EntityStatus::VillagerHappy));
            }

            m_targetSite = std::nullopt;
        }
    }
}

void LookForJobSiteGoal::_searchForJobSite()
{
    if (!m_villager || !m_villager->world()) return;

    auto* world = m_villager->world();
    auto* villageManager = world->villageManager();
    if (!villageManager) return;

    auto& poiStorage = villageManager->getPOIStorage();
    BlockPos entityPos(
        static_cast<i32>(m_villager->x()), static_cast<i32>(m_villager->y()), static_cast<i32>(m_villager->z()));

    using POIType = world::village::poi::PointOfInterestType;

    // 无职业村民搜索所有可获取的工作站类型
    // 参考 MC 原版 AcquirePoi 行为：无职业村民使用 acquirableJobSite 谓词
    const auto& allWorkstations = entity::villager::ProfessionMapping::getAcquirableWorkstations();

    BlockPos nearestPos;
    f32 nearestDist = SEARCH_RANGE;
    bool found = false;

    for (POIType wsType : allWorkstations) {
        auto site = poiStorage.findNearestFree(entityPos, wsType, SEARCH_RANGE);
        if (site.has_value()) {
            const BlockPos& pos = site.value();
            f32 dx = static_cast<f32>(pos.x - entityPos.x);
            f32 dy = static_cast<f32>(pos.y - entityPos.y);
            f32 dz = static_cast<f32>(pos.z - entityPos.z);
            f32 dist = Vector3(dx, dy, dz).length();
            if (dist < nearestDist) {
                nearestDist = dist;
                nearestPos = pos;
                found = true;
            }
        }
    }

    if (found) {
        m_targetSite = nearestPos;
    }
}

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
