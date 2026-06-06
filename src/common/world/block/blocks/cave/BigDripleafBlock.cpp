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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ON AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "BigDripleafBlock.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/tick/manager/TickManager.hpp"

namespace mc {
namespace blocks {

// 倾斜延迟（MC源码）
static constexpr i32 TILT_DELAY_UNSTABLE = 10; // NONE→UNSTABLE后等待10tick
static constexpr i32 TILT_DELAY_PARTIAL = 10;  // UNSTABLE→PARTIAL后等待10tick
static constexpr i32 TILT_DELAY_FULL = 100;    // PARTIAL→FULL后等待100tick

BigDripleafBlock::BigDripleafBlock(const BlockProperties& properties)
    : Block(properties)
    , m_fullShape(CollisionShape::fromPixelBox(0, 0, 0, 16, 16, 16))
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::TILT())
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
            .with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::None)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

void BigDripleafBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState BigDripleafBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction horizontalFacing = context.horizontalDirection();
    BlockState state = defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), horizontalFacing);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState BigDripleafBlock::updatePostPlacement(const BlockState& state,
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

const CollisionShape& BigDripleafBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_fullShape;
}

const CollisionShape& BigDripleafBlock::getCollisionShape(const BlockState& state) const
{
    BlockStateProperties::Tilt tilt = state.get(BlockStateProperties::TILT());
    // 完全倾斜时玩家会掉落
    if (tilt == BlockStateProperties::Tilt::Full) {
        return VoxelShapes::empty();
    }
    return m_fullShape;
}

const fluid::FluidState* BigDripleafBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

void BigDripleafBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    BlockStateProperties::Tilt tilt = state.get(BlockStateProperties::TILT());

    switch (tilt) {
        case BlockStateProperties::Tilt::Unstable:
            // UNSTABLE → PARTIAL
            state = state.with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Partial);
            world.setBlockState(pos, &state, 3);
            _scheduleTiltTick(world, pos, BlockStateProperties::Tilt::Partial);
            break;

        case BlockStateProperties::Tilt::Partial:
            // PARTIAL → FULL
            state = state.with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Full);
            world.setBlockState(pos, &state, 3);
            _scheduleTiltTick(world, pos, BlockStateProperties::Tilt::Full);
            break;

        case BlockStateProperties::Tilt::Full:
            // FULL → NONE (自动重置)
            _resetTilt(world, pos, state);
            break;

        default:
            break;
    }
}

void BigDripleafBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
    // 大滴叶不需要随机刻
}

void BigDripleafBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // TODO: 红石信号检测 - 需要红石系统支持
    // MC逻辑：当接收到红石信号时，立即重置倾斜状态为NONE
    // if (world.hasNeighborSignal(pos)) {
    //     BlockStateProperties::Tilt tilt = world.getBlockState(pos)->get(BlockStateProperties::TILT());
    //     if (tilt != BlockStateProperties::Tilt::None) {
    //         BlockState newState =
    //             world.getBlockState(pos)->with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::None);
    //         world.setBlockState(pos, &newState, 3);
    //     }
    // }
}

const BlockState& BigDripleafBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Direction rotated = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), rotated);
}

const BlockState& BigDripleafBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rotation = Directions::mirrorToRotation(mirror, facing);
    Direction mirrored = Directions::rotateDirection(facing, rotation);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), mirrored);
}

void BigDripleafBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(entity);

    // 只有NONE状态的叶片才触发倾斜
    if (state.get(BlockStateProperties::TILT()) == BlockStateProperties::Tilt::None) {
        // 设置为UNSTABLE并调度tick
        BlockState newState = state.with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::Unstable);
        world.setBlockState(pos, &newState, 3);
        _scheduleTiltTick(world, pos, BlockStateProperties::Tilt::Unstable);
    }
}

i32 BigDripleafBlock::_getTiltDelay(BlockStateProperties::Tilt tilt)
{
    switch (tilt) {
        case BlockStateProperties::Tilt::Unstable:
            return TILT_DELAY_UNSTABLE;
        case BlockStateProperties::Tilt::Partial:
            return TILT_DELAY_PARTIAL;
        case BlockStateProperties::Tilt::Full:
            return TILT_DELAY_FULL;
        default:
            return 0;
    }
}

void BigDripleafBlock::_scheduleTiltTick(IWorld& world, const BlockPos& pos, BlockStateProperties::Tilt tilt) const
{
    i32 delay = _getTiltDelay(tilt);
    if (delay > 0) {
        // const方法中需要移除const以匹配scheduleBlockTick的非常量Block&参数
        world.tickManager().scheduleBlockTick(pos, const_cast<BigDripleafBlock&>(*this), delay);
    }
}

void BigDripleafBlock::_resetTilt(IWorld& world, const BlockPos& pos, BlockState& state)
{
    BlockState newState = state.with(BlockStateProperties::TILT(), BlockStateProperties::Tilt::None);
    world.setBlockState(pos, &newState, 3);
}

} // namespace blocks
} // namespace mc
