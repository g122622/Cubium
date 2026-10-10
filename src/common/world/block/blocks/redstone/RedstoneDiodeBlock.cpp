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

#include "RedstoneDiodeBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/blocks/redstone/RedstoneWireBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

RedstoneDiodeBlock::RedstoneDiodeBlock(const std::string& id, const BlockProperties& properties)
    : Block(properties)
    , m_id(id)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::POWERED())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North)
            .with(BlockStateProperties::POWERED(), false));
}

BlockState RedstoneDiodeBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 朝向 = 玩家水平视线方向的反方向（输入端朝玩家、输出端背离玩家）。
    // 朝向仅由玩家 yaw 决定（水平四向 South/West/North/East），不含 pitch。
    // 此前未重写该方法，落回基类 Block::getStateForPlacement 返回 defaultState()（HORIZONTAL_FACING 恒
    // North），与预期按水平视线决定朝向的行为不一致。重写后修正为按水平视线决定朝向。
    // 中继器与比较器继承本类，继承本方法自动获得正确朝向（子类额外 state 如 DELAY/MODE 由 defaultState()
    // 保留各自默认值，with 仅覆写 FACING）。
    Direction facing = Directions::opposite(context.horizontalDirection());
    return defaultState().with(BlockStateProperties::HORIZONTAL_FACING(), facing);
}

Direction RedstoneDiodeBlock::getFacing(const BlockState& state)
{
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

bool RedstoneDiodeBlock::isPowered(const BlockState& state)
{
    return state.get(BlockStateProperties::POWERED());
}

void RedstoneDiodeBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
{
    // 放置时通知邻居更新
    notifyNeighbors(world, pos, state);
}

void RedstoneDiodeBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
{
    // 移除时通知邻居更新
    notifyNeighbors(world, pos, state);
}

void RedstoneDiodeBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (!Block::canSupportRigidBlock(world, pos.down())) {
        world.setBlockState(pos, nullptr, world::BlockUpdateFlags::UPDATE_ALL);
        return;
    }
    updateState(world, pos, *state);
}
BlockState RedstoneDiodeBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);
    if (facing == Direction::Down && !Block::canSupportRigidBlock(world, currentPos.down())) {
        return VanillaBlocks::AIR->defaultState();
    }
    return state;
}

void RedstoneDiodeBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    if (isLocked(world, pos, state)) return;
    bool powered = isPowered(state);
    bool input = shouldBePowered(world, pos, state);
    if (powered && input) return;
    // 熄灭状态收到已安排的上升沿时必须亮起，即使短脉冲已经结束。
    BlockState newState = state.with(BlockStateProperties::POWERED(), !powered);
    world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_CLIENTS);
    notifyNeighbors(world, pos, newState);
    if (!powered && !input) {
        world.tickManager().scheduleBlockTick(pos, *this, getDelay(state), world::tick::TickPriority::VeryHigh);
    }
}
i32 RedstoneDiodeBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 只有在输出方向且已充能时才输出信号
    if (!isPowered(state)) {
        return 0;
    }

    Direction facing = getFacing(state);
    if (side == facing) {
        return calculateOutputSignal(world, pos, state);
    }

    return 0;
}

i32 RedstoneDiodeBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    // 二极管输出的是强信号，可以充能方块
    return getWeakPower(state, world, pos, side);
}

const CollisionShape& RedstoneDiodeBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static const CollisionShape diodeShape = CollisionShape::fromPixelBox(0.0f, 0.0f, 0.0f, 16.0f, 2.0f, 16.0f);
    return diodeShape;
}

i32 RedstoneDiodeBlock::getInputSignal(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    Direction facing = getFacing(state);
    BlockPos inputPos = pos.offset(facing);
    i32 power = world::redstone::RedstonePower::getSignal(world, inputPos, facing);
    const BlockState* inputState = world.getBlockState(inputPos);
    if (inputState != nullptr && inputState->is(VanillaBlocks::REDSTONE_WIRE)) {
        power = std::max(power, RedstoneWireBlock::getPower(*inputState));
    }
    return power;
}

i32 RedstoneDiodeBlock::getPowerOnSides(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    Direction facing = getFacing(state);
    i32 power = 0;
    for (Direction side : Directions::horizontal()) {
        if (Directions::getAxis(side) == Directions::getAxis(facing)) continue;
        BlockPos sidePos = pos.offset(side);
        const BlockState* sideState = world.getBlockState(sidePos);
        if (sideState == nullptr) continue;
        const Block& block = sideState->getBlock();
        if (sideInputDiodesOnly()) {
            if (isDiode(*sideState)) power = std::max(power, block.getStrongPower(*sideState, world, sidePos, side));
        } else if (sideState->is(VanillaBlocks::REDSTONE_BLOCK)) {
            power = 15;
        } else if (sideState->is(VanillaBlocks::REDSTONE_WIRE)) {
            power = std::max(power, RedstoneWireBlock::getPower(*sideState));
        } else if (block.canProvidePower(*sideState)) {
            power = std::max(power, block.getStrongPower(*sideState, world, sidePos, side));
        }
    }
    return power;
}
bool RedstoneDiodeBlock::isDiode(const BlockState& state) const
{
    const Block& block = state.getBlock();
    // 检查是否是中继器或比较器
    return dynamic_cast<const RedstoneDiodeBlock*>(&block) != nullptr;
}

bool RedstoneDiodeBlock::isLocked(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    return false;
}

i32 RedstoneDiodeBlock::calculateOutputSignal(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 默认输出15，子类可以重写
    return isPowered(state) ? world::redstone::RedstonePower::MAX_POWER : 0;
}

void RedstoneDiodeBlock::updateState(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 如果被锁定，不更新
    if (isLocked(world, pos, state)) {
        return;
    }

    bool shouldPower = shouldBePowered(world, pos, state);
    bool isCurrentlyPowered = isPowered(state);

    if (shouldPower != isCurrentlyPowered) {
        // 确定优先级
        world::tick::TickPriority priority = world::tick::TickPriority::High;

        if (isFacingTowardsRepeater(world, pos, state)) {
            priority = world::tick::TickPriority::ExtremelyHigh;
        } else if (isCurrentlyPowered) {
            priority = world::tick::TickPriority::VeryHigh;
        }

        // 调度更新
        world.tickManager().scheduleBlockTick(pos, *this, getDelay(state), priority);
    }
}

bool RedstoneDiodeBlock::isFacingTowardsRepeater(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    Direction facing = getFacing(state);
    BlockPos outputPos = pos.offset(Directions::opposite(facing));

    const BlockState* outputState = world.getBlockState(outputPos);
    if (!outputState) {
        return false;
    }

    // 检查输出端是否是另一个二极管
    if (!isDiode(*outputState)) {
        return false;
    }

    // 输出端二极管的主输入若不朝向当前方块，应优先处理当前计划刻。
    Direction outputFacing = getFacing(*outputState);
    return outputFacing != Directions::opposite(facing);
}

void RedstoneDiodeBlock::notifyNeighbors(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    Direction facing = getFacing(state);
    BlockPos outputPos = pos.offset(Directions::opposite(facing));
    const BlockState* outputState = world.getBlockState(outputPos);
    if (outputState != nullptr) {
        outputState->getBlockMutable().neighborChanged(world, outputPos, *this, pos, false);
    }
    // 强充能输出方块后，它周围的消费者也要重新读取信号。
    for (Direction dir : Directions::all()) {
        if (dir == facing) continue;
        BlockPos neighborPos = outputPos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState != nullptr) {
            neighborState->getBlockMutable().neighborChanged(world, neighborPos, *this, outputPos, false);
        }
    }
}
} // namespace blocks
} // namespace mc
