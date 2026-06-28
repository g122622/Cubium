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

#include "SculkBlocks.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "util/property/Properties.hpp"
#include "world/block/WaterLoggableHelpers.hpp"
#include "world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/sculk/SculkSensorBlockEntity.hpp"
#include "world/blockentity/sculk/SculkShriekerBlockEntity.hpp"
#include "world/redstone/RedstoneSystem.hpp"

namespace mc {
namespace blocks {

// ============================================================================
// SculkSensorBlock
// ============================================================================

SculkSensorBlock::SculkSensorBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::SCULK_SENSOR_PHASE())
            .add(BlockStateProperties::POWER_0_15())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Inactive)
            .with(BlockStateProperties::POWER_0_15(), 0)
            .with(BlockStateProperties::WATERLOGGED(), false));

    m_shape = CollisionShape::fromPixelBox(0, 0, 0, 16, 8, 16);
}

void SculkSensorBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 属性已在构造函数中通过 Builder 添加
    MC_UNUSED(container);
}

BlockState SculkSensorBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state =
        defaultState()
            .with(BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Inactive)
            .with(BlockStateProperties::POWER_0_15(), 0)
            .with(BlockStateProperties::WATERLOGGED(), false);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState SculkSensorBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

i32 SculkSensorBlock::getWeakPower(
    const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const noexcept
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(side);
    return state.get(BlockStateProperties::POWER_0_15());
}

i32 SculkSensorBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return state.get(BlockStateProperties::POWER_0_15());
}

const CollisionShape& SculkSensorBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const BlockState& SculkSensorBlock::rotate(const BlockState& state, Rotation rotation) const
{
    MC_UNUSED(rotation);
    return state;
}

const BlockState& SculkSensorBlock::mirror(const BlockState& state, Mirror mirror) const
{
    MC_UNUSED(mirror);
    return state;
}

const fluid::FluidState* SculkSensorBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

std::unique_ptr<BlockEntity> SculkSensorBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::SculkSensorBlockEntity>(pos);
}

BlockEntityType SculkSensorBlock::getBlockEntityType() const
{
    return BlockEntityType::SculkSensor;
}

// ============================================================================
// CalibratedSculkSensorBlock
// ============================================================================

CalibratedSculkSensorBlock::CalibratedSculkSensorBlock(const BlockProperties& properties)
    : SculkSensorBlock(properties)
{
    // 校准幽匿感测体额外添加 HORIZONTAL_FACING 属性
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::SCULK_SENSOR_PHASE())
            .add(BlockStateProperties::POWER_0_15())
            .add(BlockStateProperties::WATERLOGGED())
            .add(BlockStateProperties::HORIZONTAL_FACING())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::SCULK_SENSOR_PHASE(), BlockStateProperties::SculkSensorPhase::Inactive)
            .with(BlockStateProperties::POWER_0_15(), 0)
            .with(BlockStateProperties::WATERLOGGED(), false)
            .with(BlockStateProperties::HORIZONTAL_FACING(), Direction::North));
}

void CalibratedSculkSensorBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState CalibratedSculkSensorBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state = SculkSensorBlock::getStateForPlacement(context);
    state = state.with(BlockStateProperties::HORIZONTAL_FACING(), Directions::opposite(context.horizontalDirection()));
    return state;
}

const BlockState& CalibratedSculkSensorBlock::rotate(const BlockState& state, Rotation rotation) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), Directions::rotateDirection(facing, rotation));
}

const BlockState& CalibratedSculkSensorBlock::mirror(const BlockState& state, Mirror mirror) const
{
    Direction facing = state.get(BlockStateProperties::HORIZONTAL_FACING());
    Rotation rot = Directions::mirrorToRotation(mirror, facing);
    return state.with(BlockStateProperties::HORIZONTAL_FACING(), Directions::rotateDirection(facing, rot));
}

// ============================================================================
// SculkCatalystBlock
// ============================================================================

SculkCatalystBlock::SculkCatalystBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::BLOOM())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::BLOOM(), false));

    m_shape = CollisionShape::fullBlock();
}

void SculkCatalystBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

const CollisionShape& SculkCatalystBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

i32 SculkCatalystBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return state.get(BlockStateProperties::BLOOM()) ? 15 : 0;
}

// ============================================================================
// SculkShriekerBlock
// ============================================================================

SculkShriekerBlock::SculkShriekerBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::SHRIEKING())
            .add(BlockStateProperties::CAN_SUMMON())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState()
            .with(BlockStateProperties::SHRIEKING(), false)
            .with(BlockStateProperties::CAN_SUMMON(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));

    m_shape = CollisionShape::fromPixelBox(0, 0, 0, 16, 8, 16);
}

void SculkShriekerBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState SculkShriekerBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state = defaultState()
                           .with(BlockStateProperties::SHRIEKING(), false)
                           .with(BlockStateProperties::CAN_SUMMON(), false)
                           .with(BlockStateProperties::WATERLOGGED(), false);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState SculkShriekerBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    return state;
}

const CollisionShape& SculkShriekerBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const fluid::FluidState* SculkShriekerBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

std::unique_ptr<BlockEntity> SculkShriekerBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::SculkShriekerBlockEntity>(pos);
}

BlockEntityType SculkShriekerBlock::getBlockEntityType() const
{
    return BlockEntityType::SculkShrieker;
}

} // namespace blocks
} // namespace mc
