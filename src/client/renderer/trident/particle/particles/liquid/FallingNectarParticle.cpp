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

#include "FallingNectarParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

FallingNectarParticle::FallingNectarParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(0.04 + m_random.nextFloat() * 0.02);

    // 金色琥珀色
    f32 r = 0.9f + m_random.nextFloat() * 0.1f;
    f32 g = 0.7f + m_random.nextFloat() * 0.2f;
    f32 b = 0.2f + m_random.nextFloat() * 0.1f;
    setColor(glm::vec4(r, g, b, 1.0f));

    setFriction(0.98);
    setHasPhysics(true);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 10.0);
}

std::unique_ptr<Particle> FallingNectarParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<FallingNectarParticle>(pos, velocity);
}

void FallingNectarParticle::tick(mc::client::ClientWorld* world)
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

    // 80% 生命后淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.8) {
        f64 fadeRatio = (lifeRatio - 0.8) / 0.2;
        m_color.a = static_cast<f32>(1.0 - fadeRatio);
    }
}

} // namespace mc::client::renderer::trident::particle::particles
