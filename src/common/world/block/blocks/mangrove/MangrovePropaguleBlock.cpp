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
 * IMPLIED, BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "MangrovePropaguleBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/base/types/PlantType.hpp"
#include "common/world/block/capability/WaterLoggableHelpers.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/MangroveBlocks.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

static constexpr i32 MAX_AGE = 4;

MangrovePropaguleBlock::MangrovePropaguleBlock(const BlockProperties& properties)
    : Block(properties)
{
    m_ticksRandomly = true;
    // 形状必须在 createBlockState() 之前填好：状态构造期会虚调用 getOpacity →
    // propagatesSkylightDown → getOcclusionShape → getShape，形状未就绪时会被缓存/冻结。
    //
    // SHAPE_PER_AGE[age] = column(2.0, SHAPE_MIN_Y[age], 16.0)：
    // 横截面 **2 像素见方**（x/z 7..9），从 SHAPE_MIN_Y[age] 一直延伸到方块顶部。
    // SHAPE_MIN_Y = {13, 10, 7, 3, 0}，即年龄越大向下延伸越多；非悬挂态固定取 age=4 的一档。
    static constexpr i32 SHAPE_MIN_Y[MAX_AGE + 1] = {13, 10, 7, 3, 0};
    for (i32 age = 0; age <= MAX_AGE; age++) {
        m_shapes.push_back(
            CollisionShape::fromPixelBox(7.0f, static_cast<f32>(SHAPE_MIN_Y[age]), 7.0f, 9.0f, 16.0f, 9.0f));
    }

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_4())
            .add(BlockStateProperties::HANGING())
            .add(BlockStateProperties::STAGE_0_1())
            .add(BlockStateProperties::WATERLOGGED())
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
            .with(BlockStateProperties::AGE_0_4(), 0)
            .with(BlockStateProperties::HANGING(), false)
            .with(BlockStateProperties::STAGE_0_1(), 0)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

void MangrovePropaguleBlock::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    MC_UNUSED(container);
}

BlockState MangrovePropaguleBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockState state =
        defaultState().with(BlockStateProperties::AGE_0_4(), 0).with(BlockStateProperties::HANGING(), false);

    if (waterloggable::shouldWaterlogAt(context.getWorld(), context.placementPos())) {
        state = state.with(BlockStateProperties::WATERLOGGED(), true);
    }

    return state;
}

BlockState MangrovePropaguleBlock::updatePostPlacement(const BlockState& state,
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

const CollisionShape& MangrovePropaguleBlock::getShape(const BlockState& state) const
{
    // 原版：i = HANGING ? AGE : 4，取 SHAPE_PER_AGE[i]。即只有悬挂态按年龄递增、越挂越饱满，
    // 非悬挂态固定取最粗的一档（age=4，column(2, 0, 16)）。
    const i32 age = state.get(BlockStateProperties::HANGING()) ? state.get(BlockStateProperties::AGE_0_4()) : MAX_AGE;
    MC_ASSERT_RELEASE(age >= 0 && age <= MAX_AGE);
    return m_shapes[static_cast<size_t>(age)];
}

CollisionShape MangrovePropaguleBlock::getShapeAt(const BlockState& state, const BlockPos& pos) const
{
    // 原版 getShape 对结果显式 move(getOffset(pos))；本方块注册时带 `offsetType(XZ)`，
    // 偏移按坐标哈希水平抖动（上限取默认 0.25）。
    const Vector3 offset = getOffsetAt(state, pos);
    return getShape(state).offset(offset.x, offset.y, offset.z);
}

const fluid::FluidState* MangrovePropaguleBlock::getFluidState(const BlockState& state) const
{
    return waterloggable::getWaterFluidState(state);
}

void MangrovePropaguleBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    if (state.get(BlockStateProperties::HANGING())) {
        i32 age = state.get(BlockStateProperties::AGE_0_4());
        if (age < MAX_AGE) {
            auto newState = state.with(BlockStateProperties::AGE_0_4(), age + 1);
            world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_ALL);
        }
    }
}

bool MangrovePropaguleBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 原版 MangrovePropaguleBlock.canSurvive：悬挂态只认上方的红树树叶，
    // 非悬挂态沿用 SaplingBlock（VegetationBlock）的「下方为 DIRT 标签或耕地」。
    if (state.get(BlockStateProperties::HANGING())) {
        const BlockPos abovePos(pos.x, pos.y + 1, pos.z);
        const BlockState* aboveState = world.getBlockState(abovePos);
        return aboveState != nullptr && block_registry::MangroveBlocks::MANGROVE_LEAVES != nullptr &&
            aboveState->is(block_registry::MangroveBlocks::MANGROVE_LEAVES);
    }

    const BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);
    if (belowState == nullptr) {
        return false;
    }

    // 原版 MangrovePropaguleBlock.mayPlaceOn 在 VegetationBlock 的「DIRT 标签 / 耕地」之外
    // 额外接受黏土。
    return BlockTags::DIRT().contains(*belowState) ||
        (VanillaBlocks::FARMLAND != nullptr && belowState->is(VanillaBlocks::FARMLAND)) ||
        (VanillaBlocks::CLAY != nullptr && belowState->is(VanillaBlocks::CLAY));
}

// ========== IPlantable 接口实现 ==========

PlantType MangrovePropaguleBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // MC 1.21.11: MangrovePropaguleBlock 继承 BushBlock
    // 红树胎生苗可种植在泥土类方块、砂土、灰化土、苔藓块、耕地和黏土上
    // 使用 PlantType::Beach 因为种植面包含泥土和沙子
    return PlantType::Beach;
}

const BlockState& MangrovePropaguleBlock::getPlant(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    return defaultState();
}

} // namespace blocks
} // namespace mc
