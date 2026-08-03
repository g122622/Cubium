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

#include "RedstoneWallTorchBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../IWorld.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/redstone/RedstoneTorchBlock.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

RedstoneWallTorchBlock::RedstoneWallTorchBlock(const BlockProperties& properties)
    : RedstoneTorchBlock(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::LIT())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::LIT(), true));
}

Direction RedstoneWallTorchBlock::getFacing(const BlockState& state)
{
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

BlockState RedstoneWallTorchBlock::withFacing(BlockState state, Direction facing)
{
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

bool RedstoneWallTorchBlock::shouldBeOff(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    Direction facing = getFacing(state);
    Direction attachDir = Directions::opposite(facing);
    BlockPos attachPos = pos.offset(attachDir);
    return world::redstone::RedstonePower::isSidePowered(world, attachPos, attachDir);
}

bool RedstoneWallTorchBlock::_canPlaceAt(IWorld& world, const BlockPos& pos, Direction facing) const
{
    // 检查附着面是否可以支撑火把
    BlockPos attachPos = pos.offset(Directions::opposite(facing));
    const BlockState* attachState = world.getBlockState(attachPos);
    if (!attachState || attachState->isAir()) {
        return false;
    }

    // 检查附着面是否是固体面
    return attachState->getBlock().isSolidSide(*attachState, world, attachPos, facing);
}

void RedstoneWallTorchBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 放置时通知邻居
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = neighborState->getBlockMutable();
            neighborBlock.neighborChanged(world, neighborPos, *this, pos, false);
        }
    }

    // 检查初始状态是否正确
    bool shouldBeLit = !shouldBeOff(world, pos, state);
    if (isLit(state) != shouldBeLit) {
        world.tickManager().scheduleBlockTick(pos, *this, 2, world::tick::TickPriority::ExtremelyHigh);
    }
}

void RedstoneWallTorchBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    // 检查支撑是否还在
    Direction facing = getFacing(*state);
    if (!_canPlaceAt(world, pos, facing)) {
        // 支撑丢失，火把掉落
        world.setBlockState(pos, nullptr, 2);
        return;
    }

    // 更新火把状态
    _updateState(world, pos, *state);
}

BlockState RedstoneWallTorchBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 检查支撑是否有效
    Direction torchFacing = getFacing(state);
    if (facing == Directions::opposite(torchFacing)) {
        // 支撑面被更新
        if (!facingState.isAir() && facingState.getBlock().isSolidSide(facingState, world, facingPos, torchFacing)) {
            return state;
        }
        // 支撑丢失，移除火把
        return state.with(BlockStateProperties::LIT(), false);
    }

    return state;
}

BlockState RedstoneWallTorchBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 尝试击中的面
    Direction hitFace = context.face();
    if (Directions::isHorizontal(hitFace)) {
        // 检查是否可以附着
        BlockPos pos = context.placementPos();
        BlockPos attachPos = pos.offset(hitFace);
        const IWorld& world = context.getWorld();
        const BlockState* attachState = world.getBlockState(attachPos);
        if (attachState &&
            attachState->getBlock().isSolidSide(
                *attachState, const_cast<IWorld&>(world), attachPos, Directions::opposite(hitFace))) {
            return defaultState()
                .with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(hitFace))
                .with(BlockStateProperties::LIT(), true);
        }
    }

    // 尝试其他水平方向
    for (Direction dir : {Direction::North, Direction::South, Direction::West, Direction::East}) {
        BlockPos pos = context.placementPos();
        BlockPos attachPos = pos.offset(dir);
        const IWorld& world = context.getWorld();
        const BlockState* attachState = world.getBlockState(attachPos);
        if (attachState &&
            attachState->getBlock().isSolidSide(
                *attachState, const_cast<IWorld&>(world), attachPos, Directions::opposite(dir))) {
            return defaultState()
                .with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(dir))
                .with(BlockStateProperties::LIT(), true);
        }
    }

    return defaultState();
}

i32 RedstoneWallTorchBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 熄灭时不输出信号
    if (!isLit(state)) {
        return 0;
    }

    // 不向附着面方向输出信号
    Direction facing = getFacing(state);
    if (side == facing) {
        return 0;
    }

    // 检查是否已烧毁
    if (world::redstone::RedstoneSystem::instance().isTorchBurnedOut(pos, world.currentTick())) {
        return 0;
    }

    // 点亮时输出强度15
    return world::redstone::RedstonePower::MAX_POWER;
}

const CollisionShape& RedstoneWallTorchBlock::getShape(const BlockState& state) const
{
    static const CollisionShape northShape = CollisionShape::fromPixelBox(5.5f, 3.0f, 11.0f, 10.5f, 13.0f, 16.0f);
    static const CollisionShape southShape = CollisionShape::fromPixelBox(5.5f, 3.0f, 0.0f, 10.5f, 13.0f, 5.0f);
    static const CollisionShape westShape = CollisionShape::fromPixelBox(11.0f, 3.0f, 5.5f, 16.0f, 13.0f, 10.5f);
    static const CollisionShape eastShape = CollisionShape::fromPixelBox(0.0f, 3.0f, 5.5f, 5.0f, 13.0f, 10.5f);

    switch (getFacing(state)) {
        case Direction::North:
            return northShape;
        case Direction::South:
            return southShape;
        case Direction::West:
            return westShape;
        case Direction::East:
            return eastShape;
        default:
            return northShape;
    }
}

void RedstoneWallTorchBlock::_updateState(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 检查是否应该改变状态
    bool shouldBeLit = !shouldBeOff(world, pos, state);
    bool isCurrentlyLit = isLit(state);

    if (isCurrentlyLit != shouldBeLit) {
        // 调度更新（使用2 tick延迟）
        world.tickManager().scheduleBlockTick(pos, *this, 2, world::tick::TickPriority::ExtremelyHigh);
    }
}

} // namespace blocks
} // namespace mc
