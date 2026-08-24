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
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/effect/EffectType.hpp"
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
    // 对齐 vanilla 1.21.11 WebBlock.entityInside（WebBlock.java:27-34）：
    //   Vec3 vec3 = new Vec3(0.25, 0.05F, 0.25);
    //   if (entity instanceof LivingEntity livingentity && livingentity.hasEffect(MobEffects.WEAVING)) {
    //       vec3 = new Vec3(0.5, 0.25, 0.5);
    //   }
    //   entity.makeStuckInBlock(state, vec3);
    //
    // Cubium 用 setMotionMultiplier 对应 vanilla makeStuckInBlock：设置本帧位移乘数，
    // 并在 setMotionMultiplier 内部 resetFallDistance（实体穿过蜘蛛网下落不累积摔落距离，
    // 落到下方实方块时不摔伤）。此前 Cubium 用 setVelocity 改下一帧速度，偏离 vanilla
    // makeStuckInBlock 的"本帧位移×乘数"语义，且未重置 fallDistance 致落蜘蛛网仍摔伤。
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    Vector3 slowdown(physics::COBWEB_SLOWDOWN_XZ, physics::COBWEB_SLOWDOWN_Y, physics::COBWEB_SLOWDOWN_XZ);

    // 受 WEAVING（纺织）效果的 LivingEntity 减速更轻 (0.5, 0.25, 0.5)。
    // WEAVING 为 1.21 下界更新新增效果，使中毒实体在蜘蛛网中行动更自如。
    auto* livingEntity = dynamic_cast<LivingEntity*>(&entity);
    if (livingEntity != nullptr && livingEntity->hasEffect(entity::effect::EffectType::Weaving)) {
        slowdown = Vector3(physics::COBWEB_WEAVING_SLOWDOWN_XZ,
            physics::COBWEB_WEAVING_SLOWDOWN_Y,
            physics::COBWEB_WEAVING_SLOWDOWN_XZ);
    }

    entity.setMotionMultiplier(slowdown);
}

} // namespace blocks
} // namespace mc
