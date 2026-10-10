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

#include "PoweredRailBlock.hpp"

#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/blocks/redstone/AbstractRailBlock.hpp"
#include "common/world/redstone/RedstonePower.hpp"
#include <cstddef>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

PoweredRailBlock::PoweredRailBlock(const BlockProperties& properties)
    : AbstractRailBlock(properties, true, true) // isStraight=true: 动力铁轨不支持弯轨, isPowered=true: 可提供红石信号
{
    // 创建状态容器（含 SHAPE、POWERED 和 WATERLOGGED 属性）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(SHAPE())
            .add(POWERED())
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
    setDefaultState(defaultState()
            .with(SHAPE(), RailShape::NorthSouth)
            .with(POWERED(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

void PoweredRailBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 状态容器在构造函数中创建，此方法留空
    MC_UNUSED(container);
}

i32 PoweredRailBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 动力铁轨不输出红石信号
    return 0;
}

void PoweredRailBlock::updateState(IWorld& world, const BlockPos& pos, const BlockState& state, Block& neighborBlock)
{
    MC_UNUSED(neighborBlock);
    bool powered = world::redstone::RedstonePower::isPowered(world, pos) ||
        _findPoweredRailSignal(world, pos, state, true, 0) || _findPoweredRailSignal(world, pos, state, false, 0);
    if (powered != isPowered(state)) {
        BlockState newState = state.with(POWERED(), powered);
        world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_ALL);
        // 台阶两端可能相差一格，除本体邻居外还需更新上下位置。
        world.updateNeighbors(pos.down(), *this);
        if (getRailShape(state) != RailShape::NorthSouth && getRailShape(state) != RailShape::EastWest) {
            world.updateNeighbors(pos.up(), *this);
        }
    }
}

bool PoweredRailBlock::_findPoweredRailSignal(
    IWorld& world, const BlockPos& startPos, const BlockState& startState, bool checkForward, i32 distance) const
{
    if (distance >= 8) return false;
    BlockPos next = startPos;
    bool checkBelow = true;
    bool eastWest = false;
    switch (getRailShape(startState)) {
        case RailShape::NorthSouth:
            next.z += checkForward ? 1 : -1;
            break;
        case RailShape::EastWest:
            next.x += checkForward ? -1 : 1;
            eastWest = true;
            break;
        case RailShape::AscendingEast:
            next.x += checkForward ? -1 : 1;
            if (!checkForward) {
                ++next.y;
                checkBelow = false;
            }
            eastWest = true;
            break;
        case RailShape::AscendingWest:
            next.x += checkForward ? -1 : 1;
            if (checkForward) {
                ++next.y;
                checkBelow = false;
            }
            eastWest = true;
            break;
        case RailShape::AscendingNorth:
            next.z += checkForward ? 1 : -1;
            if (!checkForward) {
                ++next.y;
                checkBelow = false;
            }
            break;
        case RailShape::AscendingSouth:
            next.z += checkForward ? 1 : -1;
            if (checkForward) {
                ++next.y;
                checkBelow = false;
            }
            break;
        default:
            return false;
    }
    return _isSameRailWithPower(world, next, checkForward, distance, eastWest) ||
        (checkBelow && _isSameRailWithPower(world, next.down(), checkForward, distance, eastWest));
}

bool PoweredRailBlock::_isSameRailWithPower(
    IWorld& world, const BlockPos& pos, bool checkForward, i32 distance, bool eastWest) const
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || !state->is(this)) return false;
    RailShape shape = getRailShape(*state);
    bool otherEastWest =
        shape == RailShape::EastWest || shape == RailShape::AscendingEast || shape == RailShape::AscendingWest;
    if (eastWest != otherEastWest || !isPowered(*state)) return false;
    // 带电状态只允许继续追踪，不能当成独立电源，否则会无限传电并在断电后自锁。
    return world::redstone::RedstonePower::isPowered(world, pos) ||
        _findPoweredRailSignal(world, pos, *state, checkForward, distance + 1);
}
RailShape PoweredRailBlock::getRailShape(const BlockState& state) const
{
    return state.get(SHAPE());
}

BlockState PoweredRailBlock::withRailShape(const BlockState& state, RailShape shape) const
{
    return state.with(SHAPE(), shape);
}

bool PoweredRailBlock::isPowered(const BlockState& state)
{
    return state.get(POWERED());
}

} // namespace blocks
} // namespace mc
