/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ItemParticle.hpp"
#include "common/physics/PhysicsConstants.hpp"

namespace mc::client::renderer::trident::particle::particles {

ItemParticle::ItemParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE * (0.5 + m_random.nextFloat() * 0.5))
{
    setGravity(DEFAULT_GRAVITY);
    setSize(m_initialSize);

    // 使用纹理原色
    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    setFriction(FRICTION);
    setHasPhysics(true);
    setMaxAge(DEFAULT_LIFETIME * (0.8 + m_random.nextFloat() * 0.4));
}

std::unique_ptr<Particle> ItemParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // TODO: 集成 ItemModelCache 以支持物品纹理渲染
    return std::make_unique<ItemParticle>(pos, velocity);
}

void ItemParticle::tick(mc::client::ClientWorld* world)
{
    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= static_cast<f32>(m_gravity * mc::physics::PARTICLE_GRAVITY_MULTIPLIER);

    // 随机旋转
    m_roll += 0.1;

    // 移动并碰撞
    if (m_hasPhysics) {
        move(world, m_velocity);
    } else {
        m_position += m_velocity;
    }

    // 阻力衰减
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // 地面摩擦
    if (onGround()) {
        m_velocity.x *= 0.7f;
        m_velocity.z *= 0.7f;
    }

    // 70% 生命周期后淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.7) {
        m_color.a = static_cast<f32>(1.0 - (lifeRatio - 0.7) / 0.3);
    }
}

f64 ItemParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
