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

#include "HeavyCoreBlock.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "util/property/Properties.hpp"
#include "world/block/WaterLoggableHelpers.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

HeavyCoreBlock::HeavyCoreBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 碰撞箱: 4x0x4 到 12x16x12（像素坐标），小型柱状
    // 原版 SHAPE = box(4, 0, 4, 12, 8, 12)：Y 上限 8 像素。
    // 此前写成 16（到顶），UP 面切片铺满整层，UP|CENTER 被误判为 true。
    m_shape = CollisionShape::fromPixelBox(4, 0, 4, 12, 8, 12);

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::WATERLOGGED(), false));
}

void HeavyCoreBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState HeavyCoreBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state = defaultState().with(BlockStateProperties::WATERLOGGED(), false);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState HeavyCoreBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

const CollisionShape& HeavyCoreBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const fluid::FluidState* HeavyCoreBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

bool HeavyCoreBlock::isPathfindable(const BlockState& state, PathComputationType type) const
{
    // 原版显式返回 false：形状不完整不等于可以穿过。
    MC_UNUSED(state);
    MC_UNUSED(type);
    return false;
}

} // namespace blocks
} // namespace mc
