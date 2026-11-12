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

#include "common/world/block/blocks/agricultural/JackOLanternBlock.hpp"

#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/blocks/agricultural/CarvedPumpkinBlock.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// JackOLanternBlock
// ============================================================================

JackOLanternBlock::JackOLanternBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    auto container = StateContainer<Block, BlockState>::Builder(*this).add(FACING()).create(
        [](const Block& block,
            StateValueIndices valueIndices,
            size_t propertyCount,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) {
            return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
        });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(FACING(), Direction::North));
}

BlockState JackOLanternBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState().with(FACING(), Directions::opposite(context.horizontalDirection()));
}

void JackOLanternBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
{
    MC_UNUSED(state);
    // 复用 CarvedPumpkinBlock 的傀儡生成逻辑
    CarvedPumpkinBlock::trySpawnGolem(world, pos);
}

} // namespace blocks
} // namespace mc
