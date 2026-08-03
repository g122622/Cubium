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

#include "ConduitBlock.hpp"
#include "common/core/Types.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/WaterLoggableHelpers.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/core/BlockEntityRegistry.hpp"
#include "common/world/blockentity/processing/ConduitEntity.hpp"
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace mc {
namespace blocks {

// ============================================================================
// ConduitBlock 实现
// ============================================================================

namespace {
/// 潮涌核心的碰撞箱形状 (5x5x5 到 11x11x11)
static const CollisionShape CONDUIT_SHAPE = CollisionShape::box(5.0f, 5.0f, 5.0f, 11.0f, 11.0f, 11.0f);
} // namespace

ConduitBlock::ConduitBlock(BlockProperties properties)
    : Block(std::move(properties))
{
    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
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
    setDefaultState(defaultState().with(BlockStateProperties::WATERLOGGED(), true));
}

bool ConduitBlock::isWaterlogged(const BlockState& state) const
{
    return state.get(BlockStateProperties::WATERLOGGED());
}

BlockState ConduitBlock::getStateForPlacement(BlockItemUseContext& context)
{
    // 检查放置位置是否在水中
    // 默认含水
    return defaultState().with(BlockStateProperties::WATERLOGGED(), true);
}

void ConduitBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 创建方块实体
    auto& registry = blockentity::BlockEntityRegistry::instance();
    auto blockEntity = registry.create(BlockEntityType::Conduit, pos);
    if (blockEntity != nullptr) {
        // 设置世界引用并存储方块实体
        // 注意：setBlockEntity 会接管所有权并设置世界引用
        world.setBlockEntity(pos, blockEntity.release());
    }
}

void ConduitBlock::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 移除方块实体
    world.removeBlockEntity(pos);
}

BlockState ConduitBlock::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    MC_UNUSED(facingState);
    MC_UNUSED(facingPos);

    // 如果含水，调度流体tick
    if (state.get(BlockStateProperties::WATERLOGGED())) {
        waterloggable::scheduleWaterTick(world, currentPos);
    }

    // 注意：潮涌核心的激活状态检测在 ConduitEntity::tick() 中自动完成
    // 当方块更新时不需要手动触发重新检测

    return state;
}

const CollisionShape& ConduitBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return CONDUIT_SHAPE;
}

} // namespace blocks
} // namespace mc
