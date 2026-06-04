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

namespace mc::world::tick {

/**
 * @brief Tick执行优先级
 *
 * 当多个tick在同一游戏刻需要执行时，按优先级排序执行。
 * 数值越小优先级越高，越早执行。
 *
 * 参考: net.minecraft.world.TickPriority
 *
 * 用法示例:
 * @code
 * // 调度高优先级tick（如活塞）
 * world.tickManager().scheduleBlockTick(pos, block, delay, TickPriority::ExtremelyHigh);
 *
 * // 调度普通优先级tick
 * world.tickManager().scheduleFluidTick(pos, fluid, delay);  // 默认Normal
 * @endcode
 */
enum class TickPriority : i8 {
    ExtremelyHigh = -3, ///< 极高优先级（如活塞、红石）
    VeryHigh = -2,      ///< 很高优先级
    High = -1,          ///< 高优先级
    Normal = 0,         ///< 普通优先级（默认）
    Low = 1,            ///< 低优先级
    VeryLow = 2,        ///< 很低优先级
    ExtremelyLow = 3    ///< 极低优先级
};

/**
 * @brief 从整数值获取优先级
 *
 * @param value 整数值
 * @return 对应的优先级，如果超出范围返回边界值
 */
[[nodiscard]] inline TickPriority fromInt(i32 value) noexcept
{
    if (value < -3) return TickPriority::ExtremelyHigh;
    if (value > 3) return TickPriority::ExtremelyLow;
    return static_cast<TickPriority>(value);
}

/**
 * @brief 获取优先级的整数值
 *
 * @param priority 优先级
 * @return 整数值
 */
[[nodiscard]] inline i32 toInt(TickPriority priority) noexcept
{
    return static_cast<i32>(priority);
}

} // namespace mc::world::tick
