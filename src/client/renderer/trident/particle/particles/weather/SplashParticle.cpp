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

#include "SplashParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

SplashParticle::SplashParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.5 + static_cast<f64>(m_random.nextFloat()) * 0.5));
    setColor(glm::vec4(0.8f, 0.9f, 1.0f, 0.7f)); // 淡蓝色半透明

    setFriction(0.95);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7 + static_cast<f64>(m_random.nextFloat()) * 0.6));
}

std::unique_ptr<Particle> SplashParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SplashParticle>(pos, velocity);
}

void SplashParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= m_gravity * 0.04;

    // 限制速度
    if (m_velocity.y < -0.3) {
        m_velocity.y = -0.3;
    }

    m_position += m_velocity;
    m_velocity.x *= m_friction;
    m_velocity.z *= m_friction;

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.5) {
        m_color.a = static_cast<f32>(0.7 * (1.0 - (lifeRatio - 0.5) / 0.5));
    }
}

} // namespace mc::client::renderer::trident::particle::particles
