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

#include "WeepingVinesPlantBlock.hpp"

#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

WeepingVinesPlantBlock::WeepingVinesPlantBlock(const BlockProperties& properties)
    : GrowingPlantBodyBlock(properties,
          Direction::Down,                                   // 向下生长
          CollisionShape::fromPixelBox(1, 0, 1, 15, 16, 15), // MC column(14,0,16)
          false)                                             // 茎段不调度流体 tick
{
    auto container = StateContainer<Block, BlockState>::Builder(*this).create(
        [](const Block& block,
            StateValueIndices valueIndices,
            size_t propertyCount,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) {
            return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
        });
    createBlockState(std::move(container));
}

const Block* WeepingVinesPlantBlock::getHeadBlock() const
{
    return VanillaBlocks::WEEPING_VINES;
}

const Block* WeepingVinesPlantBlock::getBodyBlock() const
{
    return VanillaBlocks::WEEPING_VINES_PLANT;
}

} // namespace blocks
} // namespace mc
