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

#include "ActivatorRailBlock.hpp"
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
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

ActivatorRailBlock::ActivatorRailBlock(const BlockProperties& properties)
    : AbstractRailBlock(properties, true, false) // isStraight=true: 激活铁轨不支持弯轨, isPowered=false: 不提供红石信号
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

void ActivatorRailBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 状态容器在构造函数中创建，此方法留空
    MC_UNUSED(container);
}

void ActivatorRailBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 检查红石信号并更新状态
    const BlockState* currentState = world.getBlockState(pos);
    if (!currentState) return;

    // 激活铁轨只需要检查直接红石信号
    bool shouldBePowered = world::redstone::RedstonePower::isPowered(world, pos);

    bool isCurrentlyPowered = isPowered(*currentState);
    if (shouldBePowered != isCurrentlyPowered) {
        BlockState newState = currentState->with(POWERED(), shouldBePowered);
        world.setBlockState(pos.x, pos.y, pos.z, &newState, 3);
    }
}

RailShape ActivatorRailBlock::getRailShape(const BlockState& state) const
{
    return state.get(SHAPE());
}

BlockState ActivatorRailBlock::withRailShape(const BlockState& state, RailShape shape) const
{
    return state.with(SHAPE(), shape);
}

bool ActivatorRailBlock::isPowered(const BlockState& state)
{
    return state.get(POWERED());
}

} // namespace blocks
} // namespace mc
