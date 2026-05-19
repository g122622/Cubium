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

#include "../../core/Types.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;
namespace fluid {
class Fluid;
class FluidState;
} // namespace fluid

/**
 * @brief 液体容器接口
 *
 * 实现此接口的方块可以容纳液体（如大锅、炼药锅等）。
 * 参考 MC 1.16.5 ILiquidContainer
 *
 * 当流体流入实现了此接口的方块时，会先调用 canContainFluid 检查是否可以容纳，
 * 然后调用 receiveFluid 来实际接收流体。
 *
 * 注意：MC 1.16.5 中 canContainFluid 使用 IBlockReader，但本项目 IBlockReader
 * 继承自 IWorld，为了简化接口统一使用 IWorld。
 */
class ILiquidContainer {
public:
    virtual ~ILiquidContainer() = default;

    /**
     * @brief 检查是否可以容纳指定流体
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param fluid 要容纳的流体
     * @return 是否可以容纳
     */
    [[nodiscard]] virtual bool canContainFluid(
        IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::Fluid& fluid) const = 0;

    /**
     * @brief 接收流体
     *
     * 当流体流入时调用此方法来更新方块状态。
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param fluidState 流入的流体状态
     * @return 是否成功接收
     */
    virtual bool receiveFluid(
        IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::FluidState& fluidState) = 0;

    /**
     * @brief 检查是否包含流体
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return 是否包含流体
     */
    [[nodiscard]] virtual bool containsFluid(IWorld& world, const BlockPos& pos, const BlockState& state) const = 0;
};

} // namespace mc
