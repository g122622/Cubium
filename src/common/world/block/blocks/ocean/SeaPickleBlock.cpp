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

#include "SeaPickleBlock.hpp"
#include "../../../../entity/core/Entity.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../IWorld.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../BlockRegistry.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

using namespace mc; // Bring BlockStateProperties into scope

SeaPickleBlock::SeaPickleBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::PICKLES_1_4())
            .add(BlockStateProperties::WATERLOGGED())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(
        defaultState().with(BlockStateProperties::PICKLES_1_4(), 1).with(BlockStateProperties::WATERLOGGED(), true));

    // 创建各数量的形状
    // 1个：小型，2个：中型，3个：大型，4个：最大
    m_shapesByCount[0] = CollisionShape::box(0.375f, 0.0f, 0.375f, 0.625f, 0.3125f, 0.625f);
    m_shapesByCount[1] = CollisionShape::box(0.25f, 0.0f, 0.25f, 0.75f, 0.375f, 0.75f);
    m_shapesByCount[2] = CollisionShape::box(0.1875f, 0.0f, 0.1875f, 0.8125f, 0.4375f, 0.8125f);
    m_shapesByCount[3] = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 0.5f, 0.875f);
}

i32 SeaPickleBlock::getPickles(const BlockState& state) const
{
    return state.get(BlockStateProperties::PICKLES_1_4());
}

BlockState SeaPickleBlock::withPickles(i32 count) const
{
    return defaultState().with(BlockStateProperties::PICKLES_1_4(), std::clamp(count, 1, 4));
}

BlockState SeaPickleBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查是否在水中（海泡菜必须在水中）
    bool waterlogged = waterloggable::shouldWaterlogAt(world, pos);

    // 检查是否已有海泡菜（堆叠）
    const BlockState* existingState = world.getBlockState(pos);
    if (existingState != nullptr && existingState->is(this)) {
        // 增加数量
        i32 count = existingState->get(BlockStateProperties::PICKLES_1_4());
        if (count < 4) {
            return existingState->with(BlockStateProperties::PICKLES_1_4(), count + 1);
        }
        return *existingState;
    }

    return defaultState()
        .with(BlockStateProperties::PICKLES_1_4(), 1)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool SeaPickleBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);

    // 检查下方是否有支撑
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (belowState == nullptr) {
        return false;
    }

    // 需要固体支撑
    return belowState->isSolid();
}

BlockState SeaPickleBlock::updatePostPlacement(const BlockState& state,
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

    // 检查下方支撑
    if (facing == Direction::Down) {
        IBlockReader& blockReader = static_cast<IBlockReader&>(world);
        if (!isValidPosition(state, blockReader, currentPos)) {
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    return state;
}

u8 SeaPickleBlock::getLightLevel(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 在水中时发光，亮度随数量增加
    bool waterlogged = state.get(BlockStateProperties::WATERLOGGED());
    if (!waterlogged) {
        return 0;
    }

    i32 count = getPickles(state);
    // 1个: 6, 2个: 9, 3个: 12, 4个: 15
    return static_cast<u8>(3 + count * 3);
}

const CollisionShape& SeaPickleBlock::getShape(const BlockState& state) const
{
    i32 count = getPickles(state);
    return m_shapesByCount[std::clamp(count - 1, 0, 3)];
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* SeaPickleBlock::getFluidState(const BlockState& state) const
{
    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

} // namespace blocks
} // namespace mc
