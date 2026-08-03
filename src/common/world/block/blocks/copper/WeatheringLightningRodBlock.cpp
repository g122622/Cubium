/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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

#include "WeatheringLightningRodBlock.hpp"

#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/block/blocks/LightningRodBlock.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== WeatheringLightningRodBlock ==========

WeatheringLightningRodBlock::WeatheringLightningRodBlock(
    const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel)
    : LightningRodBlock(properties)
    , m_oxidationLevel(oxidationLevel)
{
    // LightningRodBlock 构造函数已创建 FACING + POWERED + WATERLOGGED 状态容器，
    // 但作为可氧化方块需要添加 OXIDATION 属性，因此重建状态容器。
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(BlockStateProperties::POWERED())
            .add(BlockStateProperties::WATERLOGGED())
            .add(BlockStateProperties::OXIDATION())
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
            .with(BlockStateProperties::POWERED(), false)
            .with(BlockStateProperties::WATERLOGGED(), false)
            .with(BlockStateProperties::OXIDATION(), oxidationLevel));

    // 仅在非 Oxidized 等级时启用随机 tick（氧化完成后不需要继续 tick）
    if (oxidationLevel != BlockStateProperties::OxidationLevel::Oxidized) {
        m_ticksRandomly = true;
    }
}

void WeatheringLightningRodBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 属性已在构造函数中通过 Builder 添加
    MC_UNUSED(container);
}

void WeatheringLightningRodBlock::randomTick(
    IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 委托给 IOxidizableBlock::tryOxidize() 执行氧化算法
    (void)tryOxidize(world, pos, state, random);
}

bool WeatheringLightningRodBlock::ticksRandomly() const noexcept
{
    // 仅在非 Oxidized 等级时需要随机 tick
    return m_oxidationLevel != BlockStateProperties::OxidationLevel::Oxidized;
}

BlockState WeatheringLightningRodBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 与 LightningRodBlock 相同的放置逻辑，额外设置 OXIDATION 属性
    auto directions = context.getNearestLookingDirections();
    Direction facing = directions.empty() ? Direction::Up : directions.front();

    BlockState state = defaultState().with(FACING(), facing).with(BlockStateProperties::POWERED(), false);

    // 处理含水
    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    } else {
        state = state.with(BlockStateProperties::WATERLOGGED(), false);
    }

    // 设置氧化等级
    state = state.with(BlockStateProperties::OXIDATION(), m_oxidationLevel);

    return state;
}

// ========== WaxedLightningRodBlock ==========

WaxedLightningRodBlock::WaxedLightningRodBlock(const BlockProperties& properties)
    : LightningRodBlock(properties)
{
    // 涂蜡避雷针与普通避雷针具有完全相同的方块状态属性
    // （FACING, POWERED, WATERLOGGED），不需要 OXIDATION 属性。
    // LightningRodBlock 构造函数已创建这些属性。
    // 不需要额外操作。
}

} // namespace blocks
} // namespace mc
