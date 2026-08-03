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

#include "DoublePlantBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/EnumProperty.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/agricultural/BushBlock.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// 使用 BlockStateProperties 中的 DoubleBlockHalf
using DoubleBlockHalf = BlockStateProperties::DoubleBlockHalf;

// ========== 构造函数 ==========

DoublePlantBlock::DoublePlantBlock(const BlockProperties& properties)
    : BushBlock(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::DOUBLE_BLOCK_HALF())
            .create([this](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Lower));

    // 形状：下半部分是完整方块高度，上半部分也是完整高度
    // 但植物通常没有碰撞
    m_lowerShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    m_upperShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
    m_shape = m_lowerShape;
}

// ========== 状态属性 ==========

const EnumProperty<BlockStateProperties::DoubleBlockHalf>& DoublePlantBlock::halfProperty()
{
    return BlockStateProperties::DOUBLE_BLOCK_HALF();
}

DoubleBlockHalf DoublePlantBlock::getHalf(const BlockState& state) const
{
    return state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());
}

BlockState DoublePlantBlock::withHalf(DoubleBlockHalf half) const
{
    return defaultState().with(BlockStateProperties::DOUBLE_BLOCK_HALF(), half);
}

// ========== 放置逻辑 ==========

BlockState DoublePlantBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockPos pos = context.placementPos();
    const IWorld& world = context.getWorld();

    // 检查是否有足够空间放置两格
    const BlockPos abovePos = pos.up();
    const BlockState* aboveState = world.getBlockState(abovePos);

    if (aboveState == nullptr || aboveState->isAir()) {
        return defaultState().with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Lower);
    }

    // 无法放置上半部分，返回默认状态（取消放置由外部处理）
    return defaultState();
}

bool DoublePlantBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    DoubleBlockHalf half = getHalf(state);

    if (half == DoubleBlockHalf::Upper) {
        // 上半部分必须在下半部分之上
        const BlockPos belowPos = pos.down();
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr) {
            return false;
        }
        // 检查下方是否为同类型的下半部分
        return belowState->is(this) && getHalf(*belowState) == DoubleBlockHalf::Lower;
    } else {
        // 下半部分使用基类的放置检查
        return BushBlock::isValidPosition(state, world, pos);
    }
}

BlockState DoublePlantBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingPos);

    DoubleBlockHalf half = getHalf(state);

    // 仅处理 Y 轴方向的邻居变化
    if (Directions::getAxis(facing) == Axis::Y) {
        bool isLower = half == DoubleBlockHalf::Lower;
        bool isUpDirection = facing == Direction::Up;

        // 当变化来自连接另一半的方向时（下半部分→上方，上半部分→下方）
        if (isLower == isUpDirection) {
            // 如果邻居仍然是同类型方块的另一半，保持当前状态
            if (facingState.is(this) && getHalf(facingState) != half) {
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

// ========== 形状 ==========

const CollisionShape& DoublePlantBlock::getShape(const BlockState& state) const
{
    DoubleBlockHalf half = getHalf(state);
    return (half == DoubleBlockHalf::Upper) ? m_upperShape : m_lowerShape;
}

// ========== 静态方法 ==========

bool DoublePlantBlock::placeAt(IWorld& world, const BlockPos& pos, const BlockState& state, i32 flags)
{
    // 放置下半部分
    if (!world.setBlockState(pos, &state, flags)) {
        return false;
    }

    // 放置上半部分
    BlockPos abovePos = pos.up();
    BlockState upperState = state.with(BlockStateProperties::DOUBLE_BLOCK_HALF(), DoubleBlockHalf::Upper);
    return world.setBlockState(abovePos, &upperState, flags);
}

} // namespace blocks
} // namespace mc
