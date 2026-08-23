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
 * @brief 属性修改器 UUID 常量
 *
 * 定义标准的属性修改器 UUID。
 * 这些 UUID 用于标识装备和状态效果提供的属性修改器。
 */
namespace uuids {

// ============================================================================
// 装备槽位 UUID
// 数组索引顺序: FEET(0), LEGS(1), CHEST(2), HEAD(3), BODY(4)
// ============================================================================

/// 靴子护甲修饰器 UUID (FEET - 索引0)
constexpr const char* ARMOR_MODIFIER_UUID_FEET = "845DB27C-C624-495F-8C9F-6020A9A58B6B";

/// 护腿护甲修饰器 UUID (LEGS - 索引1)
constexpr const char* ARMOR_MODIFIER_UUID_LEGS = "D8499B04-0E66-4726-AB29-64469D734E0D";

/// 胸甲护甲修饰器 UUID (CHEST - 索引2)
constexpr const char* ARMOR_MODIFIER_UUID_CHEST = "9F3D476D-C118-4544-8365-64846904B48E";

/// 头盔护甲修饰器 UUID (HEAD - 索引3)
constexpr const char* ARMOR_MODIFIER_UUID_HEAD = "2AD3F246-FEE1-4E67-B886-69FD380BB150";

/// 身体护甲修饰器 UUID (BODY - 索引4，用于狼铠、鹦鹉螺铠甲、马铠等非玩家实体护甲)
constexpr const char* ARMOR_MODIFIER_UUID_BODY = "5C71D5E3-3F4A-4E2D-9A8B-7E1F4D6C2A93";

// ============================================================================
// 武器 UUID
// ============================================================================

/// 武器攻击伤害修饰器 UUID
constexpr const char* ATTACK_DAMAGE_MODIFIER_UUID = "CB3F55D3-645C-4F38-A497-9C13A33DB5CF";

/// 武器攻击速度修饰器 UUID
constexpr const char* ATTACK_SPEED_MODIFIER_UUID = "FA233E1C-4180-4865-B01B-BCCE9785ACA3";

// ============================================================================
// 状态效果 UUID
// ============================================================================

/// 疾跑速度加成修饰器 UUID
constexpr const char* SPRINTING_SPEED_BOOST_UUID = "662A6B8D-DA3E-4C1C-8813-96EA6097278D";

/// 缓降效果修饰器 UUID
constexpr const char* SLOW_FALLING_UUID = "A5B6CF2A-2F7C-31F5-9035-7078DCEA8A67";

/// 速度效果修饰器 UUID
constexpr const char* SPEED_BOOST_UUID = "91AEAA56-376B-4498-935B-2F7F68070635";

/// 缓慢效果修饰器 UUID
constexpr const char* SLOWNESS_UUID = "7107DE5E-7CE8-4030-803E-C3F48F5E794D";

/// 急迫效果修饰器 UUID
constexpr const char* HASTE_UUID = "AF8B6E3F-3328-4C0A-AA36-5BA2BB9DBEF3";

/// 挖掘疲劳效果修饰器 UUID
constexpr const char* MINING_FATIGUE_UUID = "55FCED67-E92A-486E-9800-B47F202C4386";

/// 力量效果修饰器 UUID
constexpr const char* STRENGTH_UUID = "648D7064-6A60-4F59-8ABE-C2C23A6DD7A9";

/// 虚弱效果修饰器 UUID
constexpr const char* WEAKNESS_UUID = "22653B89-116E-49DC-9B6B-9971489B5BE5";

/// 生命提升效果修饰器 UUID
constexpr const char* HEALTH_BOOST_UUID = "5D6F0BA2-1186-46AC-B896-C61C5CEE99CC";

/// 伤害吸收效果修饰器 UUID
constexpr const char* ABSORPTION_UUID = "EAE29CF0-701E-4ED6-883A-96F798F3DAB5";

/// 幸运效果修饰器 UUID
constexpr const char* LUCK_UUID = "03C3C89D-7037-4B42-869F-B146BCB64D2E";

/// 霉运效果修饰器 UUID
constexpr const char* UNLUCK_UUID = "CC5AF142-2BD2-4215-B636-2605AED11727";

// ============================================================================
// 游戏模式修饰器 UUID
// ============================================================================

/// 创造模式方块交互距离加成修饰器 UUID（+0.5 格）
constexpr const char* CREATIVE_BLOCK_INTERACTION_RANGE_UUID = "A8B3E4F2-5C6D-4B7E-9F80-1A2B3C4D5E6F";

/// 创造模式实体交互距离加成修饰器 UUID（+2.0 格）
constexpr const char* CREATIVE_ENTITY_INTERACTION_RANGE_UUID = "B9C4F5E3-6D7E-4C8F-AF91-2B3C4D5E6F70";

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 从字符串创建 UUID
 * @param uuidStr UUID 字符串
 * @return std::string 类型的 UUID
 */
inline std::string fromString(const char* uuidStr)
{
    return std::string(uuidStr);
}

} // namespace uuids
} // namespace attribute
} // namespace entity
} // namespace mc
