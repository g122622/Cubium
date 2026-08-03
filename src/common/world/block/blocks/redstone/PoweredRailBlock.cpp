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
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
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

void PoweredRailBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* currentState = world.getBlockState(pos);
    if (!currentState) return;

    // 检查当前位置是否被红石信号充能
    // 动力铁轨可以接收直接信号或通过其他动力铁轨传导的信号
    bool shouldBePowered = world::redstone::RedstonePower::isPowered(world, pos);

    // 如果没有直接充能，尝试从相邻的动力铁轨获取信号
    if (!shouldBePowered) {
        shouldBePowered = _findPoweredRailSignal(world, pos, *currentState, true) ||
            _findPoweredRailSignal(world, pos, *currentState, false);
    }

    bool isCurrentlyPowered = isPowered(*currentState);
    if (shouldBePowered != isCurrentlyPowered) {
        BlockState newState = currentState->with(POWERED(), shouldBePowered);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 3);

        // 通知相邻方块更新
        world.updateNeighbors(pos, *this);
    }
}

bool PoweredRailBlock::_findPoweredRailSignal(
    IWorld& world, const BlockPos& startPos, const BlockState& startState, bool checkForward) const
{
    // 迭代搜索相连的动力铁轨，最大距离8格
    // 使用 visited 集合防止重复访问
    std::unordered_set<BlockPos> visited;

    BlockPos currentPos = startPos;
    RailShape currentShape = getRailShape(startState);

    for (i32 distance = 0; distance < 8; ++distance) {
        i32 x = currentPos.x;
        i32 y = currentPos.y;
        i32 z = currentPos.z;

        // 根据铁轨形状确定搜索方向
        switch (currentShape) {
            case RailShape::NorthSouth:
                z += checkForward ? 1 : -1;
                break;
            case RailShape::EastWest:
                x += checkForward ? -1 : 1;
                break;
            case RailShape::AscendingEast:
                if (checkForward) {
                    x -= 1;
                } else {
                    x += 1;
                    y += 1;
                }
                break;
            case RailShape::AscendingWest:
                if (checkForward) {
                    x -= 1;
                    y += 1;
                } else {
                    x += 1;
                }
                break;
            case RailShape::AscendingNorth:
                if (checkForward) {
                    z += 1;
                } else {
                    z -= 1;
                    y += 1;
                }
                break;
            case RailShape::AscendingSouth:
                if (checkForward) {
                    z += 1;
                    y += 1;
                } else {
                    z -= 1;
                }
                break;
            default:
                // 弯轨不支持动力铁轨的信号传导
                return false;
        }

        // 检查当前位置是否为动力铁轨
        BlockPos checkPos(x, y, z);

        // 检查是否已访问过此位置
        if (visited.count(checkPos) > 0) {
            return false; // 防止循环
        }
        visited.insert(checkPos);

        const BlockState* checkState = world.getBlockState(checkPos);
        if (!checkState || !checkState->is(this)) {
            // 检查下方一格（针对斜坡向下）
            BlockPos belowPos(x, y - 1, z);
            if (visited.count(belowPos) > 0) {
                return false;
            }
            visited.insert(belowPos);

            const BlockState* belowState = world.getBlockState(belowPos);
            if (!belowState || !belowState->is(this)) {
                return false;
            }
            checkPos = belowPos;
            checkState = belowState;
        }

        // 检查该动力铁轨是否充能
        if (isPowered(*checkState)) {
            return true;
        }

        // 继续沿同一方向搜索
        currentPos = checkPos;
        currentShape = getRailShape(*checkState);
    }

    return false;
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
