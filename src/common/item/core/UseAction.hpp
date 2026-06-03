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
 * @brief 物品使用动作枚举
 *
 * 定义物品在使用时的动作类型，用于客户端播放正确的动画。
 * 参考: net.minecraft.item.UseAction
 */
enum class UseAction : u8 {
    None = 0,           ///< 无动作（默认）
    Eat = 1,            ///< 进食动作（食物）
    Drink = 2,          ///< 饮用动作（药水、牛奶等）
    Block = 3,          ///< 格挡动作（盾牌）
    Bow = 4,            ///< 拉弓动作（弓）
    Spear = 5,          ///< 投掷动作（三叉戟）
    Crossbow = 6,       ///< 装填动作（弩）
    Spyglass = 7,       ///< 望远镜动作
    TotemOfUndying = 8, ///< 不死图腾动作
    Trident = Spear     ///< 别名，与Spear相同
};

/**
 * @brief 将使用动作转换为字符串
 * @param action 使用动作
 * @return 字符串表示
 */
[[nodiscard]] inline constexpr const char* toString(UseAction action) noexcept
{
    switch (action) {
        case UseAction::None:
            return "none";
        case UseAction::Eat:
            return "eat";
        case UseAction::Drink:
            return "drink";
        case UseAction::Block:
            return "block";
        case UseAction::Bow:
            return "bow";
        case UseAction::Spear:
            return "spear";
        case UseAction::Crossbow:
            return "crossbow";
        case UseAction::Spyglass:
            return "spyglass";
        case UseAction::TotemOfUndying:
            return "totem_of_undying";
        default:
            return "unknown";
    }
}

} // namespace mc
