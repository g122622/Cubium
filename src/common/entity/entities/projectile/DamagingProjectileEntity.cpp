/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "DamagingProjectileEntity.hpp"

#include "ProjectileHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/ecs/components/DamagingProjectileComponent.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace entity {

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = ProjectileEntity::classInfo()）。
// 本类无同步字段，classInfo 仅作父链遍历节点供子类（AbstractFireball → Fireball/WitherSkull）
// ClassRegisterGuard 沿父链查找最高 id 时穿过。
const EntityClassInfo& DamagingProjectileEntity::classInfo()
{
    static const EntityClassInfo s_classInfo{"DamagingProjectileEntity", &ProjectileEntity::classInfo()};
    return s_classInfo;
}

DamagingProjectileEntity::DamagingProjectileEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ProjectileEntity(id, registry)
{
    setNoGravity(true);
    // 批次6 子目标2 Step1：attach DamagingProjectileComponent（加速度+伤害 4 字段）。
    // Fireball/SmallFireball/DragonFireball/WitherSkull 经 AbstractFireballEntity 继承本类，
    // 故均获得此组件。Step4 将把 m_accelerationX/Y/Z/m_damage 读写改走组件。
    m_entityContext->enttRegistry().emplace<ecs::DamagingProjectileComponent>(m_entityContext->entity());
}

// 批次6 子目标2 Step4：以下 getter/setter 经 ecs::DamagingProjectileComponent 读写。
f32 DamagingProjectileEntity::accelerationX() const
{
    const auto* c = tryGetComponent<ecs::DamagingProjectileComponent>();
    return (c != nullptr) ? c->m_directionX : 0.0f;
}

f32 DamagingProjectileEntity::accelerationY() const
{
    const auto* c = tryGetComponent<ecs::DamagingProjectileComponent>();
    return (c != nullptr) ? c->m_directionY : 0.0f;
}

f32 DamagingProjectileEntity::accelerationZ() const
{
    const auto* c = tryGetComponent<ecs::DamagingProjectileComponent>();
    return (c != nullptr) ? c->m_directionZ : 0.0f;
}

f32 DamagingProjectileEntity::accelerationPower() const
{
    const auto* c = tryGetComponent<ecs::DamagingProjectileComponent>();
    return (c != nullptr) ? c->m_accelerationPower : 0.1f;
}

void DamagingProjectileEntity::setAccelerationPower(f32 power)
{
    if (auto* c = tryGetComponent<ecs::DamagingProjectileComponent>(); c != nullptr) {
        c->m_accelerationPower = power;
    }
}

void DamagingProjectileEntity::setAcceleration(f32 x, f32 y, f32 z)
{
    auto* c = tryGetComponent<ecs::DamagingProjectileComponent>();
    if (c == nullptr) {
        return;
    }
    c->m_directionX = x;
    c->m_directionY = y;
    c->m_directionZ = z;
    // 对齐 vanilla AbstractHurtingProjectile.assignDirectionalMovement：
    // 初速度 = normalize(direction) * accelerationPower，零向量时速度置零。
    const Vector3 direction(x, y, z);
    const f32 length = direction.length();
    if (length > 1.0e-6f) {
        c->m_accelerationPower = 0.1f; // vanilla 构造期即默认值，方向重设时同步复位
        m_builtIn.velocity->m_velocity = direction * (c->m_accelerationPower / length);
    } else {
        m_builtIn.velocity->m_velocity = Vector3(0.0f, 0.0f, 0.0f);
    }
}

f32 DamagingProjectileEntity::damage() const
{
    const auto* c = tryGetComponent<ecs::DamagingProjectileComponent>();
    return (c != nullptr) ? c->m_damage : 0.0f;
}

void DamagingProjectileEntity::setDamage(f32 damage)
{
    auto* c = tryGetComponent<ecs::DamagingProjectileComponent>();
    if (c != nullptr) {
        c->m_damage = damage;
    }
}

void DamagingProjectileEntity::tick()
{
    tryUpdateLeftShooter();

    // 火球类实体每 tick 燃烧 1 秒（20 ticks）
    if (isFiery()) {
        igniteForTicks(20);
    }

    const RayTraceResult result = performRayTrace();
    if (result.type != RayTraceResultType::Miss) {
        onImpact(result);
        if (isRemoved()) {
            Entity::tick();
            return;
        }
    }

    const Vector3 velocity = m_builtIn.velocity->m_velocity;
    const Vector3 nextPosition = m_builtIn.stateVector->m_pos + velocity;

    ProjectileHelper::rotateTowardsMovement(*this, 0.2f);

    f32 motionFactor = getMotionFactor();
    if (isInWater()) {
        motionFactor = getLiquidInertia();
        // 水中生成气泡粒子尾迹
        spawnWaterParticles();
    }

    // 对齐 vanilla AbstractHurtingProjectile：
    //   setDeltaMovement(v.add(v.normalize().scale(accelerationPower)).scale(f))
    // 即沿"当前速度方向"施加加速力，而非固定发射方向；零速度时不加速。
    const f32 power = accelerationPower();
    f32 accelX = 0.0f;
    f32 accelY = 0.0f;
    f32 accelZ = 0.0f;
    const Vector3 currentVelocity = velocity;
    const f32 velocityLength = currentVelocity.length();
    if (velocityLength > 1.0e-6f) {
        const f32 scale = power / velocityLength;
        accelX = currentVelocity.x * scale;
        accelY = currentVelocity.y * scale;
        accelZ = currentVelocity.z * scale;
    }
    m_builtIn.velocity->m_velocity = Vector3((velocity.x + accelX) * motionFactor,
        (velocity.y + accelY) * motionFactor,
        (velocity.z + accelZ) * motionFactor);

    // 生成拖尾粒子，位置 Y+0.5 偏移
    spawnTrailParticles(Vector3(nextPosition.x, nextPosition.y + 0.5f, nextPosition.z));

    m_builtIn.stateVector->m_posPrev = m_builtIn.stateVector->m_pos;
    m_builtIn.stateVector->m_pos = nextPosition;

    Entity::tick();
}

particle::ParticleTypeId DamagingProjectileEntity::getParticleType() const
{
    // 默认返回 SMOKE 粒子
    return particle::ParticleTypeId::Smoke;
}

void DamagingProjectileEntity::spawnTrailParticles(const Vector3& position)
{
    // 与本模块其它抛射物（AbstractArrowEntity / TridentEntity / OtherProjectiles 等）一致：
    // Cubium 允许 m_world == nullptr（测试/反序列化中间态），凡解引用 m_world 前都需判空。
    // MC Java 侧 DamagingProjectileEntity.tick 按生命周期不变量保证 level 非空，此处不可照搬。
    // 不判空时单元测试以无世界 FireballEntity 调用 tick 会在此解引用空指针崩溃。
    if (m_world != nullptr && m_world->isClientSide()) {
        m_world->addParticle(getParticleType(), position, Vector3(0.0f, 0.0f, 0.0f));
    }
}

void DamagingProjectileEntity::spawnWaterParticles()
{
    // 水中每 tick 生成 4 个气泡粒子
    if (m_world != nullptr && m_world->isClientSide()) {
        for (i32 i = 0; i < 4; ++i) {
            constexpr f32 offset = 0.25f;
            Vector3 pos(x() - m_builtIn.velocity->m_velocity.x * offset,
                y() - m_builtIn.velocity->m_velocity.y * offset,
                z() - m_builtIn.velocity->m_velocity.z * offset);
            m_world->addParticle(particle::ParticleTypeId::Bubble, pos, m_builtIn.velocity->m_velocity);
        }
    }
}

} // namespace entity
} // namespace mc
