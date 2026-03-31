#pragma once

#include "../../core/Types.hpp"

namespace mc {
namespace entity {
namespace experience {

/**
 * @brief 经验系统常量
 *
 * 基于 Minecraft 1.16.5 的经验系统常量定义。
 * 参考: net.minecraft.entity.player.PlayerEntity, net.minecraft.entity.ExperienceOrbEntity
 */
namespace constants {

// ============================================================================
// 经验球常量
// ============================================================================

/**
 * @brief 经验球最大存活时间 (ticks)
 *
 * 6000 ticks = 5 分钟
 * 参考: ExperienceOrbEntity.MAX_AGE
 */
constexpr i32 MAX_ORB_AGE = 6000;

/**
 * @brief 经验球默认拾取延迟 (ticks)
 *
 * 经验球生成后需要等待的拾取延迟时间
 * 参考: ExperienceOrbEntity.delayBeforeCanPickup (默认值)
 */
constexpr i32 DEFAULT_PICKUP_DELAY = 10;

/**
 * @brief 经验球追踪玩家的范围 (方块)
 *
 * 经验球开始追踪玩家的最大距离
 * 参考: ExperienceOrbEntity.TRACKING_RANGE = 8.0D
 */
constexpr f32 ORB_TRACKING_RANGE = 8.0f;

/**
 * @brief 经验球最大追踪距离 (方块)
 *
 * 经验球追踪玩家的最大距离（平方）
 * 64 = 8 * 8
 */
constexpr f32 ORB_TRACKING_RANGE_SQ = ORB_TRACKING_RANGE * ORB_TRACKING_RANGE;

/**
 * @brief 经验球被吸引的最大距离 (方块)
 *
 * 超过此距离后经验球不再被玩家吸引
 */
constexpr f32 ORB_ATTRACT_DISTANCE = 8.0f;

/**
 * @brief 经验球拾取检测距离 (方块)
 *
 * 玩家在此距离内可以拾取经验球
 * 参考: ExperienceOrbEntity 拾取检测范围
 */
constexpr f32 ORB_PICKUP_DISTANCE = 1.0f;

/**
 * @brief 经验球最大值
 *
 * 单个经验球可以包含的最大经验值
 * 参考: ExperienceOrbEntity.MAX_XP_VALUE = 2477
 */
constexpr i32 MAX_ORB_VALUE = 2477;

/**
 * @brief 经验球重力加速度
 *
 * 参考: ExperienceOrbEntity 重力 = 0.03D
 */
constexpr f32 ORB_GRAVITY = 0.03f;

/**
 * @brief 经验球地面摩擦力
 *
 * 参考: ExperienceOrbEntity 地面摩擦 = 0.98F
 */
constexpr f32 ORB_GROUND_FRICTION = 0.98f;

/**
 * @brief 经验球被玩家吸引时的速度倍率
 *
 * 参考: ExperienceOrbEntity 吸引速度计算
 */
constexpr f32 ORB_ATTRACT_SPEED = 0.1f;

/**
 * @brief 经验球合并检测距离
 *
 * 两个经验球在此距离内可以合并
 */
constexpr f32 ORB_MERGE_DISTANCE = 1.0f;

// ============================================================================
// 经验值分割常量
// ============================================================================

/**
 * @brief 经验值分割表
 *
 * 当生成大量经验时，会分割成多个经验球。
 * 这11个值对应MC中11种经验球大小。
 * 参考: ExperienceOrbEntity.getXPSplit()
 */
constexpr i32 XP_SPLIT_VALUES[] = {
    2477, 1237, 617, 307, 149, 73, 37, 17, 7, 3, 1
};

/**
 * @brief 经验值分割表大小
 */
constexpr i32 XP_SPLIT_COUNT = 11;

// ============================================================================
// 玩家经验常量
// ============================================================================

/**
 * @brief 玩家拾取经验冷却时间 (ticks)
 *
 * 拾取经验球后的冷却时间
 * 参考: PlayerEntity.xpCooldown = 2
 */
constexpr i32 PLAYER_XP_COOLDOWN = 2;

/**
 * @brief 玩家死亡掉落经验上限
 *
 * 参考: PlayerEntity.getExperiencePoints() 最大 100
 */
constexpr i32 MAX_DEATH_XP_DROP = 100;

/**
 * @brief 玩家死亡时每级掉落的经验
 *
 * 参考: PlayerEntity.getExperiencePoints() = level * 7
 */
constexpr i32 DEATH_XP_PER_LEVEL = 7;

/**
 * @brief 最大经验等级
 *
 * MC 1.16.5 的经验等级上限
 */
constexpr i32 MAX_EXPERIENCE_LEVEL = 21862;

// ============================================================================
// 经验修补附魔
// ============================================================================

/**
 * @brief 每点耐久修复消耗的经验值
 *
 * 经验修补附魔：每2点经验修复1点耐久
 * 参考: ExperienceOrbEntity durabilityToXp
 */
constexpr i32 XP_PER_DURABILITY = 2;

// ============================================================================
// 矿石经验掉落常量
// ============================================================================

/**
 * @brief 煤矿经验掉落范围
 */
constexpr i32 COAL_ORE_XP_MIN = 0;
constexpr i32 COAL_ORE_XP_MAX = 2;

/**
 * @brief 钻石矿经验掉落范围
 */
constexpr i32 DIAMOND_ORE_XP_MIN = 3;
constexpr i32 DIAMOND_ORE_XP_MAX = 7;

/**
 * @brief 绿宝石矿经验掉落范围
 */
constexpr i32 EMERALD_ORE_XP_MIN = 3;
constexpr i32 EMERALD_ORE_XP_MAX = 7;

/**
 * @brief 青金石矿经验掉落范围
 */
constexpr i32 LAPIS_ORE_XP_MIN = 2;
constexpr i32 LAPIS_ORE_XP_MAX = 5;

/**
 * @brief 下界石英矿经验掉落范围
 */
constexpr i32 NETHER_QUARTZ_ORE_XP_MIN = 2;
constexpr i32 NETHER_QUARTZ_ORE_XP_MAX = 5;

/**
 * @brief 下界金矿经验掉落范围
 */
constexpr i32 NETHER_GOLD_ORE_XP_MIN = 0;
constexpr i32 NETHER_GOLD_ORE_XP_MAX = 1;

/**
 * @brief 红石矿经验掉落范围
 */
constexpr i32 REDSTONE_ORE_XP_MIN = 1;
constexpr i32 REDSTONE_ORE_XP_MAX = 5;

/**
 * @brief 刷怪笼经验掉落范围
 */
constexpr i32 SPAWNER_XP_MIN = 15;
constexpr i32 SPAWNER_XP_MAX = 44;

// ============================================================================
// 生物经验掉落常量
// ============================================================================

/**
 * @brief 被动动物最小经验掉落
 */
constexpr i32 PASSIVE_MOB_XP_MIN = 1;

/**
 * @brief 被动动物最大经验掉落
 */
constexpr i32 PASSIVE_MOB_XP_MAX = 3;

/**
 * @brief 普通怪物经验掉落
 */
constexpr i32 HOSTILE_MOB_XP = 5;

/**
 * @brief 凋灵经验掉落
 */
constexpr i32 WITHER_XP = 50;

/**
 * @brief 末影龙经验掉落
 */
constexpr i32 ENDER_DRAGON_XP = 12000;

/**
 * @brief 钓鱼经验掉落范围
 */
constexpr i32 FISHING_XP_MIN = 1;
constexpr i32 FISHING_XP_MAX = 6;

} // namespace constants

} // namespace experience
} // namespace entity
} // namespace mc
