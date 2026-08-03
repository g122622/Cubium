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

#include "EnchantingTableBlock.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/EnchantingTableEntity.hpp"
#include "common/world/tick/base/TickPriority.hpp"
#include "common/world/tick/manager/TickManager.hpp"
#include <memory>

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

EnchantingTableBlock::EnchantingTableBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 附魔台形状：桌面略大于基座
    // 底座: (1, 0, 1) -> (15, 2, 15)
    // 桌面: (0, 2, 0) -> (16, 14, 16)

    CollisionShape base =
        VoxelShapes::cube(1.0f / 16.0f, 0.0f, 1.0f / 16.0f, 15.0f / 16.0f, 2.0f / 16.0f, 15.0f / 16.0f);

    CollisionShape top = VoxelShapes::cube(0.0f, 2.0f / 16.0f, 0.0f, 1.0f, 14.0f / 16.0f, 1.0f);

    m_shape = CollisionShape::combine(base, top, CollisionShape::CombineOp::OR);
}

// ========== 方块实体 ==========

std::unique_ptr<BlockEntity> EnchantingTableBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::EnchantingTableEntity>(pos);
}

// ========== 交互 ==========

BlockActionResult EnchantingTableBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(state);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 客户端直接返回成功
    if (world.asServerWorld() == nullptr) {
        return ActionResultType::Success;
    }

    // 获取方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::EnchantingTable) {
        return ActionResultType::Pass;
    }

    // 打开附魔台GUI
    if (world.openContainer(ContainerType::Enchantment, pos, player)) {
        return ActionResultType::Consume;
    }

    return ActionResultType::Pass;
}

// ========== 形状 ==========

const CollisionShape& EnchantingTableBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& EnchantingTableBlock::getOcclusionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    // 附魔台不阻挡光线（用于渲染）
    return VoxelShapes::empty();
}

// ========== 放置和更新 ==========

void EnchantingTableBlock::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);

    // 附魔台放置时调度一个tick来重新计算附魔力量
    // 原因：onBlockAdded在方块实体创建之前被调用，此时getBlockEntity返回nullptr
    // 延迟1tick后方块实体已创建，可以安全地重新计算
    world.tickManager().scheduleBlockTick(pos, *this, 1, world::tick::TickPriority::Normal);
}

void EnchantingTableBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(isMoving);

    // 当邻居方块变化时，重新计算附魔力量
    // ServerWorld::setBlockState只通知直接邻居（1格距离），书架可在2格距离内。
    // 但BookshelfBlock::onBlockAdded/onBlockRemoved会主动通知2格范围内的附魔台，
    // 加上中间方块（空气/可替换方块）的变化也会触发此neighborChanged，
    // 因此所有影响附魔力量的方块变化场景均已覆盖。
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::EnchantingTable) {
        auto* enchantingTable = static_cast<blockentity::EnchantingTableEntity*>(blockEntity);
        enchantingTable->recalculateEnchantPower(world);
    }
}

void EnchantingTableBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(state);
    MC_UNUSED(random);

    // 延迟tick：方块实体已创建，重新计算附魔力量
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::EnchantingTable) {
        auto* enchantingTable = static_cast<blockentity::EnchantingTableEntity*>(blockEntity);
        enchantingTable->recalculateEnchantPower(world);
    }
}

} // namespace blocks
} // namespace mc
