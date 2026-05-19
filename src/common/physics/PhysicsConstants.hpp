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

#include "../core/Types.hpp"

namespace mc {
namespace physics {

// ============================================================================
// 重力和阻力
// ============================================================================

/// 标准 MC 重力加速度 (blocks/tick²)
/// MC 1.16.5 Entity.move() 默认重力值: 0.08D
/// 注意：实际重力应通过属性系统动态获取，此处为默认值
constexpr f32 GRAVITY = 0.08f;

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
constexpr f32 JUMP_VELOCITY = 0.42f;

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

/// 蓝冰滑度系数 (MC 1.16.5 Blocks.java:701)
constexpr f32 SLIPPERINESS_BLUE_ICE = 0.989f;

/// 地面移动因子计算
/// MC 公式: speed * (0.21600002F / (slipperiness^3))
constexpr f32 getGroundMoveFactor(f32 speed, f32 slipperiness = SLIPPERINESS_DEFAULT)
{
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

/// 粒子重力乘数 (MC 标准)
constexpr f32 PARTICLE_GRAVITY_MULTIPLIER = 0.04f;

/// 粒子地面摩擦
constexpr f32 PARTICLE_GROUND_FRICTION = 0.7f;

/// 粒子默认碰撞盒宽度
constexpr f32 PARTICLE_DEFAULT_BBOX_WIDTH = 0.2f;

/// 粒子默认碰撞盒高度
constexpr f32 PARTICLE_DEFAULT_BBOX_HEIGHT = 0.2f;

/// 粒子最大打包光照值 (skyLight << 4 | blockLight)
constexpr u32 PARTICLE_MAX_PACKED_LIGHT = 0xF0;

/// 粒子最小移动阈值
constexpr f64 PARTICLE_MIN_MOVEMENT = 1.0e-5;

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
/// MC 源码 PlayerEntity.java:1438 动态计算，此处为参考值
constexpr f32 SWIM_JUMP_VELOCITY = 0.1f;

/// 水中浮力（实际重力抵消值）
/// MC 源码 LivingEntity.java:2206: gravity / 16.0 = 0.08 / 16 = 0.005
constexpr f32 WATER_BUOYANCY = 0.005f;

/// 水中基础游泳速度
/// MC 源码 LivingEntity.java:2052
constexpr f32 SWIM_SPEED_BASE = 0.02f;

/// 水中冲刺游泳速度倍率
constexpr f32 SWIM_SPEED_SPRINT_MULTIPLIER = 1.3f;

/// 水中阻力（非冲刺）
constexpr f32 WATER_DRAG = 0.8f;

/// 水中阻力（冲刺）
/// MC 源码 LivingEntity.java:2051
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

/// 水中向上游泳速度 - 向下看时 (MC PlayerEntity.java:1438)
constexpr f32 SWIM_UP_SPEED_DOWN = 0.085f;

/// 水中向上游泳速度 - 向上看时 (MC PlayerEntity.java:1438)
constexpr f32 SWIM_UP_SPEED_UP = 0.06f;

/// 深度守卫附魔游泳速度加成（每级）
constexpr f32 DEPTH_STRIDER_SPEED_BONUS = 0.0333333f;

/// 深度守卫最大阻力值 (MC LivingEntity.java:2063)
constexpr f32 DEPTH_STRIDER_MAX_DRAG = 0.54600006f;

/// 深度守卫最大等级
constexpr i32 DEPTH_STRIDER_MAX_LEVEL = 3;

/// 海豚的恩惠水中阻力系数
/// MC 源码 LivingEntity.java:2067-2068: 阻力变为 0.96F
constexpr f32 DOLPHINS_GRACE_WATER_DRAG = 0.96f;

/// 水中碰撞墙后上跳速度
constexpr f32 WATER_WALL_JUMP_VELOCITY = 0.3f;

// ============================================================================
// 空气和溺水
// ============================================================================

/// 默认最大空气值（tick）
/// MC 1.16.5 LivingEntity.java: 默认 300 ticks (15秒)
constexpr i32 DEFAULT_MAX_AIR = 300;

/// 溺水伤害间隔（tick）
/// MC 1.16.5 LivingEntity.java: 每 20 ticks (1秒) 伤害一次
constexpr i32 DROWN_DAMAGE_INTERVAL = 20;

/// 溺水伤害量
constexpr f32 DROWN_DAMAGE_AMOUNT = 2.0f;

// ============================================================================
// 玩家飞行 (MC 1.16.5 PlayerAbilities.java)
// ============================================================================

/// 默认飞行速度
constexpr f32 FLY_SPEED = 0.05f;

/// 默认行走速度
constexpr f32 WALK_SPEED = 0.1f;

/// 飞行垂直阻力
constexpr f32 FLY_VERTICAL_DRAG = 0.6f;

/// 飞行水平阻力
constexpr f32 FLY_HORIZONTAL_DRAG = 0.91f;

/// 飞行垂直输入倍率
constexpr f32 FLY_VERTICAL_INPUT_MULTIPLIER = 3.0f;

/// 冲刺飞行速度倍率
constexpr f32 SPRINT_FLY_MULTIPLIER = 2.0f;

/// 冲刺速度倍率 (MC 1.16.5 LivingEntity: 0.3F)
constexpr f32 SPRINT_SPEED_MULTIPLIER = 1.3f;

/// 潜行速度倍率 (MC 1.16.5 PlayerEntity)
constexpr f32 SNEAK_SPEED_MULTIPLIER = 0.3f;

// ============================================================================
// 空中移动 (MC 1.16.5 LivingEntity.java:158)
// ============================================================================

/// 空中移动因子（跳跃移动因子）
constexpr f32 JUMP_MOVEMENT_FACTOR = 0.02f;

/// 冲刺空中移动因子
constexpr f32 SPRINT_JUMP_MOVEMENT_FACTOR = 0.026f;

// ============================================================================
// 梯子和藤蔓攀爬 (MC 1.16.5 LivingEntity.java:2218-2221)
// ============================================================================

/// 梯子上最大水平速度
constexpr f32 LADDER_SPEED_MAX = 0.15f;

/// 梯子上攀爬速度（向上）
constexpr f32 LADDER_CLIMB_SPEED = 0.15f;

/// 梯子上滑落速度（向下）
constexpr f32 LADDER_SLIDE_SPEED = -0.15f;

// ============================================================================
// 缓降附魔 (MC 1.16.5 LivingEntity.java:122)
// ============================================================================

/// 缓降重力值
/// MC 源码: 重力属性修饰符 -0.07，实际重力 = 0.08 - 0.07 = 0.01
constexpr f32 SLOW_FALLING_GRAVITY = 0.01f;

// ============================================================================
// 鞘翅飞行 (MC 1.16.5 LivingEntity.java:2133)
// ============================================================================

/// 鞘翅水平阻力
constexpr f32 ELYTRA_DRAG_HORIZONTAL = 0.99f;

/// 鞘翅垂直阻力
constexpr f32 ELYTRA_DRAG_VERTICAL = 0.98f;

/// 鞘翅最小俯仰角速度因子
constexpr f32 ELYTRA_MIN_SPEED = 0.4f;

/// 鞘翅升力系数
constexpr f32 ELYTRA_LIFT_COEFFICIENT = 0.75f;

// ============================================================================
// 物品水中物理 (MC 1.16.5 ItemEntity.java)
// ============================================================================

/// 物品水中浮力阈值
constexpr f32 ITEM_WATER_BUOYANCY_THRESHOLD = 0.06f;

/// 物品水中浮力
constexpr f32 ITEM_WATER_BUOYANCY_FORCE = 0.0005f; // 5.0E-4

/// 物品岩浆阻力
constexpr f32 ITEM_LAVA_DRAG = 0.95f;

// ============================================================================
// 特殊方块物理
// ============================================================================

/// 蜘蛛网水平减速系数 (MC 1.16.5 WebBlock: Vector3d(0.25D, 0.05F, 0.25D))
constexpr f32 COBWEB_SLOWDOWN_XZ = 0.25f;

/// 蜘蛛网垂直减速系数
constexpr f32 COBWEB_SLOWDOWN_Y = 0.05f;

/// 蜂蜜块滑动最大下落速度 (MC 1.16.5 HoneyBlock: -0.05D)
constexpr f32 HONEY_BLOCK_MAX_SLIDE_VELOCITY = 0.05f;

/// 蜂蜜块滑动触发阈值 (MC: velocity.y >= -0.08D 时开始滑动)
constexpr f32 HONEY_BLOCK_SLIDE_THRESHOLD = 0.08f;

/// 蜂蜜块跳跃因子 (MC: jumpFactor = 0.5F)
constexpr f32 HONEY_BLOCK_JUMP_FACTOR = 0.5f;

/// 灵魂沙/灵魂土速度因子 (MC: speedFactor = 0.4F)
constexpr f32 SOUL_BLOCK_SPEED_FACTOR = 0.4f;

/// 史莱姆块弹跳系数 - 生物实体 (MC 1.16.5: LivingEntity 使用 1.0)
constexpr f32 SLIME_BLOCK_BOUNCE_FACTOR_LIVING = 1.0f;

/// 史莱姆块弹跳系数 - 非生物实体 (MC 1.16.5: 其他实体使用 0.8)
constexpr f32 SLIME_BLOCK_BOUNCE_FACTOR_NON_LIVING = 0.8f;

/// 甜浆果丛水平减速系数 (MC 1.16.5 SweetBerryBushBlock: Vector3d(0.8F, 0.75D, 0.8F))
constexpr f32 SWEET_BERRY_BUSH_SLOWDOWN_XZ = 0.8f;

/// 甜浆果丛垂直减速系数
constexpr f32 SWEET_BERRY_BUSH_SLOWDOWN_Y = 0.75f;

/// 物品水面弹跳速度乘数 (MC ItemEntity: 0.5F)
constexpr f32 ITEM_WATER_BOUNCE_FACTOR = 0.5f;

// ============================================================================
// 爆炸
// ============================================================================

/// 爆炸基础伤害衰减距离
constexpr f32 EXPLOSION_RADIUS_SCALE = 2.0f;

// ============================================================================
// 粒子重力
// ============================================================================

namespace particle {

/// 红石粒子重力
constexpr f32 REDSTONE_GRAVITY = 0.003f;

/// 岩浆粒子重力
constexpr f32 LAVA_GRAVITY = 0.025f;

/// 营火粒子重力
constexpr f32 CAMPFIRE_GRAVITY = 3.0e-6f;

/// 水花粒子重力
constexpr f32 SPLASH_GRAVITY = 0.04f;

/// 挖掘粒子重力
constexpr f32 DIGGING_GRAVITY = 0.03f;

/// 村民粒子重力
constexpr f32 VILLAGER_GRAVITY = 0.02f;

/// 零重力（用于烟雾、传送门、心形等粒子）
constexpr f32 ZERO_GRAVITY = 0.0f;

/// 默认粒子重力（向后兼容）
/// @deprecated 使用具体粒子类型的重力常量
constexpr f32 DEFAULT_GRAVITY = 0.0f;

} // namespace particle

} // namespace physics
} // namespace mc
