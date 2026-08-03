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

#include "WeatheringCopperLanternBlock.hpp"

#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/copper/WeatheringCopperBlock.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "util/Direction.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/IWorld.hpp"
#include "world/block/WaterLoggableHelpers.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== WeatheringCopperLanternBlock ==========

WeatheringCopperLanternBlock::WeatheringCopperLanternBlock(
    const BlockProperties& properties, BlockStateProperties::OxidationLevel oxidationLevel, u8 lightValue)
    : WeatheringCopperBlock(properties, oxidationLevel)
    , m_lightValue(lightValue)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HANGING())
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
            .with(BlockStateProperties::HANGING(), false)
            .with(BlockStateProperties::WATERLOGGED(), false)
            .with(BlockStateProperties::OXIDATION(), oxidationLevel));

    // 创建形状 - 与普通灯笼相同
    // 站立形状：底部到中部
    m_standingShape = CollisionShape::box(5.0f, 0.0f, 5.0f, 11.0f, 7.0f, 11.0f);
    // 悬挂形状：顶部悬挂
    m_hangingShape = CollisionShape::box(5.0f, 1.0f, 5.0f, 11.0f, 8.0f, 11.0f);
}

void WeatheringCopperLanternBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState WeatheringCopperLanternBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction clickedFace = context.getClickedFace();
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查是否含水
    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    // 如果点击的是天花板，尝试悬挂
    if (clickedFace == Direction::Down) {
        return defaultState()
            .with(BlockStateProperties::HANGING(), true)
            .with(BlockStateProperties::WATERLOGGED(), waterlogged)
            .with(BlockStateProperties::OXIDATION(), m_oxidationLevel);
    }

    // 默认站立
    return defaultState()
        .with(BlockStateProperties::HANGING(), false)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged)
        .with(BlockStateProperties::OXIDATION(), m_oxidationLevel);
}

bool WeatheringCopperLanternBlock::isValidPosition(
    const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 根据悬挂状态检查支撑方块
    bool hanging = state.get(BlockStateProperties::HANGING());
    // 悬挂时检查上方方块，站立时检查下方方块
    Direction supportDir = hanging ? Direction::Up : Direction::Down;
    BlockPos supportPos = pos.offset(supportDir);
    const BlockState* supportState = world.getBlockState(supportPos);

    if (supportState == nullptr || supportState->isAir()) {
        return false;
    }

    // 检查支撑面是否为足够坚固的实体面
    return supportState->isSolidSide(world, supportPos, Directions::opposite(supportDir));
}

BlockState WeatheringCopperLanternBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 处理含水状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 如果支撑方块被移除，则移除灯笼
    bool hanging = state.get(BlockStateProperties::HANGING());
    Direction supportDir = hanging ? Direction::Up : Direction::Down;

    if (facing == supportDir) {
        // 支撑方块发生变化，检查是否仍然有效
        BlockPos supportPos = currentPos.offset(supportDir);
        const BlockState* supportState = world.getBlockState(supportPos);

        if (supportState == nullptr || supportState->isAir() ||
            !supportState->isSolidSide(world, supportPos, Directions::opposite(supportDir))) {
            return VanillaBlocks::AIR->defaultState();
        }
    }

    return state;
}

const CollisionShape& WeatheringCopperLanternBlock::getShape(const BlockState& state) const
{
    bool hanging = state.get(BlockStateProperties::HANGING());
    return hanging ? m_hangingShape : m_standingShape;
}

const fluid::FluidState* WeatheringCopperLanternBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

// ========== WaxedCopperLanternBlock ==========

WaxedCopperLanternBlock::WaxedCopperLanternBlock(const BlockProperties& properties, u8 lightValue)
    : WaxedCopperBlock(properties)
    , m_lightValue(lightValue)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HANGING())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(
        defaultState().with(BlockStateProperties::HANGING(), false).with(BlockStateProperties::WATERLOGGED(), false));

    // 创建形状 - 与风化版本相同
    m_standingShape = CollisionShape::box(5.0f, 0.0f, 5.0f, 11.0f, 7.0f, 11.0f);
    m_hangingShape = CollisionShape::box(5.0f, 1.0f, 5.0f, 11.0f, 8.0f, 11.0f);
}

void WaxedCopperLanternBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState WaxedCopperLanternBlock::getStateForPlacement(BlockItemUseContext& context)
{
    Direction clickedFace = context.getClickedFace();
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查是否含水
    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    // 如果点击的是天花板，尝试悬挂
    if (clickedFace == Direction::Down) {
        return defaultState()
            .with(BlockStateProperties::HANGING(), true)
            .with(BlockStateProperties::WATERLOGGED(), waterlogged);
    }

    // 默认站立
    return defaultState()
        .with(BlockStateProperties::HANGING(), false)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool WaxedCopperLanternBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 根据悬挂状态检查支撑方块
    bool hanging = state.get(BlockStateProperties::HANGING());
    // 悬挂时检查上方方块，站立时检查下方方块
    Direction supportDir = hanging ? Direction::Up : Direction::Down;
    BlockPos supportPos = pos.offset(supportDir);
    const BlockState* supportState = world.getBlockState(supportPos);

    if (supportState == nullptr || supportState->isAir()) {
        return false;
    }

    // 检查支撑面是否为足够坚固的实体面
    return supportState->isSolidSide(world, supportPos, Directions::opposite(supportDir));
}

BlockState WaxedCopperLanternBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 处理含水状态
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 如果支撑方块被移除，则移除灯笼
    bool hanging = state.get(BlockStateProperties::HANGING());
    Direction supportDir = hanging ? Direction::Up : Direction::Down;

    if (facing == supportDir) {
        // 支撑方块发生变化，检查是否仍然有效
        BlockPos supportPos = currentPos.offset(supportDir);
        const BlockState* supportState = world.getBlockState(supportPos);

        if (supportState == nullptr || supportState->isAir() ||
            !supportState->isSolidSide(world, supportPos, Directions::opposite(supportDir))) {
            return VanillaBlocks::AIR->defaultState();
        }
    }

    return state;
}

const CollisionShape& WaxedCopperLanternBlock::getShape(const BlockState& state) const
{
    bool hanging = state.get(BlockStateProperties::HANGING());
    return hanging ? m_hangingShape : m_standingShape;
}

const fluid::FluidState* WaxedCopperLanternBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

} // namespace blocks
} // namespace mc
