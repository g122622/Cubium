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

#include <optional>
#include <string>
#include <string_view>

namespace mc {
namespace item {
namespace component {

/**
 * @brief 数据组件类型标识（1.21.11 DataComponentType）
 *
 * 每个组件类型有一个整数 typeId（网络 wire 中的 VarInt，= Java
 * DATA_COMPONENT_TYPE 注册表 id）和一个资源位置名（NBT patch 的键，如
 * "minecraft:damage"）。typeId 严格对齐 Java 1.21.11 DataComponents.register
 * 声明顺序（见 vanilla DataComponents.java）：
 *   custom_data(0) max_stack_size(1) max_damage(2) damage(3) unbreakable(4)
 *   use_effects(5) custom_name(6) minimum_attack_charge(7) item_name(8)
 *   item_model(9) lore(10) rarity(11) enchantments(12) can_place_on(13)
 *   can_break(14) ... repair_cost(18) ... attack_range(29) enchantable(30)
 *   ... potion_contents(48)
 *
 * 本项目仅落地部分组件；未落地的类型占位与 Java 一致，将来扩展零冲击。
 * 未落地的 typeId 在 wire 读入时会因"无长度前缀无法安全跳过"而报错，故不得
 * 依赖未落地组件参与真实 Java 对端通信。
 */
enum class DataComponentType : i32 {
    CustomData = 0,      // minecraft:custom_data —— 嵌套 NBT（本项目承载 m_customData）
    MaxStackSize = 1,    // minecraft:max_stack_size —— i32（覆盖最大堆叠数）
    MaxDamage = 2,       // minecraft:max_damage —— i32（覆盖最大耐久）
    Damage = 3,          // minecraft:damage —— i32（已承受伤害）
    Unbreakable = 4,     // minecraft:unbreakable —— Unit（不可损坏）
    CustomName = 6,      // minecraft:custom_name —— Component（文本）
    ItemName = 8,        // minecraft:item_name —— Component（默认显示名覆盖）
    ItemModel = 9,       // minecraft:item_model —— Identifier（模型覆盖，仅序列化）
    Lore = 10,           // minecraft:lore —— list<Component>
    Rarity = 11,         // minecraft:rarity —— Rarity（稀有度覆盖）
    Enchantments = 12,   // minecraft:enchantments —— ItemEnchantments
    CanPlaceOn = 13,     // minecraft:can_place_on —— BlockPredicates
    CanBreak = 14,       // minecraft:can_break —— BlockPredicates
    RepairCost = 18,     // minecraft:repair_cost —— i32
    AttackRange = 29,    // minecraft:attack_range —— record（攻击范围）
    Enchantable = 30,    // minecraft:enchantable —— i32（附魔能力覆盖）
    PotionContents = 48, // minecraft:potion_contents —— record{potion,color,effects,name}
};

/**
 * @brief 组件类型名（带 "minecraft:" 前缀的资源位置）
 *
 * 用于 NBT patch 的键名。与 DataComponentType 一一对应，仅覆盖本项目落地的子集。
 * 未落地的类型返回 std::nullopt。
 */
[[nodiscard]] std::optional<std::string_view> componentName(DataComponentType type) noexcept;

/**
 * @brief 由组件资源位置名查 typeId（NBT patch 的 "!" 键反向解析时用）
 *
 * 仅覆盖本项目落地的子集；未知名返回 std::nullopt。
 */
[[nodiscard]] std::optional<DataComponentType> componentTypeByName(std::string_view name) noexcept;

/**
 * @brief 由 typeId 查类型（wire codec 读 typeId 后分发时用）
 *
 * 越界/未落地返回 std::nullopt。
 */
[[nodiscard]] std::optional<DataComponentType> componentTypeById(i32 typeId) noexcept;

/**
 * @brief 组件 typeId（= static_cast<i32>(type)，集中出口便于将来调整）
 */
[[nodiscard]] constexpr i32 componentTypeId(DataComponentType type) noexcept
{
    return static_cast<i32>(type);
}

} // namespace component
} // namespace item
} // namespace mc
