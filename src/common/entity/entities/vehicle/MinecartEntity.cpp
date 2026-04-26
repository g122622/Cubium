#include "MinecartEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../player/Player.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>

namespace mc {
namespace entity {

AbstractMinecartEntity::AbstractMinecartEntity(Type type)
    : Entity(LegacyEntityType::Item, EntityId(0)) // Using Item type temporarily
    , m_type(type)
{
    // 尺寸通过 width()/height() 设置
}

void AbstractMinecartEntity::tick() {
    Entity::tick();

    // 检查是否在铁轨上
    adjustOnRail();

    Vector3 vel = velocity();
    if (m_onRail) {
        // 在铁轨上移动
        handleRailLogic();
        moveAlongRail(m_maxSpeed);
    } else {
        // 不在铁轨上，自由移动
        vel.x *= FRICTION;
        vel.z *= FRICTION;
        setVelocity(vel);
        move(vel.x, vel.y, vel.z);
    }

    // 损坏处理
    if (m_damage > 0) {
        m_damage--;
    }
}

void AbstractMinecartEntity::adjustOnRail() {
    // TODO: 检查当前位置是否在铁轨上
    // BlockState* block = world->getBlockState(position());
    // m_onRail = block && block->isRail();
    m_onRail = false;
}

void AbstractMinecartEntity::moveAlongRail(f32 distance) {
    // 计算移动方向
    f32 yawRad = mc::math::toRadians(m_yaw);
    f32 vx = -std::sin(yawRad) * distance;
    f32 vz = std::cos(yawRad) * distance;

    // 移动
    move(vx, velocityY(), vz);
}

void AbstractMinecartEntity::dropItem() {
    // TODO: 根据类型掉落对应物品
}

void AbstractMinecartEntity::activate() {
    // 默认无操作，子类重写
}

void AbstractMinecartEntity::applyForce(f32 x, f32 z) {
    addVelocity(x, 0.0f, z);
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
