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

#include "DrownedMoveControl.hpp"

#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/core/MobEntity.hpp"
#include "common/entity/entities/monster/undead/DrownedEntity.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::entity::ai::controller {

DrownedMoveControl::DrownedMoveControl(DrownedEntity* drowned)
    : MovementController(drowned)
    , m_drowned(drowned)
{}

void DrownedMoveControl::tick()
{
    if (m_drowned == nullptr) {
        return;
    }

    // 溺尸在水中且想游泳时使用水中移动模式
    if (m_drowned->wantsToSwim() && m_drowned->isInWater()) {
        LivingEntity* target = m_drowned->attackTarget();

        // 如果目标在上方或正在搜索陆地，添加微小向上推力
        if ((target != nullptr && target->y() > m_drowned->y()) || m_drowned->isSearchingForLand()) {
            Vector3 velocity = m_drowned->velocity();
            velocity.y += 0.002f;
            m_drowned->setVelocity(velocity);
        }

        // 如果没有移动目标或导航完成，停止移动
        if (m_action != MoveAction::MoveTo || !m_drowned->navigator() || m_drowned->navigator()->isDone()) {
            m_drowned->setAIMoveSpeed(0.0f);
            return;
        }

        // 计算到目标位置的方向
        f64 dx = m_posX - m_drowned->x();
        f64 dy = m_posY - m_drowned->y();
        f64 dz = m_posZ - m_drowned->z();
        f64 dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (dist < 1.0e-8) {
            // 距离极小，停止
            m_drowned->setAIMoveSpeed(0.0f);
            return;
        }

        // 归一化 Y 分量
        f64 normDy = dy / dist;

        // 计算目标偏航角（水平方向）
        f32 targetYaw = static_cast<f32>(std::atan2(dz, dx) * math::RAD_TO_DEG - 90.0);

        // 平滑旋转，限制最大旋转速度为 90 度/tick
        f32 currentYaw = m_drowned->yaw();
        f32 newYaw = math::wrapDegreesPositive(math::clampedRotate(currentYaw, targetYaw, 90.0f));
        m_drowned->setRotation(newYaw, m_drowned->pitch());

        // 游泳时身体朝向与头部一致
        m_drowned->setRenderYawOffset(newYaw);

        // 计算移动速度
        f32 moveSpeed = static_cast<f32>(
            m_speed * m_drowned->getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2));
        f32 lerpSpeed = math::lerp(0.125f, m_drowned->aiMoveSpeed(), moveSpeed);
        m_drowned->setAIMoveSpeed(lerpSpeed);

        // 在速度方向上添加推力
        // 水平方向使用 0.005 的缩放因子，Y 方向使用 0.1 的缩放因子（更大以提供垂直推力）
        Vector3 velocity = m_drowned->velocity();
        velocity.x += lerpSpeed * static_cast<f32>(dx) * 0.005f;
        velocity.y += lerpSpeed * static_cast<f32>(normDy) * 0.1f;
        velocity.z += lerpSpeed * static_cast<f32>(dz) * 0.005f;
        m_drowned->setVelocity(velocity);
    } else {
        // 陆地模式：不在水中或不想游泳
        // 不在地面时添加微小重力
        if (!m_drowned->onGround()) {
            Vector3 velocity = m_drowned->velocity();
            velocity.y -= 0.008f;
            m_drowned->setVelocity(velocity);
        }

        // 委托给基类处理地面移动
        MovementController::tick();
    }
}

} // namespace mc::entity::ai::controller
