#include "BoatEntity.hpp"
#include "../../world/IWorld.hpp"
#include "../../entities/player/Player.hpp"
#include <cmath>

namespace mc {
namespace entity {

BoatEntity::BoatEntity(Type type)
    : Entity()
    , m_type(type)
{
    setSize(1.375f, 0.5625f);
    setMaxHealth(4.0f);
}

void BoatEntity::tick() {
    Entity::tick();

    // 检查是否在水中
    m_inWater = isInWater();

    // 更新运动状态
    updateMotion();

    // 浮力处理
    floatBoat();

    // 控制船的移动
    controlBoat();

    // 移动
    move(m_velocityX, m_velocityY, m_velocityZ);

    // 减速
    if (m_onLand) {
        m_speed *= 0.5f;
        m_rotationVelocity *= 0.5f;
    } else {
        m_speed *= 0.95f;
        m_rotationVelocity *= 0.95f;
    }
}

void BoatEntity::onEntityCollision(Entity& other) {
    // 船与实体碰撞时推动实体
    if (&other != getRider()) {
        f64 dx = other.x() - x();
        f64 dz = other.z() - z();
        f64 dist = std::sqrt(dx * dx + dz * dz);
        if (dist > 0.0) {
            f32 pushStrength = 0.05f;
            other.addVelocity(dx / dist * pushStrength, 0.0, dz / dist * pushStrength);
        }
    }
}

void BoatEntity::onRiderMounted(Entity& rider) {
    Entity::onRiderMounted(rider);
}

void BoatEntity::onRiderDismounted(Entity& rider) {
    Entity::onRiderDismounted(rider);
}

std::vector<Entity*> BoatEntity::getPassengers() const {
    return Entity::getPassengers();
}

void BoatEntity::setRotation(f32 yaw) {
    Entity::setRotation(yaw, m_pitch);
}

void BoatEntity::handleInput(f32 forward, f32 backward, f32 left, f32 right) {
    m_forwardInput = forward - backward;
    m_turnInput = right - left;
}

bool BoatEntity::canAddPassenger(Entity& rider) const {
    // 只能有一个乘客
    return getPassengers().size() < 1;
}

void BoatEntity::dropItem() {
    // TODO: 生成对应木材类型的船物品
    // ItemEntity* item = spawnItem(Items::BOATS[static_cast<u8>(m_type)]);
}

void BoatEntity::updateMotion() {
    // 根据是否在水中调整运动
    if (m_inWater) {
        // 水中漂浮
        m_velocityY += 0.03999999910593033;
    } else if (m_onLand) {
        // 陆地摩擦
        m_velocityX *= 0.5;
        m_velocityY *= 0.5;
        m_velocityZ *= 0.5;
    }
}

void BoatEntity::floatBoat() {
    // 计算浮力
    // TODO: 从世界获取水位高度
    // double waterLevel = world->getWaterLevel(position());
    // if (m_inWater) {
    //     double buoyancy = waterLevel - position().y + 0.4;
    //     m_velocityY += buoyancy * 0.05;
    // }
}

void BoatEntity::controlBoat() {
    if (getRider() != nullptr) {
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
    }

    // 应用速度
    f32 yawRad = m_yaw * 3.14159265f / 180.0f;
    m_velocityX = -std::sin(yawRad) * m_speed;
    m_velocityZ = std::cos(yawRad) * m_speed;

    // 应用旋转
    m_yaw += m_rotationVelocity * 57.295776f; // 转换为角度
}

} // namespace entity
} // namespace mc
