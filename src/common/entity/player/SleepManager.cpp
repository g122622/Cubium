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

#include "SleepManager.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/functional/BedBlock.hpp"
#include "common/world/weather/WeatherConstants.hpp"
#include <cmath>
#include <optional>
#include <vector>

namespace mc {
namespace entity {

// ========== 公共静态方法 ==========

bool SleepManager::canSleepAtTime(i64 dayTime, bool isThundering, bool isRaining)
{
    // 雷暴时任何时间都可以睡眠
    if (isThundering) {
        return true;
    }

    // 使用 WeatherConstants 中定义的时间范围
    if (isRaining) {
        // 降雨时睡眠时间范围更宽: 12010 - 23991
        // 降雨范围在同一日内，使用 AND 判断
        return dayTime >= weather::WeatherConstants::RAIN_BED_START_TIME &&
            dayTime <= weather::WeatherConstants::RAIN_BED_END_TIME;
    }

    // 晴天时只能在夜间睡眠: 12542 - 23459
    return dayTime >= weather::WeatherConstants::CLEAR_BED_START_TIME &&
        dayTime <= weather::WeatherConstants::CLEAR_BED_END_TIME;
}

std::optional<Vector3> SleepManager::findWakeUpPosition(
    const IWorld& world, const BlockPos& bedPos, Direction bedFacing, f32 entityYaw)
{
    // 委托给 BedBlock::findStandUpPosition，该算法与 MC 原版一致
    Vector3 result = blocks::BedBlock::findStandUpPosition(world, bedPos, bedFacing, entityYaw);
    // BedBlock::findStandUpPosition 始终返回有效位置（最终回退到床头正上方），
    // 所以直接返回
    return result;
}

bool SleepManager::isPlayerNearBed(const Vector3& playerPos, const BlockPos& bedPos)
{
    // 床的中心位置
    f32 bedCenterX = static_cast<f32>(bedPos.x) + 0.5f;
    f32 bedCenterY = static_cast<f32>(bedPos.y) + 0.5f;
    f32 bedCenterZ = static_cast<f32>(bedPos.z) + 0.5f;

    // 计算距离
    f32 dx = std::abs(playerPos.x - bedCenterX);
    f32 dy = std::abs(playerPos.y - bedCenterY);
    f32 dz = std::abs(playerPos.z - bedCenterZ);

    // 水平范围 3 格，垂直范围 2 格
    return dx <= 3.0f && dy <= 2.0f && dz <= 3.0f;
}

bool SleepManager::isBedObstructed(const IWorld& world, const BlockPos& bedPos, Direction bedFacing)
{

    // 检查床头和床尾上方是否有空间

    // 床头正上方
    BlockPos aboveHead = bedPos.up();
    if (!_hasStandingSpace(world, aboveHead)) {
        return true;
    }

    // 床尾正上方
    Direction footDir = Directions::opposite(bedFacing);
    BlockPos footPos = bedPos.offset(footDir);
    BlockPos aboveFoot = footPos.up();
    if (!_hasStandingSpace(world, aboveFoot)) {
        return true;
    }

    // 检查床尾前方（玩家起床位置）
    BlockPos wakePos = footPos.offset(footDir).up();
    if (!_hasStandingSpace(world, wakePos)) {
        return true;
    }

    return false;
}

bool SleepManager::isBedSurroundedByMonsters(IWorld& world, const BlockPos& bedPos, const Player& player)
{

    // 在床周围 8x5x8 范围内检测敌对生物
    // 范围：X ±4, Y ±2, Z ±4

    f32 bedCenterX = static_cast<f32>(bedPos.x) + 0.5f;
    f32 bedCenterY = static_cast<f32>(bedPos.y) + 0.5f;
    f32 bedCenterZ = static_cast<f32>(bedPos.z) + 0.5f;

    // 创建检测范围 (8x5x8 -> 半径 4x2.5x4)
    AxisAlignedBB searchBox(bedCenterX - 4.0f,
        bedCenterY - 2.0f,
        bedCenterZ - 4.0f,
        bedCenterX + 4.0f,
        bedCenterY + 2.0f,
        bedCenterZ + 4.0f);

    // 获取范围内的所有实体
    std::vector<Entity*> nearbyEntities = world.getEntitiesInAABB(searchBox, &player);

    // 检查是否有怪物
    for (Entity* entity : nearbyEntities) {
        if (entity == nullptr) {
            continue;
        }

        // 检查是否为敌对生物（MonsterEntity 或其子类）
        if (dynamic_cast<MonsterEntity*>(entity) != nullptr) {
            // 找到敌对生物
            return true;
        }
    }

    return false;
}

// ========== 私有静态方法 ==========

bool SleepManager::_hasStandingSpace(const IWorld& world, const BlockPos& pos)
{
    // 检查 pos 和 pos.up() 是否都是非固体方块
    const BlockState* state1 = world.getBlockState(pos);
    const BlockState* state2 = world.getBlockState(pos.up());

    // 两个方块都必须是空气或非固体
    bool canStand1 = (state1 == nullptr || state1->isAir() || !state1->isSolid());
    bool canStand2 = (state2 == nullptr || state2->isAir() || !state2->isSolid());

    return canStand1 && canStand2;
}

} // namespace entity
} // namespace mc
