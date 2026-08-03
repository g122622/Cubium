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

#include "HoneyBlock.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace blocks {

// ========== HoneyBlock ==========

HoneyBlock::HoneyBlock(const BlockProperties& properties)
    : Block(properties)
{
    // 蜂蜜块滑度为默认值 0.6（MC 中蜂蜜块不修改 friction）
    // 蜂蜜块的减速效果通过 speedFactor=0.4 和 jumpFactor=0.5 实现
    m_slipperiness = physics::SLIPPERINESS_HONEY;
    m_speedFactor = physics::HONEY_BLOCK_SPEED_FACTOR;
    m_jumpFactor = physics::HONEY_BLOCK_JUMP_FACTOR;

    // 蜂蜜块碰撞箱稍小
    m_collisionShape = CollisionShape::box(0.0f, 0.0f, 0.0f, 1.0f, 0.9375f, 1.0f);
}

void HoneyBlock::onLanded(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 蜂蜜块消除摔落伤害，但不弹跳
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 velocity = entity.velocity();
    // Y速度归零，但不反弹
    entity.setVelocity(velocity.x, 0.0f, velocity.z);
    // 重置摔落距离（消��摔落伤害）
    entity.setFallDistance(0.0f);
}

void HoneyBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 蜂蜜块减缓实体速度
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 velocity = entity.velocity();
    // 水平速度减少 40%（乘以 0.4）
    // 垂直下落速度减少（下落时每 tick 减速）
    entity.setVelocity(velocity.x * 0.4f, velocity.y * 0.9f, velocity.z * 0.4f);
}

Material::PushReaction HoneyBlock::getPushReaction(const BlockState& state) const
{
    MC_UNUSED(state);
    return Material::PushReaction::Normal;
}

bool HoneyBlock::isStickyBlock(const BlockState& state) const noexcept
{
    MC_UNUSED(state);
    return true;
}

bool HoneyBlock::canStickTo(const BlockState& state, const BlockState& other) const noexcept
{
    MC_UNUSED(state);
    // 蜂蜜块只能粘住蜂蜜块（不能粘住史莱姆块）
    // 如果两个都是蜂蜜块，则可以粘连
    // 检查 other 方块是否是蜂蜜块
    return other.is(VanillaBlocks::HONEY_BLOCK);
}

const CollisionShape& HoneyBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_collisionShape;
}

} // namespace blocks
} // namespace mc
