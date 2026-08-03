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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN AN EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "RailBlock.hpp"
#include "RailState.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/redstone/AbstractRailBlock.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

RailBlock::RailBlock(const BlockProperties& properties)
    : AbstractRailBlock(properties, false) // isStraight = false: 普通铁轨支持弯轨
{
    // 创建状态容器（含 SHAPE 和 WATERLOGGED 属性）
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(SHAPE())
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
    setDefaultState(
        defaultState().with(SHAPE(), RailShape::NorthSouth).with(BlockStateProperties::WATERLOGGED(), false));
}

void RailBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 状态容器在构造函数中创建，此方法留空
    MC_UNUSED(container);
}

void RailBlock::updateState(IWorld& world, const BlockPos& pos, const BlockState& state, Block& neighborBlock)
{
    // 普通铁轨的特殊行为：当邻居信号源变化且铁轨有三连接时，重新计算方向
    // 这是红石道岔（T型道岔）切换的核心逻辑
    if (neighborBlock.canProvidePower(neighborBlock.defaultState())) {
        RailState railState(world, pos, *this, state);
        if (railState.countPotentialConnections() == 3) {
            // 三连接 + 信号源变化 = 道岔切换
            (void)updateDir(world, pos, state, false);
        }
    }
}

RailShape RailBlock::getRailShape(const BlockState& state) const
{
    return state.get(SHAPE());
}

BlockState RailBlock::withRailShape(const BlockState& state, RailShape shape) const
{
    return state.with(SHAPE(), shape);
}

} // namespace blocks
} // namespace mc
