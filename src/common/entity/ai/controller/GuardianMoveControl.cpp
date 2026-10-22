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

#include "GuardianMoveControl.hpp"

#include "common/entity/ai/controller/LookController.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/entities/monster/ocean/GuardianEntity.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"

#include <cmath>

namespace mc::entity::ai::controller {

namespace {
/// 目标方向的 AI 速度插值系数（对齐 vanilla Mth.lerp(0.125F, getSpeed(), f1)）
constexpr f32 SPEED_LERP = 0.125f;
} // namespace

GuardianMoveControl::GuardianMoveControl(GuardianEntity* guardian)
    : MovementController(guardian)
    , m_guardian(guardian)
{}

void GuardianMoveControl::tick()
{
    if (m_guardian == nullptr) {
        return;
    }

    auto* nav = m_guardian->navigator();
    const bool navigating = nav != nullptr && !nav->isDone();
    if (!isUpdating() || !navigating) {
        // 对齐 vanilla else 分支：无移动目标或导航已完成时停下
        m_guardian->setAIMoveSpeed(0.0f);
        m_guardian->setMoving(false);
        return;
    }

    const f64 goalX = getX();
    const f64 goalY = getY();
    const f64 goalZ = getZ();
    const Vector3 toGoal(goalX - m_guardian->x(), goalY - m_guardian->y(), goalZ - m_guardian->z());
    const f64 goalLength = toGoal.length();
    if (goalLength <= 1.0e-9) {
        m_guardian->setAIMoveSpeed(0.0f);
        m_guardian->setMoving(false);
        return;
    }
    const f64 dirX = toGoal.x / goalLength;
    const f64 dirY = toGoal.y / goalLength;
    const f64 dirZ = toGoal.z / goalLength;

    // 转向：yaw 以 90 度/tick 逼近目标方向，身体朝向同步
    const f32 targetYaw = static_cast<f32>(std::atan2(toGoal.z, toGoal.x) * 180.0 / math::PI) - 90.0f;
    const f32 newYaw = static_cast<f32>(math::clampedRotate(m_guardian->yaw(), targetYaw, 90.0f));
    m_guardian->setRotation(newYaw, m_guardian->pitch());
    m_guardian->setRenderYawOffset(newYaw);

    // AI 速度：speedModifier * MOVEMENT_SPEED，并以 0.125 插值平滑
    const f32 speedTarget = static_cast<f32>(
        std::fabs(speed()) * m_guardian->getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED));
    const f32 newSpeed = static_cast<f32>(math::lerp(
        static_cast<f64>(SPEED_LERP), static_cast<f64>(m_guardian->aiMoveSpeed()), static_cast<f64>(speedTarget)));
    m_guardian->setAIMoveSpeed(newSpeed);

    // 正弦摆动（对齐 vanilla GuardianMoveControl.tick 的 d4/d7 计算）
    const f64 tickSeed = static_cast<f64>(m_guardian->ticksExisted()) + static_cast<f64>(m_guardian->id());
    const f64 wobbleXZ = std::sin(tickSeed * 0.5) * 0.05;
    // vanilla 用 Math.sin/cos(getYRot() * DEG2RAD)，而 Java 的 yaw 朝向为 -sin/-cos；
    // Cubium 的 yaw 朝向为 (-sin, cos)，故此处对 yaw 取负，保持两者坐标约定一致。
    const f64 yawRad = static_cast<f64>(m_guardian->yaw()) * (math::PI / 180.0);
    const f64 cosYaw = std::cos(-yawRad);
    const f64 sinYaw = std::sin(-yawRad);
    const f64 wobbleY = std::sin(tickSeed * 0.75) * 0.05;

    Vector3 velocity = m_guardian->velocity();
    velocity.x += wobbleXZ * cosYaw;
    velocity.y += wobbleY * (sinYaw + cosYaw) * 0.25 + static_cast<f64>(newSpeed) * dirY * 0.1;
    velocity.z += wobbleXZ * sinYaw;
    m_guardian->setVelocity(velocity);

    // 注视：目标方向前方 2 格；当前没有注视目标时用 0.125 插值过渡
    // （对齐 vanilla：!lookControl.isLookingAtTarget() 时与当前期望点做 lerp）
    const f64 aheadX = m_guardian->x() + dirX * 2.0;
    const f64 aheadY = m_guardian->getEyeY() + dirY / goalLength;
    const f64 aheadZ = m_guardian->z() + dirZ * 2.0;
    if (auto* lookCtrl = m_guardian->lookController()) {
        f64 lookX = aheadX;
        f64 lookY = aheadY;
        f64 lookZ = aheadZ;
        if (!lookCtrl->isLooking()) {
            lookX = math::lerp(static_cast<f64>(SPEED_LERP), lookCtrl->getLookPosX(), aheadX);
            lookY = math::lerp(static_cast<f64>(SPEED_LERP), lookCtrl->getLookPosY(), aheadY);
            lookZ = math::lerp(static_cast<f64>(SPEED_LERP), lookCtrl->getLookPosZ(), aheadZ);
        }
        lookCtrl->setLookPosition(lookX, lookY, lookZ, 10.0f, 40.0f);
    }

    m_guardian->setMoving(true);
}

} // namespace mc::entity::ai::controller