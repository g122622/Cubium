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
#include "common/world/dimension/DimensionManager.hpp" // DimensionManager::OVERWORLD/THE_END 常量

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
    // 末地传送门是立即传送的，不需要等待时间（vanilla Java EndPortalBlock 无 80tick 等待，
    // 与下界传送门经 PortalTickSystem 计时不同）。
    // 玩家进入末地传送门后会立即传送到末地出生点 (100, 49, 0)（由 changeDimension 内
    // Teleporter::getEndSpawnPosition 处理）；末地→主世界走 transformPosition（1:1 坐标）。

    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 检查传送冷却
    if (!entity.canTeleport()) {
        return; // 还在冷却中
    }

    // 设置传送冷却，防止重复传送
    entity.setPortalCooldown(300); // 15秒冷却

    // 确定目标维度：末地 → 主世界，其他（主世界）→ 末地。
    DimensionId targetDim =
        (entity.dimension() == DimensionManager::THE_END) ? DimensionManager::OVERWORLD : DimensionManager::THE_END;

    // 经虚派发触发真实传送：
    // - ServerPlayer（含 SimulatedPlayer）override 调真实 changeDimension，内部按目标维度
    //   处理末地固定出生点 (100,49,0)+黑曜石平台、末地→主世界用 transformPosition。
    // - 非玩家实体基类返回 false（当前不支持，留 TODO）。
    // 触发后立即 return：changeDimension 内会迁移 EntityManager 并改 m_world，
    // 本回调返回后 doBlockCollisions 三层 for 循环不再读已变更的 m_world（本格已处理完）。
    entity.changeDimension(targetDim);
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
