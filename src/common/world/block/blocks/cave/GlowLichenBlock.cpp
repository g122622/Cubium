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

#include "GlowLichenBlock.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"

namespace mc {
namespace blocks {

GlowLichenBlock::GlowLichenBlock(const BlockProperties& properties)
    : Block(properties)
    , m_shape(CollisionShape::fromPixelBox(0, 0, 0, 16, 1, 16))
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::NORTH())
            .add(BlockStateProperties::SOUTH())
            .add(BlockStateProperties::EAST())
            .add(BlockStateProperties::WEST())
            .add(BlockStateProperties::UP())
            .add(BlockStateProperties::DOWN())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::NORTH(), false)
            .with(BlockStateProperties::SOUTH(), false)
            .with(BlockStateProperties::EAST(), false)
            .with(BlockStateProperties::WEST(), false)
            .with(BlockStateProperties::UP(), false)
            .with(BlockStateProperties::DOWN(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

void GlowLichenBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState GlowLichenBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction clickedFace = context.getClickedFace();
    BlockState state = defaultState();

    // 设置被点击的面为true
    switch (clickedFace) {
        case Direction::North:
            state = state.with(BlockStateProperties::NORTH(), true);
            break;
        case Direction::South:
            state = state.with(BlockStateProperties::SOUTH(), true);
            break;
        case Direction::East:
            state = state.with(BlockStateProperties::EAST(), true);
            break;
        case Direction::West:
            state = state.with(BlockStateProperties::WEST(), true);
            break;
        case Direction::Up:
            state = state.with(BlockStateProperties::UP(), true);
            break;
        case Direction::Down:
            state = state.with(BlockStateProperties::DOWN(), true);
            break;
        default:
            break;
    }

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState GlowLichenBlock::updatePostPlacement(const BlockState& state,
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

const CollisionShape& GlowLichenBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // TODO: 根据激活的面返回正确的组合形状
    return m_shape;
}

const fluid::FluidState* GlowLichenBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

u8 GlowLichenBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 如果任意面激活，返回光照等级7
    if (state.get(BlockStateProperties::NORTH()) || state.get(BlockStateProperties::SOUTH()) ||
        state.get(BlockStateProperties::EAST()) || state.get(BlockStateProperties::WEST()) ||
        state.get(BlockStateProperties::UP()) || state.get(BlockStateProperties::DOWN())) {
        return 7;
    }
    return 0;
}

} // namespace blocks
} // namespace mc
