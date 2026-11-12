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
#include "common/world/block/blocks/trial/TrialSpawnerBlock.hpp"

#include "common/util/property/Properties.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/trial/TrialSpawnerBlockEntity.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// TrialSpawnerBlock
// ============================================================================

TrialSpawnerBlock::TrialSpawnerBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::TRIAL_SPAWNER_STATE())
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
            .with(BlockStateProperties::TRIAL_SPAWNER_STATE(), BlockStateProperties::TrialSpawnerState::Inactive)
            .with(BlockStateProperties::OMINOUS(), false));
}

std::unique_ptr<BlockEntity> TrialSpawnerBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<TrialSpawnerBlockEntity>(pos);
}

void TrialSpawnerBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState TrialSpawnerBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    return defaultState()
        .with(BlockStateProperties::TRIAL_SPAWNER_STATE(), BlockStateProperties::TrialSpawnerState::Inactive)
        .with(BlockStateProperties::OMINOUS(), false);
}

i32 TrialSpawnerBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    auto trialState = state.get(BlockStateProperties::TRIAL_SPAWNER_STATE());
    switch (trialState) {
        case BlockStateProperties::TrialSpawnerState::WaitingForPlayers:
            return 1;
        case BlockStateProperties::TrialSpawnerState::Active:
            return 2;
        case BlockStateProperties::TrialSpawnerState::WaitingForRewardEjection:
        case BlockStateProperties::TrialSpawnerState::EjectingReward:
            return 3;
        case BlockStateProperties::TrialSpawnerState::Cooldown:
            return 4;
        default:
            return 0;
    }
}

u8 TrialSpawnerBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 原版按状态分级：待机 4、其余非 inactive 状态 8、inactive 熄灯。
    // 由差分能力 block_light_emission 逐状态实测。
    switch (state.get(BlockStateProperties::TRIAL_SPAWNER_STATE())) {
        case BlockStateProperties::TrialSpawnerState::Inactive:
        case BlockStateProperties::TrialSpawnerState::Cooldown:
            return 0;
        case BlockStateProperties::TrialSpawnerState::WaitingForPlayers:
            return 4;
        default:
            return 8;
    }
}

} // namespace blocks
} // namespace mc
