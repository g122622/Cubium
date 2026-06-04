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

namespace mc::entity {

// 引入 mc 命名空间的类型
using mc::u8;

/**
 * @brief 实体移动类型枚举
 *
 * 标识实体移动的来源，用于区分不同类型的移动事件。
 * 例如：活塞推动、玩家推动、自身移动等。
 */
enum class MoverType : u8 {
    Self = 0,       // 自身移动（AI、行走等）
    Player = 1,     // 玩家推动
    Piston = 2,     // 活塞推动
    ShulkerBox = 3, // 潜影盒推动
    Shulker = 4     // 潜影贝推动
};

/**
 * @brief 获取移动类型名称（用于调试）
 * @param type 移动类型
 * @return 名称字符串
 */
inline const char* getMoverTypeName(MoverType type) noexcept
{
    switch (type) {
        case MoverType::Self:
            return "self";
        case MoverType::Player:
            return "player";
        case MoverType::Piston:
            return "piston";
        case MoverType::ShulkerBox:
            return "shulker_box";
        case MoverType::Shulker:
            return "shulker";
    }
    return "unknown";
}

} // namespace mc::entity
