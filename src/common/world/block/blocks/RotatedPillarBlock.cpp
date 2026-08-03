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

#include "RotatedPillarBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/DirectionProperty.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {

namespace {
// 静态属性实例
std::unique_ptr<EnumProperty<Axis>> g_axisProperty;
} // namespace

const EnumProperty<Axis>& RotatedPillarBlock::AXIS()
{
    if (!g_axisProperty) {
        g_axisProperty = AxisProperty::create("axis");
    }
    return *g_axisProperty;
}

RotatedPillarBlock::RotatedPillarBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建带有axis属性的状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(AXIS()).create(
        [](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));
    // 设置默认轴向为Y
    setDefaultState(withAxis(defaultState(), Axis::Y));
}

Axis RotatedPillarBlock::getAxis(const BlockState& state) const
{
    return state.get(AXIS());
}

const BlockState& RotatedPillarBlock::withAxis(const BlockState& state, Axis axis) const
{
    return state.with(AXIS(), axis);
}

const BlockState& RotatedPillarBlock::rotate(const BlockState& state, Rotation rotation) const
{
    // 90度旋转时，X轴和Z轴互换
    if (rotation == Rotation::Clockwise90 || rotation == Rotation::CounterClockwise90) {
        Axis currentAxis = state.get(AXIS());
        if (currentAxis == Axis::X) {
            return state.with(AXIS(), Axis::Z);
        } else if (currentAxis == Axis::Z) {
            return state.with(AXIS(), Axis::X);
        }
    }
    // Y轴旋转或无旋转时保持不变
    return state;
}

BlockState RotatedPillarBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据放置面的轴向设置初始状态
    Direction clickedFace = context.getClickedFace();
    Axis axis = Directions::getAxis(clickedFace);
    return withAxis(defaultState(), axis);
}

} // namespace mc
