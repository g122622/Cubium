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
 *
 * 对齐 MC 1.16.5 MemoryModuleType
 */
class MemoryModuleTypes {
public:
    // ========== 基础类型 ==========
    static const MemoryModuleType<void>* DUMMY;

    // ========== 位置相关 (MC 1.16.5) ==========
    static const MemoryModuleType<GlobalPos>* HOME;
    static const MemoryModuleType<GlobalPos>* JOB_SITE;
    static const MemoryModuleType<GlobalPos>* POTENTIAL_JOB_SITE;
    static const MemoryModuleType<GlobalPos>* MEETING_POINT;
    static const MemoryModuleType<BlockPos>* NEAREST_BED;
    static const MemoryModuleType<BlockPos>* HIDING_PLACE;
    static const MemoryModuleType<BlockPos>* CELEBRATE_LOCATION;
    static const MemoryModuleType<BlockPos>* NEAREST_REPELLENT;
    static const MemoryModuleType<std::vector<GlobalPos>>* SECONDARY_JOB_SITE;

    // ========== 实体列表相关 (MC 1.16.5) ==========
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

    // ========== 移动相关 (MC 1.16.5) ==========
    static const MemoryModuleType<Path>* PATH;
    static const MemoryModuleType<WalkTarget>* WALK_TARGET;
    static const MemoryModuleType<std::shared_ptr<IPositionTarget>>* LOOK_TARGET;

    // ========== 门相关 (MC 1.16.5) ==========
    static const MemoryModuleType<std::vector<GlobalPos>>* INTERACTABLE_DOORS;  // 可交互的门列表
    static const MemoryModuleType<std::unordered_set<GlobalPos>>* OPENED_DOORS;  // 打开的门集合 (MC: Set<GlobalPos>)

    // ========== 战斗相关 (MC 1.16.5) ==========
    static const MemoryModuleType<bool>* ATTACK_COOLING_DOWN;
    static const MemoryModuleType<DamageSource*>* HURT_BY;

    // ========== 时间相关 (MC 1.16.5) ==========
    static const MemoryModuleType<i64>* HEARD_BELL_TIME;
    static const MemoryModuleType<i64>* CANT_REACH_WALK_TARGET_SINCE;
    static const MemoryModuleType<i64>* LAST_SLEPT;
    static const MemoryModuleType<i64>* LAST_WOKEN;
    static const MemoryModuleType<i64>* LAST_WORKED_AT_POI;

    // ========== 状态相关 (MC 1.16.5) ==========
    static const MemoryModuleType<bool>* ADMIRING_ITEM;
    static const MemoryModuleType<bool>* ADMIRING_DISABLED;
    static const MemoryModuleType<bool>* HUNTED_RECENTLY;
    static const MemoryModuleType<bool>* DANCING;
    static const MemoryModuleType<bool>* ATE_RECENTLY;
    static const MemoryModuleType<bool>* PACIFIED;
    static const MemoryModuleType<bool>* GOLEM_DETECTED_RECENTLY;  // MC: field_242309_E
    static const MemoryModuleType<bool>* UNIVERSAL_ANGER;

    // ========== 计时器相关 (MC 1.16.5) ==========
    static const MemoryModuleType<i32>* TIME_TRYING_TO_REACH_ADMIRE_ITEM;  // MC: field_242310_O
    static const MemoryModuleType<bool>* DISABLE_WALK_TO_ADMIRE_ITEM;      // MC: field_242311_P

    // ========== 玩家相关 (MC 1.16.5) ==========
    static const MemoryModuleType<Player*>* TEMPTING_PLAYER;
    static const MemoryModuleType<Player*>* NEAREST_PLAYER_HOLDING_WANTED_ITEM;

    // ========== UUID 相关 (MC 1.16.5) ==========
    static const MemoryModuleType<u64>* ANGRY_AT;  // 原 UUID 类型，使用 u64 存储

    // ========== 猪灵/猪灵相关 (MC 1.16.5) ==========
    // 注意：需要 HoglinEntity 和 AbstractPiglinEntity 类型
    // 这里暂时使用 LivingEntity* 作为占位符，待实体类型完善后替换
    static const MemoryModuleType<LivingEntity*>* NEAREST_VISIBLE_HUNTABLE_HOGLIN;
    static const MemoryModuleType<LivingEntity*>* NEAREST_VISIBLE_BABY_HOGLIN;
    static const MemoryModuleType<Player*>* NEAREST_TARGETABLE_PLAYER_NOT_WEARING_GOLD;
    static const MemoryModuleType<std::vector<LivingEntity*>>* NEAREST_ADULT_PIGLINS;
    static const MemoryModuleType<std::vector<LivingEntity*>>* NEAREST_VISIBLE_ADULT_PIGLINS;
    static const MemoryModuleType<std::vector<LivingEntity*>>* NEAREST_VISIBLE_ADULT_HOGLINS;
    static const MemoryModuleType<LivingEntity*>* NEAREST_VISIBLE_ADULT_PIGLIN;
    static const MemoryModuleType<i32>* VISIBLE_ADULT_PIGLIN_COUNT;
    static const MemoryModuleType<i32>* VISIBLE_ADULT_HOGLIN_COUNT;

    // ========== 扩展类型 (非 MC 1.16.5 标准) ==========
    // 以下类型不属于 MC 1.16.5，保留用于未来版本或自定义实体

    // 通用扩展
    static const MemoryModuleType<bool>* IS_IN_WATER;
    static const MemoryModuleType<bool>* IS_PREGNANT;
    static const MemoryModuleType<bool>* PLAY_DEAD;
    static const MemoryModuleType<bool>* AGGRESSIVE;
    static const MemoryModuleType<i32>* PLAY_DEAD_TICKS;
    static const MemoryModuleType<i32>* TEMPTATION_COOLDOWN_TICKS;
    static const MemoryModuleType<i32>* ITEM_PICKUP_COOLDOWN;
    static const MemoryModuleType<i32>* CROPS_GROWTH;
    static const MemoryModuleType<i32>* SKY_COOLDOWN;
    static const MemoryModuleType<i32>* JUMP_COOLDOWN;
    static const MemoryModuleType<i32>* LOVING_COOLDOWN;
    static const MemoryModuleType<i32>* UNHAPPY_COUNTER;
    static const MemoryModuleType<i32>* HOME_HOLDING_TICKS;
    static const MemoryModuleType<i64>* LAST_ATTACKED_BY_PLAYER;

    // DOORS_TO_CLOSE 是扩展类型，MC 1.16.5 只有 OPENED_DOORS
    static const MemoryModuleType<std::unordered_set<GlobalPos>>* DOORS_TO_CLOSE;

    // 宠物相关扩展
    static const MemoryModuleType<LivingEntity*>* OWNER_HURT_BY;
    static const MemoryModuleType<LivingEntity*>* OWNER_HURT_TARGET;

    // 1.17+ Allay 相关 (保留用于未来扩展)
    static const MemoryModuleType<GlobalPos>* LIKED_NOTEBLOCK;
    static const MemoryModuleType<GlobalPos>* LISTENING_NOTEBLOCK;
    static const MemoryModuleType<i32>* LIKED_NOTEBLOCK_COOLDOWN_TICKS;
    static const MemoryModuleType<i32>* LISTENING_NOTEBLOCK_COOLDOWN_TICKS;

    // 1.17+ 青蛙/山羊相关 (保留用于未来扩展)
    static const MemoryModuleType<BlockPos>* TONGUE_TARGET;  // 青蛙舌头目标
    static const MemoryModuleType<Entity*>* RAM_TARGET;      // 山羊冲撞目标

    // 1.19+ Sniffer 相关 (保留用于未来扩展)
    static const MemoryModuleType<BlockPos>* SNIFFER_SNIFFING_TARGET;
    static const MemoryModuleType<bool>* SNIFFER_DIGGING;

    /**
     * @brief 初始化所有记忆模块类型
     *
     * 必须在使用前调用。
     */
    static void initialize();

private:
    static std::unordered_map<std::string, std::unique_ptr<MemoryModuleTypeBase>> s_types;
};

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
