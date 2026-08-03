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

#include "AbstractButtonBlock.hpp"
#include "../../../IWorld.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// 使用 BlockStateProperties 中的 AttachFace
using AttachFace = BlockStateProperties::AttachFace;

AbstractButtonBlock::AbstractButtonBlock(const BlockProperties& properties, i32 ticksToStayPressed)
    : Block(properties)
    , m_ticksToStayPressed(ticksToStayPressed)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::ATTACH_FACE())
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
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::ATTACH_FACE(), AttachFace::Wall));
}

bool AbstractButtonBlock::isPowered(const BlockState& state) noexcept
{
    return state.get(BlockStateProperties::POWERED());
}

BlockState AbstractButtonBlock::withPowered(BlockState state, bool powered) noexcept
{
    return state.with(BlockStateProperties::POWERED(), powered);
}

Direction AbstractButtonBlock::getFacing(const BlockState& state) noexcept
{
    return state.get(BlockStateProperties::HORIZONTAL_FACING());
}

void AbstractButtonBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 按钮放置时不触发信号
}

void AbstractButtonBlock::neighborChanged(
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
    AttachFace attachFace = state->get(BlockStateProperties::ATTACH_FACE());

    // 计算支撑方块位置
    BlockPos supportPos;
    switch (attachFace) {
        case AttachFace::Floor:
            supportPos = pos.down();
            break;
        case AttachFace::Ceiling:
            supportPos = pos.up();
            break;
        case AttachFace::Wall:
            supportPos = pos.offset(Directions::opposite(facing));
            break;
    }

    // 如果支撑方块被移除，按钮掉落
    const BlockState* supportState = world.getBlockState(supportPos);
    if (!supportState || supportState->isAir()) {
        // 按钮掉落 - 设置为空气方块
        world.setBlockState(pos, nullptr, 2);
    }
}

BlockState AbstractButtonBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);

    // 检查支撑是否有效
    Direction blockFacing = getFacing(state);
    AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());

    BlockPos supportPos;
    switch (attachFace) {
        case AttachFace::Floor:
            supportPos = currentPos.down();
            break;
        case AttachFace::Ceiling:
            supportPos = currentPos.up();
            break;
        case AttachFace::Wall:
            supportPos = currentPos.offset(Directions::opposite(blockFacing));
            break;
    }

    const BlockState* supportState = world.getBlockState(supportPos);
    if (!supportState || supportState->isAir()) {
        return state.with(BlockStateProperties::POWERED(), false);
    }

    return state;
}

void AbstractButtonBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);
    if (isPowered(state)) {
        // 按钮弹起
        BlockState newState = withPowered(state, false);
        world.setBlockState(pos, &newState, 2);

        // 播放弹起音效
        playClickSound(world, pos, false);

        // 通知相邻方块更新
        Direction facing = getFacing(state);
        notifyNeighbors(world, pos, facing);
    }
}

i32 AbstractButtonBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 按钮按下时向所有方向输出弱信号
    return isPowered(state) ? world::redstone::RedstonePower::MAX_POWER : 0;
}

i32 AbstractButtonBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 只在附着面方向输出强信号
    if (!isPowered(state)) {
        return 0;
    }

    // getFacing 返回附着面方向
    Direction facing = getFacing(state);
    AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());

    Direction outputDir = Direction::North; // 默认值
    switch (attachFace) {
        case AttachFace::Floor:
            outputDir = Direction::Up; // 地板按钮强信号向上（附着面）
            break;
        case AttachFace::Ceiling:
            outputDir = Direction::Down; // 天花板按钮强信号向下（附着面）
            break;
        case AttachFace::Wall:
            outputDir = Directions::opposite(facing); // 墙按钮强信号向附着面（背面）
            break;
        default:
            break;
    }

    // 只在输出方向输出强信号
    if (side == outputDir) {
        return world::redstone::RedstonePower::MAX_POWER;
    }

    return 0;
}

const CollisionShape& AbstractButtonBlock::getShape(const BlockState& state) const
{
    // 采用原版按钮的大致体素尺寸：墙面 6x4 像素，地板/天花板 6x6 像素。
    static const CollisionShape floorUnpressed = CollisionShape::fromPixelBox(5.0f, 0.0f, 5.0f, 11.0f, 2.0f, 11.0f);
    static const CollisionShape floorPressed = CollisionShape::fromPixelBox(5.0f, 0.0f, 5.0f, 11.0f, 1.0f, 11.0f);
    static const CollisionShape ceilingUnpressed = CollisionShape::fromPixelBox(5.0f, 14.0f, 5.0f, 11.0f, 16.0f, 11.0f);
    static const CollisionShape ceilingPressed = CollisionShape::fromPixelBox(5.0f, 15.0f, 5.0f, 11.0f, 16.0f, 11.0f);

    static const CollisionShape wallNorthUnpressed =
        CollisionShape::fromPixelBox(5.0f, 6.0f, 14.0f, 11.0f, 10.0f, 16.0f);
    static const CollisionShape wallNorthPressed = CollisionShape::fromPixelBox(5.0f, 6.0f, 15.0f, 11.0f, 10.0f, 16.0f);
    static const CollisionShape wallSouthUnpressed = CollisionShape::fromPixelBox(5.0f, 6.0f, 0.0f, 11.0f, 10.0f, 2.0f);
    static const CollisionShape wallSouthPressed = CollisionShape::fromPixelBox(5.0f, 6.0f, 0.0f, 11.0f, 10.0f, 1.0f);
    static const CollisionShape wallWestUnpressed =
        CollisionShape::fromPixelBox(14.0f, 6.0f, 5.0f, 16.0f, 10.0f, 11.0f);
    static const CollisionShape wallWestPressed = CollisionShape::fromPixelBox(15.0f, 6.0f, 5.0f, 16.0f, 10.0f, 11.0f);
    static const CollisionShape wallEastUnpressed = CollisionShape::fromPixelBox(0.0f, 6.0f, 5.0f, 2.0f, 10.0f, 11.0f);
    static const CollisionShape wallEastPressed = CollisionShape::fromPixelBox(0.0f, 6.0f, 5.0f, 1.0f, 10.0f, 11.0f);

    const bool powered = isPowered(state);
    const AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());
    const Direction facing = getFacing(state);

    if (attachFace == AttachFace::Floor) {
        return powered ? floorPressed : floorUnpressed;
    }
    if (attachFace == AttachFace::Ceiling) {
        return powered ? ceilingPressed : ceilingUnpressed;
    }

    switch (facing) {
        case Direction::North:
            return powered ? wallNorthPressed : wallNorthUnpressed;
        case Direction::South:
            return powered ? wallSouthPressed : wallSouthUnpressed;
        case Direction::West:
            return powered ? wallWestPressed : wallWestUnpressed;
        case Direction::East:
            return powered ? wallEastPressed : wallEastUnpressed;
        default:
            return powered ? wallNorthPressed : wallNorthUnpressed;
    }
}

void AbstractButtonBlock::press(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 如果已经按下，不重复触发
    if (isPowered(state)) {
        return;
    }

    // 按下按钮
    BlockState newState = withPowered(state, true);
    world.setBlockState(pos, &newState, 2);

    // 播放按下音效
    playClickSound(world, pos, true);

    // 通知相邻方块更新
    Direction facing = getFacing(state);
    notifyNeighbors(world, pos, facing);

    // 调度弹起
    world.tickManager().scheduleBlockTick(pos, *this, m_ticksToStayPressed, world::tick::TickPriority::High);
}

bool AbstractButtonBlock::canAttachToFace(Direction facing) const
{
    // 按钮可以附着在任何水平方向
    return Directions::isHorizontal(facing);
}

void AbstractButtonBlock::notifyNeighbors(IWorld& world, const BlockPos& pos, Direction facing)
{
    // 获取按钮输出方向
    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return;
    }

    AttachFace attachFace = state->get(BlockStateProperties::ATTACH_FACE());

    // 计算输出方向和支撑位置
    Direction outputDir = Direction::North; // 默认值
    BlockPos supportPos = pos;

    switch (attachFace) {
        case AttachFace::Floor:
            outputDir = Direction::Up;
            supportPos = pos.down(); // 支撑在下方
            break;
        case AttachFace::Ceiling:
            outputDir = Direction::Down;
            supportPos = pos.up(); // 支撑在上方
            break;
        case AttachFace::Wall:
            // 墙按钮向附着面方向输出，即 facing 的反方向
            outputDir = Directions::opposite(facing);
            supportPos = pos.offset(Directions::opposite(facing)); // 支撑在背面
            break;
        default:
            break;
    }

    // 通知输出方向的方块
    BlockPos outputPos = pos.offset(outputDir);
    const BlockState* outputState = world.getBlockState(outputPos);
    if (outputState && !outputState->isAir()) {
        Block& outputBlock = outputState->getBlockMutable();
        outputBlock.neighborChanged(world, outputPos, *this, pos, false);
    }

    // 通过支撑方块传递信号（支撑方块也被充能）
    const BlockState* supportState = world.getBlockState(supportPos);
    if (supportState && !supportState->isAir()) {
        Block& supportBlock = supportState->getBlockMutable();
        supportBlock.neighborChanged(world, supportPos, *this, pos, false);
    }
}

} // namespace blocks
} // namespace mc
