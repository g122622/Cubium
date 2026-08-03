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

#include "VexMovementController.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/illager/VexEntity.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::entity::ai::controller {

VexMovementController::VexMovementController(VexEntity* vex)
    : MovementController(vex)
    , m_vex(vex)
{}

void VexMovementController::tick()
{
    if (!m_vex) return;

    // 恼鬼使用直接修改velocity的方式飞行，不使用传统导航
    if (m_action == MoveAction::MoveTo) {
        // 计算到目标位置的向量
        f64 dx = m_posX - m_vex->x();
        f64 dy = m_posY - m_vex->y();
        f64 dz = m_posZ - m_vex->z();
        f64 distance = std::sqrt(dx * dx + dy * dy + dz * dz);

        // 如果距离小于碰撞箱平均边长，认为已到达
        // collisionBox.getAverageEdgeLength() = (width + height + width) / 3
        f32 avgEdgeLength = (m_vex->width() + m_vex->height() + m_vex->width()) / 3.0f;

        if (distance < static_cast<f64>(avgEdgeLength)) {
            // 已到达目标，停止移动
            m_action = MoveAction::Wait;

            // 减速
            Vector3 velocity = m_vex->velocity();
            m_vex->setVelocity(velocity * 0.5);
        } else {
            // 添加速度向量
            // 速度因子: speed * 0.05 / distance
            f64 speedFactor = m_speed * 0.05 / distance;

            Vector3 velocity = m_vex->velocity();
            velocity.x += static_cast<f32>(dx * speedFactor);
            velocity.y += static_cast<f32>(dy * speedFactor);
            velocity.z += static_cast<f32>(dz * speedFactor);
            m_vex->setVelocity(velocity);

            // 更新旋转
            LivingEntity* attackTarget = m_vex->attackTarget();

            if (attackTarget == nullptr) {
                // 无攻击目标：朝运动方向旋转
                // 使用velocity计算yaw
                f32 yaw = static_cast<f32>(std::atan2(velocity.x, velocity.z) * math::RAD_TO_DEG);
                m_vex->setRotation(yaw, m_vex->pitch());
            } else {
                // 有攻击目标：朝目标旋转
                f64 targetDx = attackTarget->x() - m_vex->x();
                f64 targetDz = attackTarget->z() - m_vex->z();
                f32 yaw = static_cast<f32>(std::atan2(targetDx, targetDz) * math::RAD_TO_DEG);
                m_vex->setRotation(yaw, m_vex->pitch());
            }

            // 设置渲染偏航角等于当前偏航角
            m_vex->setRenderYawOffset(m_vex->yaw());
        }
    }
}

} // namespace mc::entity::ai::controller
