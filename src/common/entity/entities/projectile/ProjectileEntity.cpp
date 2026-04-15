#include "ProjectileEntity.hpp"

#include "ProjectileHelper.hpp"
#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/ray/Raycast.hpp"
#include "../../../world/IWorld.hpp"

#include <cmath>

namespace mc {
namespace entity {

ProjectileEntity::ProjectileEntity(LegacyEntityType type, EntityId id)
    : Entity(type, id)
{
    m_noGravity = false;
}

void ProjectileEntity::tick()
{
    if (!m_leftShooter) {
        m_leftShooter = checkLeftShooter();
    }

    const RayTraceResult result = performRayTrace();
    if (result.type != RayTraceResultType::Miss) {
        onImpact(result);
        if (isRemoved()) {
            Entity::tick();
            return;
        }
    }

    Vector3 velocity = m_velocity;
    if (isInWater()) {
        // TODO: 接入投掷物气泡粒子
        velocity = velocity * getWaterDrag();
    } else {
        velocity = velocity * getAirDrag();
    }

    if (!m_noGravity) {
        velocity.y -= getGravity();
    }

    m_velocity = velocity;
    m_prevPosition = m_position;
    m_position = m_position + velocity;

    updateRotation();
    Entity::tick();
}

Entity* ProjectileEntity::getShooter() const
{
    if (m_world == nullptr || m_shooterEntityId == INVALID_ENTITY_ID) {
        return nullptr;
    }

    return m_world->getEntity(m_shooterEntityId);
}

void ProjectileEntity::setShooter(Entity* shooter)
{
    if (shooter == nullptr) {
        m_shooterUuid.clear();
        m_shooterEntityId = INVALID_ENTITY_ID;
        return;
    }

    m_shooterUuid = shooter->uuid();
    m_shooterEntityId = shooter->id();
}

void ProjectileEntity::shoot(f32 x, f32 y, f32 z, f32 velocity, f32 inaccuracy)
{
    f32 length = std::sqrt(x * x + y * y + z * z);
    if (length > 0.0f) {
        x /= length;
        y /= length;
        z /= length;
    }

    if (inaccuracy > 0.0f) {
        // TODO: 接入随机源后补齐 1.16.5 高斯散布
    }

    m_velocity = Vector3(x * velocity, y * velocity, z * velocity);

    const f32 horizontalLength = std::sqrt(x * x + z * z);
    m_yaw = std::atan2(x, z) * math::RAD_TO_DEG;
    m_pitch = std::atan2(y, horizontalLength) * math::RAD_TO_DEG;
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
}

void ProjectileEntity::shootFrom(
    Entity& shooter,
    f32 pitch,
    f32 yaw,
    f32 pitchOffset,
    f32 velocity,
    f32 inaccuracy)
{
    const f32 radPitch = pitch * math::DEG_TO_RAD;
    const f32 radYaw = yaw * math::DEG_TO_RAD;
    const f32 radOffset = pitchOffset * math::DEG_TO_RAD;

    const f32 cosYaw = std::cos(radYaw);
    const f32 sinYaw = std::sin(radYaw);
    const f32 cosPitch = std::cos(radPitch);
    const f32 sinPitch = std::sin(radPitch);
    const f32 sinOffset = std::sin(radOffset);

    const f32 x = -sinYaw * cosPitch;
    const f32 y = -sinPitch - sinOffset;
    const f32 z = cosYaw * cosPitch;

    shoot(x, y, z, velocity, inaccuracy);

    const Vector3 shooterVelocity = shooter.velocity();
    if (!shooter.onGround()) {
        m_velocity.y += shooterVelocity.y;
    }
    m_velocity.x += shooterVelocity.x;
    m_velocity.z += shooterVelocity.z;
}

bool ProjectileEntity::canHitEntity(const mc::Entity& target) const
{
    if (!target.isAlive() || target.isRemoved()) {
        return false;
    }

    if (!target.canBeCollidedWith()) {
        return false;
    }

    const Entity* shooter = getShooter();
    if (!m_leftShooter && shooter != nullptr && shooter == &target) {
        return false;
    }

    return true;
}

void ProjectileEntity::onEntityHit(const RayTraceResult& result)
{
    (void)result;
}

void ProjectileEntity::onBlockHit(const RayTraceResult& result)
{
    m_velocity = Vector3(0.0f, 0.0f, 0.0f);
    (void)result;
}

void ProjectileEntity::onImpact(const RayTraceResult& result)
{
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

void ProjectileEntity::updateRotation()
{
    ProjectileHelper::rotateTowardsMovement(*this, 0.2f);
}

bool ProjectileEntity::checkLeftShooter()
{
    Entity* shooter = getShooter();
    if (shooter == nullptr) {
        return true;
    }

    return !boundingBox().intersects(shooter->boundingBox());
}

RayTraceResult ProjectileEntity::performRayTrace()
{
    const Vector3 start = m_position;
    Vector3 end = m_position + m_velocity;

    const RayTraceResult blockResult = rayTraceBlocks(start, end);
    if (blockResult.type == RayTraceResultType::Block) {
        end = blockResult.hitPosition;
    }

    const RayTraceResult entityResult = rayTraceEntities(start, end);
    if (entityResult.type == RayTraceResultType::Entity) {
        return entityResult;
    }

    return blockResult;
}

RayTraceResult ProjectileEntity::rayTraceEntities(const Vector3& start, const Vector3& end)
{
    if (m_world == nullptr) {
        return RayTraceResult::miss();
    }

    const AxisAlignedBB searchBox =
        ProjectileHelper::createMovementSearchBox(*this, end - start, 1.0f);

    return ProjectileHelper::rayTraceEntities(
        *m_world,
        *this,
        start,
        end,
        searchBox,
        [this](const Entity& candidate) {
            return canHitEntity(candidate);
        });
}

RayTraceResult ProjectileEntity::rayTraceBlocks(const Vector3& start, const Vector3& end)
{
    if (m_world == nullptr) {
        return RayTraceResult::miss();
    }

    const Vector3 delta = end - start;
    if (delta.lengthSquared() <= 1.0e-6f) {
        return RayTraceResult::miss();
    }

    const RaycastContext context(Ray(start, delta.normalized()), delta.length());
    const BlockRaycastResult blockResult = raycastBlocks(context, *m_world);
    if (blockResult.isMiss()) {
        return RayTraceResult::miss();
    }

    return RayTraceResult::block(blockResult.hitPosition(), blockResult.blockPos());
}

} // namespace entity
} // namespace mc
