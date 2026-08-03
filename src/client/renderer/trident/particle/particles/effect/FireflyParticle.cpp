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

#include "FireflyParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <cmath>
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

FireflyParticle::FireflyParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_baseAlpha(0.8)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE + m_random.nextFloat() * 0.02);

    // 暖黄绿色
    setColor(glm::vec4(0.8f + m_random.nextFloat() * 0.2f,
        0.9f + m_random.nextFloat() * 0.1f,
        0.3f + m_random.nextFloat() * 0.2f,
        0.8f));

    setFriction(0.96);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 10.0);
}

std::unique_ptr<Particle> FireflyParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<FireflyParticle>(pos, velocity);
}

void FireflyParticle::tick(mc::client::ClientWorld* world)
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
    m_velocity.y += (m_random.nextFloat() - 0.5f) * 0.006f;

    m_position += m_velocity;
    m_velocity *= static_cast<f32>(m_friction);

    // 闪烁效果：alpha 随 sin 振荡
    f64 blink = std::sin(m_age * 0.5) * 0.5 + 0.5; // 范围 [0, 1]
    f64 lifeRatio = m_age / m_maxAge;

    // 生命周期 80% 后淡出
    if (lifeRatio > 0.8) {
        m_color.a = static_cast<f32>(m_baseAlpha * blink * (1.0 - (lifeRatio - 0.8) / 0.2));
    } else {
        m_color.a = static_cast<f32>(m_baseAlpha * blink);
    }
}

f64 FireflyParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
