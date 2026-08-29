/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ProjectileDeflection.hpp"
#include "ProjectileEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc {

bool applyProjectileDeflection(ProjectileDeflection deflection, entity::ProjectileEntity& projectile, Entity& deflector)
{
    if (deflection == ProjectileDeflection::None) {
        return false;
    }

    switch (deflection) {
        case ProjectileDeflection::Reverse: {
            // 反向偏转：速度乘以 -0.5，并添加随机 170~190 度偏航旋转
            math::Random rng(projectile.id() ^ projectile.ticksExisted());
            const f32 randomYaw = 170.0f + rng.nextFloat() * 20.0f;

            Vector3 velocity = projectile.velocity();
            velocity = velocity * -0.5f;
            projectile.setVelocity(velocity);

            const f32 newYaw = projectile.yaw() + randomYaw;
            projectile.setYaw(newYaw);
            projectile.setPrevYaw(projectile.prevYaw() + randomYaw);

            // 将偏转者设为新的发射者
            projectile.setShooter(&deflector);
            return true;
        }

        case ProjectileDeflection::AimDeflect: {
            // 瞄准偏转：将弹射物速度设置为偏转者的视线方向（单位向量）。
            // 对齐 vanilla ProjectileDeflection.AIM_DEFLECT（ProjectileDeflection.java:18-24）：
            //   vec3 = deflector.getLookAngle(); projectile.setDeltaMovement(vec3);
            // getLookAngle 返回单位视线向量（不乘原速），故偏转后弹射物以单位速度（1 格/tick）
            // 沿偏转者视线方向飞行。此前 Cubium 误乘以原速度大小（speed=velocity().length()），
            //   对静止弹射物（speed=0）偏转后仍静止，与 vanilla 不符（vanilla 静止弹射物偏转后以
            //   单位速度沿视线飞）。此处改为单位视线向量对齐 vanilla。
            const f32 pitchRad = deflector.pitch() * math::DEG_TO_RAD;
            const f32 yawRad = deflector.yaw() * math::DEG_TO_RAD;

            const f32 x = -std::sin(yawRad) * std::cos(pitchRad);
            const f32 y = -std::sin(pitchRad);
            const f32 z = std::cos(yawRad) * std::cos(pitchRad);

            projectile.setVelocity(Vector3(x, y, z));

            // 将偏转者设为新的发射者
            projectile.setShooter(&deflector);
            return true;
        }

        case ProjectileDeflection::MomentumDeflect: {
            // 动量偏转：将弹射物速度设置为偏转者移动方向的归一化向量。
            // 对齐 vanilla ProjectileDeflection.MOMENTUM_DEFLECT（ProjectileDeflection.java:25-31）：
            //   vec3 = deflector.getDeltaMovement().normalize(); projectile.setDeltaMovement(vec3);
            // 归一化后直接赋值（不乘原速）。偏转者静止时 normalize() 返回零向量，弹射物速度归零。
            //   此前 Cubium 误乘以弹射物原速度大小（speed=velocity().length()），与 vanilla 不符
            //   （vanilla 不保留原速，偏转后弹射物以单位速度沿偏转者移动方向飞）。
            const Vector3 motion = deflector.velocity().normalized();
            projectile.setVelocity(motion);

            // 将偏转者设为新的发射者
            projectile.setShooter(&deflector);
            return true;
        }

        case ProjectileDeflection::None:
        default:
            return false;
    }
}

} // namespace mc
