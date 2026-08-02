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
 * 声明顺序：custom_data(0) max_stack_size(1) max_damage(2) damage(3)
 * unbreakable(4) ... custom_name(6) ... lore(11) rarity(12) enchantments(13)
 * can_place_on(14) can_break(15) ... repair_cost(19) ... potion_contents(49)。
 *
 * 本项目仅落地 ItemStack 现有 9 个组件字段对应的子集，未落地的类型占位与
 * Java 一致，将来扩展零冲击。
 */
enum class DataComponentType : i32 {
    CustomData = 0,      // minecraft:custom_data —— 嵌套 NBT（本项目承载 m_customData）
    MaxStackSize = 1,    // TODO 暂未落地
    MaxDamage = 2,       // TODO 暂未落地
    Damage = 3,          // minecraft:damage —— int（已承受伤害）
    Unbreakable = 4,     // TODO 暂未落地
    CustomName = 6,      // minecraft:custom_name —— Component（文本）
    ItemName = 9,        // TODO 暂未落地
    ItemModel = 10,      // TODO 暂未落地
    Lore = 11,           // minecraft:lore —— list<Component>
    Rarity = 12,         // TODO 暂未落地
    Enchantments = 13,   // minecraft:enchantments —— ItemEnchantments
    CanPlaceOn = 14,     // minecraft:can_place_on —— BlockPredicates
    CanBreak = 15,       // minecraft:can_break —— BlockPredicates
    RepairCost = 19,     // minecraft:repair_cost —— int
    Enchantable = 31,    // TODO 暂未落地
    PotionContents = 49, // minecraft:potion_contents —— record{potion,color,effects,name}
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
