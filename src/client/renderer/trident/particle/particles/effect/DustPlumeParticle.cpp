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
 * copies of substantial portions of the Software.
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

#include "DustPlumeParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

DustPlumeParticle::DustPlumeParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.1)
{
    setGravity(-0.001); // 轻微向上漂移
    setSize(0.1 + m_random.nextFloat() * 0.05);
    m_initialSize = size();
    setFriction(0.96);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 5.0);

    // 棕灰色
    f32 r = 0.5f + m_random.nextFloat() * 0.2f;
    f32 g = 0.4f + m_random.nextFloat() * 0.15f;
    f32 b = 0.3f + m_random.nextFloat() * 0.1f;
    setColor(glm::vec4(r, g, b, 0.8f));
}

std::unique_ptr<Particle> DustPlumeParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<DustPlumeParticle>(pos, velocity);
}

void DustPlumeParticle::tick(mc::client::ClientWorld* world)
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
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.005f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.005f;

    // 轻微向上
    m_velocity.y += 0.001f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄扩大
    f64 lifeRatio = m_age / m_maxAge;
    f64 expansion = 1.0 + lifeRatio * 2.0;
    setSize(m_initialSize * expansion);

    // 淡出
    m_color.a = static_cast<f32>(0.8 * (1.0 - lifeRatio * 0.8));
}

f64 DustPlumeParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
