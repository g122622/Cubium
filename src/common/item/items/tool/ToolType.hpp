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
namespace item {
namespace tool {

/**
 * @brief 工具类型枚举
 *
 * 定义工具的类型，用于判断方块是否可以被特定工具有效挖掘。
 * 方块可以设置其需要的工具类型，工具则声明自己的类型。
 *
 * 参考: net.minecraftforge.common.ToolType
 */
enum class ToolType : u8 {
    None = 0,    ///< 无需工具或非工具物品
    Pickaxe = 1, ///< 镐 - 用于采矿（石头、矿石等）
    Axe = 2,     ///< 斧 - 用于伐木（原木、木板等）
    Shovel = 3,  ///< 锹 - 用于挖掘（泥土、沙子、雪等）
    Hoe = 4,     ///< 锄 - 用于耕作（干草、树叶等）
    Sword = 5,   ///< 剑 - 对蜘蛛网有效
    Shears = 6,  ///< 剪刀 - 用于剪羊毛、树叶等
};

/**
 * @brief 获取工具类型的名称字符串
 * @param type 工具类型
 * @return 类型名称（如 "pickaxe"、"axe" 等）
 */
[[nodiscard]] const char* toString(ToolType type);

} // namespace tool
} // namespace item
} // namespace mc
