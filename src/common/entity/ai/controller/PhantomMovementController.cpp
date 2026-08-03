/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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

#include "PhantomMovementController.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/entities/monster/basic/PhantomEntity.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <algorithm>
#include <cmath>

namespace mc::entity::ai::controller {

PhantomMovementController::PhantomMovementController(PhantomEntity* phantom)
    : MovementController(phantom)
    , m_phantom(phantom)
{
    MC_ASSERT_RELEASE(phantom != nullptr);
}

void PhantomMovementController::tick()
{
    // 水平碰撞时：自动180度转向并降低速度
    if (m_phantom->collidedHorizontally()) {
        m_phantom->setRotation(m_phantom->yaw() + 180.0f, m_phantom->pitch());
        m_speed = 0.1f;
    }

    // 获取移动目标点（orbitOffset）
    Vector3 target = m_phantom->orbitOffset();
    f64 dx = static_cast<f64>(target.x) - m_phantom->x();
    f64 dy = static_cast<f64>(target.y) - m_phantom->y();
    f64 dz = static_cast<f64>(target.z) - m_phantom->z();
    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

    if (std::abs(horizontalDist) <= 1.0E-5) {
        // 水平距离过近，不调整飞行方向
        return;
    }

    // 缩放水平分量以补偿垂直移动：当垂直分量较大时，减小水平速度
    f64 horizontalScale = 1.0 - std::abs(dy * 0.7) / horizontalDist;
    dx *= horizontalScale;
    dz *= horizontalScale;
    horizontalDist = std::sqrt(dx * dx + dz * dz);
    f64 dist3d = std::sqrt(dx * dx + dy * dy + dz * dz);

    // 保存旧偏航角用于后续方向判断
    f32 oldYaw = m_phantom->yaw();

    // 计算目标偏航角：atan2(dz, dx) 转为角度
    f32 targetYawDeg = static_cast<f32>(std::atan2(dz, dx) * (180.0 / math::PI));
    f32 currentYawPlus90 = math::wrapDegrees(m_phantom->yaw() + 90.0f);
    f32 wrappedTarget = math::wrapDegrees(targetYawDeg);

    // 平滑转向：每tick最大4度
    f32 newYaw = math::clampedRotate(currentYawPlus90, wrappedTarget, 4.0f) - 90.0f;
    m_phantom->setRotation(newYaw, m_phantom->pitch());

    // 同步身体旋转
    m_phantom->setRenderYawOffset(m_phantom->yaw());

    // 判断是否接近目标方向：角度差小于3度时加速
    // MC 原版: Mth.degreesDifferenceAbs(oldYaw, getYRot()) < 3.0F
    // 正确计算角度差：先 wrapDegrees 计算有符号差值，再取绝对值
    f32 yawDifference = std::abs(math::wrapDegrees(m_phantom->yaw() - oldYaw));
    if (yawDifference < 3.0f) {
        // 接近目标方向：加速到 1.8
        // MC 原版: Mth.approach(speed, 1.8F, 0.005F * (1.8F / speed))
        f32 step = 0.005f * (1.8f / m_speed);
        if (m_speed < 1.8f) {
            m_speed = std::min(m_speed + step, 1.8f);
        } else {
            m_speed = std::max(m_speed - step, 1.8f);
        }
    } else {
        // 远离目标方向：减速到 0.2
        // MC 原版: Mth.approach(speed, 0.2F, 0.025F)
        if (m_speed < 0.2f) {
            m_speed = std::min(m_speed + 0.025f, 0.2f);
        } else {
            m_speed = std::max(m_speed - 0.025f, 0.2f);
        }
    }

    // 计算俯仰角
    f32 pitch = static_cast<f32>(-(std::atan2(-dy, horizontalDist) * 180.0 / math::PI));
    m_phantom->setRotation(m_phantom->yaw(), pitch);

    // 计算速度向量
    f32 yawRad = (m_phantom->yaw() + 90.0f) * math::DEG_TO_RAD;
    f32 pitchRad = pitch * math::DEG_TO_RAD;

    f64 motionX = static_cast<f64>(m_speed) * std::cos(static_cast<f64>(yawRad)) * std::abs(dx / dist3d);
    f64 motionZ = static_cast<f64>(m_speed) * std::sin(static_cast<f64>(yawRad)) * std::abs(dz / dist3d);
    f64 motionY = static_cast<f64>(m_speed) * std::sin(static_cast<f64>(pitchRad)) * std::abs(dy / dist3d);

    // 使用 20% 惯性混合（与 MC 原版一致）
    Vector3 currentVel = m_phantom->velocity();
    Vector3 blendedVel(currentVel.x + (motionX - currentVel.x) * 0.2,
        currentVel.y + (motionY - currentVel.y) * 0.2,
        currentVel.z + (motionZ - currentVel.z) * 0.2);

    m_phantom->setVelocity(blendedVel);
}

} // namespace mc::entity::ai::controller
