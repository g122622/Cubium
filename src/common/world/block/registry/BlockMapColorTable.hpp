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
 * @brief 查原版方块状态的地图颜色
 *
 * 数值与原版 `MapColor.id` 同值（0~61）。查不到返回 0（AIR）。
 *
 * **按状态而非按方块**：床（part）、原木（axis）、含水方块（waterlogged）的颜色
 * 随状态变化，按方块查会把它们抹平。
 *
 * @param stateName 方块状态的规范字符串（属性按名字排序，如 "minecraft:oak_log[axis=y]"）
 * @return 地图颜色 id
 */
[[nodiscard]] u8 lookupVanillaMapColor(std::string_view stateName);

/**
 * @brief 快照表的条目数（用于自检）
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
