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

#include "EndPortalBlock.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"

namespace mc {
namespace blocks {

EndPortalBlock::EndPortalBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 传送门没有碰撞箱
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.75f, 1.0f);
}

void EndPortalBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 末地传送门是立即传送的，不需要等待时间
    // 玩家进入末地传送门后会立即传送到末地出生点 (100, 49, 0)

    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 检查传送冷却
    if (!entity.canTeleport()) {
        return; // 还在冷却中
    }

    // 设置传送冷却，防止重复传送
    entity.setPortalCooldown(300); // 15秒冷却

    // 确定目标维度
    // 主世界 -> 末地: 传送到固定出生点 (100, 49, 0)
    // 末地 -> 主世界: 返回重生点或床
    DimensionId targetDim = (entity.dimension() == 1) ? DimensionId(0) : DimensionId(1);

    // 设置实体的目标维度标志
    // 实际传送由 ServerDimensionManager 处理
    // 这里只设置传送请求标志
    entity.setDimension(targetDim);

    // 注意：实际的维度切换逻辑由服务端的 ServerDimensionManager 处理
    // 客户端只需要处理动画效果
}

const CollisionShape& EndPortalBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

const CollisionShape& EndPortalBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    static CollisionShape emptyShape = CollisionShape::empty();
    return emptyShape;
}

} // namespace blocks
} // namespace mc
