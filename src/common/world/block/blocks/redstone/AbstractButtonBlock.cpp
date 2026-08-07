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
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include <array>
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
    MC_UNUSED(world);
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 对齐 vanilla：按钮（FaceAttachedHorizontalDirectionalBlock）不在 neighborChanged 中
    // 检查支撑——支撑检查在 updatePostPlacement（vanilla updateShape）中，受 setBlockState
    // 的 flags 门控（结构放置 flags=18 含 UPDATE_KNOWN_SHAPE 跳过形状更新，故不触发自毁）。
    // 此前项目把支撑自毁放在 neighborChanged，而红石线 _notifyWireNeighbors 会绕过 flags
    // 直接调 neighborChanged，导致结构放置时按钮在支撑（红石线）尚未放置前被通知自毁。
    // TODO: vanilla 按钮继承默认 neighborChanged（无 override）；项目保留空实现以兼容现有调用约定。
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

    // 对齐 vanilla FaceAttachedHorizontalDirectionalBlock.updateShape：
    //   getConnectedDirection(state).getOpposite() == direction && !canSurvive => AIR
    // getConnectedDirection（vanilla）：CEILING→Down, FLOOR→Up, WALL→FACING（朝向/输出方向）。
    // 其 opposite = 支撑方向。仅当邻居变化方向 == 支撑方向时检查 canSurvive。
    // canSurvive = 支撑方块 isFaceSturdy；项目用 isAir 判定支撑缺失（简化）。
    // 此方法由 setBlockState 邻居循环调用，受 flags&UPDATE_NEIGHBORS 门控；结构放置 flags=18
    // 跳过邻居循环故不触发，避免按钮在支撑未放置时自毁。
    const AttachFace attachFace = state.get(BlockStateProperties::ATTACH_FACE());
    Direction connectedDir;
    switch (attachFace) {
        case AttachFace::Ceiling:
            connectedDir = Direction::Down;
            break;
        case AttachFace::Floor:
            connectedDir = Direction::Up;
            break;
        case AttachFace::Wall:
        default:
            connectedDir = getFacing(state);
            break;
    }

    const Direction supportDir = Directions::opposite(connectedDir);
    if (supportDir != facing) {
        return state;
    }

    const BlockPos supportPos = currentPos.offset(supportDir);
    const BlockState* supportState = world.getBlockState(supportPos);
    if (!supportState || supportState->isAir()) {
        return *BlockRegistry::instance().airState();
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
    MC_UNUSED(side);

    // 基岩版红石语义：按钮按下时向所有方向输出强信号（基岩红石是方块充能模型，
    // 不区分 attach face 的方向性强输出）。minecraft-gametests 行为包的 floor 按钮
    // 需激活水平相邻的红石线，故此处全向输出 MAX_POWER。
    // TODO: Java 红石体系就绪后，按 attachFace/facing 精确输出（仅 outputDir 方向）。
    return isPowered(state) ? world::redstone::RedstonePower::MAX_POWER : 0;
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

    // 基岩版红石语义：按钮 press 后向所有 6 向相邻方块输出红石信号（基岩红石是方块充能模型，
    // 不区分 attach face 的方向性输出，区别于 Java 按钮仅向 outputDir + supportDir 输出）。
    // minecraft-gametests 行为包的按钮结构（如 clone_command 的 floor 按钮 fd=1）据此设计：
    // floor 按钮需激活水平相邻的红石线以触发命令方块。项目 Java 红石完整实现尚未就绪，
    // 此处采用基岩语义以打通 GameTest；Java 语义对齐留待红石体系完善时处理。
    // TODO: Java 红石体系就绪后，按 attachFace/facing 精确输出（outputDir + supportDir）。
    static const std::array<Direction, 6> kAllDirections = {
        Direction::Down, Direction::Up, Direction::North, Direction::South, Direction::West, Direction::East};

    for (Direction dir : kAllDirections) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = neighborState->getBlockMutable();
            neighborBlock.neighborChanged(world, neighborPos, *this, pos, false);
        }
    }

    // 保留 facing 参数引用（基岩语义下不依赖 facing，但签名保留以兼容调用方）
    MC_UNUSED(facing);
}

} // namespace blocks
} // namespace mc
