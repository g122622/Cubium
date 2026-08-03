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

#include "HeartParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

HeartParticle::HeartParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8 + static_cast<f64>(m_random.nextFloat()) * 0.4));

    // 爱心颜色：红色
    setColor(glm::vec4(1.0f, 0.2f, 0.2f, 1.0f));

    setFriction(0.95);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.8 + static_cast<f64>(m_random.nextFloat()) * 0.4));

    // 爱心向上飘动
    m_velocity.y = 0.02f + m_random.nextFloat() * 0.01f;
}

std::unique_ptr<Particle> HeartParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<HeartParticle>(pos, velocity);
}

void HeartParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 爱心向上飘动
    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.5) {
        m_color.a = static_cast<f32>(1.0 - (lifeRatio - 0.5) * 2.0);
    }
}

} // namespace mc::client::renderer::trident::particle::particles
