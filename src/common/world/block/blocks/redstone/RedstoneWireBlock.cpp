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

#include "RedstoneWireBlock.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../IWorld.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "../../../tick/base/TickPriority.hpp"
#include "../../../tick/manager/TickManager.hpp"
#include "../../Block.hpp"
#include "ObserverBlock.hpp"
#include "RedstoneDiodeBlock.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 静态形状常量定义 ==========
// 像素单位转方块单位（16像素=1方块）
constexpr f32 P = 1.0f / 16.0f;

// 中心点形状 (3, 0, 3) -> (13, 1, 13)
const CollisionShape RedstoneWireBlock::s_centerShape =
    CollisionShape::box(3.0f * P, 0.0f, 3.0f * P, 13.0f * P, 1.0f * P, 13.0f * P);

// 各方向水平连接形状
const CollisionShape RedstoneWireBlock::s_northSideShape =
    CollisionShape::box(3.0f * P, 0.0f, 0.0f, 13.0f * P, 1.0f * P, 13.0f * P);
const CollisionShape RedstoneWireBlock::s_southSideShape =
    CollisionShape::box(3.0f * P, 0.0f, 3.0f * P, 13.0f * P, 1.0f * P, 1.0f);
const CollisionShape RedstoneWireBlock::s_eastSideShape =
    CollisionShape::box(3.0f * P, 0.0f, 3.0f * P, 1.0f, 1.0f * P, 13.0f * P);
const CollisionShape RedstoneWireBlock::s_westSideShape =
    CollisionShape::box(0.0f, 0.0f, 3.0f * P, 13.0f * P, 1.0f * P, 13.0f * P);

// 各方向向上连接形状（水平部分 + 向上延伸部分）
const CollisionShape RedstoneWireBlock::s_northAscendingShape =
    CollisionShape::combine(CollisionShape::box(3.0f * P, 0.0f, 0.0f, 13.0f * P, 1.0f * P, 13.0f * P),
        CollisionShape::box(3.0f * P, 0.0f, 0.0f, 13.0f * P, 1.0f, 1.0f * P),
        CollisionShape::CombineOp::OR);
const CollisionShape RedstoneWireBlock::s_southAscendingShape =
    CollisionShape::combine(CollisionShape::box(3.0f * P, 0.0f, 3.0f * P, 13.0f * P, 1.0f * P, 1.0f),
        CollisionShape::box(3.0f * P, 0.0f, 15.0f * P, 13.0f * P, 1.0f, 1.0f),
        CollisionShape::CombineOp::OR);
const CollisionShape RedstoneWireBlock::s_eastAscendingShape =
    CollisionShape::combine(CollisionShape::box(3.0f * P, 0.0f, 3.0f * P, 1.0f, 1.0f * P, 13.0f * P),
        CollisionShape::box(15.0f * P, 0.0f, 3.0f * P, 1.0f, 1.0f, 13.0f * P),
        CollisionShape::CombineOp::OR);
const CollisionShape RedstoneWireBlock::s_westAscendingShape =
    CollisionShape::combine(CollisionShape::box(0.0f, 0.0f, 3.0f * P, 13.0f * P, 1.0f * P, 13.0f * P),
        CollisionShape::box(0.0f, 0.0f, 3.0f * P, 1.0f * P, 1.0f, 13.0f * P),
        CollisionShape::CombineOp::OR);

// 使用 BlockStateProperties 中的红石线属性
// 不再需要自定义的 NORTH_PROP 等

RedstoneWireBlock::RedstoneWireBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器 - 使用 BlockStateProperties 中的 REDSTONE_NORTH 等属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::POWER_0_15())
            .add(BlockStateProperties::REDSTONE_NORTH())
            .add(BlockStateProperties::REDSTONE_EAST())
            .add(BlockStateProperties::REDSTONE_SOUTH())
            .add(BlockStateProperties::REDSTONE_WEST())
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
            .with(BlockStateProperties::POWER_0_15(), 0)
            .with(BlockStateProperties::REDSTONE_NORTH(), BlockStateProperties::RedstoneSide::None)
            .with(BlockStateProperties::REDSTONE_EAST(), BlockStateProperties::RedstoneSide::None)
            .with(BlockStateProperties::REDSTONE_SOUTH(), BlockStateProperties::RedstoneSide::None)
            .with(BlockStateProperties::REDSTONE_WEST(), BlockStateProperties::RedstoneSide::None));
}

i32 RedstoneWireBlock::getPower(const BlockState& state)
{
    return state.get(BlockStateProperties::POWER_0_15());
}

BlockState RedstoneWireBlock::withPower(BlockState state, i32 power)
{
    return state.with(BlockStateProperties::POWER_0_15(), std::clamp(power, 0, 15));
}

bool RedstoneWireBlock::isNormalCube(const BlockState& state)
{
    return state.isSolid() && state.isOpaque() && !state.isAir();
}

bool RedstoneWireBlock::canConnectTo(const BlockState& state)
{
    // 基础检查：如果方块可以输出红石信号，则可以连接
    return state.getBlock().canProvidePower(state);
}

bool RedstoneWireBlock::canConnectTo(const BlockState& state, Direction side)
{
    const Block& block = state.getBlock();

    // 红石线总是可以连接到其他红石线
    if (state.is(VanillaBlocks::REDSTONE_WIRE)) {
        return true;
    }

    // 检查中继器 - 只有朝向正确时才连接
    if (state.is(VanillaBlocks::REDSTONE_REPEATER) || state.is(VanillaBlocks::REDSTONE_COMPARATOR)) {
        Direction facing = RedstoneDiodeBlock::getFacing(state);
        // 中继器/比较器的输出端朝向我们时才连接
        return side == facing;
    }

    // 检查观察者 - 只有观察者的输出端朝向我们时才连接
    if (state.is(VanillaBlocks::OBSERVER)) {
        Direction facing = ObserverBlock::getFacing(state);
        // 观察者的输出端朝向我们时才连接
        return side == facing;
    }

    // 其他方块：检查 canProvidePower 和 canConnectRedstone
    if (block.canProvidePower(state)) {
        return true;
    }

    // 调用方块的 canConnectRedstone 方法
    return block.canConnectRedstone(state, side);
}

bool RedstoneWireBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    // IBlockReader 继承自 IWorld，可隐式向上转换为 IWorld& 传入 _canSurviveAt
    return _canSurviveAt(world, pos);
}

bool RedstoneWireBlock::_canSurviveAt(IWorld& world, const BlockPos& pos) const
{
    // 对齐 vanilla RedStoneWireBlock#canSurvive：检查下方方块是否为 solid top surface 或漏斗
    const BlockPos below = pos.down();
    const BlockState* belowState = world.getBlockState(below);
    if (belowState == nullptr) {
        return false;
    }
    // vanilla canSurviveOn: isFaceSturdy(world, below, Direction.UP, SupportType.CENTER)
    //                      || state.is(Blocks.HOPPER)
    return belowState->isFaceSturdy(world, below, Direction::Up, SupportType::Center) ||
        belowState->is(VanillaBlocks::HOPPER);
}

BlockState RedstoneWireBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 对齐 vanilla RedStoneWireBlock#updateShape(Direction.DOWN)：
    // 下方支撑方块变化时，若不再满足 canSurvive，则红石线变 AIR（由调用方移除并掉落）。
    if (facing == Direction::Down) {
        if (!_canSurviveAt(world, currentPos)) {
            if (const BlockState* air = BlockRegistry::instance().airState(); air != nullptr) {
                return *air;
            }
        }
        return state;
    }

    // 只有水平方向影响连接状态
    if (!Directions::isHorizontal(facing)) {
        return state;
    }

    // 计算新的连接状态
    BlockStateProperties::RedstoneSide connection = getConnection(world, currentPos, facing);

    // 根据方向设置连接属性
    switch (facing) {
        case Direction::North:
            return state.with(BlockStateProperties::REDSTONE_NORTH(), connection);
        case Direction::East:
            return state.with(BlockStateProperties::REDSTONE_EAST(), connection);
        case Direction::South:
            return state.with(BlockStateProperties::REDSTONE_SOUTH(), connection);
        case Direction::West:
            return state.with(BlockStateProperties::REDSTONE_WEST(), connection);
        default:
            return state;
    }
}

void RedstoneWireBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    // 对齐 vanilla RedStoneWireBlock#onPlace：放置即重算 power 与四方向连接形态（updatePower 内部
    // 无条件 calculateConnections，不依赖 power 是否变化）。红石线视觉连接（连红石线/电源元件/
    // 向上爬墙）在放置瞬间即正确，与信号有无无关。
    updatePower(world, pos);
}

void RedstoneWireBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    // 通知相邻方块更新
    _notifyWireNeighbors(world, pos);
}

void RedstoneWireBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 调度更新
    world.tickManager().scheduleBlockTick(pos, *this, 0, world::tick::TickPriority::High);
}

void RedstoneWireBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(state);
    MC_UNUSED(random);
    // 对齐 vanilla RedStoneWireBlock#neighborChanged→updatePowerStrength：邻居变化触发的延迟 tick
    // 无条件重算 power 与连接形态（updatePower 内部用 stateId 比较避免无谓写入）。原实现仅在
    // oldPower!=newPower 时重算连接，与 updatePower 旧版同缺陷（无信号场景连接不重算），现统一委托
    // updatePower，消除重复逻辑并修复 tick 路径的连接重算缺失。
    updatePower(world, pos);
}

i32 RedstoneWireBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 如果暂时禁用信号输出，返回0
    if (!m_canProvidePower) {
        return 0;
    }

    // 红石线不向下输出信号
    if (side == Direction::Down) {
        return 0;
    }

    i32 power = getPower(state);
    if (power == 0) {
        return 0;
    }

    // 对于向上方向：直接返回信号强度
    // 对于水平方向：只有该方向有连接时才输出信号

    if (side == Direction::Up) {
        // 向上方向：总是输出信号
        return power;
    }

    // 水平方向：需要检查连接
    BlockStateProperties::RedstoneSide connection = BlockStateProperties::RedstoneSide::None;
    switch (side) {
        case Direction::North:
            connection = state.get(BlockStateProperties::REDSTONE_NORTH());
            break;
        case Direction::East:
            connection = state.get(BlockStateProperties::REDSTONE_EAST());
            break;
        case Direction::South:
            connection = state.get(BlockStateProperties::REDSTONE_SOUTH());
            break;
        case Direction::West:
            connection = state.get(BlockStateProperties::REDSTONE_WEST());
            break;
        default:
            return 0;
    }

    // 只有该方向有连接时才输出信号
    if (connection == BlockStateProperties::RedstoneSide::None) {
        return 0;
    }

    return power;
}

i32 RedstoneWireBlock::getStrongPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    // 红石线的 getStrongPower 委托给 getWeakPower
    // 这使得红石线可以充能相邻的实体方块
    return getWeakPower(state, world, pos, side);
}

bool RedstoneWireBlock::updatePower(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (!state || !state->is(this)) {
        return false;
    }

    i32 newPower = _calculateInputPower(world, pos, *state);

    // 对齐 vanilla RedStoneWireBlock#onPlace/neighborChanged→updatePowerStrength：无条件重算 power
    // 与四方向连接形态，不依赖 power 是否变化。原实现仅在 oldPower!=newPower 时才 calculateConnections
    // 重算连接，导致无信号场景（如连拉杆/按钮/红石火把等未激活电源元件）下连接保持 defaultState
    // (四方向 none)，与 vanilla「放置/邻居变化即自动调整形状」（wiki tech_红石粉.txt#形状 :174-177）
    // 不一致。红石线视觉连接判定（shouldConnectTo）与电源是否激活无关，仅看相邻方块是否为红石线/
    // canProvidePower 元件。故这里分离 power 更新与连接重算：连接恒重算，写回与否用整体 stateId 比较
    // （power 或任一方向连接变化才 setBlockState + 通知邻居），避免无谓写入。
    BlockState newState = withPower(*state, newPower);
    newState = calculateConnections(world, pos, newState);

    if (newState != *state) {
        world.setBlockState(pos, &newState, 2);

        // 通知相邻红石线更新
        _notifyWireNeighbors(world, pos);
        return true;
    }

    return false;
}

BlockState RedstoneWireBlock::calculateConnections(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    BlockState result = state;

    // 计算四个方向的连接状态
    result = result.with(BlockStateProperties::REDSTONE_NORTH(), getConnection(world, pos, Direction::North));
    result = result.with(BlockStateProperties::REDSTONE_EAST(), getConnection(world, pos, Direction::East));
    result = result.with(BlockStateProperties::REDSTONE_SOUTH(), getConnection(world, pos, Direction::South));
    result = result.with(BlockStateProperties::REDSTONE_WEST(), getConnection(world, pos, Direction::West));

    return result;
}

BlockStateProperties::RedstoneSide RedstoneWireBlock::getConnection(
    IWorld& world, const BlockPos& pos, Direction direction) const
{
    BlockPos neighborPos = pos.offset(direction);
    const BlockState* neighborState = world.getBlockState(neighborPos);

    if (!neighborState || neighborState->isAir()) {
        return BlockStateProperties::RedstoneSide::None;
    }

    // canConnectTo 检查相邻方块是否可以连接红石
    // 参数 side 是从红石线指向相邻方块的方向
    if (canConnectTo(*neighborState, direction)) {
        return BlockStateProperties::RedstoneSide::Side;
    }

    // 检查向上连接
    // 如果相邻方块是实体方块，检查其上方是否有红石线
    if (isNormalCube(*neighborState)) {
        BlockPos upPos = neighborPos.up();
        const BlockState* upState = world.getBlockState(upPos);
        if (upState && upState->is(this)) {
            return BlockStateProperties::RedstoneSide::Up;
        }
    } else {
        // 相邻不是实体方块时，需要检查两种情况：
        // 1. 相邻方块下方是否有红石线
        // 2. 当前红石线位置下方是否有红石线（用于向上爬墙的情况）

        // 检查相邻方块下方是否有红石线
        BlockPos neighborDownPos = neighborPos.down();
        const BlockState* neighborDownState = world.getBlockState(neighborDownPos);
        if (neighborDownState && neighborDownState->is(this)) {
            return BlockStateProperties::RedstoneSide::Side;
        }

        // 还需要检查当前红石线下方的方块位置
        // 当红石线在悬崖边时，可以向下连接到低一格的红石线
        BlockPos downPos = pos.down();
        const BlockState* downState = world.getBlockState(downPos);
        if (downState && !downState->isAir()) {
            // 检查下方方块是否是实体方块，以及其实体方块旁边是否有红石线
            if (!isNormalCube(*downState)) {
                // 当前红石线下方不是实体方块，检查该位置周围的红石线
                BlockPos downNeighborPos = downPos.offset(direction);
                const BlockState* downNeighborState = world.getBlockState(downNeighborPos);
                if (downNeighborState && downNeighborState->is(this)) {
                    return BlockStateProperties::RedstoneSide::Side;
                }
            }
        }
    }

    return BlockStateProperties::RedstoneSide::None;
}

i32 RedstoneWireBlock::_calculateInputPower(IWorld& world, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(state);

    i32 maxPower = 0;

    // 防止循环依赖
    bool prevCanProvidePower = m_canProvidePower;
    m_canProvidePower = false;

    // 1. 从相邻方块获取强信号
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);

        if (!neighborState || neighborState->isAir()) {
            continue;
        }

        const Block& neighborBlock = neighborState->getBlock();

        // 获取强信号
        if (neighborBlock.canProvidePower(*neighborState)) {
            Direction oppositeDir = Directions::opposite(dir);
            i32 strongPower = neighborBlock.getStrongPower(*neighborState, world, neighborPos, oppositeDir);
            if (strongPower > maxPower) {
                maxPower = strongPower;
            }
        }
    }

    // 2. 从相邻红石线获取信号（衰减1）
    if (maxPower < 15) {
        for (Direction dir : Directions::horizontal()) {
            BlockPos neighborPos = pos.offset(dir);
            const BlockState* neighborState = world.getBlockState(neighborPos);

            if (!neighborState) {
                continue;
            }

            // 检查是否是红石线
            if (neighborState->is(this)) {
                i32 wirePower = getPower(*neighborState) - 1;
                if (wirePower > maxPower) {
                    maxPower = wirePower;
                }
            }

            // 检查向上连接
            if (isNormalCube(*neighborState)) {
                BlockPos upPos = neighborPos.up();
                const BlockState* upState = world.getBlockState(upPos);
                if (upState && upState->is(this)) {
                    i32 wirePower = getPower(*upState) - 1;
                    if (wirePower > maxPower) {
                        maxPower = wirePower;
                    }
                }
            } else {
                // 检查向下连接
                BlockPos downPos = neighborPos.down();
                const BlockState* downState = world.getBlockState(downPos);
                if (downState && downState->is(this)) {
                    i32 wirePower = getPower(*downState) - 1;
                    if (wirePower > maxPower) {
                        maxPower = wirePower;
                    }
                }
            }
        }
    }

    m_canProvidePower = prevCanProvidePower;
    return maxPower;
}

i32 RedstoneWireBlock::_getWirePower(IWorld& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state || !state->is(this)) {
        return 0;
    }
    return getPower(*state);
}

void RedstoneWireBlock::_notifyWireNeighbors(IWorld& world, const BlockPos& pos)
{
    // 通知六个方向的相邻方块
    for (Direction dir : Directions::all()) {
        BlockPos neighborPos = pos.offset(dir);
        const BlockState* neighborState = world.getBlockState(neighborPos);

        if (neighborState && !neighborState->isAir()) {
            Block& neighborBlock = neighborState->getBlockMutable();
            neighborBlock.neighborChanged(world, neighborPos, *this, pos, false);
        }
    }

    // 更新相邻红石线的信号
    updatePower(world, pos);
}

BlockActionResult RedstoneWireBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 冒险/旁观模式下无建造权限时，禁止切换红石线连接模式
    if (!player.mayBuild()) {
        return ActionResultType::Pass;
    }

    // 右键点击可以在十字连接和点状连接之间切换
    // 检查是否是十字连接或点状连接模式
    bool isCross = _isCrossConnection(state);
    bool isDot = _isDotConnection(state);

    if (isCross || isDot) {
        // 切换模式：十字 -> 点状，点状 -> 十字
        BlockState newState = isCross ? _createDotState(state) : _createCrossState(state);
        newState = calculateConnections(world, pos, newState);

        if (newState != state) {
            world.setBlockState(pos, &newState, 3);

            // 通知对角邻居更新
            _notifyDiagonalNeighbors(world, pos, state, newState);
            return ActionResultType::Success;
        }
    }

    return ActionResultType::Pass;
}

bool RedstoneWireBlock::_isCrossConnection(const BlockState& state) const
{
    // 检查四个方向是否都有连接
    return state.get(BlockStateProperties::REDSTONE_NORTH()) != BlockStateProperties::RedstoneSide::None &&
        state.get(BlockStateProperties::REDSTONE_SOUTH()) != BlockStateProperties::RedstoneSide::None &&
        state.get(BlockStateProperties::REDSTONE_EAST()) != BlockStateProperties::RedstoneSide::None &&
        state.get(BlockStateProperties::REDSTONE_WEST()) != BlockStateProperties::RedstoneSide::None;
}

bool RedstoneWireBlock::_isDotConnection(const BlockState& state) const
{
    // 检查四个方向是否都没有连接
    return state.get(BlockStateProperties::REDSTONE_NORTH()) == BlockStateProperties::RedstoneSide::None &&
        state.get(BlockStateProperties::REDSTONE_SOUTH()) == BlockStateProperties::RedstoneSide::None &&
        state.get(BlockStateProperties::REDSTONE_EAST()) == BlockStateProperties::RedstoneSide::None &&
        state.get(BlockStateProperties::REDSTONE_WEST()) == BlockStateProperties::RedstoneSide::None;
}

BlockState RedstoneWireBlock::_createDotState(const BlockState& state) const
{
    // 创建点状连接状态（所有方向都无连接）
    return state.with(BlockStateProperties::REDSTONE_NORTH(), BlockStateProperties::RedstoneSide::None)
        .with(BlockStateProperties::REDSTONE_SOUTH(), BlockStateProperties::RedstoneSide::None)
        .with(BlockStateProperties::REDSTONE_EAST(), BlockStateProperties::RedstoneSide::None)
        .with(BlockStateProperties::REDSTONE_WEST(), BlockStateProperties::RedstoneSide::None);
}

BlockState RedstoneWireBlock::_createCrossState(const BlockState& state) const
{
    // 创建十字连接状态（所有方向都有 Side 连接）
    return state.with(BlockStateProperties::REDSTONE_NORTH(), BlockStateProperties::RedstoneSide::Side)
        .with(BlockStateProperties::REDSTONE_SOUTH(), BlockStateProperties::RedstoneSide::Side)
        .with(BlockStateProperties::REDSTONE_EAST(), BlockStateProperties::RedstoneSide::Side)
        .with(BlockStateProperties::REDSTONE_WEST(), BlockStateProperties::RedstoneSide::Side);
}

void RedstoneWireBlock::_notifyDiagonalNeighbors(
    IWorld& world, const BlockPos& pos, const BlockState& oldState, const BlockState& newState)
{
    // 当连接状态改变时，通知对角方向的方块更新
    for (Direction dir : Directions::horizontal()) {
        BlockStateProperties::RedstoneSide oldConnection = BlockStateProperties::RedstoneSide::None;
        BlockStateProperties::RedstoneSide newConnection = BlockStateProperties::RedstoneSide::None;

        switch (dir) {
            case Direction::North:
                oldConnection = oldState.get(BlockStateProperties::REDSTONE_NORTH());
                newConnection = newState.get(BlockStateProperties::REDSTONE_NORTH());
                break;
            case Direction::South:
                oldConnection = oldState.get(BlockStateProperties::REDSTONE_SOUTH());
                newConnection = newState.get(BlockStateProperties::REDSTONE_SOUTH());
                break;
            case Direction::East:
                oldConnection = oldState.get(BlockStateProperties::REDSTONE_EAST());
                newConnection = newState.get(BlockStateProperties::REDSTONE_EAST());
                break;
            case Direction::West:
                oldConnection = oldState.get(BlockStateProperties::REDSTONE_WEST());
                newConnection = newState.get(BlockStateProperties::REDSTONE_WEST());
                break;
            default:
                break;
        }

        // 如果连接状态发生变化，通知对角邻居
        bool oldIsConnected = (oldConnection != BlockStateProperties::RedstoneSide::None);
        bool newIsConnected = (newConnection != BlockStateProperties::RedstoneSide::None);

        if (oldIsConnected != newIsConnected) {
            BlockPos neighborPos = pos.offset(dir);

            // 通知对角方向的方块
            BlockPos diagDownPos = neighborPos.down();
            const BlockState* diagDownState = world.getBlockState(diagDownPos);
            if (diagDownState && !diagDownState->isAir()) {
                Block& diagBlock = diagDownState->getBlockMutable();
                diagBlock.neighborChanged(world, diagDownPos, *this, pos, false);
            }

            BlockPos diagUpPos = neighborPos.up();
            const BlockState* diagUpState = world.getBlockState(diagUpPos);
            if (diagUpState && !diagUpState->isAir()) {
                Block& diagBlock = diagUpState->getBlockMutable();
                diagBlock.neighborChanged(world, diagUpPos, *this, pos, false);
            }
        }
    }
}

const CollisionShape& RedstoneWireBlock::getShape(const BlockState& state) const
{
    // 使用POWER=0的状态作为形状缓存的键（形状不依赖于POWER）
    u32 cacheKey = 0;
    cacheKey |= (static_cast<u32>(state.get(BlockStateProperties::REDSTONE_NORTH())) << 0);
    cacheKey |= (static_cast<u32>(state.get(BlockStateProperties::REDSTONE_EAST())) << 2);
    cacheKey |= (static_cast<u32>(state.get(BlockStateProperties::REDSTONE_SOUTH())) << 4);
    cacheKey |= (static_cast<u32>(state.get(BlockStateProperties::REDSTONE_WEST())) << 6);

    auto it = m_shapeCache.find(cacheKey);
    if (it != m_shapeCache.end()) {
        return it->second;
    }

    // 计算并缓存形状
    CollisionShape shape = _computeShapeForState(state);
    m_shapeCache[cacheKey] = shape;
    return m_shapeCache[cacheKey];
}

CollisionShape RedstoneWireBlock::_computeShapeForState(const BlockState& state) const
{
    // 从中心点开始，根据各方向的连接状态添加形状
    CollisionShape shape = s_centerShape;

    // 北面连接
    BlockStateProperties::RedstoneSide north = state.get(BlockStateProperties::REDSTONE_NORTH());
    if (north == BlockStateProperties::RedstoneSide::Side) {
        shape = CollisionShape::combine(shape, s_northSideShape, CollisionShape::CombineOp::OR);
    } else if (north == BlockStateProperties::RedstoneSide::Up) {
        shape = CollisionShape::combine(shape, s_northAscendingShape, CollisionShape::CombineOp::OR);
    }

    // 南面连接
    BlockStateProperties::RedstoneSide south = state.get(BlockStateProperties::REDSTONE_SOUTH());
    if (south == BlockStateProperties::RedstoneSide::Side) {
        shape = CollisionShape::combine(shape, s_southSideShape, CollisionShape::CombineOp::OR);
    } else if (south == BlockStateProperties::RedstoneSide::Up) {
        shape = CollisionShape::combine(shape, s_southAscendingShape, CollisionShape::CombineOp::OR);
    }

    // 东面连接
    BlockStateProperties::RedstoneSide east = state.get(BlockStateProperties::REDSTONE_EAST());
    if (east == BlockStateProperties::RedstoneSide::Side) {
        shape = CollisionShape::combine(shape, s_eastSideShape, CollisionShape::CombineOp::OR);
    } else if (east == BlockStateProperties::RedstoneSide::Up) {
        shape = CollisionShape::combine(shape, s_eastAscendingShape, CollisionShape::CombineOp::OR);
    }

    // 西面连接
    BlockStateProperties::RedstoneSide west = state.get(BlockStateProperties::REDSTONE_WEST());
    if (west == BlockStateProperties::RedstoneSide::Side) {
        shape = CollisionShape::combine(shape, s_westSideShape, CollisionShape::CombineOp::OR);
    } else if (west == BlockStateProperties::RedstoneSide::Up) {
        shape = CollisionShape::combine(shape, s_westAscendingShape, CollisionShape::CombineOp::OR);
    }

    return shape;
}

} // namespace blocks
} // namespace mc
