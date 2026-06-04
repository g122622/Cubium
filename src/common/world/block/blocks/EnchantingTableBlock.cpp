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

#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/blockentity/interactive/EnchantingTableEntity.hpp"

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

ActionResultType EnchantingTableBlock::onBlockActivated(const BlockState& state,
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

    // TODO: 当附魔台放置时，需要重新计算附魔力量
    // 当前 IWorld 接口无法直接调用附魔力量计算，需要在 World 中处理或扩展 IWorld 接口
}

} // namespace blocks
} // namespace mc
