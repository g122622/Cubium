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

#include "core/Types.hpp"
#include <string>
#include <string_view>

namespace mc::world::block {

/**
 * @brief 某个方块状态相对所属方块的默认地图颜色例外
 *
 * 只有床（按 part）、原木/竹块（按 axis）、barrier/light（按 waterlogged）、小麦（按 age）
 * 共 29 个方块的颜色随状态变化；例外按「方块名 + 变色属性 + 属性值」记录，其余状态与方块
 * 默认色相同。
 */
struct MapColorStateOverride {
    std::string_view blockName; ///< 方块名（如 "minecraft:oak_log"）
    std::string_view value;     ///< 变色属性取该值时使用 color（如 "y"、"head"、"true"）
    std::string_view attribute; ///< 决定颜色的属性名（如 "axis"、"part"）
    u8 color;                   ///< 该取值下的地图颜色 id
};

/**
 * @brief 一个原版方块的地图颜色
 */
struct MapColorBlock {
    std::string_view name; ///< 方块名（如 "minecraft:oak_log"）
    u8 color;              ///< 不含状态例外时使用的颜色
};

/**
 * @brief 查原版方块状态的地图颜色
 *
 * 数值与原版 `MapColor.id` 同值（0~61）。查不到返回 0（AIR）。
 *
 * 有 29 个方块的颜色随状态变化（床按 part、原木按 axis、含水方块按 waterlogged、
 * 小麦按 age），按方块查会把它们抹平。传入的状态名会先规范化再查。
 *
 * @param stateName 方块状态的字符串表示，属性顺序不限
 * @return 地图颜色 id
 */
[[nodiscard]] u8 lookupVanillaMapColor(std::string_view stateName);

/**
 * @brief 快照表的条目数（用于自检）
 *
 * 只统计方块条目，不含例外状态条目。
 */
[[nodiscard]] i32 vanillaMapColorTableSize();

/**
 * @brief 把状态的字符串表示规范化（属性按名字排序）
 *
 * `BlockState::toString()` 按属性注册顺序输出，而快照表的键是排好序的规范名。
 *
 * @param raw 原始状态字符串
 * @return 规范名
 */
[[nodiscard]] std::string canonicalStateName(const std::string& raw);

} // namespace mc::world::block
