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

#include "SlimeBlock.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"

namespace mc {
namespace blocks {

// ========== SlimeBlock ==========

SlimeBlock::SlimeBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 史莱姆块滑度为 0.8
    m_slipperiness = physics::SLIPPERINESS_SLIME;
}

void SlimeBlock::onLanded(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 如果实体向下落，反弹
    // 反弹系数：LivingEntity 使用 1.0，其他实体使用 0.8
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 velocity = entity.velocity();
    if (velocity.y < 0.0f) {
        // 反弹：Y速度取反并乘以弹跳系数
        // 使用非生物实体的弹跳系数（保守值），LivingEntity 会单独处理
        entity.setVelocity(velocity.x, -velocity.y * physics::SLIME_BLOCK_BOUNCE_FACTOR_NON_LIVING, velocity.z);
    } else {
        // 向上或静止时，Y速度归零
        entity.setVelocity(velocity.x, 0.0f, velocity.z);
    }
}

void SlimeBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 史莱姆块会减缓实体的Y轴速度（类似于蜘蛛网的效果，但更温和）
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 这个效果主要用于实体在史莱姆块内部时减速
    // 实际弹跳在 onLanded 中处理
}

Material::PushReaction SlimeBlock::getPushReaction(const BlockState& state) const
{
    MC_UNUSED(state);
    return Material::PushReaction::Normal;
}

bool SlimeBlock::isStickyBlock(const BlockState& state) const noexcept
{
    MC_UNUSED(state);
    return true;
}

bool SlimeBlock::canStickTo(const BlockState& state, const BlockState& other) const noexcept
{
    MC_UNUSED(state);
    // 史莱姆块可以粘住史莱姆块和蜂蜜块
    const Block& otherBlock = other.getBlock();
    return otherBlock.isStickyBlock(other);
}

} // namespace blocks
} // namespace mc
