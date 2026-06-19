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

#include "BigDripleafStemBlock.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

BigDripleafStemBlock::BigDripleafStemBlock(const BlockProperties& properties)
    : Block(BlockProperties(properties).noCollision())
    , m_shape(CollisionShape::fromPixelBox(5, 0, 5, 11, 16, 11))
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
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
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

void BigDripleafStemBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState BigDripleafStemBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction horizontalFacing = context.horizontalDirection();
    BlockState state = defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), horizontalFacing);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

bool BigDripleafStemBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // 大滴叶茎可以存活当且仅当：
    // 1. 下方是另一个大滴叶茎 或 BIG_DRIPLEAF_PLACEABLE 标签中的方块
    // 2. 上方是另一个大滴叶茎 或 大滴叶（BigDripleafBlock）
    // 参考: net.minecraft.block.BigDripleafStemBlock.canSurvive
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState == nullptr) {
        return false;
    }
    bool validBelow = belowState->is(this) || BlockTags::BIG_DRIPLEAF_PLACEABLE().contains(*belowState);
    if (!validBelow) {
        return false;
    }

    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState == nullptr) {
        return false;
    }
    bool validAbove = aboveState->is(this) || aboveState->is(VanillaBlocks::BIG_DRIPLEAF);
    return validAbove;
}

BlockState BigDripleafStemBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 当上方或下方方块变化导致无法存活时，延迟1tick后销毁
    // 参考: net.minecraft.block.BigDripleafStemBlock.updateShape
    if ((facing == Direction::Down || facing == Direction::Up) &&
        !isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
        // 延迟销毁：MC原版使用scheduleTick(this, 1)来延迟销毁
        // 当前项目中暂时直接返回空气
        return VanillaBlocks::AIR->defaultState();
    }

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

const CollisionShape& BigDripleafStemBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const fluid::FluidState* BigDripleafStemBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

const BlockState& BigDripleafStemBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& BigDripleafStemBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction mirrored = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), mirrored);
}

} // namespace blocks
} // namespace mc
