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
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
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
    // 实体落到粘液块上：向下落且非潜行时反弹，否则按普通方块着地（Y 速度归零）。
    // 参考: net.minecraft.world.level.block.SlimeBlock#updateEntityMovementAfterFallOn / bounceUp
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 潜行实体（isSteppingCarefully）阻尼弹跳，不反弹，调用基类 onLanded 归零 Y 速度。
    if (entity.isSteppingCarefully()) {
        Block::onLanded(state, world, pos, entity);
        return;
    }

    Vector3 velocity = entity.velocity();
    if (velocity.y < 0.0f) {
        // 反弹：Y 速度取反并乘以弹跳系数。
        // LivingEntity 使用 1.0（完全反弹），其他实体使用 0.8（每次损失 20%）。
        const f32 bounceFactor = (dynamic_cast<LivingEntity*>(&entity) != nullptr)
            ? physics::SLIME_BLOCK_BOUNCE_FACTOR_LIVING
            : physics::SLIME_BLOCK_BOUNCE_FACTOR_NON_LIVING;
        entity.setVelocity(velocity.x, -velocity.y * bounceFactor, velocity.z);
    } else {
        // 向上或静止时，Y 速度归零
        entity.setVelocity(velocity.x, 0.0f, velocity.z);
    }
}

void SlimeBlock::onFallenUpon(
    IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance)
{
    // 粘液块免疫摔落伤害：以 damageMultiplier=0.0 调用 causeFallDamage。
    // multiplier=0 使 LivingEntity::causeFallDamage 计算伤害 (distance-3)*0=0，不受伤；
    // 同时仍走 causeFallDamage 以传播摔落给乘客（对齐 Java fallOn 调 causeFallDamage(distance,0.0F,fall)）。
    // 参考: net.minecraft.world.level.block.SlimeBlock#fallOn
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    entity.causeFallDamage(fallDistance, 0.0f, DamageSources::fall());
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
