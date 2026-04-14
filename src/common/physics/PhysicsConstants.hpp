#pragma once

#include "../core/Types.hpp"

namespace mc {
namespace physics {

// ============================================================================
// 重力和阻力
// ============================================================================

/// 标准 MC 重力加速度 (blocks/tick²)
constexpr f32 GRAVITY = 0.01f;

/// 空气阻力系数 (每 tick)
constexpr f32 DRAG_AIR = 0.98f;

/// 地面摩擦系数
constexpr f32 DRAG_GROUND = 0.91f;

/// 水中阻力
constexpr f32 DRAG_WATER = 0.8f;

/// 岩浆阻力
constexpr f32 DRAG_LAVA = 0.5f;

// ============================================================================
// 运动参数
// ============================================================================

/// 标准跳跃初速度
constexpr f32 JUMP_VELOCITY = 0.20f;

/// 玩家最大步进高度
constexpr f32 STEP_HEIGHT = 0.6f;

/// 速度归零阈值
constexpr f32 MOTION_THRESHOLD = 0.003f;

/// 默认滑度系数
constexpr f32 SLIPPERINESS_DEFAULT = 0.6f;

/// 冰滑度系数
constexpr f32 SLIPPERINESS_ICE = 0.98f;

/// 滑度冰滑度系数
constexpr f32 SLIPPERINESS_SLIME = 0.8f;

/// 地面移动因子计算
/// MC 公式: speed * (0.21600002F / (slipperiness^3))
constexpr f32 getGroundMoveFactor(f32 speed, f32 slipperiness = SLIPPERINESS_DEFAULT) {
    return speed * 0.21600002f / (slipperiness * slipperiness * slipperiness);
}

// ============================================================================
// 物品物理
// ============================================================================

/// 物品重力
constexpr f32 ITEM_GRAVITY = 0.04f;

/// 物品阻力
constexpr f32 ITEM_DRAG = 0.98f;

/// 物品水平阻力
constexpr f32 ITEM_HORIZONTAL_DRAG = 0.98f;

// ============================================================================
// 粒子物理
// ============================================================================

/// 雨滴重力
constexpr f32 RAIN_GRAVITY = 0.06f;

/// 雪花重力
constexpr f32 SNOW_GRAVITY = 0.02f;

// ============================================================================
// 实体运动限制
// ============================================================================

/// 最大移动速度 (blocks/tick)
constexpr f32 MAX_MOVEMENT_SPEED = 100.0f;

/// 最大下落速度
constexpr f32 MAX_FALL_SPEED = 3.0f;

// ============================================================================
// 游泳和潜水
// ============================================================================

/// 水中跳跃初速度（向上游泳）
constexpr f32 SWIM_JUMP_VELOCITY = 0.1f;

/// 水中重力（浮力抵消）
constexpr f32 WATER_GRAVITY = 0.02f;

/// 水中基础游泳速度
constexpr f32 SWIM_SPEED_BASE = 0.02f;

/// 水中冲刺游泳速度倍率
constexpr f32 SWIM_SPEED_SPRINT_MULTIPLIER = 1.3f;

/// 水中阻力（非冲刺）
constexpr f32 WATER_DRAG = 0.8f;

/// 水中阻力（冲刺）
constexpr f32 WATER_DRAG_SPRINT = 0.9f;

/// 岩浆阻力
constexpr f32 LAVA_DRAG = 0.5f;

/// 岩浆重力（浮力抵消）
constexpr f32 LAVA_GRAVITY = 0.02f;

/// 岩浆基础移动速度
constexpr f32 LAVA_SWIM_SPEED = 0.02f;

/// 水中向上游泳速度
constexpr f32 SWIM_UP_SPEED = 0.04f;

/// 水中潜行下沉速度
constexpr f32 SWIM_DOWN_SPEED = 0.04f;

/// 深度守卫附魔游泳速度加成（每级）
constexpr f32 DEPTH_STRIDER_SPEED_BONUS = 0.0333333f;

/// 深度守卫最大等级
constexpr i32 DEPTH_STRIDER_MAX_LEVEL = 3;

/// 海豚的恩惠游泳速度加成
constexpr f32 DOLPHINS_GRACE_SPEED_BONUS = 0.96f;

/// 水中碰撞墙后上跳速度
constexpr f32 WATER_WALL_JUMP_VELOCITY = 0.3f;

// ============================================================================
// 空气和溺水
// ============================================================================

/// 默认最大空气值（tick）
constexpr i32 DEFAULT_MAX_AIR = 300;  // 15秒 = 300 ticks

/// 溺水伤害间隔（tick）
constexpr i32 DROWN_DAMAGE_INTERVAL = 20;  // 每秒伤害一次

/// 溺水伤害量
constexpr f32 DROWN_DAMAGE_AMOUNT = 2.0f;

// ============================================================================
// 爆炸
// ============================================================================

/// 爆炸基础伤害衰减距离
constexpr f32 EXPLOSION_RADIUS_SCALE = 2.0f;

} // namespace physics
} // namespace mc
