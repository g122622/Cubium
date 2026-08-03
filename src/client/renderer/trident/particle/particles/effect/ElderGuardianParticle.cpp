/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software or
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

#include "ElderGuardianParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// ElderGuardianParticle
// ============================================================================

ElderGuardianParticle::ElderGuardianParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(0.0f);
    setSize(0.1f + m_random.nextFloat() * 0.04f);
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 4.0);

    // 深蓝灰色
    f32 r = 0.1f + m_random.nextFloat() * 0.1f;
    f32 g = 0.2f + m_random.nextFloat() * 0.1f;
    f32 b = 0.4f + m_random.nextFloat() * 0.2f;
    setColor(glm::vec4(r, g, b, 0.6f));
}

std::unique_ptr<Particle> ElderGuardianParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<ElderGuardianParticle>(pos, velocity);
}

void ElderGuardianParticle::tick(mc::client::ClientWorld* world)
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

    // 向上漂浮
    m_velocity.y += 0.002f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.6 * (1.0 - lifeRatio));
}

} // namespace mc::client::renderer::trident::particle::particles
