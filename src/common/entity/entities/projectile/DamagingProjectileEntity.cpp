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
        // TODO: 接入火球/龙息/凋灵头的水下粒子反馈
    }

    m_velocity = Vector3((velocity.x + m_accelerationX) * motionFactor,
        (velocity.y + m_accelerationY) * motionFactor,
        (velocity.z + m_accelerationZ) * motionFactor);

    spawnTrailParticles(Vector3(nextPosition.x, nextPosition.y + 0.5f, nextPosition.z));

    m_prevPosition = m_position;
    m_position = nextPosition;

    Entity::tick();
}

void DamagingProjectileEntity::spawnTrailParticles(const Vector3& /*position*/)
{
    // TODO: 接入 projectile 粒子系统后补齐烟雾、龙息与凋灵头拖尾
}

} // namespace entity
} // namespace mc
