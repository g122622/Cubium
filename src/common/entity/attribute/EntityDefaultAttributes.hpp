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

#include "common/core/Types.hpp"

namespace mc {
namespace entity {
namespace attribute {
namespace defaults {

// ============================================================================
// 玩家属性默认值
// ============================================================================

namespace player {
/// 玩家移动速度
constexpr f32 MOVEMENT_SPEED = 0.1f;

/// 玩家飞行速度
constexpr f32 FLYING_SPEED = 0.05f;

/// 玩家最大生命值
constexpr f32 MAX_HEALTH = 20.0f;

/// 玩家基础攻击伤害
constexpr f32 ATTACK_DAMAGE = 1.0f;

/// 玩家攻击速度
constexpr f32 ATTACK_SPEED = 4.0f;

/// 玩家方块交互距离基础值（生存/冒险模式），创造模式额外 +0.5
constexpr f32 BLOCK_INTERACTION_RANGE = 4.5f;

/// 玩家实体交互距离基础值（生存/冒险模式），创造模式额外 +2.0
constexpr f32 ENTITY_INTERACTION_RANGE = 3.0f;
} // namespace player

// ============================================================================
// 生物基类属性默认值
// ============================================================================

namespace mob {
/// 生物默认移动速度
constexpr f32 MOVEMENT_SPEED = 0.25f;

/// 生物默认最大生命值
constexpr f32 MAX_HEALTH = 20.0f;

/// 生物默认跟随范围
constexpr f32 FOLLOW_RANGE = 32.0f;

/// 生物默认攻击伤害
constexpr f32 ATTACK_DAMAGE = 2.0f;
} // namespace mob

// ============================================================================
// 动物基类属性默认值
// ============================================================================

namespace animal {
/// 动物移动速度
constexpr f32 MOVEMENT_SPEED = 0.2f;

/// 动物最大生命值
constexpr f32 MAX_HEALTH = 10.0f;
} // namespace animal

// ============================================================================
// 具体生物属性默认值
// ============================================================================

namespace pig {
constexpr f32 MOVEMENT_SPEED = 0.25f;
constexpr f32 MAX_HEALTH = 10.0f;
} // namespace pig

namespace sheep {
constexpr f32 MOVEMENT_SPEED = 0.23f;
constexpr f32 MAX_HEALTH = 8.0f;
} // namespace sheep

namespace cow {
constexpr f32 MOVEMENT_SPEED = 0.2f;
constexpr f32 MAX_HEALTH = 10.0f;
} // namespace cow

namespace chicken {
constexpr f32 MOVEMENT_SPEED = 0.25f;
constexpr f32 MAX_HEALTH = 4.0f;
} // namespace chicken

namespace wolf {
constexpr f32 MOVEMENT_SPEED = 0.3f;
constexpr f32 MAX_HEALTH = 8.0f;        // 未驯服
constexpr f32 TAMED_MAX_HEALTH = 20.0f; // 驯服后
} // namespace wolf

namespace cat {
constexpr f32 MOVEMENT_SPEED = 0.3f;
constexpr f32 MAX_HEALTH = 10.0f;
} // namespace cat

namespace rabbit {
constexpr f32 MOVEMENT_SPEED = 0.3f;
constexpr f32 MAX_HEALTH = 3.0f;
} // namespace rabbit

namespace fox {
constexpr f32 MOVEMENT_SPEED = 0.3f;
constexpr f32 MAX_HEALTH = 10.0f;
} // namespace fox

namespace bee {
/// 蜜蜂属性
constexpr f32 MOVEMENT_SPEED = 0.3f;
constexpr f32 FLYING_SPEED = 0.6f;
constexpr f32 MAX_HEALTH = 10.0f;
constexpr f32 ATTACK_DAMAGE = 2.0f;
constexpr f32 FOLLOW_RANGE = 48.0f;
} // namespace bee

namespace panda {
constexpr f32 MOVEMENT_SPEED = 0.15f;
constexpr f32 MAX_HEALTH = 20.0f;
} // namespace panda

namespace dolphin {
constexpr f32 SWIM_SPEED = 0.6f;
constexpr f32 MAX_HEALTH = 10.0f;
} // namespace dolphin

namespace squid {
constexpr f32 MOVEMENT_SPEED = 0.3f;
constexpr f32 MAX_HEALTH = 10.0f;
} // namespace squid

namespace villager {
constexpr f32 MOVEMENT_SPEED = 0.5f;
constexpr f32 MAX_HEALTH = 20.0f;
} // namespace villager

// ============================================================================
// 怪物属性默认值
// ============================================================================

namespace zombie {
constexpr f32 MOVEMENT_SPEED = 0.23f;
constexpr f32 MAX_HEALTH = 20.0f;
constexpr f32 ATTACK_DAMAGE = 3.0f;
constexpr f32 ARMOR = 2.0f;
constexpr f32 FOLLOW_RANGE = 35.0f;
} // namespace zombie

namespace skeleton {
constexpr f32 MOVEMENT_SPEED = 0.25f;
constexpr f32 MAX_HEALTH = 20.0f;
constexpr f32 ATTACK_DAMAGE = 2.0f;
constexpr f32 FOLLOW_RANGE = 16.0f;
} // namespace skeleton

namespace creeper {
constexpr f32 MOVEMENT_SPEED = 0.25f;
constexpr f32 MAX_HEALTH = 20.0f;
constexpr f32 FOLLOW_RANGE = 16.0f;
} // namespace creeper

namespace spider {
constexpr f32 MOVEMENT_SPEED = 0.3f;
constexpr f32 MAX_HEALTH = 16.0f;
constexpr f32 ATTACK_DAMAGE = 2.0f;
} // namespace spider

namespace enderman {
constexpr f32 MOVEMENT_SPEED = 0.3f;
constexpr f32 MAX_HEALTH = 40.0f;
constexpr f32 ATTACK_DAMAGE = 7.0f;
constexpr f32 FOLLOW_RANGE = 64.0f;
} // namespace enderman

namespace phantom {
constexpr f32 MOVEMENT_SPEED = 0.0f; // 飞行实体
constexpr f32 FLYING_SPEED = 0.9f;
constexpr f32 MAX_HEALTH = 20.0f;
constexpr f32 ATTACK_DAMAGE = 6.0f;
} // namespace phantom

namespace blaze {
constexpr f32 MOVEMENT_SPEED = 0.23f;
constexpr f32 MAX_HEALTH = 20.0f;
constexpr f32 ATTACK_DAMAGE = 6.0f;
constexpr f32 FLYING_SPEED = 0.23f;
} // namespace blaze

namespace wither {
constexpr f32 MOVEMENT_SPEED = 0.6f;
constexpr f32 MAX_HEALTH = 300.0f;
constexpr f32 FLYING_SPEED = 0.6f;
} // namespace wither

namespace ender_dragon {
constexpr f32 MOVEMENT_SPEED = 0.3f;
constexpr f32 MAX_HEALTH = 200.0f;
constexpr f32 FLYING_SPEED = 0.3f;
} // namespace ender_dragon

// ============================================================================
// 马匹属性默认值
// ============================================================================

namespace horse {
/// 马匹速度范围
constexpr f32 SPEED_MIN = 0.1127f;
constexpr f32 SPEED_MAX = 0.3375f;

/// 马匹跳跃力量范围
constexpr f32 JUMP_STRENGTH_MIN = 0.4f;
constexpr f32 JUMP_STRENGTH_MAX = 1.0f;

constexpr f32 MAX_HEALTH = 20.0f; // 随机范围 15-30
} // namespace horse

namespace mule {
constexpr f32 MOVEMENT_SPEED = 0.175f;
constexpr f32 MAX_HEALTH = 20.0f;
constexpr f32 JUMP_STRENGTH = 0.5f;
} // namespace mule

namespace donkey {
constexpr f32 MOVEMENT_SPEED = 0.175f;
constexpr f32 MAX_HEALTH = 20.0f; // 随机范围 15-30
constexpr f32 JUMP_STRENGTH = 0.5f;
} // namespace donkey

} // namespace defaults
} // namespace attribute
} // namespace entity
} // namespace mc
