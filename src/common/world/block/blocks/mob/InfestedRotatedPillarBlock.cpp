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

#include "InfestedRotatedPillarBlock.hpp"

#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/RotatedPillarBlock.hpp"
#include "common/world/block/blocks/mob/InfestedBlock.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

InfestedRotatedPillarBlock::InfestedRotatedPillarBlock(u32 hostBlock, const BlockProperties& properties)
    : InfestedBlock(hostBlock, properties)
{
    // 创建带有 axis 属性的状态容器（覆盖 InfestedBlock 的空状态容器）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(RotatedPillarBlock::AXIS())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认轴向为 Y
    setDefaultState(defaultState().with(RotatedPillarBlock::AXIS(), Axis::Y));
}

const BlockState& InfestedRotatedPillarBlock::rotate(const BlockState& state, Rotation rotation) const
{
    // 90 度旋转时 X/Z 轴互换（对齐 MC RotatedPillarBlock.rotatePillar）
    if (rotation == Rotation::Clockwise90 || rotation == Rotation::CounterClockwise90) {
        Axis currentAxis = state.get(RotatedPillarBlock::AXIS());
        if (currentAxis == Axis::X) {
            return state.with(RotatedPillarBlock::AXIS(), Axis::Z);
        }
        if (currentAxis == Axis::Z) {
            return state.with(RotatedPillarBlock::AXIS(), Axis::X);
        }
    }
    return state;
}

BlockState InfestedRotatedPillarBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 按放置面轴向设置 AXIS（对齐 MC InfestedRotatedPillarBlock.getStateForPlacement）
    Direction clickedFace = context.getClickedFace();
    Axis axis = Directions::getAxis(clickedFace);
    return defaultState().with(RotatedPillarBlock::AXIS(), axis);
}

} // namespace blocks
} // namespace mc
