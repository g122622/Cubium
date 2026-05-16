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

#include "DespawnManager.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::world::spawn {

void DespawnManager::tick(::mc::server::ServerWorld& world)
{
    if (!m_enabled) {
        return;
    }

    // 获取所有实体
    std::vector<MobEntity*> mobsToCheck;

    world.entityManager().forEachEntity([&mobsToCheck](Entity* entity) {
        // 只检查生物实体
        if (auto* mob = dynamic_cast<MobEntity*>(entity)) {
            if (!mob->isRemoved()) {
                mobsToCheck.push_back(mob);
            }
        }
        return true; // 继续遍历
    });

    // 检查实体数量限制
    i32 checksThisTick = 0;

    for (auto* mob : mobsToCheck) {
        if (checksThisTick >= MAX_CHECKS_PER_TICK) {
            break;
        }

        if (shouldDespawn(*mob, world)) {
            mob->remove();
            spdlog::debug("DespawnManager: Despawning mob {} at ({}, {}, {})",
                mob->id(),
                mob->position().x,
                mob->position().y,
                mob->position().z);
        }

        ++checksThisTick;
    }
}

bool DespawnManager::shouldDespawn(MobEntity& mob, ::mc::server::ServerWorld& world) const
{
    // 已移除的实体不需要检查
    if (mob.isRemoved()) {
        return false;
    }

    // MC 1.16.5: checkDespawn() 第一步 - 和平模式下特定生物消失
    // MonsterEntity.isDespawnPeaceful() 返回 true
    if (world.difficulty() == Difficulty::Peaceful && mob.isDespawnPeaceful()) {
        return true;
    }

    // MC 1.16.5: 持久化实体或正在骑乘的实体不会消失
    // isNoDespawnRequired() || preventDespawn() 时，重置空闲时间并返回
    if (mob.isNoDespawnRequired() || mob.preventDespawn()) {
        mob.setIdleTime(0);
        return false;
    }

    // MC 1.16.5: 获取实体类型信息
    auto& registry = entity::EntityRegistry::instance();
    const std::string& typeId = mob.getTypeId();
    const entity::EntityType* type = registry.getType(typeId);

    if (!type) {
        return false;
    }

    // 获取分类的消失距离
    auto classification = type->classification();
    auto info = entity::EntityClassificationInfo::get(classification);
    f64 instantDespawnDistSq = static_cast<f64>(info.despawnDistance) * info.despawnDistance;
    f64 randomDespawnDistSq = static_cast<f64>(info.randomDespawnDistance) * info.randomDespawnDistance;

    // 获取最近玩家距离 - 使用 IWorld 接口方法
    f64 playerDistSq = world.getClosestPlayerDistanceSq(mob.position());

    // MC 1.16.5: 如果没有玩家，立即消失
    if (playerDistSq == std::numeric_limits<f64>::max()) {
        return mob.canDespawn(std::numeric_limits<f64>::max());
    }

    f64 playerDist = std::sqrt(playerDistSq);

    // MC 1.16.5: 立即消失距离检查
    if (playerDistSq > instantDespawnDistSq && mob.canDespawn(playerDist)) {
        return true;
    }

    // MC 1.16.5: 随机消失距离检查
    if (playerDistSq > randomDespawnDistSq) {
        i32 idleTime = mob.idleTime();
        if (idleTime > MIN_IDLE_TIME) {
            // 1/800 的概率消失
            // MC 1.16.5 使用实体ID和tick作为随机种子
            math::Random random(static_cast<u64>(mob.id()) + static_cast<u64>(world.currentTick()));
            i32 chance = random.nextInt(DESPAWN_CHANCE_DENOMINATOR);
            if (chance == 0 && mob.canDespawn(playerDist)) {
                return true;
            }
        }
    } else {
        // MC 1.16.5: 玩家在随机消失距离内时，重置空闲时间
        mob.setIdleTime(0);
    }

    return false;
}

} // namespace mc::world::spawn
