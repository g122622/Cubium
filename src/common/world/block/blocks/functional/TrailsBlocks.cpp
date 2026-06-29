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

#include "TrailsBlocks.hpp"
#include "item/context/BlockItemUseContext.hpp"
#include "util/math/random/IRandom.hpp"
#include "util/property/Properties.hpp"
#include "world/IWorld.hpp"
#include "world/block/WaterLoggableHelpers.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include "world/blockentity/interactive/DecoratedPotBlockEntity.hpp"

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
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
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
    if (state.get(BlockStateProperties::SLOT_5_OCCUPIED()))
        lastOccupied = 5;
    else if (state.get(BlockStateProperties::SLOT_4_OCCUPIED()))
        lastOccupied = 4;
    else if (state.get(BlockStateProperties::SLOT_3_OCCUPIED()))
        lastOccupied = 3;
    else if (state.get(BlockStateProperties::SLOT_2_OCCUPIED()))
        lastOccupied = 2;
    else if (state.get(BlockStateProperties::SLOT_1_OCCUPIED()))
        lastOccupied = 1;
    else if (state.get(BlockStateProperties::SLOT_0_OCCUPIED()))
        lastOccupied = 0;

    return lastOccupied + 1;
}

// ============================================================================
// DecoratedPotBlock
// ============================================================================

DecoratedPotBlock::DecoratedPotBlock(const BlockProperties& properties)
    : HorizontalBlock(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(FACING())
            .add(BlockStateProperties::CRACKED())
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
            .with(FACING(), Direction::North)
            .with(BlockStateProperties::CRACKED(), false)
            .with(BlockStateProperties::WATERLOGGED(), false));

    // 饰纹陶罐形状: 缩小为圆柱
    m_shape = CollisionShape::fromPixelBox(1, 0, 1, 15, 16, 15);
}

void DecoratedPotBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState DecoratedPotBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state = defaultState()
                           .with(FACING(), Directions::opposite(context.horizontalDirection()))
                           .with(BlockStateProperties::CRACKED(), false)
                           .with(BlockStateProperties::WATERLOGGED(), false);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState DecoratedPotBlock::updatePostPlacement(const BlockState& state,
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

const CollisionShape& DecoratedPotBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const fluid::FluidState* DecoratedPotBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

std::unique_ptr<BlockEntity> DecoratedPotBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::DecoratedPotBlockEntity>(pos);
}

i32 DecoratedPotBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(state);
    BlockEntity* be = world.getBlockEntity(pos);
    if (be != nullptr && be->getType() == BlockEntityType::DecoratedPot) {
        auto* potEntity = static_cast<blockentity::DecoratedPotBlockEntity*>(be);
        return potEntity->getComparatorSignal();
    }
    return 0;
}

// ============================================================================
// BrushableBlock
// ============================================================================

BrushableBlock::BrushableBlock(const BlockProperties& properties)
    : FallingBlock(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::DUSTED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::DUSTED(), 0));
}

void BrushableBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

// ============================================================================
// SnifferEggBlock
// ============================================================================

SnifferEggBlock::SnifferEggBlock(const BlockProperties& properties)
    : Block(properties)
{
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::HATCH_0_2())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    setDefaultState(defaultState().with(BlockStateProperties::HATCH_0_2(), 0));

    m_noCrackShape = CollisionShape::fromPixelBox(1, 0, 2, 15, 16, 14);
    m_crackedShape = CollisionShape::fromPixelBox(1, 0, 2, 15, 16, 14);
    m_hatchingShape = CollisionShape::fromPixelBox(1, 0, 2, 15, 16, 14);
}

void SnifferEggBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState SnifferEggBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    return defaultState().with(BlockStateProperties::HATCH_0_2(), 0);
}

const CollisionShape& SnifferEggBlock::getShape(const BlockState& state) const
{
    i32 hatch = state.get(BlockStateProperties::HATCH_0_2());
    if (hatch == 2) {
        return m_hatchingShape;
    }
    if (hatch == 1) {
        return m_crackedShape;
    }
    return m_noCrackShape;
}

void SnifferEggBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    i32 hatch = state.get(BlockStateProperties::HATCH_0_2());
    if (hatch < 2) {
        // 增加孵化进度
        world.setBlockState(pos, &state.with(BlockStateProperties::HATCH_0_2(), hatch + 1), 3);
    } else {
        // 孵化完成 - 生成嗅探兽实体
        // TODO: 当实体系统完善后实现嗅探兽生成
    }
}

} // namespace blocks
} // namespace mc
