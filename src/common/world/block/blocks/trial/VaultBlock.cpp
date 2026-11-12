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
#include "common/world/block/blocks/trial/VaultBlock.hpp"

#include "common/util/property/Properties.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/trial/VaultBlockEntity.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// VaultBlock
// ============================================================================

VaultBlock::VaultBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(BlockStateProperties::VAULT_STATE())
            .add(BlockStateProperties::OMINOUS())
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
            .with(BlockStateProperties::VAULT_STATE(), BlockStateProperties::VaultState::Inactive)
            .with(BlockStateProperties::OMINOUS(), false));
}

std::unique_ptr<BlockEntity> VaultBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<VaultBlockEntity>(pos);
}

void VaultBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState VaultBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState()
        .with(FACING(), Directions::opposite(context.horizontalDirection()))
        .with(BlockStateProperties::VAULT_STATE(), BlockStateProperties::VaultState::Inactive)
        .with(BlockStateProperties::OMINOUS(), false);
}

u8 VaultBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 原版按状态分级：未激活 6、激活后（active / unlocking / ejecting）12。
    // 由差分能力 block_light_emission 逐状态实测。
    return state.get(BlockStateProperties::VAULT_STATE()) == BlockStateProperties::VaultState::Inactive ? 6 : 12;
}

} // namespace blocks
} // namespace mc
