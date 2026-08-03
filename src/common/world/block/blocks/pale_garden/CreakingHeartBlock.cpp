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

#include "CreakingHeartBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/RotatedPillarBlock.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

CreakingHeartBlock::CreakingHeartBlock(const BlockProperties& properties)
    : RotatedPillarBlock(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AXIS())
            .add(BlockStateProperties::CREAKING_HEART_STATE())
            .add(BlockStateProperties::NATURAL())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::CREAKING_HEART_STATE(), BlockStateProperties::CreakingHeartState::Uprooted)
            .with(BlockStateProperties::NATURAL(), false));
}

void CreakingHeartBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState CreakingHeartBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Axis axis = Axis::Y;
    Direction dir = context.horizontalDirection();
    if (dir == Direction::North || dir == Direction::South) {
        axis = Axis::Z;
    } else if (dir == Direction::East || dir == Direction::West) {
        axis = Axis::X;
    }

    return defaultState()
        .with(BlockStateProperties::AXIS(), axis)
        .with(BlockStateProperties::CREAKING_HEART_STATE(), BlockStateProperties::CreakingHeartState::Dormant)
        .with(BlockStateProperties::NATURAL(), true);
}

i32 CreakingHeartBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    auto creakingState = state.get(BlockStateProperties::CREAKING_HEART_STATE());
    switch (creakingState) {
        case BlockStateProperties::CreakingHeartState::Uprooted:
            return 0;
        case BlockStateProperties::CreakingHeartState::Dormant:
            return 1;
        case BlockStateProperties::CreakingHeartState::Awake:
            return 2;
        default:
            return 0;
    }
}

} // namespace blocks
} // namespace mc
