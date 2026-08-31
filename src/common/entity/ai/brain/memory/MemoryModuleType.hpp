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

#pragma once

#include "Memory.hpp"
#include "common/core/Types.hpp"

#include <cstddef>
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
class HoglinEntity;
class AbstractPiglinEntity;
class BlockPos;
class GlobalPos;
class DamageSource;

namespace entity {
namespace ai {
namespace pathfinding {
class Path;
}
} // namespace ai
} // namespace entity

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
    {}

    virtual ~MemoryModuleTypeBase() = default;

    [[nodiscard]] const std::string& getName() const { return m_name; }

    [[nodiscard]] virtual size_t getTypeHash() const = 0;

    bool operator==(const MemoryModuleTypeBase& other) const { return m_name == other.m_name; }

    bool operator!=(const MemoryModuleTypeBase& other) const { return !(*this == other); }

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
    {}

    [[nodiscard]] size_t getTypeHash() const override { return std::hash<std::string>{}(m_name); }
};

/**
 * @brief 全局记忆模块类型注册表
 */
class MemoryModuleTypes {
public:
    // ========== 基础类型 ==========
    static const MemoryModuleType<void>* DUMMY;

    // ========== 位置相关 ==========
    static const MemoryModuleType<GlobalPos>* HOME;
    static const MemoryModuleType<GlobalPos>* JOB_SITE;
    static const MemoryModuleType<GlobalPos>* POTENTIAL_JOB_SITE;
    static const MemoryModuleType<GlobalPos>* MEETING_POINT;
    static const MemoryModuleType<BlockPos>* NEAREST_BED;
    static const MemoryModuleType<BlockPos>* HIDING_PLACE;
    static const MemoryModuleType<BlockPos>* CELEBRATE_LOCATION;
    static const MemoryModuleType<BlockPos>* NEAREST_REPELLENT;
    static const MemoryModuleType<std::vector<GlobalPos>>* SECONDARY_JOB_SITE;

    // ========== 实体列表相关 ==========
    // 实体类记忆统一存 EntityInstanceId 而非裸指针：id 永不悬垂（单调递增不复用），
    // 消费方经 world->getEntity(id) 反查 + isAlive() 校验即可安全解引用。
    // 此前存 LivingEntity* 等裸指针，sensor 20 tick 重扫窗口内实体析构即 UAF
    // （见 MovementTasks.hpp LookAtEntityTask 崩溃案例）。
    static const MemoryModuleType<std::vector<EntityInstanceId>>* MOBS;
    static const MemoryModuleType<std::vector<EntityInstanceId>>* VISIBLE_MOBS;
    static const MemoryModuleType<std::vector<EntityInstanceId>>* VISIBLE_VILLAGER_BABIES;
    static const MemoryModuleType<std::vector<EntityInstanceId>>* NEAREST_PLAYERS;
    static const MemoryModuleType<EntityInstanceId>* NEAREST_VISIBLE_PLAYER;
    static const MemoryModuleType<EntityInstanceId>* NEAREST_VISIBLE_TARGETABLE_PLAYER;
    static const MemoryModuleType<EntityInstanceId>* ATTACK_TARGET;
    static const MemoryModuleType<EntityInstanceId>* INTERACTION_TARGET;
    static const MemoryModuleType<EntityInstanceId>* HURT_BY_ENTITY;
    static const MemoryModuleType<EntityInstanceId>* AVOID_TARGET;
    static const MemoryModuleType<EntityInstanceId>* NEAREST_HOSTILE;
    static const MemoryModuleType<EntityInstanceId>* NEAREST_VISIBLE_ZOMBIFIED;
    static const MemoryModuleType<EntityInstanceId>* BREED_TARGET;
    static const MemoryModuleType<EntityInstanceId>* NEAREST_VISIBLE_ADULT;
    static const MemoryModuleType<EntityInstanceId>* RIDE_TARGET;
    static const MemoryModuleType<EntityInstanceId>* NEAREST_VISIBLE_NEMESIS;
    static const MemoryModuleType<EntityInstanceId>* NEAREST_VISIBLE_WANTED_ITEM;

    // ========== 移动相关 ==========
    static const MemoryModuleType<pathfinding::Path>* PATH;
    static const MemoryModuleType<WalkTarget>* WALK_TARGET;
    static const MemoryModuleType<std::shared_ptr<IPositionTarget>>* LOOK_TARGET;

    // ========== 门相关 ==========
    static const MemoryModuleType<std::vector<GlobalPos>>* INTERACTABLE_DOORS;  // 可交互的门列表
    static const MemoryModuleType<std::unordered_set<GlobalPos>>* OPENED_DOORS; // 打开的门集合

    // ========== 战斗相关 ==========
    static const MemoryModuleType<bool>* ATTACK_COOLING_DOWN;
    static const MemoryModuleType<DamageSource*>* HURT_BY;

    // ========== 时间相关 ==========
    static const MemoryModuleType<i64>* HEARD_BELL_TIME;
    static const MemoryModuleType<i64>* CANT_REACH_WALK_TARGET_SINCE;
    static const MemoryModuleType<i64>* LAST_SLEPT;
    static const MemoryModuleType<i64>* LAST_WOKEN;
    static const MemoryModuleType<i64>* LAST_WORKED_AT_POI;

    // ========== 状态相关 ==========
    static const MemoryModuleType<bool>* ADMIRING_ITEM;
    static const MemoryModuleType<bool>* ADMIRING_DISABLED;
    static const MemoryModuleType<bool>* HUNTED_RECENTLY;
    static const MemoryModuleType<bool>* DANCING;
    static const MemoryModuleType<bool>* ATE_RECENTLY;
    static const MemoryModuleType<bool>* PACIFIED;
    static const MemoryModuleType<bool>* GOLEM_DETECTED_RECENTLY;
    static const MemoryModuleType<bool>* UNIVERSAL_ANGER;

    // ========== 计时器相关 ==========
    static const MemoryModuleType<i32>* TIME_TRYING_TO_REACH_ADMIRE_ITEM;
    static const MemoryModuleType<bool>* DISABLE_WALK_TO_ADMIRE_ITEM;

    // ========== 玩家相关 ==========
    static const MemoryModuleType<EntityInstanceId>* TEMPTING_PLAYER;
    static const MemoryModuleType<EntityInstanceId>* NEAREST_PLAYER_HOLDING_WANTED_ITEM;

    // ========== UUID 相关 ==========
    static const MemoryModuleType<u64>* ANGRY_AT; // 原 UUID 类型，使用 u64 存储

    // ========== 猪灵/疣兽相关 ==========
    static const MemoryModuleType<EntityInstanceId>* NEAREST_VISIBLE_HUNTABLE_HOGLIN;
    static const MemoryModuleType<EntityInstanceId>* NEAREST_VISIBLE_BABY_HOGLIN;
    static const MemoryModuleType<EntityInstanceId>* NEAREST_TARGETABLE_PLAYER_NOT_WEARING_GOLD;
    static const MemoryModuleType<std::vector<EntityInstanceId>>* NEAREST_ADULT_PIGLINS;
    static const MemoryModuleType<std::vector<EntityInstanceId>>* NEAREST_VISIBLE_ADULT_PIGLINS;
    static const MemoryModuleType<std::vector<EntityInstanceId>>* NEAREST_VISIBLE_ADULT_HOGLINS;
    static const MemoryModuleType<EntityInstanceId>* NEAREST_VISIBLE_ADULT_PIGLIN;
    static const MemoryModuleType<i32>* VISIBLE_ADULT_PIGLIN_COUNT;
    static const MemoryModuleType<i32>* VISIBLE_ADULT_HOGLIN_COUNT;

    // ========== 扩展类型 ==========
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
    static const MemoryModuleType<EntityInstanceId>* OWNER_HURT_BY;
    static const MemoryModuleType<EntityInstanceId>* OWNER_HURT_TARGET;

    // 1.17+ Allay 相关 (保留用于未来扩展)
    static const MemoryModuleType<GlobalPos>* LIKED_NOTEBLOCK;
    static const MemoryModuleType<GlobalPos>* LISTENING_NOTEBLOCK;
    static const MemoryModuleType<i32>* LIKED_NOTEBLOCK_COOLDOWN_TICKS;
    static const MemoryModuleType<i32>* LISTENING_NOTEBLOCK_COOLDOWN_TICKS;

    // 1.17+ 青蛙/山羊相关 (保留用于未来扩展)
    static const MemoryModuleType<BlockPos>* TONGUE_TARGET;      // 青蛙舌头目标
    static const MemoryModuleType<EntityInstanceId>* RAM_TARGET; // 山羊冲撞目标

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
