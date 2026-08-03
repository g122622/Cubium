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

#include "ChainBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../IWorld.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

ChainBlock::ChainBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 创建状态容器（AXIS 属性）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AXIS())
            .add(BlockStateProperties::WATERLOGGED())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));
    setDefaultState(
        defaultState().with(BlockStateProperties::AXIS(), Axis::Y).with(BlockStateProperties::WATERLOGGED(), false));

    // 创建形状 - 锁链是细长的柱子
    // Y轴：垂直锁链
    m_yShape = CollisionShape::box(6.0f, 0.0f, 6.0f, 10.0f, 16.0f, 10.0f);
    // X轴：水平锁链（东西方向）
    m_xShape = CollisionShape::box(0.0f, 6.0f, 6.0f, 16.0f, 10.0f, 10.0f);
    // Z轴：水平锁链（南北方向）
    m_zShape = CollisionShape::box(6.0f, 6.0f, 0.0f, 10.0f, 10.0f, 16.0f);
}

BlockState ChainBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 根据点击面确定轴向
    Direction face = context.getClickedFace();
    Axis axis = Directions::getAxis(face);

    // 检查是否含水
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();
    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    return defaultState()
        .with(BlockStateProperties::AXIS(), axis)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

const BlockState& ChainBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Axis axis = state.get(BlockStateProperties::AXIS());

    if (rotation == Rotation::Clockwise90 || rotation == Rotation::CounterClockwise90) {
        // 旋转90度切换X和Z轴
        if (axis == Axis::X) {
            return state.with(BlockStateProperties::AXIS(), Axis::Z);
        } else if (axis == Axis::Z) {
            return state.with(BlockStateProperties::AXIS(), Axis::X);
        }
    }

    return state;
}

const CollisionShape& ChainBlock::getShape(const BlockState& state) const
{
    Axis axis = state.get(BlockStateProperties::AXIS());

    switch (axis) {
        case Axis::X:
            return m_xShape;
        case Axis::Z:
            return m_zShape;
        case Axis::Y:
        default:
            return m_yShape;
    }
}

BlockState ChainBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 处理含水状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* ChainBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

} // namespace blocks
} // namespace mc
