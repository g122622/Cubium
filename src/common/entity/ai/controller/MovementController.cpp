#include "MovementController.hpp"
#include "JumpController.hpp"
#include "../../core/MobEntity.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../attribute/Attributes.hpp"
#include "../../../util/math/MathUtils.hpp"
#include <cmath>

namespace mc::entity::ai::controller {

MovementController::MovementController(MobEntity* mob)
    : m_mob(mob)
{}

void MovementController::setMoveTo(f64 x, f64 y, f64 z, f64 speed) {
    m_posX = x;
    m_posY = y;
    m_posZ = z;
    m_speed = speed;

    if (m_action != MoveAction::Jumping) {
        m_action = MoveAction::MoveTo;
    }
}

void MovementController::strafe(f32 forward, f32 strafe) {
    m_action = MoveAction::Strafe;
    m_moveForward = forward;
    m_moveStrafe = strafe;
    m_speed = 0.25;  // 默认横向移动速度
}

void MovementController::tick() {
    if (!m_mob) return;

    if (m_action == MoveAction::Strafe) {
        // 横向移动模式
        m_mob->setMoveForward(m_moveForward);
        m_mob->setMoveStrafing(m_moveStrafe);
        m_action = MoveAction::Wait;
    }
    else if (m_action == MoveAction::MoveTo || m_action == MoveAction::Jumping) {
        // MoveTo 和 Jumping 状态持续执行直到到达目标或完成跳跃
        f64 dx = m_posX - m_mob->x();
        f64 dy = m_posY - m_mob->y();
        f64 dz = m_posZ - m_mob->z();

        f64 distSq = dx * dx + dz * dz;  // 只检查水平距离

        // 如果水平距离足够近，停止移动
        if (distSq < 0.5) {  // 到达目标（约0.7格距离内）
            m_mob->setMoveForward(0.0f);
            m_mob->setMoveStrafing(0.0f);
            m_action = MoveAction::Wait;
            return;
        }

        // 计算目标偏航角
        f32 targetYaw = static_cast<f32>(std::atan2(dz, dx) * math::RAD_TO_DEG - 90.0);

        // 限制旋转速度（每帧最多旋转90度）
        f32 currentYaw = m_mob->yaw();
        f32 newYaw = math::clampAngle(currentYaw, targetYaw, 30.0f);

        m_mob->setRotation(newYaw, m_mob->pitch());

        // 设置移动速度
        f32 speed = static_cast<f32>(m_speed * m_mob->getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2));
        m_mob->setMoveForward(speed);

        // 检查是否需要跳跃（目标位置比当前位置高，且水平距离较近）
        if (m_action == MoveAction::MoveTo && dy > m_mob->stepHeight() && distSq < 1.0) {
            if (auto* jumpCtrl = m_mob->jumpController()) {
                jumpCtrl->setJumping();
            }
            m_action = MoveAction::Jumping;
        }

        // 检查跳跃状态
        if (m_action == MoveAction::Jumping && m_mob->onGround()) {
            m_action = MoveAction::MoveTo;  // 着陆后继续移动
        }
    }
    else {
        // Wait 状态
        m_mob->setMoveForward(0.0f);
        m_mob->setMoveStrafing(0.0f);
    }
}

} // namespace mc::entity::ai::controller
