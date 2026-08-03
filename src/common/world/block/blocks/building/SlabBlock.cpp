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

#include "SlabBlock.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../item/core/Item.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/assert/AssertAll.hpp"
#include "../../../IWorld.hpp"
#include "../../WaterLoggableHelpers.hpp"
#include "common/core/Types.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/Block.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

SlabBlock::SlabBlock(const BlockProperties& properties)
    : Block(properties)
    , m_bottomShape(CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f))
    , m_topShape(CollisionShape::box(0.0f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f))
    , m_fullCubeShape(CollisionShape::fullBlock())
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::SLAB_TYPE())
            .add(BlockStateProperties::WATERLOGGED())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState()
            .with(BlockStateProperties::SLAB_TYPE(), BlockStateProperties::SlabType::Bottom)
            .with(BlockStateProperties::WATERLOGGED(), false));
}

// ========== 放置和更新 ==========

BlockState SlabBlock::getStateForPlacement(BlockItemUseContext& context)
{
    BlockPos pos = context.placementPos();
    const BlockState* existingState = context.getWorld().getBlockState(pos);

    // 如果点击位置已有同类型台阶，变成双层
    if (existingState != nullptr && &existingState->getBlock() == this) {
        return existingState->with(BlockStateProperties::SLAB_TYPE(), BlockStateProperties::SlabType::Double)
            .with(BlockStateProperties::WATERLOGGED(), false);
    }

    // 检查是否含水
    bool waterlogged = waterloggable::shouldWaterlogAt(context.getWorld(), pos);

    // 根据点击位置决定上半/下半
    Direction clickedFace = context.getClickedFace();
    bool isTop = clickedFace == Direction::Down || (clickedFace != Direction::Up && context.getHitY() > 0.5f);

    return defaultState()
        .with(BlockStateProperties::SLAB_TYPE(),
            isTop ? BlockStateProperties::SlabType::Top : BlockStateProperties::SlabType::Bottom)
        .with(BlockStateProperties::WATERLOGGED(), waterlogged);
}

bool SlabBlock::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{

    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 台阶可以放置在任何位置（除非需要特殊支撑）
    return true;
}

bool SlabBlock::isReplaceable(const BlockState& state, const BlockItemUseContext& context) const
{

    // 参考: net.minecraft.block.SlabBlock#isReplaceable
    // 只有单层台阶可以被替换为双层台阶
    BlockStateProperties::SlabType slabType = state.get(BlockStateProperties::SLAB_TYPE());
    if (slabType == BlockStateProperties::SlabType::Double) {
        // 双层台阶不可替换
        return false;
    }

    // 获取玩家手中的物品
    const ItemStack& stack = context.itemStack();
    const Item* item = stack.getItem();
    if (item == nullptr) {
        return false;
    }

    // 检查物品是否为同类型台阶对应的物品
    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(*this);
    if (blockItem == nullptr || item != blockItem) {
        // 不是同类型台阶
        return false;
    }

    // 玩家点击的是这个方块
    if (context.replacingClickedBlock()) {
        // 根据点击位置和台阶类型决定是否可以替换
        // hitY 是相对于方块底部的 Y 坐标（0-1范围）
        f32 hitY = context.getHitY();
        Direction clickedFace = context.getClickedFace();

        if (slabType == BlockStateProperties::SlabType::Bottom) {
            // 底部台阶：可以从上方点击，或从侧面点击上半部分
            return clickedFace == Direction::Up ||
                (clickedFace != Direction::Up && clickedFace != Direction::Down && hitY > 0.5f);
        } else {
            // 顶部台阶：可以从下方点击，或从侧面点击下半部分
            return clickedFace == Direction::Down ||
                (clickedFace != Direction::Up && clickedFace != Direction::Down && hitY <= 0.5f);
        }
    } else {
        // 不是替换点击的方块（可能是相邻放置），允许替换
        return true;
    }
}

BlockState SlabBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{

    MC_UNUSED(facing);
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 单层台阶含水时调度流体 tick
    if (state.get(BlockStateProperties::SLAB_TYPE()) != BlockStateProperties::SlabType::Double) {
        if (state.get(BlockStateProperties::WATERLOGGED())) {
            waterloggable::scheduleWaterTick(world, currentPos);
        }
    }

    return state;
}

// ========== 形状 ==========

const CollisionShape& SlabBlock::getShape(const BlockState& state) const
{
    BlockStateProperties::SlabType type = state.get(BlockStateProperties::SLAB_TYPE());

    switch (type) {
        case BlockStateProperties::SlabType::Bottom:
            return m_bottomShape;
        case BlockStateProperties::SlabType::Top:
            return m_topShape;
        case BlockStateProperties::SlabType::Double:
        default:
            return m_fullCubeShape;
    }
}

const CollisionShape& SlabBlock::getCollisionShape(const BlockState& state) const
{
    return getShape(state);
}

// ========== 静态方法 ==========

bool SlabBlock::isDouble(const BlockState& state)
{
    return state.get(BlockStateProperties::SLAB_TYPE()) == BlockStateProperties::SlabType::Double;
}

// ========== IWaterLoggable 接口实现 ==========

const fluid::FluidState* SlabBlock::getFluidState(const BlockState& state) const
{
    // 双层台阶不能含水
    if (state.get(BlockStateProperties::SLAB_TYPE()) == BlockStateProperties::SlabType::Double) {
        return Block::getFluidState(state);
    }

    const fluid::FluidState* waterState = waterloggable::getWaterFluidState(state);
    return waterState != nullptr ? waterState : Block::getFluidState(state);
}

} // namespace blocks
} // namespace mc
