#pragma once

#include "../Sensor.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../../world/IWorld.hpp"
#include <vector>

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace sensor {

/**
 * @brief 最近玩家传感器
 *
 * 检测附近的玩家并存储到记忆模块。
 *
 * 参考 MC 1.16.5 NearestPlayersSensor
 */
template <typename E>
class NearestPlayersSensor : public Sensor<E> {
public:
    NearestPlayersSensor()
        : Sensor<E>(20)  // 每20tick更新一次
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::NEAREST_PLAYERS,
            memory::MemoryModuleTypes::NEAREST_VISIBLE_PLAYER
        };
    }

protected:
    void update(ServerWorld* world, E* entity) override {
        if (!entity || !entity->isAlive()) {
            return;
        }

        // 获取附近玩家
        std::vector<Player*> nearbyPlayers;
        std::vector<Player*> visiblePlayers;

        // TODO: 从世界获取附近玩家
        // f32 range = 16.0f;
        // auto players = world->getPlayersInRange(entity->position(), range);

        // for (auto* player : players) {
        //     nearbyPlayers.push_back(player);
        //     if (entity->canSee(*player)) {
        //         visiblePlayers.push_back(player);
        //     }
        // }

        // 按距离排序
        // std::sort(nearbyPlayers.begin(), nearbyPlayers.end(),
        //     [entity](Player* a, Player* b) {
        //         return entity->distanceSqTo(*a) < entity->distanceSqTo(*b);
        //     });

        // 存储到记忆模块
        // entity->getBrain()->setMemory(memory::MemoryModuleTypes::NEAREST_PLAYERS, nearbyPlayers);

        // if (!visiblePlayers.empty()) {
        //     entity->getBrain()->setMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_PLAYER, visiblePlayers[0]);
        // } else {
        //     entity->getBrain()->removeMemory(memory::MemoryModuleTypes::NEAREST_VISIBLE_PLAYER);
        // }
    }
};

/**
 * @brief 最近可见生物传感器
 *
 * 检测附近可见的生物并存储到记忆模块。
 *
 * 参考 MC 1.16.5 NearestVisibleLivingEntitySensor
 */
template <typename E>
class NearestVisibleLivingEntitySensor : public Sensor<E> {
public:
    explicit NearestVisibleLivingEntitySensor(f32 range = 16.0f, i32 interval = 20)
        : Sensor<E>(interval)
        , m_range(range)
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::VISIBLE_MOBS,
            memory::MemoryModuleTypes::NEAREST_VISIBLE_NEMESIS
        };
    }

protected:
    void update(ServerWorld* world, E* entity) override {
        if (!entity || !entity->isAlive()) {
            return;
        }

        std::vector<LivingEntity*> visibleMobs;

        // TODO: 从世界获取附近生物
        // auto mobs = world->getLivingEntitiesInRange(entity->position(), m_range);

        // for (auto* mob : mobs) {
        //     if (mob != entity && mob->isAlive() && entity->canSee(*mob)) {
        //         visibleMobs.push_back(mob);
        //     }
        // }

        // entity->getBrain()->setMemory(memory::MemoryModuleTypes::VISIBLE_MOBS, visibleMobs);
    }

private:
    f32 m_range;
};

/**
 * @brief 受伤传感器
 *
 * 检测实体受到的伤害并存储到记忆模块。
 *
 * 参考 MC 1.16.5 HurtBySensor
 */
template <typename E>
class HurtBySensor : public Sensor<E> {
public:
    HurtBySensor()
        : Sensor<E>(1)  // 每tick检查
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::HURT_BY,
            memory::MemoryModuleTypes::HURT_BY_ENTITY
        };
    }

protected:
    void update(ServerWorld* world, E* entity) override {
        if (!entity || !entity->isAlive()) {
            return;
        }

        // TODO: 获取实体最近的伤害来源
        // auto lastDamageSource = entity->getLastDamageSource();
        // if (lastDamageSource) {
        //     entity->getBrain()->setMemoryWithTTL(
        //         memory::MemoryModuleTypes::HURT_BY,
        //         lastDamageSource,
        //         100  // 5秒
        //     );
        //
        //     auto attacker = lastDamageSource->getEntity();
        //     if (attacker && attacker->isAlive()) {
        //         LivingEntity* livingAttacker = dynamic_cast<LivingEntity*>(attacker);
        //         if (livingAttacker) {
        //             entity->getBrain()->setMemoryWithTTL(
        //                 memory::MemoryModuleTypes::HURT_BY_ENTITY,
        //                 livingAttacker,
        //                 100
        //             );
        //         }
        //     }
        // }
    }
};

/**
 * @brief 附近实体传感器
 *
 * 检测附近的所有实体并分类存储。
 *
 * 参考 MC 1.16.5 MobSensor
 */
template <typename E>
class MobSensor : public Sensor<E> {
public:
    explicit MobSensor(f32 range = 16.0f, i32 interval = 20)
        : Sensor<E>(interval)
        , m_range(range)
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::MOBS,
            memory::MemoryModuleTypes::NEAREST_HOSTILE
        };
    }

protected:
    void update(ServerWorld* world, E* entity) override {
        if (!entity || !entity->isAlive()) {
            return;
        }

        std::vector<LivingEntity*> nearbyMobs;
        LivingEntity* nearestHostile = nullptr;
        f32 minHostileDist = m_range;

        // TODO: 从世界获取附近实体
        // auto entities = world->getEntitiesInRange(entity->position(), m_range);

        // for (auto* e : entities) {
        //     LivingEntity* living = dynamic_cast<LivingEntity*>(e);
        //     if (living && living != entity && living->isAlive()) {
        //         nearbyMobs.push_back(living);
        //
        //         // 检查是否是敌对生物
        //         if (entity->isHostileTo(*living)) {
        //             f32 dist = entity->distanceTo(*living);
        //             if (dist < minHostileDist) {
        //                 minHostileDist = dist;
        //                 nearestHostile = living;
        //             }
        //         }
        //     }
        // }

        // entity->getBrain()->setMemory(memory::MemoryModuleTypes::MOBS, nearbyMobs);

        // if (nearestHostile) {
        //     entity->getBrain()->setMemory(
        //         memory::MemoryModuleTypes::NEAREST_HOSTILE,
        //         nearestHostile
        //     );
        // } else {
        //     entity->getBrain()->removeMemory(memory::MemoryModuleTypes::NEAREST_HOSTILE);
        // }
    }

private:
    f32 m_range;
};

/**
 * @brief 工作站点传感器
 *
 * 检测村民的工作站点。
 *
 * 参考 MC 1.16.5 SecondaryPointsSensor
 */
template <typename E>
class WorkStationSensor : public Sensor<E> {
public:
    WorkStationSensor()
        : Sensor<E>(40)  // 每2秒更新一次
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::JOB_SITE,
            memory::MemoryModuleTypes::POTENTIAL_JOB_SITE
        };
    }

protected:
    void update(ServerWorld* world, E* entity) override {
        if (!entity || !entity->isAlive()) {
            return;
        }

        // TODO: 检测工作站点
        // 这需要访问村民数据来获取工作站点类型
        // VillagerEntity* villager = dynamic_cast<VillagerEntity*>(entity);
        // if (!villager) return;

        // 搜索最近的工作站点
        // auto workstation = findNearestWorkstation(world, entity);
        // if (workstation) {
        //     entity->getBrain()->setMemory(
        //         memory::MemoryModuleTypes::JOB_SITE,
        //         workstation
        //     );
        // }
    }
};

/**
 * @brief 居住点传感器
 *
 * 检测村民的家和集会点。
 *
 * 参考 MC 1.16.5 VillagerPoiSensor
 */
template <typename E>
class VillagePoiSensor : public Sensor<E> {
public:
    VillagePoiSensor()
        : Sensor<E>(40)  // 每2秒更新一次
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::HOME,
            memory::MemoryModuleTypes::MEETING_POINT
        };
    }

protected:
    void update(ServerWorld* world, E* entity) override {
        if (!entity || !entity->isAlive()) {
            return;
        }

        // TODO: 检测床（家）和钟（集会点）
        // auto home = findNearestBed(world, entity);
        // auto meetingPoint = findNearestBell(world, entity);

        // if (home) {
        //     entity->getBrain()->setMemory(memory::MemoryModuleTypes::HOME, home);
        // }
        // if (meetingPoint) {
        //     entity->getBrain()->setMemory(memory::MemoryModuleTypes::MEETING_POINT, meetingPoint);
        // }
    }
};

/**
 * @brief 幼崽传感器
 *
 * 检测附近的幼年实体。
 *
 * 参考 MC 1.16.5 GolemSensor
 */
template <typename E>
class BabySensor : public Sensor<E> {
public:
    BabySensor()
        : Sensor<E>(20)
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::VISIBLE_VILLAGER_BABIES,
            memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT
        };
    }

protected:
    void update(ServerWorld* world, E* entity) override {
        if (!entity || !entity->isAlive()) {
            return;
        }

        std::vector<LivingEntity*> babies;
        LivingEntity* nearestAdult = nullptr;
        f32 minAdultDist = 16.0f;

        // TODO: 检测幼年和成年实体
        // auto entities = world->getEntitiesInRange(entity->position(), 16.0f);

        // for (auto* e : entities) {
        //     AgeableEntity* ageable = dynamic_cast<AgeableEntity*>(e);
        //     if (ageable && ageable != entity && ageable->isAlive()) {
        //         if (ageable->isChild()) {
        //             babies.push_back(ageable);
        //         } else {
        //             f32 dist = entity->distanceTo(*ageable);
        //             if (dist < minAdultDist) {
        //                 minAdultDist = dist;
        //                 nearestAdult = ageable;
        //             }
        //         }
        //     }
        // }

        // entity->getBrain()->setMemory(
        //     memory::MemoryModuleTypes::VISIBLE_VILLAGER_BABIES,
        //     babies
        // );

        // if (nearestAdult) {
        //     entity->getBrain()->setMemory(
        //         memory::MemoryModuleTypes::NEAREST_VISIBLE_ADULT,
        //         nearestAdult
        //     );
        // }
    }
};

/**
 * @brief 避险传感器
 *
 * 检测需要避险的目标。
 *
 * 参考 MC 1.16.5 AvoidEntitySensor
 */
template <typename E>
class AvoidEntitySensor : public Sensor<E> {
public:
    explicit AvoidEntitySensor(f32 range = 16.0f, i32 interval = 20)
        : Sensor<E>(interval)
        , m_range(range)
    {}

    [[nodiscard]] std::unordered_set<const memory::MemoryModuleTypeBase*> getUsedMemories() const override {
        return {
            memory::MemoryModuleTypes::AVOID_TARGET,
            memory::MemoryModuleTypes::NEAREST_REPELLENT
        };
    }

protected:
    void update(ServerWorld* world, E* entity) override {
        if (!entity || !entity->isAlive()) {
            return;
        }

        LivingEntity* avoidTarget = nullptr;
        f32 minDist = m_range;

        // TODO: 检测需要避险的实体
        // auto entities = world->getEntitiesInRange(entity->position(), m_range);

        // for (auto* e : entities) {
        //     LivingEntity* living = dynamic_cast<LivingEntity*>(e);
        //     if (living && living != entity && living->isAlive()) {
        //         if (shouldAvoid(entity, living)) {
        //             f32 dist = entity->distanceTo(*living);
        //             if (dist < minDist) {
        //                 minDist = dist;
        //                 avoidTarget = living;
        //             }
        //         }
        //     }
        // }

        // if (avoidTarget) {
        //     entity->getBrain()->setMemoryWithTTL(
        //         memory::MemoryModuleTypes::AVOID_TARGET,
        //         avoidTarget,
        //         100
        //     );
        // } else {
        //     entity->getBrain()->removeMemory(memory::MemoryModuleTypes::AVOID_TARGET);
        // }
    }

private:
    f32 m_range;

    // 判断是否应该避险某实体
    // bool shouldAvoid(E* self, LivingEntity* other) {
    //     // 根据实体类型判断
    //     // 例如：羊躲避狼
    //     return false;
    // }
};

} // namespace sensor
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
