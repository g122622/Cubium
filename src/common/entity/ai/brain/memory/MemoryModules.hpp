#pragma once

#include "MemoryModuleType.hpp"

namespace mc {
namespace entity {
namespace ai {
namespace brain {
namespace memory {

/**
 * @brief 村民记忆模块便捷别名
 *
 * 提供村民AI使用的记忆类型的便捷访问。
 * 这些是 MemoryModuleTypes 的别名，统一使用标准记忆类型。
 *
 * 注意：使用前必须调用 MemoryModuleTypes::initialize()
 *
 * 参考 MC 1.16.5 MemoryModuleType
 *
 * 使用示例：
 * @code
 * // 注册记忆模块
 * brain.registerMemory(MemoryModules::HOME);
 * brain.registerMemory(MemoryModules::JOB_SITE);
 *
 * // 设置记忆值
 * brain.setMemory(MemoryModules::HOME, GlobalPos(dimensionId, bedPos));
 *
 * // 获取记忆值
 * auto homePos = brain.getMemory<GlobalPos>(MemoryModules::HOME);
 * @endcode
 */
namespace MemoryModules {

// ========== 位置相关 (MC 1.16.5) ==========

/// 家的位置（床位）- 使用 GlobalPos 支持跨维度
constexpr auto& HOME = MemoryModuleTypes::HOME;

/// 工作站点位置
constexpr auto& JOB_SITE = MemoryModuleTypes::JOB_SITE;

/// 潜在工作站点
constexpr auto& POTENTIAL_JOB_SITE = MemoryModuleTypes::POTENTIAL_JOB_SITE;

/// 聚集点位置（钟）
constexpr auto& MEETING_POINT = MemoryModuleTypes::MEETING_POINT;

/// 最近的床
constexpr auto& NEAREST_BED = MemoryModuleTypes::NEAREST_BED;

/// 行走目标
constexpr auto& WALK_TARGET = MemoryModuleTypes::WALK_TARGET;

/// 看向目标
constexpr auto& LOOK_TARGET = MemoryModuleTypes::LOOK_TARGET;

/// 隐藏位置
constexpr auto& HIDING_PLACE = MemoryModuleTypes::HIDING_PLACE;

/// 次要工作站点
constexpr auto& SECONDARY_JOB_SITE = MemoryModuleTypes::SECONDARY_JOB_SITE;

// ========== 实体相关 (MC 1.16.5) ==========

/// 可见的生物列表
constexpr auto& VISIBLE_MOBS = MemoryModuleTypes::VISIBLE_MOBS;

/// 可见的村民婴儿列表
constexpr auto& VISIBLE_VILLAGER_BABIES = MemoryModuleTypes::VISIBLE_VILLAGER_BABIES;

/// 最近的玩家列表
constexpr auto& NEAREST_PLAYERS = MemoryModuleTypes::NEAREST_PLAYERS;

/// 最近可见的玩家
constexpr auto& NEAREST_VISIBLE_PLAYER = MemoryModuleTypes::NEAREST_VISIBLE_PLAYER;

/// 最近可见的可攻击玩家
constexpr auto& NEAREST_VISIBLE_TARGETABLE_PLAYER = MemoryModuleTypes::NEAREST_VISIBLE_TARGETABLE_PLAYER;

/// 攻击目标
constexpr auto& ATTACK_TARGET = MemoryModuleTypes::ATTACK_TARGET;

/// 互动目标
constexpr auto& INTERACTION_TARGET = MemoryModuleTypes::INTERACTION_TARGET;

/// 最近被攻击的实体
constexpr auto& HURT_BY_ENTITY = MemoryModuleTypes::HURT_BY_ENTITY;

/// 逃避目标
constexpr auto& AVOID_TARGET = MemoryModuleTypes::AVOID_TARGET;

/// 最近的敌对生物
constexpr auto& NEAREST_HOSTILE = MemoryModuleTypes::NEAREST_HOSTILE;

/// 繁殖目标
constexpr auto& BREED_TARGET = MemoryModuleTypes::BREED_TARGET;

/// 最近可见的成年生物
constexpr auto& NEAREST_VISIBLE_ADULT = MemoryModuleTypes::NEAREST_VISIBLE_ADULT;

/// 最近可见的物品
constexpr auto& NEAREST_VISIBLE_WANTED_ITEM = MemoryModuleTypes::NEAREST_VISIBLE_WANTED_ITEM;

// ========== 移动相关 (MC 1.16.5) ==========

/// 路径
constexpr auto& PATH = MemoryModuleTypes::PATH;

// ========== 门相关 (MC 1.16.5) ==========

/// 可交互的门列表
constexpr auto& INTERACTABLE_DOORS = MemoryModuleTypes::INTERACTABLE_DOORS;

/// 打开的门集合
constexpr auto& OPENED_DOORS = MemoryModuleTypes::OPENED_DOORS;

// ========== 战斗相关 (MC 1.16.5) ==========

/// 攻击冷却中
constexpr auto& ATTACK_COOLING_DOWN = MemoryModuleTypes::ATTACK_COOLING_DOWN;

/// 被攻击来源
constexpr auto& HURT_BY = MemoryModuleTypes::HURT_BY;

// ========== 时间相关 (MC 1.16.5) ==========

/// 听到铃声时间
constexpr auto& HEARD_BELL_TIME = MemoryModuleTypes::HEARD_BELL_TIME;

/// 无法到达行走目标的时间
constexpr auto& CANT_REACH_WALK_TARGET_SINCE = MemoryModuleTypes::CANT_REACH_WALK_TARGET_SINCE;

/// 最后一次睡觉时间
constexpr auto& LAST_SLEPT = MemoryModuleTypes::LAST_SLEPT;

/// 最后一次醒来时间
constexpr auto& LAST_WOKEN = MemoryModuleTypes::LAST_WOKEN;

/// 最后一次在工作站点工作的时间
constexpr auto& LAST_WORKED = MemoryModuleTypes::LAST_WORKED_AT_POI;

// ========== 状态相关 (MC 1.16.5) ==========

/// 正在欣赏物品
constexpr auto& ADMIRING_ITEM = MemoryModuleTypes::ADMIRING_ITEM;

/// 欣赏被禁用
constexpr auto& ADMIRING_DISABLED = MemoryModuleTypes::ADMIRING_DISABLED;

/// 最近被狩猎
constexpr auto& HUNTED_RECENTLY = MemoryModuleTypes::HUNTED_RECENTLY;

/// 正在跳舞
constexpr auto& DANCING = MemoryModuleTypes::DANCING;

/// 最近吃过
constexpr auto& ATE_RECENTLY = MemoryModuleTypes::ATE_RECENTLY;

/// 被安抚
constexpr auto& PACIFIED = MemoryModuleTypes::PACIFIED;

/// 最近检测到铁傀儡 (MC: field_242309_E)
constexpr auto& GOLEM_DETECTED_RECENTLY = MemoryModuleTypes::GOLEM_DETECTED_RECENTLY;

/// 通用愤怒
constexpr auto& UNIVERSAL_ANGER = MemoryModuleTypes::UNIVERSAL_ANGER;

// ========== 玩家相关 (MC 1.16.5) ==========

/// 诱惑玩家
constexpr auto& TEMPTING_PLAYER = MemoryModuleTypes::TEMPTING_PLAYER;

/// 持有想要物品的最近玩家
constexpr auto& NEAREST_PLAYER_HOLDING_WANTED_ITEM = MemoryModuleTypes::NEAREST_PLAYER_HOLDING_WANTED_ITEM;

// ========== 猪灵相关 (MC 1.16.5) ==========

/// 最近可见的可狩猎猪灵兽
constexpr auto& NEAREST_VISIBLE_HUNTABLE_HOGLIN = MemoryModuleTypes::NEAREST_VISIBLE_HUNTABLE_HOGLIN;

/// 最近可见的幼年猪灵兽
constexpr auto& NEAREST_VISIBLE_BABY_HOGLIN = MemoryModuleTypes::NEAREST_VISIBLE_BABY_HOGLIN;

/// 最近未穿金装备的可攻击玩家
constexpr auto& NEAREST_TARGETABLE_PLAYER_NOT_WEARING_GOLD =
    MemoryModuleTypes::NEAREST_TARGETABLE_PLAYER_NOT_WEARING_GOLD;

/// 附近的成年猪灵列表
constexpr auto& NEAREST_ADULT_PIGLINS = MemoryModuleTypes::NEAREST_ADULT_PIGLINS;

/// 最近可见的成年猪灵列表
constexpr auto& NEAREST_VISIBLE_ADULT_PIGLINS = MemoryModuleTypes::NEAREST_VISIBLE_ADULT_PIGLINS;

/// 最近可见的成年猪灵兽列表
constexpr auto& NEAREST_VISIBLE_ADULT_HOGLINS = MemoryModuleTypes::NEAREST_VISIBLE_ADULT_HOGLINS;

/// 最近可见的成年猪灵
constexpr auto& NEAREST_VISIBLE_ADULT_PIGLIN = MemoryModuleTypes::NEAREST_VISIBLE_ADULT_PIGLIN;

/// 可见成年猪兽数量
constexpr auto& VISIBLE_ADULT_PIGLIN_COUNT = MemoryModuleTypes::VISIBLE_ADULT_PIGLIN_COUNT;

/// 可见成年猪灵兽数量
constexpr auto& VISIBLE_ADULT_HOGLIN_COUNT = MemoryModuleTypes::VISIBLE_ADULT_HOGLIN_COUNT;

// ========== 扩展类型 (非 MC 1.16.5) ==========

constexpr auto& IS_IN_WATER = MemoryModuleTypes::IS_IN_WATER;
constexpr auto& IS_PREGNANT = MemoryModuleTypes::IS_PREGNANT;
constexpr auto& PLAY_DEAD = MemoryModuleTypes::PLAY_DEAD;
constexpr auto& AGGRESSIVE = MemoryModuleTypes::AGGRESSIVE;
constexpr auto& TEMPTATION_COOLDOWN_TICKS = MemoryModuleTypes::TEMPTATION_COOLDOWN_TICKS;
constexpr auto& ITEM_PICKUP_COOLDOWN = MemoryModuleTypes::ITEM_PICKUP_COOLDOWN;
constexpr auto& JUMP_COOLDOWN = MemoryModuleTypes::JUMP_COOLDOWN;
constexpr auto& LOVING_COOLDOWN = MemoryModuleTypes::LOVING_COOLDOWN;
constexpr auto& DOORS_TO_CLOSE = MemoryModuleTypes::DOORS_TO_CLOSE;

/**
 * @brief 初始化所有记忆模块类型
 *
 * 必须在使用前调用。实际上调用 MemoryModuleTypes::initialize()。
 */
inline void initialize()
{
    MemoryModuleTypes::initialize();
}

} // namespace MemoryModules

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
