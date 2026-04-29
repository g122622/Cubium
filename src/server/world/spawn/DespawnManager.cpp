#include "DespawnManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/combat/DifficultyHelper.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/entity/entities/player/Player.hpp"
#include <spdlog/spdlog.h>

namespace mc::world::spawn {

void DespawnManager::tick(::mc::server::ServerWorld& world) {
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
        return true;  // 继续遍历
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

bool DespawnManager::shouldDespawn(MobEntity& mob, ::mc::server::ServerWorld& world) const {
    // 已移除的实体不需要检查
    if (mob.isRemoved()) {
        return false;
    }

    // 获取实体类型信息
    auto& registry = entity::EntityRegistry::instance();
    const String& typeId = mob.getTypeId();
    const entity::EntityType* type = registry.getType(typeId);

    if (!type) {
        return false;
    }

    // 和平模式下，Monster 分类的实体应该立即消失
    // 参考 MC 1.16.5 MobEntity.checkDespawn()
    // 注意：MonsterEntity::isDespawnPeaceful() 默认返回 true
    if (type->classification() == entity::EntityClassification::Monster
        && !entity::combat::DifficultyHelper::allowsMobSpawning(world.difficulty())) {
        return true;
    }

    // 只有 Monster 分类才进行距离消失检查
    // 动物、环境生物等不会自然消失
    if (type->classification() != entity::EntityClassification::Monster) {
        return false;
    }

    // 持久化实体不会消失（如命名牌命名的生物）
    // TODO: 添加持久化标记检查

    // 检查距离消失条件
    return checkDistanceDespawn(mob, world);
}

bool DespawnManager::checkDistanceDespawn(MobEntity& mob, ::mc::server::ServerWorld& world) const {
    f64 playerDistSq = getClosestPlayerDistanceSq(world, mob.position());

    // 距离玩家 > 128 格：立即消失
    if (playerDistSq > INSTANT_DESPAWN_DISTANCE_SQ) {
        return true;
    }

    // 距离玩家 > 32 格 且 空闲时间 > 600 tick：随机消失
    if (playerDistSq > RANDOM_DESPAWN_DISTANCE_SQ) {
        i32 idleTime = mob.idleTime();
        if (idleTime > MIN_IDLE_TIME) {
            // 1/800 的概率消失
            // MC 1.16.5 使用实体ID和tick作为随机种子
            math::Random random(static_cast<u64>(mob.id()) + static_cast<u64>(world.currentTick()));
            i32 chance = random.nextInt(DESPAWN_CHANCE_DENOMINATOR);
            if (chance == 0) {
                return true;
            }
        }
    }

    return false;
}

f64 DespawnManager::getClosestPlayerDistanceSq(::mc::server::ServerWorld& world, const Vector3& pos) const {
    f64 closestDistSq = std::numeric_limits<f64>::max();

    auto players = world.entityManager().getPlayers();
    for (const Entity* player : players) {
        if (!player || player->isRemoved()) {
            continue;
        }

        Vector3 playerPos = player->position();
        f64 dx = playerPos.x - pos.x;
        f64 dy = playerPos.y - pos.y;
        f64 dz = playerPos.z - pos.z;
        f64 distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < closestDistSq) {
            closestDistSq = distSq;
        }
    }

    return closestDistSq;
}

} // namespace mc::world::spawn
