#pragma once

#include "Memory.hpp"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {

class Entity;
class LivingEntity;
class Player;
class MobEntity;
class AgeableEntity;
class ItemEntity;
class BlockPos;
class GlobalPos;
class Path;
class DamageSource;

namespace entity {
namespace ai {
namespace brain {
namespace memory {

class IPositionTarget;
class WalkTarget;

/**
 * @brief 记忆模块类型基类
 */
class MemoryModuleTypeBase {
public:
    explicit MemoryModuleTypeBase(const std::string& name)
        : m_name(name)
    {
    }

    virtual ~MemoryModuleTypeBase() = default;

    [[nodiscard]] const std::string& getName() const
    {
        return m_name;
    }

    [[nodiscard]] virtual size_t getTypeHash() const = 0;

    bool operator==(const MemoryModuleTypeBase& other) const
    {
        return m_name == other.m_name;
    }

    bool operator!=(const MemoryModuleTypeBase& other) const
    {
        return !(*this == other);
    }

protected:
    std::string m_name;
};

/**
 * @brief 类型化记忆模块类型
 */
template <typename T>
class MemoryModuleType : public MemoryModuleTypeBase {
public:
    explicit MemoryModuleType(const std::string& name)
        : MemoryModuleTypeBase(name)
    {
    }

    [[nodiscard]] size_t getTypeHash() const override
    {
        return std::hash<std::string>{}(m_name);
    }
};

/**
 * @brief 全局记忆模块类型注册表
 */
class MemoryModuleTypes {
public:
    static const MemoryModuleType<void>* DUMMY;

    static const MemoryModuleType<GlobalPos>* HOME;
    static const MemoryModuleType<GlobalPos>* JOB_SITE;
    static const MemoryModuleType<GlobalPos>* POTENTIAL_JOB_SITE;
    static const MemoryModuleType<GlobalPos>* MEETING_POINT;
    static const MemoryModuleType<BlockPos>* NEAREST_BED;
    static const MemoryModuleType<BlockPos>* HIDING_PLACE;
    static const MemoryModuleType<BlockPos>* CELEBRATE_LOCATION;
    static const MemoryModuleType<BlockPos>* NEAREST_REPELLENT;

    static const MemoryModuleType<std::vector<LivingEntity*>>* MOBS;
    static const MemoryModuleType<std::vector<LivingEntity*>>* VISIBLE_MOBS;
    static const MemoryModuleType<std::vector<LivingEntity*>>* VISIBLE_VILLAGER_BABIES;
    static const MemoryModuleType<std::vector<Player*>>* NEAREST_PLAYERS;
    static const MemoryModuleType<Player*>* NEAREST_VISIBLE_PLAYER;
    static const MemoryModuleType<Player*>* NEAREST_VISIBLE_TARGETABLE_PLAYER;
    static const MemoryModuleType<LivingEntity*>* ATTACK_TARGET;
    static const MemoryModuleType<LivingEntity*>* INTERACTION_TARGET;
    static const MemoryModuleType<LivingEntity*>* HURT_BY_ENTITY;
    static const MemoryModuleType<LivingEntity*>* AVOID_TARGET;
    static const MemoryModuleType<LivingEntity*>* NEAREST_HOSTILE;
    static const MemoryModuleType<LivingEntity*>* NEAREST_VISIBLE_ZOMBIFIED;
    static const MemoryModuleType<AgeableEntity*>* BREED_TARGET;
    static const MemoryModuleType<AgeableEntity*>* NEAREST_VISIBLE_ADULT;
    static const MemoryModuleType<Entity*>* RIDE_TARGET;
    static const MemoryModuleType<MobEntity*>* NEAREST_VISIBLE_NEMESIS;
    static const MemoryModuleType<ItemEntity*>* NEAREST_VISIBLE_WANTED_ITEM;

    static const MemoryModuleType<Path>* PATH;
    static const MemoryModuleType<WalkTarget>* WALK_TARGET;
    static const MemoryModuleType<std::shared_ptr<IPositionTarget>>* LOOK_TARGET;

    static const MemoryModuleType<bool>* ATTACK_COOLING_DOWN;
    static const MemoryModuleType<DamageSource*>* HURT_BY;

    static const MemoryModuleType<i64>* HEARD_BELL_TIME;
    static const MemoryModuleType<i64>* CANT_REACH_WALK_TARGET_SINCE;
    static const MemoryModuleType<i64>* LAST_SLEPT;
    static const MemoryModuleType<i64>* LAST_WOKEN;
    static const MemoryModuleType<i64>* LAST_WORKED_AT_POI;

    static const MemoryModuleType<bool>* ADMIRING_ITEM;
    static const MemoryModuleType<bool>* ADMIRING_DISABLED;
    static const MemoryModuleType<bool>* HUNTED_RECENTLY;
    static const MemoryModuleType<bool>* DANCING;
    static const MemoryModuleType<bool>* ATE_RECENTLY;
    static const MemoryModuleType<bool>* PACIFIED;
    static const MemoryModuleType<bool>* IS_IN_WATER;
    static const MemoryModuleType<bool>* IS_PREGNANT;
    static const MemoryModuleType<bool>* GOLEM_DETECTED_RECENTLY;
    static const MemoryModuleType<bool>* AGGRESSIVE;
    static const MemoryModuleType<bool>* UNIVERSAL_ANGER;
    static const MemoryModuleType<bool>* PLAY_DEAD;
    static const MemoryModuleType<bool>* DISABLE_WALK_TO_ADMIRE_ITEM;

    static const MemoryModuleType<i32>* PLAY_DEAD_TICKS;
    static const MemoryModuleType<i32>* TEMPTATION_COOLDOWN_TICKS;
    static const MemoryModuleType<i32>* ITEM_PICKUP_COOLDOWN;
    static const MemoryModuleType<i32>* CROPS_GROWTH;
    static const MemoryModuleType<i32>* SKY_COOLDOWN;
    static const MemoryModuleType<i32>* JUMP_COOLDOWN;
    static const MemoryModuleType<i32>* LOVING_COOLDOWN;
    static const MemoryModuleType<i32>* UNHAPPY_COUNTER;
    static const MemoryModuleType<i32>* TIME_TRYING_TO_REACH_ADMIRE_ITEM;
    static const MemoryModuleType<i32>* HOME_HOLDING_TICKS;
    static const MemoryModuleType<i32>* LIKED_NOTEBLOCK_COOLDOWN_TICKS;
    static const MemoryModuleType<i32>* LISTENING_NOTEBLOCK_COOLDOWN_TICKS;

    static const MemoryModuleType<Player*>* TEMPTING_PLAYER;

    static const MemoryModuleType<std::vector<GlobalPos>>* OPENED_DOORS;
    static const MemoryModuleType<std::unordered_set<GlobalPos>>* DOORS_TO_CLOSE;

    static const MemoryModuleType<GlobalPos>* SECONDARY_JOB_SITE;
    static const MemoryModuleType<GlobalPos>* LIKED_NOTEBLOCK;
    static const MemoryModuleType<GlobalPos>* LISTENING_NOTEBLOCK;
    static const MemoryModuleType<BlockPos>* TONGUE_TARGET;
    static const MemoryModuleType<BlockPos>* SNIFFER_SNIFFING_TARGET;

    static const MemoryModuleType<LivingEntity*>* OWNER_HURT_BY;
    static const MemoryModuleType<LivingEntity*>* OWNER_HURT_TARGET;
    static const MemoryModuleType<Entity*>* RAM_TARGET;

    static const MemoryModuleType<u64>* ANGRY_AT;
    static const MemoryModuleType<i64>* LAST_ATTACKED_BY_PLAYER;
    static const MemoryModuleType<bool>* SNIFFER_DIGGING;

    static void initialize();

private:
    static std::unordered_map<std::string, std::unique_ptr<MemoryModuleTypeBase>> s_types;
};

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
