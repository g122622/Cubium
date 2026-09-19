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
#include <string>

namespace mc {
namespace entity {
namespace attribute {

/**
 * @brief 属性修饰符 id 常量
 *
 * 修饰符 id 是语义化资源名，会随 update_attributes 包上线，客户端按 Identifier 的规则解析
 * ——**只接受小写字母、数字、`_`、`.`、`/`**。历史上这里用的是 UUID 字符串（含大写字母与
 * 连字符），客户端解析时会直接抛异常并断开连接，故统一迁移为与数据包一致的形式。
 *
 * 命名规律（与数据修复映射一致）：
 *   - 装备/武器/游戏模式来源：裸路径，如 `minecraft:armor.boots`、`minecraft:base_attack_damage`
 *   - 状态效果来源：`effect.` 二级前缀，如 `minecraft:effect.speed`
 *   - 附魔来源：`enchantment.` 二级前缀，如 `minecraft:enchantment.respiration`
 *
 * 同一槽位的护甲值/护甲韧性/击退抗性共用一条 id（靠所属属性区分），这与原版一致；
 * 修饰符 id 在属性内是唯一键，"同 id 先移除再添加"即由此成立。
 */
namespace ids {

// ============================================================================
// 装备槽位 id
// 数组索引顺序: FEET(0), LEGS(1), CHEST(2), HEAD(3), BODY(4)
// ============================================================================

/// 靴子护甲修饰符 id（FEET - 索引0）
constexpr const char* ARMOR_MODIFIER_FEET = "minecraft:armor.boots";

/// 护腿护甲修饰符 id（LEGS - 索引1）
constexpr const char* ARMOR_MODIFIER_LEGS = "minecraft:armor.leggings";

/// 胸甲护甲修饰符 id（CHEST - 索引2）
constexpr const char* ARMOR_MODIFIER_CHEST = "minecraft:armor.chestplate";

/// 头盔护甲修饰符 id（HEAD - 索引3）
constexpr const char* ARMOR_MODIFIER_HEAD = "minecraft:armor.helmet";

/// 身体护甲修饰符 id（BODY - 索引4，用于狼铠、鹦鹉螺铠甲、马铠等非玩家实体护甲）
constexpr const char* ARMOR_MODIFIER_BODY = "minecraft:armor.body";

// ============================================================================
// 武器 id
// ============================================================================

/// 装备提供的攻击伤害修饰符 id
constexpr const char* ATTACK_DAMAGE_MODIFIER = "minecraft:base_attack_damage";

/// 装备提供的攻击速度修饰符 id
constexpr const char* ATTACK_SPEED_MODIFIER = "minecraft:base_attack_speed";

// ============================================================================
// 游戏模式 id
// ============================================================================

/// 创造模式方块交互距离加成修饰符 id（+0.5 格）
constexpr const char* CREATIVE_BLOCK_INTERACTION_RANGE_MODIFIER = "minecraft:creative_mode_block_range";

/// 创造模式实体交互距离加成修饰符 id（+2.0 格）
constexpr const char* CREATIVE_ENTITY_INTERACTION_RANGE_MODIFIER = "minecraft:creative_mode_entity_range";

} // namespace ids
} // namespace attribute
} // namespace entity
} // namespace mc
