/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SpitParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

SpitParticle::SpitParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(0.05 + m_random.nextFloat() * 0.02);

    // 白黄色
    setColor(glm::vec4(0.9f, 0.85f, 0.7f, 1.0f));

    setFriction(0.96);
    setHasPhysics(true);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 10.0);
}

std::unique_ptr<Particle> SpitParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SpitParticle>(pos, velocity);
}

void SpitParticle::tick(mc::client::ClientWorld* world)
{
    m_prevPosition = m_position;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 重力
    m_velocity.y -= static_cast<f32>(m_gravity * mc::physics::PARTICLE_GRAVITY_MULTIPLIER);

    // 碰撞移动
    if (m_hasPhysics && world != nullptr) {
        move(world, m_velocity);
    } else {
        m_position += m_velocity;
    }

    // 地面摩擦
    if (m_collisionContext.onGround) {
        m_velocity.x *= 0.7f;
        m_velocity.z *= 0.7f;
    }

    // 旋转
    m_roll += 0.1;

    // 70% 生命后淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.7) {
        f64 fadeRatio = (lifeRatio - 0.7) / 0.3;
        m_color.a = static_cast<f32>(1.0 - fadeRatio);
    }
}

} // namespace mc::client::renderer::trident::particle::particles
