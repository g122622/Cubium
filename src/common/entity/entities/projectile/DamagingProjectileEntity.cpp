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

DamagingProjectileEntity::DamagingProjectileEntity(LegacyEntityType type, EntityId id)
    : ProjectileEntity(type, id)
{
    setNoGravity(true);
}

void DamagingProjectileEntity::tick()
{
    if (!m_leftShooter) {
        m_leftShooter = checkLeftShooter();
    }

    // MC 1.16.5: 火球类实体每 tick 燃烧 1 秒（20 ticks）
    if (isFiery()) {
        setFire(20); // 1 秒 = 20 ticks
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
        // MC 1.16.5 DamagingProjectileEntity.tick() 第88-95行
        // 水中生成气泡粒子尾迹
        spawnWaterParticles();
    }

    m_velocity = Vector3((velocity.x + m_accelerationX) * motionFactor,
        (velocity.y + m_accelerationY) * motionFactor,
        (velocity.z + m_accelerationZ) * motionFactor);

    // MC 1.16.5 DamagingProjectileEntity.tick() 第98行
    // 生成拖尾粒子，位置 Y+0.5 偏移
    spawnTrailParticles(Vector3(nextPosition.x, nextPosition.y + 0.5f, nextPosition.z));

    m_prevPosition = m_position;
    m_position = nextPosition;

    Entity::tick();
}

client::renderer::trident::particle::ParticleTypeId DamagingProjectileEntity::getParticleType() const
{
    // MC 1.16.5 DamagingProjectileEntity.getParticle()
    // 默认返回 SMOKE 粒子
    return client::renderer::trident::particle::ParticleTypeId::Smoke;
}

void DamagingProjectileEntity::spawnTrailParticles(const Vector3& position)
{
    // MC 1.16.5 DamagingProjectileEntity.tick() 第98行
    // world.addParticle(this.getParticle(), d0, d1 + 0.5D, d2, 0.0D, 0.0D, 0.0D);
    if (m_world != nullptr && m_world->isClientSide()) {
        m_world->addParticle(getParticleType(), position, Vector3(0.0f, 0.0f, 0.0f));
    }
}

void DamagingProjectileEntity::spawnWaterParticles()
{
    // MC 1.16.5 DamagingProjectileEntity.tick() 第88-95行
    // if (this.isInWater()) {
    //     for(int i = 0; i < 4; ++i) {
    //         this.world.addParticle(ParticleTypes.BUBBLE,
    //             d0 - vector3d.x * 0.25D,
    //             d1 - vector3d.y * 0.25D,
    //             d2 - vector3d.z * 0.25D,
    //             vector3d.x, vector3d.y, vector3d.z);
    //     }
    // }
    if (m_world != nullptr && m_world->isClientSide()) {
        for (int i = 0; i < 4; ++i) {
            f32 offset = 0.25f;
            Vector3 pos(
                x() - m_velocity.x * offset,
                y() - m_velocity.y * offset,
                z() - m_velocity.z * offset);
            m_world->addParticle(
                client::renderer::trident::particle::ParticleTypeId::Bubble,
                pos,
                m_velocity);
        }
    }
}

} // namespace entity
} // namespace mc
