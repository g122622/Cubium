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

#include "common/core/EnumSet.hpp"
#include "common/core/Types.hpp"

namespace mc::entity::ai {

/**
 * @brief AI目标互斥标志
 *
 * 用于控制多个AI目标之间的互斥关系。
 * 如果两个目标共享相同的标志，则不能同时运行。
 */
enum class GoalFlag : u8 {
    Move,   // 移动
    Look,   // 视线
    Jump,   // 跳跃
    Target, // 目标选择
    Count   // 标志数量
};

/**
 * @brief 获取所有目标标志的集合
 * @return 包含所有标志的集合
 */
[[nodiscard]] inline EnumSet<GoalFlag> allGoalFlags() noexcept
{
    return EnumSet<GoalFlag>{GoalFlag::Move, GoalFlag::Look, GoalFlag::Jump, GoalFlag::Target};
}

} // namespace mc::entity::ai
