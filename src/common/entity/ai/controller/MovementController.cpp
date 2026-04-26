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
    m_action = MoveAction::MoveTo;
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
        // MC 的 STRAFE 模式实现
        // 计算移动速度
        f32 baseSpeed = static_cast<f32>(m_mob->getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2));
        f32 moveSpeed = static_cast<f32>(m_speed) * baseSpeed;

        // 归一化和缩放移动向量
        f32 forward = m_moveForward;
        f32 strafe = m_moveStrafe;
        f32 length = std::sqrt(forward * forward + strafe * strafe);
        if (length < 1.0f) {
            length = 1.0f;
        }
        f32 scaledForward = (forward / length) * moveSpeed;
        f32 scaledStrafe = (strafe / length) * moveSpeed;

        // TODO: 检查目标位置是否可行走 (func_234024_b_)
        // 当前简化实现：直接设置移动

        m_mob->setAIMoveSpeed(moveSpeed);
        m_mob->setMoveForward(forward);
        m_mob->setMoveStrafing(strafe);
        m_action = MoveAction::Wait;
    }
    else if (m_action == MoveAction::MoveTo) {
        // MC: MOVE_TO 状态在tick开头立即转为WAIT
        m_action = MoveAction::Wait;

        f64 dx = m_posX - m_mob->x();
        f64 dy = m_posY - m_mob->y();
        f64 dz = m_posZ - m_mob->z();

        // MC 使用3D距离平方，阈值极小（2.5000003E-7F ≈ 0.0005格）
        f64 distSq = dx * dx + dy * dy + dz * dz;
        if (distSq < 2.5000003E-7) {
            // 已到达目标
            m_mob->setMoveForward(0.0f);
            m_mob->setMoveStrafing(0.0f);
            return;
        }

        // 计算目标偏航角
        f32 targetYaw = static_cast<f32>(std::atan2(dz, dx) * math::RAD_TO_DEG - 90.0);

        // 限制旋转速度（MC默认30度/tick）
        f32 currentYaw = m_mob->yaw();
        f32 newYaw = math::clampedRotate(currentYaw, targetYaw, 30.0f);

        m_mob->setRotation(newYaw, m_mob->pitch());

        // 设置移动速度
        f32 moveSpeed = static_cast<f32>(m_speed * m_mob->getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2));
        m_mob->setAIMoveSpeed(moveSpeed);
        m_mob->setMoveForward(1.0f);  // 向前移动

        // 检查是否需要跳跃（目标位置比当前位置高，且水平距离小于实体宽度）
        f64 horizontalDistSq = dx * dx + dz * dz;
        f32 entityWidth = m_mob->width();
        f32 maxDist = std::max(1.0f, entityWidth);
        if (dy > m_mob->stepHeight() && horizontalDistSq < static_cast<f64>(maxDist * maxDist)) {
            // TODO: 检查脚下方块碰撞
            if (auto* jumpCtrl = m_mob->jumpController()) {
                jumpCtrl->setJumping();
            }
            m_action = MoveAction::Jumping;
        }
    }
    else if (m_action == MoveAction::Jumping) {
        // MC: JUMPING 状态设置移动速度
        f32 moveSpeed = static_cast<f32>(m_speed * m_mob->getAttributeValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2));
        m_mob->setAIMoveSpeed(moveSpeed);

        if (m_mob->onGround()) {
            m_action = MoveAction::Wait;  // MC: 着陆后设为WAIT
        }
    }
    else {
        // Wait 状态
        m_mob->setMoveForward(0.0f);
        m_mob->setMoveStrafing(0.0f);
    }
}

} // namespace mc::entity::ai::controller
