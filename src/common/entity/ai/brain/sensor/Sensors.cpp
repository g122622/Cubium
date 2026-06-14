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
#include "common/entity/core/EntityUtils.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/entities/villager/ProfessionMapping.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"
#include <algorithm>
#include <limits>

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
    Vector3 pos = entity->position();

    // 获取范围内所有实体
    auto entities = world->getEntitiesInRange(pos, m_range, entity);

    for (Entity* e : entities) {
        LivingEntity* living = dynamic_cast<LivingEntity*>(e);
        if (!living || living == entity || !living->isAlive()) {
            continue;
        }

        MobEntity* mob = dynamic_cast<MobEntity*>(living);
        if (!mob) {
            continue;
        }

        nearbyMobs.push_back(living);
    }

    entity->brain().setMemory(memory::MemoryModuleTypes::MOBS, nearbyMobs);
    // 注意：MobSensor 仅收集 MOBS 列表，不负责 NEAREST_HOSTILE。
    // NEAREST_HOSTILE 应由专门的传感器（如 VillagerHostilesSensor）来设置，
    // 因为不同实体对"敌对生物"的定义不同。
}

// ============================================================================
// VillagerHostilesSensor
// ============================================================================

template <typename E>
std::unordered_map<entity::EntityTypeId, f32> VillagerHostilesSensor<E>::createHostileDistanceMap()
{
    // 参考原版 VillagerHostilesSensor.ACCEPTABLE_DISTANCE_FROM_HOSTILES
    // 每种敌对生物有独立的检测距离阈值，而非统一使用 MobEntity 判断
    return {
        {entity::EntityTypeIdNumber::DROWNED, 8.0f},
        {entity::EntityTypeIdNumber::EVOKER, 12.0f},
        {entity::EntityTypeIdNumber::HUSK, 8.0f},
        {entity::EntityTypeIdNumber::ILLUSIONER, 12.0f},
        {entity::EntityTypeIdNumber::PILLAGER, 15.0f},
        {entity::EntityTypeIdNumber::RAVAGER, 12.0f},
        {entity::EntityTypeIdNumber::VEX, 8.0f},
        {entity::EntityTypeIdNumber::VINDICATOR, 10.0f},
        {entity::EntityTypeIdNumber::ZOGLIN, 10.0f},
        {entity::EntityTypeIdNumber::ZOMBIE, 8.0f},
        {entity::EntityTypeIdNumber::ZOMBIE_VILLAGER, 8.0f},
    };
}

template <typename E>
f32 VillagerHostilesSensor<E>::getHostileDetectionRange(const LivingEntity* entity)
{
    static const auto hostileMap = createHostileDistanceMap();
    auto it = hostileMap.find(entity->typeId());
    if (it != hostileMap.end()) {
        return it->second;
    }
    return 0.0f;
}

template <typename E>
void VillagerHostilesSensor<E>::update(IWorld* world, E* entity)
{
    if (!entity || !entity->isAlive()) {
        return;
    }

    std::vector<LivingEntity*> nearbyMobs;
    LivingEntity* nearestHostile = nullptr;
    f32 nearestHostileDistSq = std::numeric_limits<f32>::max();
    Vector3 pos = entity->position();

    // 获取范围内所有实体
    auto entities = world->getEntitiesInRange(pos, 16.0f, entity);

    for (Entity* e : entities) {
        LivingEntity* living = dynamic_cast<LivingEntity*>(e);
        if (!living || living == entity || !living->isAlive()) {
            continue;
        }

        MobEntity* mob = dynamic_cast<MobEntity*>(living);
        if (!mob) {
            continue;
        }

        nearbyMobs.push_back(living);

        // 使用精确的实体类型到距离映射来判断敌对生物
        f32 detectionRange = getHostileDetectionRange(living);
        if (detectionRange > 0.0f) {
            f32 distSq = entity->distanceSqTo(*living);
            if (distSq <= detectionRange * detectionRange && distSq < nearestHostileDistSq) {
                nearestHostileDistSq = distSq;
                nearestHostile = living;
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

    // 傻子村民不寻找工作站
    auto* villager = dynamic_cast<VillagerEntity*>(entity);
    if (villager && villager->isNitwit()) {
        entity->brain().removeMemory(memory::MemoryModuleTypes::JOB_SITE);
        entity->brain().removeMemory(memory::MemoryModuleTypes::POTENTIAL_JOB_SITE);
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

    using POIType = world::village::poi::PointOfInterestType;

    if (villager) {
        VillagerProfession profession = villager->profession();

        if (villager::ProfessionMapping::hasWorkstation(profession)) {
            // 有职业的村民：只搜索自己职业对应的工作站类型
            POIType workstationPOI = villager::ProfessionMapping::getWorkstationPOI(profession);

            // 查找已占用的最近工作站（JOB_SITE）
            auto jobSite = poiStorage.findNearestUnacquired(
                entityPos, workstationPOI, SEARCH_RANGE, static_cast<u64>(entity->id()));

            if (jobSite.has_value()) {
                GlobalPos globalPos(world->dimension(), jobSite.value());
                entity->brain().setMemory(memory::MemoryModuleTypes::JOB_SITE, globalPos);
            } else {
                entity->brain().removeMemory(memory::MemoryModuleTypes::JOB_SITE);
            }

            // 查找潜在工作站点（POTENTIAL_JOB_SITE）
            auto potentialSite = poiStorage.findNearest(entityPos, workstationPOI, SEARCH_RANGE);

            if (potentialSite.has_value()) {
                GlobalPos globalPos(world->dimension(), potentialSite.value());
                entity->brain().setMemory(memory::MemoryModuleTypes::POTENTIAL_JOB_SITE, globalPos);
            } else {
                entity->brain().removeMemory(memory::MemoryModuleTypes::POTENTIAL_JOB_SITE);
            }
        } else {
            // 无职业村民：搜索所有可获取的工作站类型，寻找潜在工作站点
            // 参考 MC 原版 AcquirePoi 行为，使用 acquirableJobSite 谓词搜索全部工作站POI
            const auto& allWorkstations = villager::ProfessionMapping::getAcquirableWorkstations();

            // 无职业村民不需要 JOB_SITE，清除旧记忆
            entity->brain().removeMemory(memory::MemoryModuleTypes::JOB_SITE);

            // 遍历所有工作站类型，找到最近的未占用工作站作为 POTENTIAL_JOB_SITE
            BlockPos nearestPos;
            f32 nearestDist = SEARCH_RANGE;
            bool found = false;

            for (POIType wsType : allWorkstations) {
                auto site =
                    poiStorage.findNearestUnacquired(entityPos, wsType, SEARCH_RANGE, static_cast<u64>(entity->id()));
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
                GlobalPos globalPos(world->dimension(), nearestPos);
                entity->brain().setMemory(memory::MemoryModuleTypes::POTENTIAL_JOB_SITE, globalPos);
            } else {
                entity->brain().removeMemory(memory::MemoryModuleTypes::POTENTIAL_JOB_SITE);
            }
        }
    } else {
        // 非VillagerEntity类型的实体（如流浪商人），不做工作站搜索
        entity->brain().removeMemory(memory::MemoryModuleTypes::JOB_SITE);
        entity->brain().removeMemory(memory::MemoryModuleTypes::POTENTIAL_JOB_SITE);
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
    // 使用 IMob 标记接口判断敌对生物。
    // 只有继承 MonsterEntity 并实现 IMob 接口的实体才被视为敌对。
    // 这与原版的 Enemy 接口语义一致：MonsterEntity implements Enemy(IMob)。
    // 被动生物（牛、羊、猪等）不会触发避险。
    entity::IMob* mob = dynamic_cast<entity::IMob*>(other);
    if (mob != nullptr) {
        // 玩家在创造/旁观模式下不需要避险
        Player* player = dynamic_cast<Player*>(other);
        if (player && (player->isCreative() || player->isSpectator())) {
            return false;
        }
        return true;
    }

    return false;
}

// 显式实例化常用类型
template class NearestPlayersSensor<VillagerEntity>;
template class NearestVisibleLivingEntitySensor<VillagerEntity>;
template class HurtBySensor<VillagerEntity>;
template class VillagerHostilesSensor<VillagerEntity>;
template class WorkStationSensor<VillagerEntity>;
template class VillagePoiSensor<VillagerEntity>;
template class BabySensor<VillagerEntity>;
template class AvoidEntitySensor<VillagerEntity>;

} // namespace sensor
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
