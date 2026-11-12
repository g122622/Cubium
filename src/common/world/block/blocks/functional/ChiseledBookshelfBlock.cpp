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

#include "common/world/block/blocks/functional/ChiseledBookshelfBlock.hpp"

#include "common/util/property/Properties.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/redstone/RedstoneSystem.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// ChiseledBookshelfBlock
// ============================================================================

ChiseledBookshelfBlock::ChiseledBookshelfBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    // HorizontalBlock 已添加 HORIZONTAL_FACING，需额外添加 SLOT 占用属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(BlockStateProperties::SLOT_0_OCCUPIED())
            .add(BlockStateProperties::SLOT_1_OCCUPIED())
            .add(BlockStateProperties::SLOT_2_OCCUPIED())
            .add(BlockStateProperties::SLOT_3_OCCUPIED())
            .add(BlockStateProperties::SLOT_4_OCCUPIED())
            .add(BlockStateProperties::SLOT_5_OCCUPIED())
            .create([](const Block& block,
                        StateValueIndices valueIndices,
                        size_t propertyCount,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(FACING(), Direction::North)
            .with(BlockStateProperties::SLOT_0_OCCUPIED(), false)
            .with(BlockStateProperties::SLOT_1_OCCUPIED(), false)
            .with(BlockStateProperties::SLOT_2_OCCUPIED(), false)
            .with(BlockStateProperties::SLOT_3_OCCUPIED(), false)
            .with(BlockStateProperties::SLOT_4_OCCUPIED(), false)
            .with(BlockStateProperties::SLOT_5_OCCUPIED(), false));
}

void ChiseledBookshelfBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState ChiseledBookshelfBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState().with(FACING(), Directions::opposite(context.horizontalDirection()));
}

i32 ChiseledBookshelfBlock::getComparatorInputOverride(
    const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 比较器输出 = 最后一个被占用的槽位索引 + 1
    i32 lastOccupied = -1;
    if (state.get(BlockStateProperties::SLOT_5_OCCUPIED())) {
        lastOccupied = 5;
    } else if (state.get(BlockStateProperties::SLOT_4_OCCUPIED())) {
        lastOccupied = 4;
    } else if (state.get(BlockStateProperties::SLOT_3_OCCUPIED())) {
        lastOccupied = 3;
    } else if (state.get(BlockStateProperties::SLOT_2_OCCUPIED())) {
        lastOccupied = 2;
    } else if (state.get(BlockStateProperties::SLOT_1_OCCUPIED())) {
        lastOccupied = 1;
    } else if (state.get(BlockStateProperties::SLOT_0_OCCUPIED())) {
        lastOccupied = 0;
    }

    return lastOccupied + 1;
}

} // namespace blocks
} // namespace mc
