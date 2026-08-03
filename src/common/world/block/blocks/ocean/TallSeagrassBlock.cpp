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

#include "TallSeagrassBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../IWorld.hpp"
#include "../../BlockRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/PlantType.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

using DoubleBlockHalf = BlockStateProperties::DoubleBlockHalf;

TallSeagrassBlock::TallSeagrassBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::DOUBLE_BLOCK_HALF())
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
    setDefaultState(defaultState()
            .with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Lower)
            .with(BlockStateProperties::WATERLOGGED(), true));

    // 形状
    m_lowerShape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 1.0f, 0.875f);
    m_upperShape = CollisionShape::box(0.125f, 0.0f, 0.125f, 0.875f, 1.0f, 0.875f);
}

BlockStateProperties::DoubleBlockHalf TallSeagrassBlock::getHalf(const BlockState& state) const
{
    return state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());
}

BlockState TallSeagrassBlock::getStateForPlacement(BlockItemUseContext& context)
{
    const IWorld& world = context.getWorld();
    BlockPos pos = context.placementPos();

    // 检查上方是否有空间
    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState == nullptr || aboveState->isAir()) {
        return defaultState().with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Lower);
    }

    return defaultState();
}

bool TallSeagrassBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    auto half = getHalf(state);

    if (half == DoubleBlockHalf::Upper) {
        // 上半部分需要在下半部分之上
        BlockPos belowPos(pos.x, pos.y - 1, pos.z);
        const BlockState* belowState = world.getBlockState(belowPos);
        return belowState != nullptr && belowState->is(this) &&
            belowState->get(BlockStateProperties::DOUBLE_BLOCK_HALF()) == DoubleBlockHalf::Lower;
    } else {
        // 下半部分需要支撑
        BlockPos belowPos(pos.x, pos.y - 1, pos.z);
        const BlockState* belowState = world.getBlockState(belowPos);
        return belowState != nullptr && belowState->isSolid();
    }
}

BlockState TallSeagrassBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingPos);

    auto half = getHalf(state);

    // 仅处理 Y 轴方向的邻居变化
    if (Directions::getAxis(facing) == Axis::Y) {
        bool isLower = half == DoubleBlockHalf::Lower;
        bool isUpDirection = facing == Direction::Up;

        // 当变化来自连接另一半的方向时（下半部分→上方，上半部分→下方）
        if (isLower == isUpDirection) {
            // 如果邻居仍然是同类型方块的另一半，保持当前状态
            if (facingState.is(this) && facingState.get(BlockStateProperties::DOUBLE_BLOCK_HALF()) != half) {
                return state;
            }
            // 另一半已消失，当前半部分也应消失
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    // 下半部分额外检查：如果下方支撑方块不再有效，也应消失
    if (half == DoubleBlockHalf::Lower && facing == Direction::Down) {
        if (!isValidPosition(state, static_cast<IBlockReader&>(world), currentPos)) {
            if (auto* airState = BlockRegistry::instance().airState()) {
                return *airState;
            }
        }
    }

    return state;
}

const CollisionShape& TallSeagrassBlock::getShape(const BlockState& state) const
{
    auto half = getHalf(state);
    return (half == DoubleBlockHalf::Upper) ? m_upperShape : m_lowerShape;
}

const CollisionShape& TallSeagrassBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

// ========== IPlantable 接口实现 ==========

PlantType TallSeagrassBlock::getPlantType(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return PlantType::Water;
}

const BlockState& TallSeagrassBlock::getPlant(IBlockReader& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return defaultState();
}

} // namespace blocks
} // namespace mc
