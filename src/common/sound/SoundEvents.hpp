#pragma once

#include "common/resource/ResourceLocation.hpp"

namespace mc {

/**
 * @brief 声音事件常量
 *
 * 包含所有 Minecraft 1.16.5 原版声音事件的资源位置。
 * 参考: net.minecraft.util.SoundEvents
 *
 * 使用示例:
 * @code
 * player.playSound(SoundEvents::ENTITY_GENERIC_EAT, 1.0f, 1.0f);
 * @endcode
 */
namespace SoundEvents {

// ============================================================================
// 实体通用声音
// ============================================================================

/// 通用进食声音
extern const ResourceLocation ENTITY_GENERIC_EAT;

/// 通用饮水声音
extern const ResourceLocation ENTITY_GENERIC_DRINK;

/// 通用窒息声音
extern const ResourceLocation ENTITY_GENERIC_HURT;

/// 通用死亡声音
extern const ResourceLocation ENTITY_GENERIC_DEATH;

/// 通用燃烧声音
extern const ResourceLocation ENTITY_GENERIC_BURN;

/// 通用 extinguish（灭火）声音
extern const ResourceLocation ENTITY_GENERIC_EXTINGUISH_FIRE;

/// 通用大脚步声
extern const ResourceLocation ENTITY_GENERIC_BIG_FALL;

/// 通用小脚步声
extern const ResourceLocation ENTITY_GENERIC_SMALL_FALL;

/// 通用溅水声
extern const ResourceLocation ENTITY_GENERIC_SPLASH;

/// 通用游泳声
extern const ResourceLocation ENTITY_GENERIC_SWIM;

// ============================================================================
// 玩家声音
// ============================================================================

/// 玩家打嗝声音（进食完成后）
extern const ResourceLocation ENTITY_PLAYER_BURP;

/// 玩家受伤声音
extern const ResourceLocation ENTITY_PLAYER_HURT;

/// 玩家死亡声音
extern const ResourceLocation ENTITY_PLAYER_DEATH;

/// 玩家溅水声
extern const ResourceLocation ENTITY_PLAYER_SPLASH;

/// 玩家游泳声
extern const ResourceLocation ENTITY_PLAYER_SWIM;

/// 玩家脚步声
extern const ResourceLocation ENTITY_PLAYER_STEP;

/// 玩家攻击（击中）声音
extern const ResourceLocation ENTITY_PLAYER_ATTACK_SWEEP;

/// 玩家攻击（暴击）声音
extern const ResourceLocation ENTITY_PLAYER_ATTACK_CRIT;

/// 玩家攻击（击中实体）声音
extern const ResourceLocation ENTITY_PLAYER_ATTACK_KNOCKBACK;

/// 玩家攻击（未击中）声音
extern const ResourceLocation ENTITY_PLAYER_ATTACK_NODAMAGE;

/// 玩家呼吸声（水下）
extern const ResourceLocation ENTITY_PLAYER_BREATH;

/// 玩家等级提升声音
extern const ResourceLocation ENTITY_PLAYER_LEVELUP;

// ============================================================================
// 食物声音（按类型）
// ============================================================================

// MC 1.16.5 使用通用的 ENTITY_GENERIC_EAT 声音，
// 但会根据食物类型调整音调和随机播放不同的变种。

// ============================================================================
// 方块声音（由 BlockSoundType 管理）
// ============================================================================

// 方块声音由 BlockSoundTypes 命名空间管理，不在此定义。

// ============================================================================
// 环境声音
// ============================================================================

/// 环境天气雨声
extern const ResourceLocation WEATHER_RAIN;

/// 环境天气雷声
extern const ResourceLocation WEATHER_THUNDER;

// ============================================================================
// 初始化
// ============================================================================

/**
 * @brief 初始化所有声音事件常量
 *
 * 必须在使用任何声音事件前调用。
 * 通常在游戏启动时调用。
 */
void initialize();

} // namespace SoundEvents

} // namespace mc
