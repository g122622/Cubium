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

#include "ExperienceDropHandler.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/experience/ExperienceUtils.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace entity {

// ============================================================================
// 经验球生成
// ============================================================================

i32 ExperienceDropHandler::spawnExperienceOrbs(IWorld* world, f64 x, f64 y, f64 z, i32 totalXp, math::Random* rng)
{
    if (world == nullptr || totalXp <= 0) {
        return 0;
    }

    // 分割经验值
    std::vector<i32> xpValues;
    experience::utils::splitExperience(totalXp, xpValues);

    // 如果没有提供随机数生成器，使用默认的
    math::Random defaultRng(static_cast<u64>(std::hash<f64>{}(x) ^ std::hash<f64>{}(y) ^ std::hash<f64>{}(z)));
    math::Random* random = rng ? rng : &defaultRng;

    i32 spawnedCount = 0;

    for (i32 xpValue : xpValues) {
        // 为每个经验球生成随机速度
        f32 vx = static_cast<f32>((random->nextDouble() - 0.5) * 0.2);
        f32 vy = static_cast<f32>(random->nextDouble() * 0.2 + 0.1);
        f32 vz = static_cast<f32>((random->nextDouble() - 0.5) * 0.2);

        // 创建经验球实体
        ExperienceOrbEntity* orb = _createExperienceOrb(world, x, y, z, xpValue, vx, vy, vz);
        if (orb != nullptr) {
            spawnedCount++;
        }
    }

    return spawnedCount;
}

i32 ExperienceDropHandler::spawnExperienceOrbs(Entity* entity, i32 totalXp, math::Random* rng)
{
    if (entity == nullptr || totalXp <= 0) {
        return 0;
    }

    return spawnExperienceOrbs(entity->world(), entity->x(), entity->y(), entity->z(), totalXp, rng);
}

// ============================================================================
// 玩家死亡经验掉落
// ============================================================================

i32 ExperienceDropHandler::spawnPlayerDeathXp(Player* player)
{
    if (player == nullptr || player->world() == nullptr) {
        return 0;
    }

    // 计算玩家死亡掉落的经验
    i32 droppedXp = experience::utils::calculateDeathDropXp(player->experienceLevel());
    if (droppedXp <= 0) {
        return 0;
    }

    // 在玩家位置生成经验球
    return spawnExperienceOrbs(player->world(), player->x(), player->y(), player->z(), droppedXp);
}

// ============================================================================
// 矿石经验掉落
// ============================================================================

i32 ExperienceDropHandler::spawnOreExperience(IWorld* world, f64 x, f64 y, f64 z, i32 oreType, math::Random& rng)
{
    if (world == nullptr) {
        return 0;
    }

    // 生成随机经验值
    i32 xp = experience::utils::randomOreExperience(rng, oreType);
    if (xp <= 0) {
        return 0;
    }

    return spawnExperienceOrbs(world, x, y, z, xp, &rng);
}

// ============================================================================
// 钓鱼经验掉落
// ============================================================================

i32 ExperienceDropHandler::spawnFishingExperience(IWorld* world, f64 x, f64 y, f64 z, math::Random& rng)
{
    if (world == nullptr) {
        return 0;
    }

    // 生成随机钓鱼经验 (1-6)
    i32 xp = experience::utils::randomFishingExperience(rng);
    return spawnExperienceOrbs(world, x, y, z, xp, &rng);
}

// ============================================================================
// 被动动物经验掉落
// ============================================================================

i32 ExperienceDropHandler::spawnPassiveMobExperience(IWorld* world, f64 x, f64 y, f64 z, math::Random& rng)
{
    if (world == nullptr) {
        return 0;
    }

    // 被动动物掉落 1-3 点经验
    i32 xp = experience::utils::randomPassiveMobExperience(rng);
    return spawnExperienceOrbs(world, x, y, z, xp, &rng);
}

// ============================================================================
// 怪物经验掉落
// ============================================================================

i32 ExperienceDropHandler::spawnHostileMobExperience(IWorld* world, f64 x, f64 y, f64 z, i32 baseXp, math::Random* rng)
{
    if (world == nullptr || baseXp <= 0) {
        return 0;
    }

    return spawnExperienceOrbs(world, x, y, z, baseXp, rng);
}

// ============================================================================
// 私有方法
// ============================================================================

ExperienceOrbEntity* ExperienceDropHandler::_createExperienceOrb(
    IWorld* world, f64 x, f64 y, f64 z, i32 xpValue, f32 vx, f32 vy, f32 vz)
{
    if (world == nullptr) {
        return nullptr;
    }

    // 创建经验球实体
    auto orb = std::make_unique<ExperienceOrbEntity>(world, x, y, z, xpValue);

    // 直接构造的实体需要显式设置 typeId（注册表路径会自动设置）
    orb->setTypeId(EntityTypeKeys::EXPERIENCE_ORB);

    // 设置初始速度
    orb->setVelocity(vx, vy, vz);

    // 获取原始指针用于返回
    ExperienceOrbEntity* orbPtr = orb.get();

    // 生成实体到世界
    EntityInstanceId entityId = world->spawnEntity(std::move(orb));
    if (entityId == EntityInstanceId(0)) {
        // 生成失败
        return nullptr;
    }

    return orbPtr;
}

} // namespace entity
} // namespace mc
