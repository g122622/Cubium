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
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include "common/world/block/PlantType.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
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
    // 原版实测：**所有 age / stage 的形状都相同**，都是
    //   box(0.1875, 0, 0.1875 -> 0.3125, 1, 0.3125)
    // 即横截面只有 2 像素见方、高度贯穿整格。形状不随生长阶段变化。
    //
    // 此前按 age 递增高度（4..12 像素）、宽度 12 像素，两个维度都不对。
    const CollisionShape propaguleShape = CollisionShape::box(0.1875f, 0.0f, 0.1875f, 0.3125f, 1.0f, 0.3125f);
    for (i32 age = 0; age <= MAX_AGE; age++) {
        m_shapes.push_back(propaguleShape);
    }
    // 悬挂状态用同一形状
    m_hangingShape = propaguleShape;

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
    // 悬挂状态下 AGE 决定胚根的**长度**（顶部固定贴在天花板上、向下伸长）：
    //
    //   hanging=true, age=0 -> y 0.8125..1.0    (13/16 .. 1)
    //   hanging=true, age=1 -> y 0.6250..1.0
    //   hanging=true, age=2 -> y 0.4375..1.0
    //   hanging=true, age=3 -> y 0.1875..1.0
    //   hanging=true, age=4 -> y 0.0000..1.0
    //
    // 非悬挂状态 AGE 不影响形状（恒为 y 0..1 的细柱）。
    //
    // 此前悬挂状态无条件返回与地面状态相同的细柱，胚胎看起来像已经长到底了——
    // 刚挂上去的胚根和成熟的一样长。
    if (state.get(BlockStateProperties::HANGING())) {
        static const std::array<CollisionShape, 5> hangingShapes = {
            CollisionShape::box(0.1875f, 0.8125f, 0.1875f, 0.3125f, 1.0f, 0.3125f),
            CollisionShape::box(0.1875f, 0.6250f, 0.1875f, 0.3125f, 1.0f, 0.3125f),
            CollisionShape::box(0.1875f, 0.4375f, 0.1875f, 0.3125f, 1.0f, 0.3125f),
            CollisionShape::box(0.1875f, 0.1875f, 0.1875f, 0.3125f, 1.0f, 0.3125f),
            CollisionShape::box(0.1875f, 0.0000f, 0.1875f, 0.3125f, 1.0f, 0.3125f),
        };
        const i32 hangingAge = state.get(BlockStateProperties::AGE_0_4());
        MC_ASSERT(hangingAge >= 0 && hangingAge < static_cast<i32>(hangingShapes.size()));
        return hangingShapes[static_cast<size_t>(hangingAge)];
    }

    i32 age = state.get(BlockStateProperties::AGE_0_4());
    if (age >= 0 && age < static_cast<i32>(m_shapes.size())) {
        return m_shapes[age];
    }
    return m_shapes[0];
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
