#include "MinecartEntity.hpp"
#include "../../world/IWorld.hpp"
#include "../../entities/player/Player.hpp"
#include <cmath>

namespace mc {
namespace entity {

AbstractMinecartEntity::AbstractMinecartEntity(Type type)
    : Entity()
    , m_type(type)
{
    setSize(0.98f, 0.7f);
    setMaxHealth(4.0f);
}

void AbstractMinecartEntity::tick() {
    Entity::tick();

    // 检查是否在铁轨上
    adjustOnRail();

    if (m_onRail) {
        // 在铁轨上移动
        handleRailLogic();
        moveAlongRail(m_maxSpeed);
    } else {
        // 不在铁轨上，自由移动
        m_velocityX *= FRICTION;
        m_velocityZ *= FRICTION;
        move(m_velocityX, m_velocityY, m_velocityZ);
    }

    // 损坏处理
    if (m_damage > 0) {
        m_damage--;
    }
}

void AbstractMinecartEntity::onEntityCollision(Entity& other) {
    // 矿车碰撞逻辑
    if (m_type == Type::RIDEABLE && getPassengers().empty()) {
        // 空矿车可以被推
        f64 dx = other.x() - x();
        f64 dz = other.z() - z();
        f64 dist = std::sqrt(dx * dx + dz * dz);
        if (dist > 0.0) {
            applyForce(static_cast<f32>(-dx / dist * 0.1), static_cast<f32>(-dz / dist * 0.1));
        }
    }
}

void AbstractMinecartEntity::onRiderMounted(Entity& rider) {
    Entity::onRiderMounted(rider);
}

void AbstractMinecartEntity::onRiderDismounted(Entity& rider) {
    Entity::onRiderDismounted(rider);
}

std::vector<Entity*> AbstractMinecartEntity::getPassengers() const {
    return Entity::getPassengers();
}

void AbstractMinecartEntity::adjustOnRail() {
    // TODO: 检查当前位置是否在铁轨上
    // BlockState* block = world->getBlockState(position());
    // m_onRail = block && block->isRail();
    m_onRail = false;
}

void AbstractMinecartEntity::moveAlongRail(f32 distance) {
    // 计算移动方向
    f32 yawRad = m_yaw * 3.14159265f / 180.0f;
    m_velocityX = -std::sin(yawRad) * distance;
    m_velocityZ = std::cos(yawRad) * distance;

    // 移动
    move(m_velocityX, m_velocityY, m_velocityZ);
}

void AbstractMinecartEntity::dropItem() {
    // TODO: 根据类型掉落对应物品
}

void AbstractMinecartEntity::activate() {
    // 默认无操作，子类重写
}

void AbstractMinecartEntity::applyForce(f32 x, f32 z) {
    m_velocityX += x;
    m_velocityZ += z;
}

void AbstractMinecartEntity::handleRailLogic() {
    // 计算轨道方向
    calculateRailDirection();

    // 处理动力轨道
    if (isPoweredRail()) {
        m_maxSpeed = DEFAULT_MAX_SPEED + POWERED_RAIL_SPEED;
    } else {
        m_maxSpeed = DEFAULT_MAX_SPEED;
    }

    // 处理激活轨道
    if (isActivatorRail()) {
        activate();
    }

    // 处理轨道分支
    handleRailJunction();
}

void AbstractMinecartEntity::calculateRailDirection() {
    // TODO: 根据铁轨方块状态计算方向
    // BlockState* rail = world->getBlockState(position());
    // m_railDirection = rail->getProperty("shape");
}

void AbstractMinecartEntity::handleRailJunction() {
    // TODO: 处理铁轨交叉点
}

bool AbstractMinecartEntity::isPoweredRail() const {
    // TODO: 检查是否为动力铁轨
    return false;
}

bool AbstractMinecartEntity::isDetectorRail() const {
    // TODO: 检查是否为探测铁轨
    return false;
}

bool AbstractMinecartEntity::isActivatorRail() const {
    // TODO: 检查是否为激活铁轨
    return false;
}

void CommandBlockMinecartEntity::executeCommand() {
    // TODO: 执行命令方块命令
    // CommandSource source = CommandSource::fromEntity(this);
    // m_successCount = world->getServer()->getCommandManager()->execute(m_command, source);
}

} // namespace entity
} // namespace mc
