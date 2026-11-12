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

#include "common/world/block/blocks/coral/CoralBlock.hpp"

#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/capability/WaterLoggableHelpers.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/support/SupportType.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

namespace {

[[nodiscard]] bool hasNearbyWater(mc::IWorld& world, const mc::BlockPos& pos)
{
    for (mc::Direction dir : {mc::Direction::North,
             mc::Direction::South,
             mc::Direction::East,
             mc::Direction::West,
             mc::Direction::Up,
             mc::Direction::Down}) {
        const mc::fluid::FluidState* fluidState = world.getFluidState(pos.offset(dir));
        if (mc::waterloggable::isWaterFluidState(fluidState)) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] const mc::BlockState* getDeadBlockState(mc::u32 deadBlockId)
{
    mc::Block* deadBlock = mc::BlockRegistry::instance().getBlock(deadBlockId);
    return deadBlock != nullptr ? &deadBlock->defaultState() : nullptr;
}

} // namespace

// ========== CoralBlock ==========

CoralBlock::CoralBlock(CoralColor color, u32 deadBlock, const BlockProperties& properties)
    : Block(properties)
    , m_color(color)
    , m_deadBlock(deadBlock)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::WATERLOGGED())
            .create([this](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::WATERLOGGED(), false));
}

BlockState CoralBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    return defaultState().with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

BlockState CoralBlock::updatePostPlacement(const BlockState& state,
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

    if (!hasNearbyWater(world, currentPos)) {
        if (const BlockState* deadState = getDeadBlockState(m_deadBlock); deadState != nullptr) {
            return *deadState;
        }
    }

    return state;
}

bool CoralBlock::isInWater(const BlockState& state) const
{
    return state.get(BlockStateProperties::WATERLOGGED());
}

bool CoralBlock::isWaterNearby(IWorld& world, const BlockPos& pos) const
{
    return hasNearbyWater(world, pos);
}

const CollisionShape& CoralBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape fullShape = CollisionShape::fullBlock();
    return fullShape;
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* CoralBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

} // namespace blocks
} // namespace mc
