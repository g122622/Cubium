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
#include "common/perfetto/TraceEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/world/ServerWorld.hpp"

#include <cmath>
#include <limits>
#include <vector>

namespace mc::world::spawn {

void DespawnManager::tick(::mc::server::ServerWorld& world)
{
    MC_TRACE_EVENT("server.tick", "DespawnManager::tick");

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

        if (_shouldDespawn(*mob, world)) {
            mob->remove();
        }

        ++checksThisTick;
    }
}

bool DespawnManager::_shouldDespawn(MobEntity& mob, ::mc::server::ServerWorld& world) const
{
    // 已移除的实体不需要检查
    if (mob.isRemoved()) {
        return false;
    }

    // 和平模式下特定生物消失
    if (world.difficulty() == Difficulty::Peaceful && mob.isDespawnPeaceful()) {
        return true;
    }

    // 持久化实体或正在骑乘的实体不会消失
    if (mob.isNoDespawnRequired() || mob.preventDespawn()) {
        mob.setIdleTime(0);
        return false;
    }

    // 获取实体类型信息
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

    // 获取最近玩家距离
    f64 playerDistSq = world.getClosestPlayerDistanceSq(mob.position());

    // 如果没有玩家，立即消失
    if (playerDistSq == std::numeric_limits<f64>::max()) {
        return mob.canDespawn(std::numeric_limits<f64>::max());
    }

    f64 playerDist = std::sqrt(playerDistSq);

    // 立即消失距离检查
    if (playerDistSq > instantDespawnDistSq && mob.canDespawn(playerDist)) {
        return true;
    }

    // 随机消失距离检查
    if (playerDistSq > randomDespawnDistSq) {
        i32 idleTime = mob.idleTime();
        if (idleTime > MIN_IDLE_TIME) {
            // 1/800 的概率消失
            math::Random random(static_cast<u64>(mob.id()) + static_cast<u64>(world.currentTick()));
            i32 chance = random.nextInt(DESPAWN_CHANCE_DENOMINATOR);
            if (chance == 0 && mob.canDespawn(playerDist)) {
                return true;
            }
        }
    } else {
        // 玩家在随机消失距离内时，重置空闲时间
        mob.setIdleTime(0);
    }

    return false;
}

} // namespace mc::world::spawn
