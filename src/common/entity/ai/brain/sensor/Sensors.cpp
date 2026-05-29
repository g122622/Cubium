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

#include "Sensors.hpp"
#include "../../../../entity/core/EntityUtils.hpp"
#include "../../../../world/GlobalPos.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/village/VillageManager.hpp"
#include "../../../../world/village/poi/PointOfInterestStorage.hpp"
#include "../../../../world/village/poi/PointOfInterestType.hpp"
#include "../../../entities/villager/VillagerEntity.hpp"
#include <algorithm>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace sensor {

// ============================================================================
// NearestPlayersSensor
// ============================================================================

template <typename E>
void NearestPlayersSensor<E>::update(IWorld* world, E* entity)
{
    if (!entity || !entity->isAlive()) {
        return;
    }

    // 获取附近玩家（16格范围）
    constexpr f32 RANGE = 16.0f;
    Vector3 pos = entity->position();

    auto players = world->getPlayers();

    // 过滤并排序玩家
    std::vector<Player*> nearbyPlayers;
    std::vector<Player*> visiblePlayers;

    for (Entity* playerEntity : players) {
        if (!playerEntity || playerEntity->isRemoved() || !playerEntity->isAlive()) {
            continue;
        }

        Player* player = dynamic_cast<Player*>(playerEntity);
        if (!player) {
            continue;
        }

        // 排除旁观者模式玩家
        if (player->isSpectator()) {
            continue;
        }

        // 检查距离
        f32 distSq = pos.distanceSquared(player->position());
        if (distSq > RANGE * RANGE) {
            continue;
        }

        nearbyPlayers.push_back(player);

        // 检查可见性
        if (entity->canSee(*player)) {
            visiblePlayers.push_back(player);
        }
    }

    // 按距离排序（近到远）
    std::sort(nearbyPlayers.begin(), nearbyPlayers.end(), [entity](Player* a, Player* b) {
        return entity->distanceSqTo(*a) < entity->distanceSqTo(*b);
    });

    std::sort(visiblePlayers.begin(), visiblePlayers.end(), [entity](Player* a, Player* b) {
        return entity->distanceSqTo(*a) < entity->distanceSqTo(*b);
    });

    // 存储到记忆模块
    entity->brain().setMemory(memory::MemoryModuleTypes::NEAREST_PLAYERS, nearbyPlayers);

    // 设置最近可见玩家
    if (!visiblePlayers.empty()) {
        entity->brain().setMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_PLAYER, visiblePlayers[0]);

        // 设置可攻击的最近可见玩家（非创造/旁观模式）
        for (Player* player : visiblePlayers) {
            if (!player->isCreative() && !player->isSpectator()) {
                entity->brain().setMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_TARGETABLE_PLAYER, player);
                break;
            }
        }
    } else {
        entity->brain().removeMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_PLAYER);
        entity->brain().removeMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_TARGETABLE_PLAYER);
    }
}

// ============================================================================
// NearestVisibleLivingEntitySensor
// ============================================================================

template <typename E>
void NearestVisibleLivingEntitySensor<E>::update(IWorld* world, E* entity)
{
    if (!entity || !entity->isAlive()) {
        return;
    }

    std::vector<LivingEntity*> visibleMobs;
    Vector3 pos = entity->position();

    // 获取范围内所有实体
    auto entities = world->getEntitiesInRange(pos, m_range, entity);

    for (Entity* e : entities) {
        if (!e || !e->isAlive()) {
            continue;
        }

        LivingEntity* living = dynamic_cast<LivingEntity*>(e);
        if (!living || living == entity) {
            continue;
        }

        // 检查可见性
        if (entity->canSee(*living)) {
            visibleMobs.push_back(living);
        }
    }

    entity->brain().setMemory(memory::MemoryModuleTypes::VISIBLE_MOBS, visibleMobs);
}

// ============================================================================
// HurtBySensor
// ============================================================================

template <typename E>
void HurtBySensor<E>::update(IWorld* world, E* entity)
{
    if (!entity || !entity->isAlive()) {
        return;
    }

    // 获取最后伤害来源
    DamageSource* lastDamageSource = entity->lastDamageSource();
    if (lastDamageSource) {
        // 存储伤害来源（100 tick = 5秒）
        entity->brain().setMemoryWithTTL(memory::MemoryModuleTypes::HURT_BY, lastDamageSource, 100);

        // 获取攻击者
        Entity* attacker = lastDamageSource->getTrueSource();
        if (attacker && attacker->isAlive()) {
            LivingEntity* livingAttacker = dynamic_cast<LivingEntity*>(attacker);
            if (livingAttacker) {
                entity->brain().setMemoryWithTTL(memory::MemoryModuleTypes::HURT_BY_ENTITY, livingAttacker, 100);
            }
        } else {
            entity->brain().removeMemory(memory::MemoryModuleTypes::HURT_BY_ENTITY);
        }
    } else {
        entity->brain().removeMemory(memory::MemoryModuleTypes::HURT_BY);
    }

    // 检查攻击者是否还有效（存活且在同一世界）
    auto attackerMemory = entity->brain().template getMemory<LivingEntity*>(memory::MemoryModuleTypes::HURT_BY_ENTITY);
    if (attackerMemory.has_value()) {
        LivingEntity* attacker = attackerMemory.value();
        if (!attacker || !attacker->isAlive() || attacker->world() != world) {
            entity->brain().removeMemory(memory::MemoryModuleTypes::HURT_BY_ENTITY);
        }
    }
}

// ============================================================================
// MobSensor
// ============================================================================

template <typename E>
void MobSensor<E>::update(IWorld* world, E* entity)
{
    if (!entity || !entity->isAlive()) {
        return;
    }

    std::vector<LivingEntity*> nearbyMobs;
    LivingEntity* nearestHostile = nullptr;
    f32 minHostileDist = m_range;
    Vector3 pos = entity->position();

    // 获取范围内所有实体
    auto entities = world->getEntitiesInRange(pos, m_range, entity);

    for (Entity* e : entities) {
        LivingEntity* living = dynamic_cast<LivingEntity*>(e);
        if (!living || living == entity || !living->isAlive()) {
            continue;
        }

        nearbyMobs.push_back(living);

        // 检查是否是敌对生物（非玩家）
        Player* player = dynamic_cast<Player*>(living);
        if (!player && !living->isRemoved()) {
            // 简化判断：如果是 MobEntity 且不是被动生物，视为敌对
            MobEntity* mob = dynamic_cast<MobEntity*>(living);
            if (mob) {
                f32 dist = entity->distanceTo(*living);
                if (dist < minHostileDist) {
                    minHostileDist = dist;
                    nearestHostile = living;
                }
            }
        }
    }

    entity->brain().setMemory(memory::MemoryModuleTypes::MOBS, nearbyMobs);

    if (nearestHostile) {
        entity->brain().setMemory(memory::MemoryModuleTypes::NEAREST_HOSTILE, nearestHostile);
    } else {
        entity->brain().removeMemory(memory::MemoryModuleTypes::NEAREST_HOSTILE);
    }
}

// ============================================================================
// WorkStationSensor
// ============================================================================

template <typename E>
void WorkStationSensor<E>::update(IWorld* world, E* entity)
{
    if (!entity || !entity->isAlive()) {
        return;
    }

    // 获取 VillageManager（只有 ServerWorld 有）
    auto* villageManager = world->villageManager();
    if (!villageManager) {
        return;
    }

    auto& poiStorage = villageManager->getPOIStorage();
    BlockPos entityPos(static_cast<i32>(entity->x()), static_cast<i32>(entity->y()), static_cast<i32>(entity->z()));

    // 查找工作站点（简化实现：查找最近的工作站类型 POI）
    constexpr f32 SEARCH_RANGE = 48.0f;

    // 查找最近的工作站
    auto jobSite = poiStorage.findNearestUnacquired(entityPos,
        world::village::poi::PointOfInterestType::Smoker, // 默认类型，实际应根据村民职业
        SEARCH_RANGE,
        static_cast<u64>(entity->id()));

    if (jobSite.has_value()) {
        GlobalPos globalPos(world->dimension(), jobSite.value());
        entity->brain().setMemory(memory::MemoryModuleTypes::JOB_SITE, globalPos);
    }

    // 查找潜在工作站点
    auto potentialSite =
        poiStorage.findNearest(entityPos, world::village::poi::PointOfInterestType::Smoker, SEARCH_RANGE);

    if (potentialSite.has_value()) {
        GlobalPos globalPos(world->dimension(), potentialSite.value());
        entity->brain().setMemory(memory::MemoryModuleTypes::POTENTIAL_JOB_SITE, globalPos);
    }
}

// ============================================================================
// VillagePoiSensor
// ============================================================================

template <typename E>
void VillagePoiSensor<E>::update(IWorld* world, E* entity)
{
    if (!entity || !entity->isAlive()) {
        return;
    }

    // 获取 VillageManager（只有 ServerWorld 有）
    auto* villageManager = world->villageManager();
    if (!villageManager) {
        return;
    }

    auto& poiStorage = villageManager->getPOIStorage();
    BlockPos entityPos(static_cast<i32>(entity->x()), static_cast<i32>(entity->y()), static_cast<i32>(entity->z()));

    constexpr f32 SEARCH_RANGE = 48.0f;

    // 查找最近的床（家）
    // 检查所有床颜色类型
    using POIType = world::village::poi::PointOfInterestType;
    std::vector<POIType> bedTypes = {POIType::BedRed,
        POIType::BedBlack,
        POIType::BedBlue,
        POIType::BedBrown,
        POIType::BedCyan,
        POIType::BedGray,
        POIType::BedGreen,
        POIType::BedLightBlue,
        POIType::BedLightGray,
        POIType::BedLime,
        POIType::BedMagenta,
        POIType::BedOrange,
        POIType::BedPink,
        POIType::BedPurple,
        POIType::BedWhite,
        POIType::BedYellow};

    BlockPos nearestBedPos;
    f32 nearestBedDist = SEARCH_RANGE;
    for (auto bedType : bedTypes) {
        auto bed = poiStorage.findNearestFree(entityPos, bedType, SEARCH_RANGE);
        if (bed.has_value()) {
            const BlockPos& bedPos = bed.value();
            f32 dx = static_cast<f32>(bedPos.x - entityPos.x);
            f32 dy = static_cast<f32>(bedPos.y - entityPos.y);
            f32 dz = static_cast<f32>(bedPos.z - entityPos.z);
            f32 dist = Vector3(dx, dy, dz).length();
            if (dist < nearestBedDist) {
                nearestBedDist = dist;
                nearestBedPos = bedPos;
            }
        }
    }

    if (nearestBedDist < SEARCH_RANGE) {
        GlobalPos homePos(world->dimension(), nearestBedPos);
        entity->brain().setMemory(memory::MemoryModuleTypes::HOME, homePos);
        entity->brain().setMemory(memory::MemoryModuleTypes::NEAREST_BED, nearestBedPos);
    }

    // 查找钟（集会点）
    auto meetingPoint = poiStorage.findNearestFree(entityPos, POIType::Bell, SEARCH_RANGE);

    if (meetingPoint.has_value()) {
        GlobalPos globalPos(world->dimension(), meetingPoint.value());
        entity->brain().setMemory(memory::MemoryModuleTypes::MEETING_POINT, globalPos);
    }
}

// ============================================================================
// BabySensor
// ============================================================================

template <typename E>
void BabySensor<E>::update(IWorld* world, E* entity)
{
    if (!entity || !entity->isAlive()) {
        return;
    }

    std::vector<LivingEntity*> babies;
    AgeableEntity* nearestAdult = nullptr;
    f32 minAdultDist = 16.0f;
    Vector3 pos = entity->position();

    // 获取范围内所有实体
    auto entities = world->getEntitiesInRange(pos, 16.0f, entity);

    for (Entity* e : entities) {
        AgeableEntity* ageable = dynamic_cast<AgeableEntity*>(e);
        if (!ageable || ageable == entity || !ageable->isAlive()) {
            continue;
        }

        if (ageable->isChild()) {
            // 幼年实体
            babies.push_back(static_cast<LivingEntity*>(ageable));
        } else {
            // 成年实体
            f32 dist = entity->distanceTo(*ageable);
            if (dist < minAdultDist) {
                minAdultDist = dist;
                nearestAdult = ageable;
            }
        }
    }

    entity->brain().setMemory(memory::MemoryModuleTypes::VISIBLE_VILLAGER_BABIES, babies);

    if (nearestAdult) {
        entity->brain().setMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT, nearestAdult);
    } else {
        entity->brain().removeMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT);
    }
}

// ============================================================================
// AvoidEntitySensor
// ============================================================================

template <typename E>
void AvoidEntitySensor<E>::update(IWorld* world, E* entity)
{
    if (!entity || !entity->isAlive()) {
        return;
    }

    LivingEntity* avoidTarget = nullptr;
    f32 minDist = m_range;
    Vector3 pos = entity->position();

    // 获取范围内所有实体
    auto entities = world->getEntitiesInRange(pos, m_range, entity);

    for (Entity* e : entities) {
        LivingEntity* living = dynamic_cast<LivingEntity*>(e);
        if (!living || living == entity || !living->isAlive()) {
            continue;
        }

        // 检查是否需要避险
        if (shouldAvoid(entity, living)) {
            f32 dist = entity->distanceTo(*living);
            if (dist < minDist) {
                minDist = dist;
                avoidTarget = living;
            }
        }
    }

    if (avoidTarget) {
        entity->brain().setMemoryWithTTL(memory::MemoryModuleTypes::AVOID_TARGET,
            avoidTarget,
            100 // 5秒
        );
    } else {
        entity->brain().removeMemory(memory::MemoryModuleTypes::AVOID_TARGET);
    }
}

template <typename E>
bool AvoidEntitySensor<E>::shouldAvoid(E* self, LivingEntity* other)
{
    // 默认实现：检查其他实体是否是敌对生物
    // 羊躲避狼、村民躲避僵尸等
    // 这里简化处理，实际应该根据实体类型判断

    // 检查其他实体是否是 MobEntity（可能是敌对的）
    MobEntity* mob = dynamic_cast<MobEntity*>(other);
    Player* player = dynamic_cast<Player*>(other);

    // 玩家在创造/旁观模式下不需要避险
    if (player && (player->isCreative() || player->isSpectator())) {
        return false;
    }

    // 简化：假设所有 MobEntity 都可能是危险源
    // 实际应用中应该根据实体类型精确判断
    return mob != nullptr;
}

// 显式实例化常用类型
template class NearestPlayersSensor<VillagerEntity>;
template class NearestVisibleLivingEntitySensor<VillagerEntity>;
template class HurtBySensor<VillagerEntity>;
template class MobSensor<VillagerEntity>;
template class WorkStationSensor<VillagerEntity>;
template class VillagePoiSensor<VillagerEntity>;
template class BabySensor<VillagerEntity>;
template class AvoidEntitySensor<VillagerEntity>;

} // namespace sensor
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
