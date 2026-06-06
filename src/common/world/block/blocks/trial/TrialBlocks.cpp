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

#include "TrialBlocks.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "util/property/Properties.hpp"
#include "world/redstone/RedstoneSystem.hpp"
#include "world/tick/manager/TickManager.hpp"

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
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::TRIAL_SPAWNER_STATE(), BlockStateProperties::TrialSpawnerState::Inactive)
            .with(BlockStateProperties::OMINOUS(), false));
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
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(FACING(), Direction::North)
            .with(BlockStateProperties::VAULT_STATE(), BlockStateProperties::VaultState::Inactive)
            .with(BlockStateProperties::OMINOUS(), false));
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

// ============================================================================
// CrafterBlock
// ============================================================================

CrafterBlock::CrafterBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(BlockStateProperties::TRIGGERED())
            .add(BlockStateProperties::CRAFTING())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(FACING(), Direction::North)
            .with(BlockStateProperties::TRIGGERED(), false)
            .with(BlockStateProperties::CRAFTING(), false));
}

void CrafterBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState CrafterBlock::getStateForPlacement(BlockItemUseContext& context)
{
    return defaultState()
        .with(FACING(), Directions::opposite(context.horizontalDirection()))
        .with(BlockStateProperties::TRIGGERED(), false)
        .with(BlockStateProperties::CRAFTING(), false);
}

void CrafterBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return;
    }

    bool isPowered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, pos);
    bool wasTriggered = state->get(BlockStateProperties::TRIGGERED());

    if (isPowered && !wasTriggered) {
        // 红石信号上升沿：开始合成
        BlockState newState =
            state->with(BlockStateProperties::TRIGGERED(), true).with(BlockStateProperties::CRAFTING(), true);
        world.setBlockState(pos, &newState, 3);
        // TODO: 安排合成tick（6 tick后完成）
    } else if (!isPowered && wasTriggered) {
        // 红石信号下降沿：重置触发状态
        BlockState newState = state->with(BlockStateProperties::TRIGGERED(), false);
        world.setBlockState(pos, &newState, 3);
    }
}

BlockState CrafterBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(world);
    MC_UNUSED(currentPos);
    MC_UNUSED(facingPos);
    return state;
}

i32 CrafterBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);

    // 合成完成时输出信号
    if (state.get(BlockStateProperties::CRAFTING())) {
        return 15;
    }
    return 0;
}

} // namespace blocks
} // namespace mc
