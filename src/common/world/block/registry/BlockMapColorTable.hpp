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
 * 只有床（按 part）、原木（按 axis）、barrier/light（按 waterlogged）等少数方块
 * 的颜色随状态变化，绝大多数状态与方块默认色相同，因此不必逐状态存表。
 */
struct MapColorStateOverride {
    u8 color;                   ///< 该状态的默认地图颜色 id
    std::string_view stateName; ///< 规范化状态名（属性按名字排序）
};

/**
 * @brief 一个原版方块的地图颜色及其状态例外
 */
struct MapColorBlock {
    std::string_view name;                             ///< 方块名（如 "minecraft:oak_log"）
    u8 color;                                          ///< 不含例外状态时使用的颜色
    std::vector<MapColorStateOverride> stateOverrides; ///< 与默认色不同的状态
};

/**
 * @brief 查原版方块状态的地图颜色
 *
 * 数值与原版 `MapColor.id` 同值（0~61）。查不到返回 0（AIR）。
 *
 * **按状态而非按方块**：床（part）、原木（axis）、含水方块（waterlogged）的颜色
 * 随状态变化，按方块查会把它们抹平。传入的状态名会先规范化再查。
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

/**
 * @brief 把状态名驻留到一块进程生命期内地址稳定的存储
 *
 * 返回的 `string_view` 要么指向快照表里的常量字符串，要么指向一个内部缓存槽位；
 * 两者都不会因调用方的字符串对象析构而悬空，可直接作为缓存键长期保存。
 * 快照外的状态按固定容量的轮转槽位缓存，被挤出的旧视图随之失效。
 *
 * @param stateName 方块状态的字符串表示，属性顺序不限
 * @return 地址稳定的状态名视图
 */
[[nodiscard]] std::string_view internStateName(std::string_view stateName);

} // namespace mc::world::block
