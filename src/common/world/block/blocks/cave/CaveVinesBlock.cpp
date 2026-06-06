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

#include "CaveVinesBlock.hpp"
#include "common/util/property/Properties.hpp"

namespace mc {
namespace blocks {

CaveVinesBlock::CaveVinesBlock(const BlockProperties& properties)
    : Block(properties)
    , m_shape(CollisionShape::fromPixelBox(1, 0, 1, 15, 16, 15))
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_25())
            .add(BlockStateProperties::BERRIES())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(
        defaultState().with(BlockStateProperties::AGE_0_25(), 0).with(BlockStateProperties::BERRIES(), false));
}

void CaveVinesBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

const CollisionShape& CaveVinesBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

void CaveVinesBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    MC_UNUSED(random);
    // TODO: 实现洞穴藤蔓生长逻辑
}

} // namespace blocks
} // namespace mc
