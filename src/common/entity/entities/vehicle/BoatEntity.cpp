#include "BoatEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../util/math/MathConstants.hpp"
#include <cmath>

namespace mc {
namespace entity {

BoatEntity::BoatEntity(Type type)
    : Entity(LegacyEntityType::Boat, EntityId(0))
    , m_type(type)
{
    // MC 1.16.5: preventEntitySpawning = true
    // 设置尺寸通过 width()/height()
}

void BoatEntity::tick() {
    // MC 1.16.5 BoatEntity.tick()

    // 更新损伤计时器
    if (m_timeSinceHit > 0) {
        m_timeSinceHit--;
    }
    if (m_damageTaken > 0.0f) {
        m_damageTaken -= 0.05f;
        if (m_damageTaken < 0.0f) {
            m_damageTaken = 0.0f;
        }
    }

    // 更新失控计时器
    BoatStatus prevStatus = m_status;
    updateStatus();

    // 检查落下伤害
    f64 currentYd = y() - prevY();
    if (!onGround() && currentYd < -0.5) {
        m_lastYd = currentYd;
    }

    // 更新状态
    m_previousStatus = prevStatus;

    // 处理气泡柱
    updateRocking();

    // 更新插值
    tickLerp();

    // 更新运动
    updateMotion();

    // 控制船
    controlBoat();

    // 更新乘客位置
    updatePassengerPosition();

    // 调用父类tick
    Entity::tick();
}

void BoatEntity::handleInput(bool left, bool right, bool forward, bool backward) {
    // MC 1.16.5: 设置输入状态
    m_leftInputDown = left;
    m_rightInputDown = right;
    m_forwardInputDown = forward;
    m_backwardInputDown = backward;
}

void BoatEntity::dropItem() {
    // MC 1.16.5: 掉落对应类型的船物品
    // TODO: 根据类型掉落物品
    // ItemEntity* item = spawnItem(Items::getBoat(m_type));
}

void BoatEntity::updateMotion() {
    // MC 1.16.5 BoatEntity.updateMotion()
    f64 gravity = -GRAVITY;
    f32 friction = 0.05f;

    switch (m_status) {
        case BoatStatus::InWater:
            friction = WATER_FRICTION;
            break;
        case BoatStatus::UnderWater:
        case BoatStatus::UnderFlowingWater:
            gravity = -7.0e-4;
            friction = 0.45f;
            break;
        case BoatStatus::OnLand:
            friction = m_boatGlide;
            if (m_forwardInputDown) {
                friction /= 2.0f;
            }
            break;
        case BoatStatus::InAir:
            friction = WATER_FRICTION;
            break;
    }

    // 应用重力和摩擦
    Vector3 vel = velocity();
    vel.x *= friction;
    vel.y += static_cast<f32>(gravity);
    vel.z *= friction;
    setVelocity(vel);
}

void BoatEntity::floatBoat() {
    // MC 1.16.5: 计算浮力
    // TODO: 从世界获取水位高度
    // 计算浮力并调整位置
}

void BoatEntity::controlBoat() {
    // MC 1.16.5 BoatEntity.controlBoat()
    if (getPassengers().empty()) {
        return;
    }

    // 前进/后退
    if (m_forwardInputDown) {
        m_speed += WATER_SPEED_MULT;
    }
    if (m_backwardInputDown) {
        m_speed -= 0.005f;
    }

    // 转向
    if (m_leftInputDown) {
        m_deltaRotation -= 0.1f;
    }
    if (m_rightInputDown) {
        m_deltaRotation += 0.1f;
    }

    // 没有前进输入时减速
    if (!m_forwardInputDown && !m_backwardInputDown) {
        m_speed *= 0.95f;
    }

    // 限制速度
    m_speed = std::max(-MAX_SPEED, std::min(MAX_SPEED, m_speed));

    // 应用转向
    m_yaw += m_deltaRotation;
    m_deltaRotation *= 0.8f;

    // 应用速度
    f32 yawRad = math::toRadians(m_yaw);
    f32 vx = -std::sin(yawRad) * m_speed;
    f32 vz = std::cos(yawRad) * m_speed;
    setVelocity(vx, velocityY(), vz);

    // 更新桨状态
    setPaddleState(m_leftInputDown || m_forwardInputDown,
                   m_rightInputDown || m_forwardInputDown);
}

void BoatEntity::tickLerp() {
    // MC 1.16.5: 插值更新
    if (m_interpolationSteps > 0) {
        f64 lerpFactor = 1.0 / static_cast<f64>(m_interpolationSteps);
        f64 dx = m_interpolationX - x();
        f64 dy = m_interpolationY - y();
        f64 dz = m_interpolationZ - z();
        f64 dYaw = m_interpolationYaw - static_cast<f64>(m_yaw);
        f64 dPitch = m_interpolationPitch - static_cast<f64>(m_pitch);

        setPosition(static_cast<f32>(x() + dx * lerpFactor),
                    static_cast<f32>(y() + dy * lerpFactor),
                    static_cast<f32>(z() + dz * lerpFactor));
        Entity::setRotation(static_cast<f32>(m_yaw + dYaw * lerpFactor),
                    static_cast<f32>(m_pitch + dPitch * lerpFactor));
        m_interpolationSteps--;
    } else {
        setVelocity(velocity());
    }
}

void BoatEntity::updateStatus() {
    // MC 1.16.5: 更新船的状态
    // TODO: 检查是否在水中、陆地上或空中
    // m_status = ...
}

void BoatEntity::updatePassengerPosition() {
    // MC 1.16.5: 更新乘客位置
    // TODO: 根据乘客数量调整位置
}

void BoatEntity::updateRocking() {
    // MC 1.16.5: 更新气泡柱摇晃
    if (m_rockingTicks > 0) {
        m_rockingTicks--;
        m_prevRockingAngle = m_rockingAngle;
        m_rockingAngle += m_rockingIntensity;
    }
}

} // namespace entity
} // namespace mc
