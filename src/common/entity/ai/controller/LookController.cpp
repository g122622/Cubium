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

#include "LookController.hpp"

#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/util/math/MathUtils.hpp"

#include <cmath>

namespace mc::entity::ai::controller {

LookController::LookController(MobEntity* mob)
    : m_mob(mob)
{}

void LookController::setLookPosition(f64 x, f64 y, f64 z)
{
    if (m_mob) {
        setLookPosition(x, y, z, m_mob->getFaceRotSpeed(), m_mob->getVerticalFaceSpeed());
    } else {
        setLookPosition(x, y, z, 10.0f, 10.0f);
    }
}

void LookController::setLookPosition(f64 x, f64 y, f64 z, f32 deltaYaw, f32 deltaPitch)
{
    m_posX = x;
    m_posY = y;
    m_posZ = z;
    m_deltaLookYaw = deltaYaw;
    m_deltaLookPitch = deltaPitch;
    m_isLooking = true;
}

void LookController::setLookPositionWithEntity(const Entity& entity, f32 deltaYaw, f32 deltaPitch)
{
    // LivingEntity 使用眼睛位置，其他实体使用碰撞盒中心
    f64 eyeY;
    if (const auto* living = dynamic_cast<const LivingEntity*>(&entity)) {
        eyeY = living->y() + living->eyeHeight();
    } else {
        const auto& box = entity.boundingBox();
        eyeY = (static_cast<f64>(box.minY) + static_cast<f64>(box.maxY)) / 2.0;
    }
    setLookPosition(entity.x(), eyeY, entity.z(), deltaYaw, deltaPitch);
}

void LookController::tick()
{
    if (!m_mob) return;

    // 首先处理俯仰角重置
    if (shouldResetPitch()) {
        m_mob->setRotationPitch(0.0f);
    }

    // 处理观看逻辑
    if (m_isLooking) {
        m_isLooking = false;

        // 计算目标角度并限制旋转速度
        f32 targetYaw = getTargetYaw();
        f32 targetPitch = getTargetPitch();

        f32 currentYaw = m_mob->rotationYawHead();
        f32 currentPitch = m_mob->pitch();

        f32 newYaw = math::clampedRotate(currentYaw, targetYaw, m_deltaLookYaw);
        f32 newPitch = math::clampedRotate(currentPitch, targetPitch, m_deltaLookPitch);

        // 分别设置头部旋转和俯仰角
        m_mob->setRotationYawHead(newYaw);
        m_mob->setRotationPitch(newPitch);
    } else {
        // 空闲时让头部朝向身体朝向
        f32 currentYaw = m_mob->rotationYawHead();
        f32 bodyYaw = m_mob->renderYawOffset();
        f32 newYaw = math::clampedRotate(currentYaw, bodyYaw, 10.0f);
        m_mob->setRotationYawHead(newYaw);
    }

    // 如果有导航路径，限制头部与身体的角度差
    auto* navigator = m_mob->navigator();
    if (navigator && !navigator->noPath()) {
        f32 currentYaw = m_mob->rotationYawHead();
        f32 bodyYaw = m_mob->renderYawOffset();
        f32 maxRotate = m_mob->getHorizontalFaceSpeed();
        f32 newYaw = math::approachTargetAngle(currentYaw, bodyYaw, maxRotate);
        m_mob->setRotationYawHead(newYaw);
    }
}

f32 LookController::getTargetYaw() const
{
    if (!m_mob) return 0.0f;

    f64 dx = m_posX - m_mob->x();
    f64 dz = m_posZ - m_mob->z();

    // atan2(dz, dx) 返回弧度，转换为度数
    f32 yaw = static_cast<f32>(std::atan2(dz, dx) * math::RAD_TO_DEG - 90.0);

    return yaw;
}

f32 LookController::getTargetPitch() const
{
    if (!m_mob) return 0.0f;

    f64 dx = m_posX - m_mob->x();
    f64 dy = m_posY - (m_mob->y() + m_mob->eyeHeight()); // 眼睛高度
    f64 dz = m_posZ - m_mob->z();

    f64 horizontalDist = std::sqrt(dx * dx + dz * dz);

    // 俯仰角为负值表示向上看
    f32 pitch = static_cast<f32>(-(std::atan2(dy, horizontalDist) * math::RAD_TO_DEG));

    return pitch;
}

} // namespace mc::entity::ai::controller
