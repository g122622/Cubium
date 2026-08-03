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

#include "GoToBedGoal.hpp"
#include "VillagerGoalUtils.hpp"
#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/goal/GoalFlag.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/blocks/functional/BedBlock.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace goal {
namespace villager {

using namespace constants;

// ============================================================================
// GoToBedGoal - 村民前往床位目标
// ============================================================================

GoToBedGoal::GoToBedGoal(VillagerEntity* villager)
    : m_villager(villager)
{
    setMutexFlags(EnumSet<GoalFlag>{GoalFlag::Move});
}

bool GoToBedGoal::shouldExecute()
{
    if (!m_villager) return false;

    // 检查是否是夜间
    if (!m_villager->world()) return false;
    if (!m_villager->isNightTime()) return false;

    // 已经在睡眠中则不重新开始
    if (m_villager->isSleeping()) return false;

    // 检查是否有绑定的床位（从Brain的HOME记忆获取）
    // HOME记忆使用GlobalPos类型，包含维度信息
    auto homeMemory = m_villager->brain().getMemory<GlobalPos>(ai::brain::memory::MemoryModuleTypes::HOME);
    if (homeMemory.has_value()) {
        // 检查是否在同一维度
        if (m_villager->world() && homeMemory->getDimensionId() == m_villager->world()->dimension()) {
            m_bedPos = homeMemory->getPos();
            return true;
        }
    }

    // 没有绑定床位，尝试查找最近的可用床
    // 村民会搜索48格范围内的床
    auto* villageManager = m_villager->world()->villageManager();
    if (!villageManager) return false;

    auto& poiStorage = villageManager->getPOIStorage();
    BlockPos villagerPos(
        static_cast<i32>(m_villager->x()), static_cast<i32>(m_villager->y()), static_cast<i32>(m_villager->z()));
    constexpr f32 BED_SEARCH_RANGE = 48.0f;

    // 遍历所有床类型（BedRed 到 BedYellow）
    using namespace world::village::poi;
    for (i32 bedType = static_cast<i32>(PointOfInterestType::BedRed);
        bedType <= static_cast<i32>(PointOfInterestType::BedYellow);
        ++bedType) {

        PointOfInterestType poiType = static_cast<PointOfInterestType>(bedType);
        auto result = poiStorage.findNearestFree(villagerPos, poiType, BED_SEARCH_RANGE);

        if (result.has_value()) {
            m_bedPos = result.value();
            return true;
        }
    }

    return false;
}

bool GoToBedGoal::shouldContinueExecuting()
{
    if (!m_villager) return false;

    return !m_reachedBed;
}

void GoToBedGoal::startExecuting()
{
    m_reachedBed = false;

    if (m_villager && m_bedPos != BlockPos::zero()) {
        m_villager->tryMoveTo(m_bedPos.x + 0.5, m_bedPos.y, m_bedPos.z + 0.5, SPEED_MODIFIER);
    }
}

void GoToBedGoal::resetTask()
{
    m_reachedBed = false;
    m_bedPos = BlockPos::zero();

    if (m_villager) {
        // 如果正在睡眠，停止睡眠
        if (m_villager->isSleeping()) {
            m_villager->stopSleeping();
        }
        m_villager->clearNavigation();
    }
}

void GoToBedGoal::tick()
{
    if (!m_villager) return;

    if (m_bedPos == BlockPos::zero()) return;

    // 检查是否到达床位
    if (isWithinDistance(m_villager, m_bedPos, 1.5f)) {
        m_reachedBed = true;

        // 检查床位是否仍然有效
        const auto* blockState = m_villager->world()->getBlockState(m_bedPos);
        if (blockState && mc::blocks::BedBlock::isBed(*m_villager->world(), m_bedPos)) {
            // 通过VillageManager获取POI存储，占用床位
            auto* villageManager = m_villager->world()->villageManager();
            if (villageManager) {
                auto& poiStorage = villageManager->getPOIStorage();

                // 检查床位是否已被占用
                const auto* poi = poiStorage.getPOI(m_bedPos);
                if (!poi || !poi->isOccupied()) {
                    // 占用床位
                    poiStorage.acquirePOI(
                        m_bedPos, static_cast<u64>(m_villager->id()), m_villager->world()->currentTick());

                    // 开始睡眠
                    m_villager->startSleeping(m_bedPos);

                    // 将床位保存到Brain的HOME记忆（使用GlobalPos包含维度信息）
                    GlobalPos homePos(m_villager->world()->dimension(), m_bedPos);
                    m_villager->brain().setMemory(ai::brain::memory::MemoryModuleTypes::HOME, homePos);
                }
            }
        }
    } else {
        m_villager->tryMoveTo(m_bedPos.x + 0.5, m_bedPos.y, m_bedPos.z + 0.5, SPEED_MODIFIER);
    }
}

} // namespace villager
} // namespace goal
} // namespace ai
} // namespace entity
} // namespace mc
