#include "BoatEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../player/Player.hpp"
#include <cmath>

namespace mc {
namespace entity {

BoatEntity::BoatEntity(Type type)
    : Entity(LegacyEntityType::Item, EntityId(0)) // Using Item type temporarily
    , m_type(type)
{
    // 尺寸通过 width()/height() 设置
}

void BoatEntity::tick() {
    Entity::tick();

    // 检查是否在水中
    m_inWater = false; // isInWater();

    // 更新运动状态
    updateMotion();

    // 浮力处理
    floatBoat();

    // 控制船的移动
    controlBoat();

    // 减速
    if (m_onLand) {
        m_speed *= 0.5f;
        m_rotationVelocity *= 0.5f;
    } else {
        m_speed *= 0.95f;
        m_rotationVelocity *= 0.95f;
    }
}

void BoatEntity::setRotation(f32 yaw) {
    Entity::setRotation(yaw, m_pitch);
}

void BoatEntity::handleInput(f32 forward, f32 backward, f32 left, f32 right) {
    m_forwardInput = forward - backward;
    m_turnInput = right - left;
}

void BoatEntity::dropItem() {
    // TODO: 生成对应木材类型的船物品
    // ItemEntity* item = spawnItem(Items::BOATS[static_cast<u8>(m_type)]);
}

void BoatEntity::updateMotion() {
    Vector3 vel = velocity();
    // 根据是否在水中调整运动
    if (m_inWater) {
        // 水中漂浮
        vel.y += 0.03999999910593033f;
    } else if (m_onLand) {
        // 陆地摩擦
        vel.x *= 0.5f;
        vel.y *= 0.5f;
        vel.z *= 0.5f;
    }
    setVelocity(vel);
}

void BoatEntity::floatBoat() {
    // 计算浮力
    // TODO: 从世界获取水位高度
}

void BoatEntity::controlBoat() {
    // 被骑乘时根据输入控制
    if (m_forwardInput > 0.0f) {
        m_speed += m_inWater ? WATER_SPEED_MULT : LAND_SPEED_MULT;
    }
    if (m_forwardInput < 0.0f) {
        m_speed -= m_inWater ? WATER_SPEED_MULT : LAND_SPEED_MULT;
    }

    // 转向
    if (m_turnInput != 0.0f) {
        m_rotationVelocity += m_turnInput * 0.1f;
    }

    // 限制最大速度
    m_speed = std::max(-MAX_SPEED, std::min(MAX_SPEED, m_speed));

    // 应用速度
    f32 yawRad = m_yaw * 3.14159265f / 180.0f;
    f32 vx = -std::sin(yawRad) * m_speed;
    f32 vz = std::cos(yawRad) * m_speed;
    setVelocity(vx, velocityY(), vz);

    // 应用旋转
    m_yaw += m_rotationVelocity * 57.295776f; // 转换为角度
}

} // namespace entity
} // namespace mc
