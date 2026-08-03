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

#include "FlyingMovementController.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::entity::ai::controller {

// 最小速度阈值，低于此值认为已到达目标
static constexpr f64 MIN_SPEED_SQR = 2.5000003E-7;

FlyingMovementController::FlyingMovementController(MobEntity* mob, i32 maxTurn, bool hoversInPlace)
    : MovementController(mob)
    , m_maxTurn(maxTurn)
    , m_hoversInPlace(hoversInPlace)
{}

void FlyingMovementController::tick()
{
    if (m_action == MoveAction::MoveTo) {
        m_action = MoveAction::Wait;

        // 有移动目标时，消除重力
        m_mob->setNoGravity(true);

        // 计算到目标位置的向量
        f64 dx = m_posX - m_mob->x();
        f64 dy = m_posY - m_mob->y();
        f64 dz = m_posZ - m_mob->z();
        f64 distSqr = dx * dx + dy * dy + dz * dz;

        // 距离过近则停止移动
        if (distSqr < MIN_SPEED_SQR) {
            m_mob->setMoveForward(0.0f);
            m_mob->setMoveStrafing(0.0f);
            return;
        }

        // 计算目标偏航角（Y轴旋转，即水平朝向），以90度/tick旋转
        f32 targetYaw = static_cast<f32>(std::atan2(dz, dx) * (180.0 / math::PI)) - 90.0f;
        m_mob->setRotation(math::clampedRotate(m_mob->yaw(), targetYaw, 90.0f), m_mob->pitch());

        // 根据是否在地面选择速度属性
        f32 speed;
        if (m_mob->onGround()) {
            speed = static_cast<f32>(m_speed * m_mob->getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED));
        } else {
            speed = static_cast<f32>(m_speed * m_mob->getAttributeValue(entity::attribute::Attributes::FLYING_SPEED));
        }
        m_mob->setAIMoveSpeed(speed);

        // 计算俯仰角
        f64 horizontalDist = std::sqrt(dx * dx + dz * dz);
        if (std::abs(dy) > 1.0E-5 || std::abs(horizontalDist) > 1.0E-5) {
            f32 targetPitch = static_cast<f32>(-(std::atan2(dy, horizontalDist) * (180.0 / math::PI)));
            m_mob->setRotation(
                m_mob->yaw(), math::clampedRotate(m_mob->pitch(), targetPitch, static_cast<f32>(m_maxTurn)));
            // 根据俯仰方向设置上下移动输入
            m_mob->setMoveForward(dy > 0.0 ? 1.0f : -1.0f);
        }
    } else {
        // 无移动目标时
        if (!m_hoversInPlace) {
            // 非悬停模式：恢复重力，凋灵会缓慢下落
            m_mob->setNoGravity(false);
        }

        m_mob->setMoveForward(0.0f);
        m_mob->setMoveStrafing(0.0f);
    }
}

} // namespace mc::entity::ai::controller
