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

#include "EndGatewayBlock.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "world/IWorld.hpp"
#include "world/blockentity/interactive/EndGatewayEntity.hpp"
#include <memory>

namespace mc {
namespace blocks {

EndGatewayBlock::EndGatewayBlock(const BlockProperties& properties)
    : Block(properties)
{
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

std::unique_ptr<BlockEntity> EndGatewayBlock::createBlockEntity(const BlockPos& pos)
{
    return std::make_unique<blockentity::EndGatewayEntity>(pos);
}

void EndGatewayBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(state);

    // 检查实体是否可以传送
    // 检查实体是否是旁观者、是否有乘客/正在骑乘
    // 实际传送逻辑由 EndGatewayEntity::tick() 处理
    // 这里只需要标记实体在折跃门内，方块实体会在 tick 中检测并处理

    // 获取方块实体
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity == nullptr || blockEntity->getType() != BlockEntityType::EndGateway) {
        return;
    }

    auto* gatewayEntity = static_cast<blockentity::EndGatewayEntity*>(blockEntity);

    // 检查冷却状态 - 实体在冷却期间不会被传送
    if (gatewayEntity->isCoolingDown()) {
        return;
    }

    // 检查实体是否可以传送
    // - 不是旁观者模式
    // - 最低骑乘实体不是正在使用盾牌格挡（不适用于本项目）
    if (!entity.canTeleport()) {
        return;
    }

    // 传送实体
    // 注意：传送逻辑在 EndGatewayEntity::teleportEntity 中实现
    // 这里直接调用，因为实体已经进入方块
    gatewayEntity->teleportEntity(world, entity);
}

const CollisionShape& EndGatewayBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& EndGatewayBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

} // namespace blocks
} // namespace mc
