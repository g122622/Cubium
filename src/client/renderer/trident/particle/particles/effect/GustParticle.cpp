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

#include "GustParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// GustParticle
// ============================================================================

GustParticle::GustParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.15 + m_random.nextFloat() * 0.05)
{
    setGravity(0.0f);
    setSize(m_initialSize);
    setFriction(0.94f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 5.0);

    // 白色-淡蓝色
    f32 r = 0.85f + m_random.nextFloat() * 0.15f;
    f32 g = 0.9f + m_random.nextFloat() * 0.1f;
    f32 b = 0.95f + m_random.nextFloat() * 0.05f;
    setColor(glm::vec4(r, g, b, 0.7f));
}

std::unique_ptr<Particle> GustParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<GustParticle>(pos, velocity);
}

void GustParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 随机漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.006f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.006f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 膨胀
    f64 lifeRatio = m_age / m_maxAge;
    f64 expansion = 1.0 + lifeRatio * 2.0;
    setSize(m_initialSize * expansion);

    // 60% 生命周期后淡出
    if (lifeRatio > 0.6f) {
        f64 fadeRatio = (lifeRatio - 0.6) / 0.4;
        m_color.a = static_cast<f32>(0.7 * (1.0 - fadeRatio));
    }
}

f64 GustParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

// ============================================================================
// SmallGustParticle
// ============================================================================

SmallGustParticle::SmallGustParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.08 + m_random.nextFloat() * 0.03)
{
    setGravity(0.0f);
    setSize(m_initialSize);
    setFriction(0.94f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 3.0);

    // 白色
    f32 v = 0.9f + m_random.nextFloat() * 0.1f;
    setColor(glm::vec4(v, v, 1.0f, 0.7f));
}

std::unique_ptr<Particle> SmallGustParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SmallGustParticle>(pos, velocity);
}

void SmallGustParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 随机漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.006f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.006f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 膨胀（比 GustParticle 更小幅度）
    f64 lifeRatio = m_age / m_maxAge;
    f64 expansion = 1.0 + lifeRatio * 1.5;
    setSize(m_initialSize * expansion);

    // 60% 生命周期后淡出
    if (lifeRatio > 0.6f) {
        f64 fadeRatio = (lifeRatio - 0.6) / 0.4;
        m_color.a = static_cast<f32>(0.7 * (1.0 - fadeRatio));
    }
}

f64 SmallGustParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
