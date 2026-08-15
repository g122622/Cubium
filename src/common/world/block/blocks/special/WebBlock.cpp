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

#include "WebBlock.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"

namespace mc {
namespace blocks {

// ========== WebBlock ==========

WebBlock::WebBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 蜘蛛网形状：完整方块，透明
    m_shape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
}

const CollisionShape& WebBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

void WebBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 蜘蛛网大幅减缓实体速度
    // 实际效果是水平速度 * 0.25，垂直下落 * 0.05（对齐 Java CobwebBlock）
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 velocity = entity.velocity();
    entity.setVelocity(velocity.x * physics::COBWEB_SLOWDOWN_XZ,
        velocity.y < 0.0f ? velocity.y * physics::COBWEB_SLOWDOWN_Y : velocity.y, // 只减速下落
        velocity.z * physics::COBWEB_SLOWDOWN_XZ);
}

} // namespace blocks
} // namespace mc
