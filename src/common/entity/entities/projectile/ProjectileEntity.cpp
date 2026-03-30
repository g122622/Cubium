#include "ProjectileEntity.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../util/math/MathUtils.hpp"
#include <cmath>

namespace mc {
namespace entity {

ProjectileEntity::ProjectileEntity(LegacyEntityType type, EntityId id)
    : Entity(type, id)
{
    m_noGravity = false;
}

void ProjectileEntity::tick() {
    // 检查是否离开发射者
    if (!m_leftShooter) {
        m_leftShooter = checkLeftShooter();
    }

    // 执行射线追踪
    RayTraceResult result = performRayTrace();

    // 处理碰撞
    if (result.type != RayTraceResultType::Miss) {
        onImpact(result);
    }

    // 更新物理
    Vector3 velocity = m_velocity;

    // 水中阻力
    if (isInWater()) {
        for (int i = 0; i < 4; ++i) {
            // TODO: 生成气泡粒子
        }
        velocity = velocity * getWaterDrag();
    } else {
        velocity = velocity * getAirDrag();
    }

    // 应用重力
    if (!m_noGravity) {
        velocity.y -= getGravity();
    }

    m_velocity = velocity;

    // 更新位置
    m_prevPosition = m_position;
    m_position = m_position + velocity;

    // 更新旋转
    updateRotation();

    // 调用基类tick
    Entity::tick();
}

Entity* ProjectileEntity::getShooter() const {
    if (m_world) {
        // 先通过UUID查找
        if (!m_shooterUuid.empty()) {
            // TODO: 实现通过UUID查找实体
        }
        // 再通过Entity ID查找
        if (m_shooterEntityId != INVALID_ENTITY_ID) {
            // TODO: 实现通过ID查找实体
        }
    }
    return nullptr;
}

void ProjectileEntity::setShooter(Entity* shooter) {
    if (shooter) {
        m_shooterUuid = shooter->uuid();
        m_shooterEntityId = shooter->id();
    }
}

void ProjectileEntity::shoot(f32 x, f32 y, f32 z, f32 velocity, f32 inaccuracy) {
    // 归一化方向
    f32 length = std::sqrt(x * x + y * y + z * z);
    if (length > 0.0f) {
        x /= length;
        y /= length;
        z /= length;
    }

    // 添加散布
    if (inaccuracy > 0.0f) {
        // 使用高斯分布添加散布
        // TODO: 使用随机数生成器
        // x += (rand.nextGaussian() * 0.0075 * inaccuracy);
        // y += (rand.nextGaussian() * 0.0075 * inaccuracy);
        // z += (rand.nextGaussian() * 0.0075 * inaccuracy);
    }

    // 设置速度
    m_velocity.x = x * velocity;
    m_velocity.y = y * velocity;
    m_velocity.z = z * velocity;

    // 计算旋转
    f32 horizontalLength = std::sqrt(x * x + z * z);
    m_yaw = std::atan2(x, z) * (180.0f / MathUtils::PI);
    m_pitch = std::atan2(y, horizontalLength) * (180.0f / MathUtils::PI);
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
}

void ProjectileEntity::shootFrom(Entity& shooter, f32 pitch, f32 yaw,
                                  f32 pitchOffset, f32 velocity, f32 inaccuracy) {
    // 计算发射方向
    f32 radPitch = pitch * (MathUtils::PI / 180.0f);
    f32 radYaw = yaw * (MathUtils::PI / 180.0f);
    f32 radOffset = pitchOffset * (MathUtils::PI / 180.0f);

    f32 cosYaw = std::cos(radYaw);
    f32 sinYaw = std::sin(radYaw);
    f32 cosPitch = std::cos(radPitch);
    f32 sinPitch = std::sin(radPitch);
    f32 sinOffset = std::sin(radOffset);

    f32 x = -sinYaw * cosPitch;
    f32 y = -sinPitch - sinOffset;
    f32 z = cosYaw * cosPitch;

    shoot(x, y, z, velocity, inaccuracy);

    // 添加发射者的速度
    Vector3 shooterVel = shooter.velocity();
    if (!shooter.onGround()) {
        m_velocity.y += shooterVel.y;
    }
    m_velocity.x += shooterVel.x;
    m_velocity.z += shooterVel.z;
}

bool ProjectileEntity::canHitEntity(const mc::Entity& target) const {
    // 不能命中旁观者
    // 不能命中不可碰撞的实体
    // 如果还没离开发射者，不能命中发射者及其骑乘的实体
    if (!target.isAlive()) {
        return false;
    }

    // TODO: 实现完整的检查
    // - 检查是否是旁观者模式
    // - 检查 canBeCollidedWith

    return true;
}

void ProjectileEntity::onEntityHit(const RayTraceResult& result) {
    // 子类实现
    MC_UNUSED(result);
}

void ProjectileEntity::onBlockHit(const RayTraceResult& result) {
    // 默认实现：停止移动
    m_velocity = Vector3(0.0f, 0.0f, 0.0f);
    MC_UNUSED(result);
}

void ProjectileEntity::onImpact(const RayTraceResult& result) {
    switch (result.type) {
        case RayTraceResultType::Entity:
            onEntityHit(result);
            break;
        case RayTraceResultType::Block:
            onBlockHit(result);
            break;
        case RayTraceResultType::Miss:
        default:
            break;
    }
}

void ProjectileEntity::updateRotation() {
    f32 horizontalLength = std::sqrt(m_velocity.x * m_velocity.x +
                                      m_velocity.z * m_velocity.z);

    // 平滑旋转更新
    f32 targetPitch = std::atan2(m_velocity.y, horizontalLength) *
                      (180.0f / MathUtils::PI);
    f32 targetYaw = std::atan2(m_velocity.x, m_velocity.z) *
                    (180.0f / MathUtils::PI);

    // 角度插值
    while (targetYaw - m_yaw < -180.0f) m_yaw -= 360.0f;
    while (targetYaw - m_yaw >= 180.0f) m_yaw += 360.0f;

    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
    m_yaw = m_yaw + (targetYaw - m_yaw) * 0.2f;
    m_pitch = m_pitch + (targetPitch - m_pitch) * 0.2f;
}

bool ProjectileEntity::checkLeftShooter() {
    Entity* shooter = getShooter();
    if (!shooter) {
        return true;
    }

    // TODO: 实现检查
    // 检查投掷物的碰撞箱是否与发射者的碰撞箱重叠
    // 如果不重叠，则已经离开

    return true;
}

RayTraceResult ProjectileEntity::performRayTrace() {
    Vector3 start = m_position;
    Vector3 end = m_position + m_velocity;

    // 先检测方块
    RayTraceResult blockResult = rayTraceBlocks(start, end);
    if (blockResult.type == RayTraceResultType::Block) {
        end = blockResult.hitPosition;
    }

    // 再检测实体
    RayTraceResult entityResult = rayTraceEntities(start, end);

    // 返回更近的碰撞
    if (entityResult.type == RayTraceResultType::Entity) {
        return entityResult;
    }

    return blockResult;
}

RayTraceResult ProjectileEntity::rayTraceEntities(const Vector3& start,
                                                   const Vector3& end) {
    if (!m_world) {
        return RayTraceResult::miss();
    }

    // TODO: 实现实体射线追踪
    // 遍历范围内的实体，检测射线是否穿过实体的碰撞箱
    // 返回最近的命中的实体

    return RayTraceResult::miss();
}

RayTraceResult ProjectileEntity::rayTraceBlocks(const Vector3& start,
                                                 const Vector3& end) {
    if (!m_world) {
        return RayTraceResult::miss();
    }

    // TODO: 实现方块射线追踪
    // 使用 IWorld 的射线追踪接口

    return RayTraceResult::miss();
}

} // namespace entity
} // namespace mc
