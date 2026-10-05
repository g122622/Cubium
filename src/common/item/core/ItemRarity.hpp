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
#include <string_view>

namespace mc {

/**
 * @brief 物品稀有度（1.21.11 net.minecraft.world.item.Rarity）
 *
 * id 严格对齐 vanilla Rarity 枚举声明顺序：common(0) uncommon(1) rare(2) epic(3)。
 * 该 id 既用于 rarity 组件的网络 wire（VarInt），也用于数据包 JSON 的枚举名解析。
 */
enum class ItemRarity : u8 {
    Common = 0,   // 普通 - 白色
    Uncommon = 1, // 少见 - 黄色
    Rare = 2,     // 稀有 - 青色
    Epic = 3      // 史诗 - 紫色
};

/**
 * @brief 稀有度 → 数据包枚举名（common/uncommon/rare/epic）
 *
 * 对应 vanilla StringRepresentable.fromValues(Rarity::values)，用于 rarity 组件的
 * 持久化 NBT/JSON 编解码。非法值返回 std::nullopt。
 */
[[nodiscard]] std::optional<std::string_view> rarityName(ItemRarity rarity) noexcept;

/**
 * @brief 枚举名 → 稀有度（rarityName 的逆）
 *
 * 未知名返回 std::nullopt。
 */
[[nodiscard]] std::optional<ItemRarity> rarityFromName(std::string_view name) noexcept;

/**
 * @brief id → 稀有度（网络 wire 读取用）
 *
 * 对应 vanilla Rarity.BY_ID（ByIdMap.continuous + OutOfBoundsStrategy.ZERO），
 * 越界返回 Common。故此函数恒有返回值。
 */
[[nodiscard]] ItemRarity rarityFromId(i32 id) noexcept;

/**
 * @brief 稀有度 id（集中出口，便于将来调整）
 */
[[nodiscard]] constexpr i32 rarityId(ItemRarity rarity) noexcept
{
    return static_cast<i32>(rarity);
}

} // namespace mc
