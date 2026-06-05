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

#include "RailBlock.hpp"

namespace mc {
namespace blocks {

RailBlock::RailBlock(const BlockProperties& properties)
    : AbstractRailBlock(properties, false)
{
    // 创建状态容器
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(SHAPE()).create(
        [this](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(SHAPE(), RailShape::NorthSouth));
}

void RailBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 状态容器在构造函数中创建，此方法留空
    MC_UNUSED(container);
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
