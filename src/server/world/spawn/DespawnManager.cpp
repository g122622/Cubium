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

#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/world/ServerWorld.hpp"

#include <cmath>
#include <limits>
#include <vector>

using namespace mc::trace;

namespace mc::world::spawn {

void DespawnManager::tick(::mc::server::ServerWorld& world)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "DespawnManager::tick");

    if (!m_enabled) {
        return;
    }

    const Difficulty difficulty = world.difficulty();
    const u64 currentTick = world.currentTick();

    // 一次性取出玩家列表，避免每实体每 tick 重复 getPlayers()（后者会加锁+遍历全实体+分配 vector）。
    const auto players = world.entityManager().getPlayers();

    // 计算指定位置到最近非旁观玩家的距离平方；无玩家返回 max()（对应 getNearestPlayer 返回 null）。
    auto closestPlayerDistSq = [&](const Vector3& pos) -> f64 {
        f64 closest = std::numeric_limits<f64>::max();
        for (const Entity* entity : players) {
            if (entity == nullptr || entity->isRemoved()) {
                continue;
            }
            // 旁观模式玩家不计入距离检查
            const auto* player = dynamic_cast<const Player*>(entity);
            if (player == nullptr || player->isSpectator()) {
                continue;
            }
            const Vector3 playerPos = entity->position();
            const f64 dx = static_cast<f64>(playerPos.x) - pos.x;
            const f64 dy = static_cast<f64>(playerPos.y) - pos.y;
            const f64 dz = static_cast<f64>(playerPos.z) - pos.z;
            const f64 distSq = dx * dx + dy * dy + dz * dz;
            if (distSq < closest) {
                closest = distSq;
            }
        }
        return closest;
    };

    // Mob.checkDespawn：每实体每 tick 都检查，无每 tick 上限。
    world.entityManager().forEachEntity([&](Entity* entity) {
        auto* mob = dynamic_cast<MobEntity*>(entity);
        if (mob == nullptr || mob->isRemoved()) {
            return true;
        }

        // 无玩家时 closestPlayerDistSq 保持 max() → 保留实体。
        const f64 distSq = closestPlayerDistSq(mob->position());
        const f64 effectiveDistSq = (distSq == std::numeric_limits<f64>::max()) ? kNoPlayer : distSq;

        math::Random random(static_cast<u64>(mob->id()) + currentTick);
        if (shouldDespawn(*mob, effectiveDistSq, difficulty, currentTick, random)) {
            mob->remove();
        }
        return true;
    });
}

bool DespawnManager::shouldDespawn(
    MobEntity& mob, f64 closestPlayerDistSq, Difficulty difficulty, u64 currentTick, math::Random& random)
{
    (void)currentTick;

    // 已移除的实体不处理
    if (mob.isRemoved()) {
        return false;
    }

    // 和平难度下不允许的生物立即消失（isAllowedInPeaceful 取反）
    if (difficulty == Difficulty::Peaceful && mob.isDespawnPeaceful()) {
        return true;
    }

    // 持久化生物（命名/桶装/骑乘）重置空闲时间，永不消失
    if (mob.isNoDespawnRequired() || mob.preventDespawn()) {
        mob.setIdleTime(0);
        return false;
    }

    // 无玩家时保留（getNearestPlayer 返回 null 不做任何事）
    if (closestPlayerDistSq < 0.0) {
        return false;
    }

    // 获取分类消失距离
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* type = registry.getType(mob.getTypeId());
    if (type == nullptr) {
        return false;
    }
    const auto info = entity::EntityClassificationInfo::get(type->classification());
    const f64 instantDespawnDistSq = static_cast<f64>(info.despawnDistance) * info.despawnDistance;
    const f64 noDespawnDistSq = static_cast<f64>(info.randomDespawnDistance) * info.randomDespawnDistance;
    const f64 playerDist = std::sqrt(closestPlayerDistSq);

    // 立即消失距离检查
    if (closestPlayerDistSq > instantDespawnDistSq && mob.canDespawn(playerDist)) {
        return true;
    }

    // 随机消失距离检查：距玩家 > 32 且空闲 > 600 时 1/800 概率消失
    if (closestPlayerDistSq > noDespawnDistSq) {
        if (mob.idleTime() > MIN_IDLE_TIME) {
            if (random.nextInt(DESPAWN_CHANCE_DENOMINATOR) == 0 && mob.canDespawn(playerDist)) {
                return true;
            }
        }
    } else {
        // 玩家在 32 格内时重置空闲时间
        mob.setIdleTime(0);
    }

    return false;
}

bool DespawnManager::shouldDespawn(MobEntity& mob, f64 closestPlayerDistSq, Difficulty difficulty, u64 currentTick)
{
    math::Random random(static_cast<u64>(mob.id()) + currentTick);
    return shouldDespawn(mob, closestPlayerDistSq, difficulty, currentTick, random);
}

} // namespace mc::world::spawn
