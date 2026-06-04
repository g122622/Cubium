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

/**
 * @brief 挖掘工具类型值
 *
 * 定义挖掘工具的类型值，用于方块和工具系统之间的通信。
 * 这些值被方块系统用于指定需要的工具类型。
 *
 * 注意：这些值必须与 item::tool::ToolType 枚举值保持同步。
 *
 * 参考: net.minecraftforge.common.ToolType
 */
namespace HarvestTool {
constexpr u8 None = 0;    ///< 无需工具或非工具物品
constexpr u8 Pickaxe = 1; ///< 镐 - 用于采矿（石头、矿石等）
constexpr u8 Axe = 2;     ///< 斧 - 用于伐木（原木、木板等）
constexpr u8 Shovel = 3;  ///< 锹 - 用于挖掘（泥土、沙子、雪等）
constexpr u8 Hoe = 4;     ///< 锄 - 用于耕作（干草、树叶等）
constexpr u8 Sword = 5;   ///< 剑 - 对蜘蛛网有效
constexpr u8 Shears = 6;  ///< 剪刀 - 用于剪羊毛、树叶等
} // namespace HarvestTool

} // namespace mc
