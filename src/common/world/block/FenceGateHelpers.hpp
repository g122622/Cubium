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

#include "../../util/Direction.hpp"
#include "../../util/property/Properties.hpp"

namespace mc {
namespace fencehelpers {

/**
 * @brief 检查方块是否为栅栏门
 *
 * 通过检查 OPEN 和 IN_WALL 属性来判断方块是否为栅栏门类型。
 *
 * @param state 方块状态
 * @return 如果是栅栏门返回 true
 */
[[nodiscard]] inline bool isFenceGate(const BlockState& state)
{
    return state.hasProperty(BlockStateProperties::OPEN()) && state.hasProperty(BlockStateProperties::IN_WALL());
}

/**
 * @brief 检查栅栏门是否与给定方向平行（可连接）
 *
 * 参考: MC 1.16.5 FenceGateBlock.isParallel()
 * 栅栏门朝向轴与连接方向轴垂直时，可以连接。
 * 例如：栅栏门朝南（Z轴方向），可以连接东西方向的栅栏（X轴方向）。
 *
 * @param state 方块状态（必须是栅栏门）
 * @param connectionSide 连接方向（从栅栏/墙指向栅栏门的方向）
 * @return 如果栅栏门平行于连接方向返回 true
 */
[[nodiscard]] inline bool isFenceGateParallel(const BlockState& state, Direction connectionSide)
{
    Direction gateFacing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Axis gateAxis = Directions::getAxis(gateFacing);
    // 栅栏门朝向轴的垂直轴
    Axis perpendicularAxis = (gateAxis == Axis::X) ? Axis::Z : Axis::X;
    return Directions::getAxis(connectionSide) == perpendicularAxis;
}

} // namespace fencehelpers
} // namespace mc
