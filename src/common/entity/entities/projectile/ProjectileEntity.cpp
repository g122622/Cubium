#include "ProjectileEntity.hpp"

#include "../../../util/math/MathUtils.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../util/math/ray/Raycast.hpp"
#include "../../../world/IWorld.hpp"
#include "ProjectileHelper.hpp"

#include <cmath>

namespace mc {
namespace entity {

namespace {

// 辅助函数：基于实体ID和tick创建随机数生成器
math::Random createRandomFromEntity(const Entity& entity)
{
    // 使用实体ID和存活时间作为种子
    u64 seed = static_cast<u64>(entity.id()) << 32 | static_cast<u64>(entity.ticksExisted());
    return math::Random(seed);
}

} // anonymous namespace

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
        // 水中阻力（子类可重写 getWaterDrag()）
        // 水中气泡粒子由子类（ThrowableEntity、AbstractArrowEntity 等）自行处理
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

    // 参考 MC 1.16.5 ProjectileEntity.shoot() 第97行
    // 高斯随机散布
    if (inaccuracy > 0.0f) {
        math::Random rng = createRandomFromEntity(*this);
        // MC 1.16.5: this.rand.nextGaussian() * (double)0.0075F * (double)inaccuracy
        f32 gaussianX = static_cast<f32>(rng.nextGaussian()) * 0.0075f * inaccuracy;
        f32 gaussianY = static_cast<f32>(rng.nextGaussian()) * 0.0075f * inaccuracy;
        f32 gaussianZ = static_cast<f32>(rng.nextGaussian()) * 0.0075f * inaccuracy;
        x += gaussianX;
        y += gaussianY;
        z += gaussianZ;
    }

    m_velocity = Vector3(x * velocity, y * velocity, z * velocity);

    const f32 horizontalLength = std::sqrt(x * x + z * z);
    m_yaw = std::atan2(x, z) * math::RAD_TO_DEG;
    m_pitch = std::atan2(y, horizontalLength) * math::RAD_TO_DEG;
    m_prevYaw = m_yaw;
    m_prevPitch = m_pitch;
}

void ProjectileEntity::shootFrom(Entity& shooter, f32 pitch, f32 yaw, f32 pitchOffset, f32 velocity, f32 inaccuracy)
{
    // 参考 MC 1.16.5 ProjectileEntity.func_234612_a_() 第106-113行
    const f32 radPitch = pitch * math::DEG_TO_RAD;
    const f32 radYaw = yaw * math::DEG_TO_RAD;
    const f32 radOffset = pitchOffset * math::DEG_TO_RAD;

    const f32 cosYaw = std::cos(radYaw);
    const f32 sinYaw = std::sin(radYaw);
    const f32 cosPitch = std::cos(radPitch);
    const f32 sinPitch = std::sin(radPitch);
    const f32 sinOffset = std::sin(radOffset);

    // MC 1.16.5 计算方向向量
    const f32 x = -sinYaw * cosPitch;
    const f32 y = -sinPitch - sinOffset;
    const f32 z = cosYaw * cosPitch;

    shoot(x, y, z, velocity, inaccuracy);

    // MC 1.16.5: 添加发射者速度
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
    if (!m_leftShooter && shooter != nullptr) {
        // 参考 MC 1.16.5 ProjectileEntity.func_230298_a_() 第158-159行
        // 检查是否骑乘同一实体
        // if (entity.isRidingSameEntity(shooter)) {
        //     return false;
        // }
        // 简化检查：直接比较
        if (shooter == &target) {
            return false;
        }
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

    // 参考 MC 1.16.5 ProjectileEntity.func_234615_h_()
    // 检查投掷物是否已离开发射者的碰撞箱
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

    const AxisAlignedBB searchBox = ProjectileHelper::createMovementSearchBox(*this, end - start, 1.0f);

    return ProjectileHelper::rayTraceEntities(
        *m_world, *this, start, end, searchBox, [this](const Entity& candidate) { return canHitEntity(candidate); });
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

    // 参考 MC 1.16.5: 使用 COLLIDER 模式进行射线追踪
    const RaycastContext context(Ray(start, delta.normalized()), delta.length());
    const BlockRaycastResult blockResult = raycastBlocks(context, *m_world);
    if (blockResult.isMiss()) {
        return RayTraceResult::miss();
    }

    return RayTraceResult::block(blockResult.hitPosition(), blockResult.blockPos());
}

} // namespace entity
} // namespace mc
