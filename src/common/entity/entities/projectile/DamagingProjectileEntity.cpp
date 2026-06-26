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
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/world/IWorld.hpp"

namespace mc {
namespace entity {

DamagingProjectileEntity::DamagingProjectileEntity(EntityId id)
    : ProjectileEntity(id)
{
    setNoGravity(true);
}

void DamagingProjectileEntity::tick()
{
    if (!m_leftShooter) {
        m_leftShooter = checkLeftShooter();
    }

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

    const Vector3 velocity = m_velocity;
    const Vector3 nextPosition = m_position + velocity;

    ProjectileHelper::rotateTowardsMovement(*this, 0.2f);

    f32 motionFactor = getMotionFactor();
    if (isInWater()) {
        motionFactor = 0.8f;
        // 水中生成气泡粒子尾迹
        spawnWaterParticles();
    }

    m_velocity = Vector3((velocity.x + m_accelerationX) * motionFactor,
        (velocity.y + m_accelerationY) * motionFactor,
        (velocity.z + m_accelerationZ) * motionFactor);

    // 生成拖尾粒子，位置 Y+0.5 偏移
    spawnTrailParticles(Vector3(nextPosition.x, nextPosition.y + 0.5f, nextPosition.z));

    m_prevPosition = m_position;
    m_position = nextPosition;

    Entity::tick();
}

client::renderer::trident::particle::ParticleTypeId DamagingProjectileEntity::getParticleType() const
{
    // 默认返回 SMOKE 粒子
    return client::renderer::trident::particle::ParticleTypeId::Smoke;
}

void DamagingProjectileEntity::spawnTrailParticles(const Vector3& position)
{
    if (m_world->isClientSide()) {
        m_world->addParticle(getParticleType(), position, Vector3(0.0f, 0.0f, 0.0f));
    }
}

void DamagingProjectileEntity::spawnWaterParticles()
{
    // 水中每 tick 生成 4 个气泡粒子
    if (m_world->isClientSide()) {
        for (i32 i = 0; i < 4; ++i) {
            constexpr f32 offset = 0.25f;
            Vector3 pos(x() - m_velocity.x * offset, y() - m_velocity.y * offset, z() - m_velocity.z * offset);
            m_world->addParticle(client::renderer::trident::particle::ParticleTypeId::Bubble, pos, m_velocity);
        }
    }
}

} // namespace entity
} // namespace mc
