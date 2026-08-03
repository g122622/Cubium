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

#include "RedstoneTorchBlock.hpp"

#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

RedstoneTorchBlock::RedstoneTorchBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
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
    setDefaultState(defaultState().with(BlockStateProperties::LIT(), true));
}

bool RedstoneTorchBlock::shouldBeOff(IWorld& world, const BlockPos& pos) const
{
    // 检查火把附着方块（下方）是否从下方方向接收到强信号
    // 即：检查附着方块是否有来自其下方的强信号输入
    BlockPos belowPos = pos.down();
    return world::redstone::RedstonePower::isSidePowered(world, belowPos, Direction::Down);
}

bool RedstoneTorchBlock::isLit(const BlockState& state)
{
    return state.get(BlockStateProperties::LIT());
}

void RedstoneTorchBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 放置时通知六个方向的邻居
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = neighborState->getBlockMutable();
            neighborBlock.neighborChanged(world, neighborPos, *this, pos, false);
        }
    }

    // 检查初始状态是否正确
    bool shouldBeLit = !shouldBeOff(world, pos);
    if (isLit(state) != shouldBeLit) {
        // 需要更新状态
        world.tickManager().scheduleBlockTick(
            pos, *this, world::REDSTONE_DELAY, world::tick::TickPriority::ExtremelyHigh);
    }
}

void RedstoneTorchBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 移除时清理烧毁记录
    world::redstone::RedstoneSystem::instance().clearTorchRecord(pos);

    // 通知相邻方块更新
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = neighborState->getBlockMutable();
            neighborBlock.neighborChanged(world, neighborPos, *this, pos, false);
        }
    }
}

void RedstoneTorchBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 更新火把状态
    const BlockState* state = world.getBlockState(pos);
    if (state) {
        updateState(world, pos, *state);
    }
}

void RedstoneTorchBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    // 检查当前应该的状态
    bool shouldBeLit = !shouldBeOff(world, pos);
    bool isCurrentlyLit = isLit(state);

    if (isCurrentlyLit != shouldBeLit) {
        // 记录翻转并检查烧毁
        if (world::redstone::RedstoneSystem::instance().checkAndRecordTorchFlip(pos, world.currentTick())) {
            // 烧毁！保持当前状态，调度下一次检查
            world.tickManager().scheduleBlockTick(pos,
                *this,
                world::redstone::RedstoneSystem::BURNOUT_COOLDOWN,
                world::tick::TickPriority::ExtremelyHigh);
            return;
        }

        // 改变状态
        BlockState newState = state.with(BlockStateProperties::LIT(), shouldBeLit);
        world.setBlockState(pos, &newState, 3);

        // 更新相邻方块
        world::redstone::RedstoneSystem::instance().updateNeighborsExcept(world, pos, *this, Direction::Down);
    }
}

i32 RedstoneTorchBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 熄灭时不输出信号
    if (!isLit(state)) {
        return 0;
    }

    // 不向下输出信号
    if (side == Direction::Down) {
        return 0;
    }

    // 检查是否已烧毁
    if (world::redstone::RedstoneSystem::instance().isTorchBurnedOut(pos, world.currentTick())) {
        return 0;
    }

    // 点亮时输出强度15
    return world::redstone::RedstonePower::MAX_POWER;
}

i32 RedstoneTorchBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    // 只在向下方向输出强信号（充能下方方块）
    return side == Direction::Down ? getWeakPower(state, world, pos, side) : 0;
}

const CollisionShape& RedstoneTorchBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static const CollisionShape torchShape = CollisionShape::fromPixelBox(7.0f, 0.0f, 7.0f, 9.0f, 10.0f, 9.0f);
    return torchShape;
}

void RedstoneTorchBlock::updateState(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 检查是否应该改变状态
    bool shouldBeLit = !shouldBeOff(world, pos);
    bool isCurrentlyLit = isLit(state);

    if (isCurrentlyLit != shouldBeLit) {
        // 调度更新
        world.tickManager().scheduleBlockTick(
            pos, *this, world::REDSTONE_DELAY, world::tick::TickPriority::ExtremelyHigh);
    }
}

} // namespace blocks
} // namespace mc
