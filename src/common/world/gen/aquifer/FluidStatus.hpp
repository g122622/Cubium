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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "common/core/Types.hpp"
#include <functional>

namespace mc {

// 前向声明
class BlockState;

namespace world::gen::aquifer {

// ============================================================================
// FluidStatus — 流体状态记录
// ============================================================================

/**
 * @brief 流体状态（MC 1.21 Aquifer.FluidStatus）
 *
 * 记录流体液面高度和流体方块类型。
 * 对应 MC 的 record FluidStatus(int fluidLevel, BlockState fluidType)。
 */
struct FluidStatus {
    i32 fluidLevel;
    const BlockState* fluidType;

    /**
     * @brief 获取指定 Y 高度处的方块状态
     * @param y 方块 Y 坐标
     * @return 如果 y < fluidLevel 则返回流体，否则返回空气
     */
    [[nodiscard]] const BlockState* at(i32 y) const;

    /**
     * @brief 比较两个流体状态是否相等
     *
     * MC 1.21 中 FluidStatus 是 record，equals 比较两个字段。
     * 用于含水层流动更新调度判断。
     */
    [[nodiscard]] bool operator==(const FluidStatus& other) const
    {
        return fluidLevel == other.fluidLevel && fluidType == other.fluidType;
    }

    [[nodiscard]] bool operator!=(const FluidStatus& other) const { return !(*this == other); }
};

// ============================================================================
// FluidPicker — 流体选择器接口
// ============================================================================

/**
 * @brief 流体选择器（MC 1.21 Aquifer.FluidPicker）
 *
 * 根据位置返回该处的全局流体状态。
 * 主世界实现：Y < -54 返回熔岩，Y < seaLevel 返回水，否则返回空气。
 */
using FluidPicker = std::function<FluidStatus(i32 x, i32 y, i32 z)>;

} // namespace world::gen::aquifer
} // namespace mc
