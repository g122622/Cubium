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

#include "CopperBulbBlock.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../IWorld.hpp"
#include "../../../redstone/RedstoneSystem.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/copper/WeatheringCopperBlock.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== CopperBulbBlock ==========

CopperBulbBlock::CopperBulbBlock(const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel)
    : WeatheringCopperBlock(properties, oxidationLevel)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::LIT())
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::OXIDATION())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::LIT(), false)
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::OXIDATION(), oxidationLevel));
}

BlockState CopperBulbBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    bool isPowered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, currentPos);
    bool wasPowered = state.get(BlockStateProperties::POWERED());

    if (isPowered != wasPowered) {
        // 红石信号变化时：上升沿切换LIT，下降沿不切换
        if (isPowered) {
            bool isLit = state.get(BlockStateProperties::LIT());
            return state.with(BlockStateProperties::POWERED(), true).with(BlockStateProperties::LIT(), !isLit);
        }
        return state.with(BlockStateProperties::POWERED(), false);
    }

    return state;
}

void CopperBulbBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 状态已在构造函数中通过 Builder 创建
}

bool CopperBulbBlock::hasComparatorInputOverride(const BlockState& state) const noexcept
{
    MC_UNUSED(state);
    return true;
}

i32 CopperBulbBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return state.get(BlockStateProperties::LIT()) ? 15 : 0;
}

// ========== WaxedCopperBulbBlock ==========

WaxedCopperBulbBlock::WaxedCopperBulbBlock(const BlockProperties& properties)
    : WaxedCopperBlock(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::LIT())
            .add(BlockStateProperties::POWERED())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(
        defaultState().with(BlockStateProperties::LIT(), false).with(BlockStateProperties::POWERED(), false));
}

BlockState WaxedCopperBulbBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    bool isPowered = world::redstone::RedstoneSystem::instance().isBlockPowered(world, currentPos);
    bool wasPowered = state.get(BlockStateProperties::POWERED());

    if (isPowered != wasPowered) {
        if (isPowered) {
            bool isLit = state.get(BlockStateProperties::LIT());
            return state.with(BlockStateProperties::POWERED(), true).with(BlockStateProperties::LIT(), !isLit);
        }
        return state.with(BlockStateProperties::POWERED(), false);
    }

    return state;
}

void WaxedCopperBulbBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 状态已在构造函数中通过 Builder 创建
}

bool WaxedCopperBulbBlock::hasComparatorInputOverride(const BlockState& state) const noexcept
{
    MC_UNUSED(state);
    return true;
}

i32 WaxedCopperBulbBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return state.get(BlockStateProperties::LIT()) ? 15 : 0;
}

} // namespace blocks
} // namespace mc
