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
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"

namespace mc {
namespace blocks {

static constexpr int MAX_AGE = 4;

MangrovePropaguleBlock::MangrovePropaguleBlock(const BlockProperties& properties)
    : Block(properties)
{
    m_ticksRandomly = true;
    // 非悬挂状态的碰撞形状（根据AGE变化高度）
    // AGE 0: 4像素高, AGE 1: 6像素, AGE 2: 8像素, AGE 3: 10像素, AGE 4: 12像素
    for (int age = 0; age <= MAX_AGE; age++) {
        int height = 4 + age * 2;
        m_shapes.push_back(CollisionShape::fromPixelBox(2, 0, 2, 14, height, 14));
    }
    // 悬挂状态的碰撞形状
    m_hangingShape = CollisionShape::fromPixelBox(2, 0, 2, 14, 12, 14);

    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::AGE_0_4())
            .add(BlockStateProperties::HANGING())
            .add(BlockStateProperties::STAGE_0_1())
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
    if (state.get(BlockStateProperties::HANGING())) {
        return m_hangingShape;
    }

    int age = state.get(BlockStateProperties::AGE_0_4());
    if (age >= 0 && age < static_cast<int>(m_shapes.size())) {
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
        int age = state.get(BlockStateProperties::AGE_0_4());
        if (age < MAX_AGE) {
            auto newState = state.with(BlockStateProperties::AGE_0_4(), age + 1);
            world.setBlockState(pos, &newState, 3);
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
