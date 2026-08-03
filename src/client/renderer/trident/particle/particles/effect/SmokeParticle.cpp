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

#include "SmokeParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

SmokeParticle::SmokeParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8f + m_random.nextFloat() * 0.4f));
    m_initialSize = size();

    // 灰色烟雾
    f64 gray = 0.3f + m_random.nextFloat() * 0.2f;
    setColor(glm::vec4(gray, gray, gray, 0.8f));

    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7f + m_random.nextFloat() * 0.6f));
}

std::unique_ptr<Particle> SmokeParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SmokeParticle>(pos, velocity);
}

void SmokeParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 烟雾向上缓慢飘动
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.005f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.005f;
    m_velocity.y += 0.001f; // 轻微向上

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄变大并淡出
    f64 lifeRatio = m_age / m_maxAge;
    f64 scale = 1.0f + lifeRatio * 2.0f;
    setSize(m_initialSize * scale);

    // 淡出
    m_color.a = 0.8f * (1.0f - lifeRatio);
}

f64 SmokeParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

} // namespace mc::client::renderer::trident::particle::particles
