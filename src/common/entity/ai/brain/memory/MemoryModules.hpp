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

// ========== 位置相关 ==========

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

// ========== 实体相关 ==========

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

// ========== 移动相关 ==========

/// 路径
constexpr auto& PATH = MemoryModuleTypes::PATH;

// ========== 战斗相关 ==========

/// 攻击冷却中
constexpr auto& ATTACK_COOLING_DOWN = MemoryModuleTypes::ATTACK_COOLING_DOWN;

/// 被攻击来源
constexpr auto& HURT_BY = MemoryModuleTypes::HURT_BY;

// ========== 时间相关 ==========

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

// ========== 状态相关 ==========

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

/// 在水中
constexpr auto& IS_IN_WATER = MemoryModuleTypes::IS_IN_WATER;

/// 怀孕
constexpr auto& IS_PREGNANT = MemoryModuleTypes::IS_PREGNANT;

/// 最近检测到铁傀儡
constexpr auto& GOLEM_DETECTED_RECENTLY = MemoryModuleTypes::GOLEM_DETECTED_RECENTLY;

/// 激进状态
constexpr auto& AGGRESSIVE = MemoryModuleTypes::AGGRESSIVE;

/// 通用愤怒
constexpr auto& UNIVERSAL_ANGER = MemoryModuleTypes::UNIVERSAL_ANGER;

/// 装死
constexpr auto& PLAY_DEAD = MemoryModuleTypes::PLAY_DEAD;

/// 禁用走到欣赏物品
constexpr auto& DISABLE_WALK_TO_ADMIRE_ITEM = MemoryModuleTypes::DISABLE_WALK_TO_ADMIRE_ITEM;

// ========== 冷却/计时相关 ==========

/// 诱惑冷却
constexpr auto& TEMPTATION_COOLDOWN_TICKS = MemoryModuleTypes::TEMPTATION_COOLDOWN_TICKS;

/// 物品拾取冷却
constexpr auto& ITEM_PICKUP_COOLDOWN = MemoryModuleTypes::ITEM_PICKUP_COOLDOWN;

/// 跳跃冷却
constexpr auto& JUMP_COOLDOWN = MemoryModuleTypes::JUMP_COOLDOWN;

/// 求爱冷却
constexpr auto& LOVING_COOLDOWN = MemoryModuleTypes::LOVING_COOLDOWN;

// ========== 玩家相关 ==========

/// 诱惑玩家
constexpr auto& TEMPTING_PLAYER = MemoryModuleTypes::TEMPTING_PLAYER;

// ========== 门相关 ==========

/// 打开的门列表
constexpr auto& OPENED_DOORS = MemoryModuleTypes::OPENED_DOORS;

/// 需要关闭的门
constexpr auto& DOORS_TO_CLOSE = MemoryModuleTypes::DOORS_TO_CLOSE;

// ========== 其他 ==========

/// 次要工作站点
constexpr auto& SECONDARY_JOB_SITE = MemoryModuleTypes::SECONDARY_JOB_SITE;

/**
 * @brief 初始化所有记忆模块类型
 *
 * 必须在使用前调用。实际上调用 MemoryModuleTypes::initialize()。
 */
inline void initialize() {
    MemoryModuleTypes::initialize();
}

} // namespace MemoryModules

} // namespace memory
} // namespace brain
} // namespace ai
} // namespace entity
} // namespace mc
