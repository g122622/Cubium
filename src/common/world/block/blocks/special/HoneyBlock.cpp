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
#include "common/entity/damage/DamageSource.hpp"
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
    // 蜂蜜块不弹跳：Y 速度归零（对齐 Java updateEntityMovementAfterFallOn 走基类 super 不反弹）。
    // 不重置 fallDistance——摔落减伤由 onFallenUpon 以 multiplier=0.2 处理（updateFallDistance 在
    // onLanded 之后调用 onFallenUpon，fallDistance 仍是着地前累积值）。
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 velocity = entity.velocity();
    entity.setVelocity(velocity.x, 0.0f, velocity.z);
}

void HoneyBlock::onFallenUpon(
    IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance)
{
    // 蜂蜜块减伤 80%（保留 20%）：以 damageMultiplier=0.2 调 causeFallDamage。
    // 对齐 Java HoneyBlock#fallOn（causeFallDamage(distance, 0.2F, fall)）与 wiki
    // "摔在蜂蜜块上的生物受到的跌落伤害会减少80%"。LivingEntity::causeFallDamage 计算
    // (distance-3)*0.2，大落差仍受少量伤害（非完全免疫，区别于粘液块的 0.0 完全免疫）。
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    entity.causeFallDamage(fallDistance, 0.2f, DamageSources::fall());
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
