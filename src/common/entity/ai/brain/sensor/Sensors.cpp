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
#include "common/core/Types.hpp"
#include "common/entity/ai/brain/memory/MemoryModuleType.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/AgeableEntity.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/ecs/components/MobFlagComponent.hpp"
#include "common/entity/entities/passive/tamable/TameableEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/entities/villager/ProfessionMapping.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/GlobalPos.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/DoorBlock.hpp"
#include "common/world/village/VillageManager.hpp"
#include "common/world/village/poi/PointOfInterestStorage.hpp"
#include "common/world/village/poi/PointOfInterestType.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

    // 存储到记忆模块（存 id：id 永不悬垂，消费方经 getEntity(id) 反查 + isAlive 校验）
    std::vector<EntityInstanceId> nearbyPlayerIds;
    nearbyPlayerIds.reserve(nearbyPlayers.size());
    for (Player* player : nearbyPlayers) {
        nearbyPlayerIds.push_back(player->id());
    }
    entity->brain().setMemory(memory::MemoryModuleTypes::NEAREST_PLAYERS, nearbyPlayerIds);

    // 设置最近可见玩家
    if (!visiblePlayers.empty()) {
        entity->brain().setMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_PLAYER, visiblePlayers[0]->id());

        // 设置可攻击的最近可见玩家（非创造/旁观模式）
        for (Player* player : visiblePlayers) {
            if (!player->isCreative() && !player->isSpectator()) {
                entity->brain().setMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_TARGETABLE_PLAYER, player->id());
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

    // 存 id 而非 LivingEntity*：sensor 20 tick 重扫窗口内实体可能已析构，
    // 消费方（如 LookAtEntityTask）拿到裸指针遍历即 UAF（mob_behavior 全量跑崩溃根因）。
    // id 永不悬垂（单调递增不复用），消费方经 world->getEntity(id) 反查 + isAlive 校验。
    std::vector<EntityInstanceId> visibleMobIds;
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
            visibleMobIds.push_back(living->id());
        }
    }

    entity->brain().setMemory(memory::MemoryModuleTypes::VISIBLE_MOBS, visibleMobIds);
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

    // 获取最后伤害来源（带 40 tick 过期守卫，对齐 vanilla getLastDamageSource:1391-1397）。
    // 注意：lastDamageSource() 返回的 DamageSource* 指向村民自有的 m_lastDamageSource，村民存活期间
    // 有效（过期时 getter 已 reset 返回 nullptr）。HURT_BY memory 存此指针供 FindHiddenBlockTask
    // 判 presence（不解引用，见 MovementTasks.hpp:525），故 HURT_BY 路径安全。
    DamageSource* lastDamageSource = entity->lastDamageSource();
    if (lastDamageSource) {
        // 存储伤害来源（100 tick = 5秒）
        entity->brain().setMemoryWithTTL(memory::MemoryModuleTypes::HURT_BY, lastDamageSource, 100);

        // 获取攻击者——经 lastDamageSourceTrueId() + IWorld::getEntity(id) 安全校验（任务 #272 根治）。
        // 对齐 vanilla HurtBySensor:23-26 用 getLastDamageSource().getEntity() 取攻击者，但 vanilla
        // 靠 Java GC 保证 getEntity() 返回的引用安全；Cubium 无 GC，m_lastDamageSource clone 持真凶
        // 裸 Entity* 指针，真凶析构后 getTrueSource() 返回悬垂指针，直接解引用（取 id/isAlive）即 UAF
        // 段错误。故改用 actuallyHurt 同步上下文捕获的 m_lastDamageSourceTrueId（真凶必活时取 id），
        // 经 world->getEntity(id) 校验：真凶析构后返回 nullptr，不再解引用悬垂指针。
        // id 永不悬垂（EntityInstanceId 单调递增不复用）。
        EntityInstanceId attackerId = entity->lastDamageSourceTrueId();
        Entity* attacker =
            (attackerId != INVALID_ENTITY_ID && world != nullptr) ? world->getEntity(attackerId) : nullptr;
        if (attacker && attacker->isAlive()) {
            // HURT_BY_ENTITY 存 id（同 VISIBLE_MOBS 的 UAF 根治理由）
            entity->brain().setMemoryWithTTL(memory::MemoryModuleTypes::HURT_BY_ENTITY, attacker->id(), 100);
        } else {
            entity->brain().removeMemory(memory::MemoryModuleTypes::HURT_BY_ENTITY);
        }
    } else {
        entity->brain().removeMemory(memory::MemoryModuleTypes::HURT_BY);
        entity->brain().removeMemory(memory::MemoryModuleTypes::HURT_BY_ENTITY);
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

    // 存 id（UAF 根治理由见 NearestVisibleLivingEntitySensor）
    std::vector<EntityInstanceId> nearbyMobIds;
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

        nearbyMobIds.push_back(living->id());
    }

    entity->brain().setMemory(memory::MemoryModuleTypes::MOBS, nearbyMobIds);
    // 注意：MobSensor 仅收集 MOBS 列表，不负责 NEAREST_HOSTILE。
    // NEAREST_HOSTILE 应由专门的传感器（如 VillagerHostilesSensor）来设置，
    // 因为不同实体对"敌对生物"的定义不同。
}

// ============================================================================
// VillagerHostilesSensor
// ============================================================================

template <typename E>
std::unordered_map<const entity::EntityType*, f32> VillagerHostilesSensor<E>::createHostileDistanceMap()
{
    // 参考原版 VillagerHostilesSensor.ACCEPTABLE_DISTANCE_FROM_HOSTILES
    // 每种敌对生物有独立的检测距离阈值，而非统一使用 MobEntity 判断
    // key 为注册表内 const EntityType* 指针，与 VillagerHostilesSensor 调用方
    // entity->entityType() 同源（均来自 EntityRegistry::m_types），可安全指针 hash/比较。
    return {
        {entity::VanillaEntityTypeKeys::DROWNED, 8.0f},
        {entity::VanillaEntityTypeKeys::EVOKER, 12.0f},
        {entity::VanillaEntityTypeKeys::HUSK, 8.0f},
        {entity::VanillaEntityTypeKeys::ILLUSIONER, 12.0f},
        {entity::VanillaEntityTypeKeys::PILLAGER, 15.0f},
        {entity::VanillaEntityTypeKeys::RAVAGER, 12.0f},
        {entity::VanillaEntityTypeKeys::VEX, 8.0f},
        {entity::VanillaEntityTypeKeys::VINDICATOR, 10.0f},
        {entity::VanillaEntityTypeKeys::ZOGLIN, 10.0f},
        {entity::VanillaEntityTypeKeys::ZOMBIE, 8.0f},
        {entity::VanillaEntityTypeKeys::ZOMBIE_VILLAGER, 8.0f},
    };
}

template <typename E>
f32 VillagerHostilesSensor<E>::getHostileDetectionRange(const LivingEntity* entity)
{
    static const auto hostileMap = createHostileDistanceMap();
    const entity::EntityType* type = entity->entityType();
    if (type == nullptr) {
        return 0.0f;
    }
    auto it = hostileMap.find(type);
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

    // 存 id（UAF 根治理由见 NearestVisibleLivingEntitySensor）
    std::vector<EntityInstanceId> nearbyMobIds;
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

        nearbyMobIds.push_back(living->id());

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

    entity->brain().setMemory(memory::MemoryModuleTypes::MOBS, nearbyMobIds);

    if (nearestHostile) {
        entity->brain().setMemory(memory::MemoryModuleTypes::NEAREST_HOSTILE, nearestHostile->id());
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

    // 存 id（UAF 根治理由见 NearestVisibleLivingEntitySensor）
    std::vector<EntityInstanceId> babyIds;
    EntityInstanceId nearestAdultId = INVALID_ENTITY_ID;
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
            babyIds.push_back(ageable->id());
        } else {
            // 成年实体
            f32 dist = entity->distanceTo(*ageable);
            if (dist < minAdultDist) {
                minAdultDist = dist;
                nearestAdultId = ageable->id();
            }
        }
    }

    entity->brain().setMemory(memory::MemoryModuleTypes::VISIBLE_VILLAGER_BABIES, babyIds);

    if (nearestAdultId != INVALID_ENTITY_ID) {
        entity->brain().setMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT, nearestAdultId);
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

    // 存 id（UAF 根治理由见 NearestVisibleLivingEntitySensor）
    EntityInstanceId avoidTargetId = INVALID_ENTITY_ID;
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
                avoidTargetId = living->id();
            }
        }
    }

    if (avoidTargetId != INVALID_ENTITY_ID) {
        entity->brain().setMemoryWithTTL(memory::MemoryModuleTypes::AVOID_TARGET,
            avoidTargetId,
            100 // 5秒
        );
    } else {
        entity->brain().removeMemory(memory::MemoryModuleTypes::AVOID_TARGET);
    }
}

template <typename E>
bool AvoidEntitySensor<E>::shouldAvoid(E* self, LivingEntity* other)
{
    // 使用 MobFlagComponent 标记组件判断敌对生物。
    // 只有继承 MonsterEntity 的实体才 attach MobFlagComponent（IMob 接口的 tag 层），
    // 被动生物（牛、羊、猪等）不会触发避险。
    if (other->hasComponent<ecs::MobFlagComponent>()) {
        // 玩家在创造/旁观模式下不需要避险
        Player* player = dynamic_cast<Player*>(other);
        if (player && (player->isCreative() || player->isSpectator())) {
            return false;
        }
        return true;
    }

    return false;
}

// ============================================================================
// TemptingPlayerSensor
// ============================================================================

template <typename E>
void TemptingPlayerSensor<E>::update(IWorld* world, E* entity)
{
    if (!entity || !entity->isAlive()) {
        return;
    }

    auto& brain = entity->brain();

    // 获取附近玩家
    auto players = world->getPlayers();
    Vector3 pos = entity->position();

    Player* nearestTemptingPlayer = nullptr;
    f32 nearestDistSq = m_range * m_range;

    for (Entity* playerEntity : players) {
        if (!playerEntity || playerEntity->isRemoved() || !playerEntity->isAlive()) {
            continue;
        }

        Player* player = dynamic_cast<Player*>(playerEntity);
        if (!player) {
            continue;
        }

        // 排除旁观者模式玩家（参考 MC 原版 TemptingSensor）
        if (player->isSpectator()) {
            continue;
        }

        // 排除被该玩家骑乘的情况（参考 MC 原版：!mob.hasPassenger(player)）
        if (entity->isPassenger(player->id())) {
            continue;
        }

        // 检查距离
        f32 distSq = pos.distanceSquared(player->position());
        if (distSq > nearestDistSq) {
            continue;
        }

        // 检查玩家手持物品是否为诱惑物品
        const ItemStack& mainHand = player->getMainHandItem();
        const ItemStack& offHand = player->getOffHandItem();
        bool isTempting = m_itemPredicate(mainHand) || m_itemPredicate(offHand);

        if (!isTempting) {
            continue;
        }

        // 检查可见性
        if (!entity->canSee(*player)) {
            continue;
        }

        nearestDistSq = distSq;
        nearestTemptingPlayer = player;
    }

    if (nearestTemptingPlayer) {
        // 存 id（UAF 根治理由见 NearestVisibleLivingEntitySensor）
        brain.setMemory(memory::MemoryModuleTypes::TEMPTING_PLAYER, nearestTemptingPlayer->id());
    } else {
        brain.removeMemory(memory::MemoryModuleTypes::TEMPTING_PLAYER);
    }
}

// ============================================================================
// InteractableDoorsSensor
// ============================================================================

template <typename E>
void InteractableDoorsSensor<E>::update(IWorld* world, E* entity)
{
    if (!entity || !entity->isAlive()) {
        return;
    }

    auto& brain = entity->brain();

    // 检查实体是否能够使用门（canEnterDoors 或 canOpenDoors）
    auto* navigator = entity->navigator();
    if (!navigator) {
        brain.removeMemory(memory::MemoryModuleTypes::INTERACTABLE_DOORS);
        return;
    }

    if (!navigator->canEnterDoors()) {
        brain.removeMemory(memory::MemoryModuleTypes::INTERACTABLE_DOORS);
        return;
    }

    // 参考 MC 原版 InteractWithDoor 行为：
    // 扫描实体附近的木门，将可交互的门位置存入 INTERACTABLE_DOORS 记忆。
    // MC 原版在 InteractWithDoor 行为中直接检查路径上的门节点，
    // 但我们这里先做基于范围的门扫描（与 MC 原版 Villager 登记门的逻辑一致），
    // 然后在实际交互时由 InteractWithDoorTask 根据路径判断是否需要开门。
    Vector3 pos = entity->position();
    BlockPos entityBlockPos(
        static_cast<i32>(std::floor(pos.x)), static_cast<i32>(std::floor(pos.y)), static_cast<i32>(std::floor(pos.z)));

    std::vector<GlobalPos> interactableDoors;
    DimensionId dimension = entity->dimension();

    // 扫描范围内的方块，寻找木门
    i32 rangeInt = static_cast<i32>(std::ceil(m_range));
    for (i32 dx = -rangeInt; dx <= rangeInt; ++dx) {
        for (i32 dy = -1; dy <= 2; ++dy) { // 检查门可能占据的 y 范围（底部和上半部分）
            for (i32 dz = -rangeInt; dz <= rangeInt; ++dz) {
                BlockPos checkPos(entityBlockPos.x + dx, entityBlockPos.y + dy, entityBlockPos.z + dz);

                const BlockState* state = world->getBlockState(checkPos);
                if (!state) {
                    continue;
                }

                // 检查是否为木门（isWooden 已排除铁门等非木门）
                if (!blocks::DoorBlock::isWooden(*state)) {
                    continue;
                }

                // 检查是否为门的上半部分，如果是则跳过
                // DoorBlock 使用 DOUBLE_BLOCK_HALF 属性：Lower 为下半部分，Upper 为上半部分
                // 我们只登记下半部分的位置，避免重复
                if (state->get(BlockStateProperties::DOUBLE_BLOCK_HALF()) ==
                    BlockStateProperties::DoubleBlockHalf::Upper) {
                    continue;
                }

                interactableDoors.emplace_back(dimension, checkPos);
            }
        }
    }

    brain.setMemory(memory::MemoryModuleTypes::INTERACTABLE_DOORS, interactableDoors);

    // 如果 OPENED_DOORS 记忆不存在，初始化为空集合
    auto openedDoors = brain.template getMemory<std::unordered_set<GlobalPos>>(memory::MemoryModuleTypes::OPENED_DOORS);
    if (!openedDoors.has_value()) {
        brain.setMemory(memory::MemoryModuleTypes::OPENED_DOORS, std::unordered_set<GlobalPos>{});
    }
}

// ============================================================================
// OwnerHurtBySensor
// ============================================================================

template <typename E>
void OwnerHurtBySensor<E>::update(IWorld* world, E* entity)
{
    if (!entity || !entity->isAlive()) {
        return;
    }

    auto& brain = entity->brain();

    // 仅适用于 TameableEntity 子类
    auto* tameable = dynamic_cast<TameableEntity*>(entity);
    if (!tameable || !tameable->isTamed()) {
        brain.removeMemory(memory::MemoryModuleTypes::OWNER_HURT_BY);
        return;
    }

    Player* owner = tameable->getOwner();
    if (!owner || !owner->isAlive()) {
        brain.removeMemory(memory::MemoryModuleTypes::OWNER_HURT_BY);
        return;
    }

    // 获取主人最后被攻击的实体
    LivingEntity* ownerHurtBy = owner->getLastHurtBy();
    if (ownerHurtBy && ownerHurtBy->isAlive()) {
        // 不要让宠物攻击自己主人（如果主人被自己伤害则无意义）
        if (ownerHurtBy != static_cast<LivingEntity*>(entity) && ownerHurtBy != owner) {
            // 参考 MC 原版 OwnerHurtByTargetGoal：
            // 使用 lastHurtByTimestamp 判断是否有新的伤害事件
            // 这里简单地将攻击者 id 写入记忆（UAF 根治理由见 NearestVisibleLivingEntitySensor），
            // 由 ProtectOwnerTask 经 getEntity(id) 反查后判断是否执行攻击
            brain.setMemoryWithTTL(memory::MemoryModuleTypes::OWNER_HURT_BY, ownerHurtBy->id(), 100);
        } else {
            brain.removeMemory(memory::MemoryModuleTypes::OWNER_HURT_BY);
        }
    } else {
        brain.removeMemory(memory::MemoryModuleTypes::OWNER_HURT_BY);
    }
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
template class InteractableDoorsSensor<VillagerEntity>;
template class TemptingPlayerSensor<VillagerEntity>;
template class OwnerHurtBySensor<VillagerEntity>;
// TODO: OwnerHurtBySensor 目前为 VillagerEntity 实例化用于测试，
// 但 VillagerEntity 不是 TameableEntity 子类，因此该传感器对 VillagerEntity 无实际效果。
// 当 TameableEntity 子类（WolfEntity、CatEntity 等）集成 Brain 系统后，
// 需要在对应实体的 initializeBrain() 中注册 OwnerHurtBySensor 并实例化对应模板。
// TODO: TemptingPlayerSensor 和 FollowParentTask/BabySensor 需要在 AnimalEntity 子类（如
// CowEntity、PigEntity、SheepEntity 等） 集成 Brain 系统后进行注册和实例化。

} // namespace sensor
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
