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

#include "DamageIndicatorParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

DamageIndicatorParticle::DamageIndicatorParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.04 * (0.8 + m_random.nextDouble() * 0.4))
{
    setGravity(0.0);
    setSize(m_initialSize);
    setFriction(0.96);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME / (0.2 + m_random.nextDouble() * 0.8));
    // 金黄色伤害指示
    setColor(glm::vec4(1.0f, 0.9f, 0.3f, 1.0f));
    // 向上弹出
    m_velocity.y += 0.2;
}

std::unique_ptr<Particle> DamageIndicatorParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<DamageIndicatorParticle>(pos, velocity);
}

void DamageIndicatorParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    m_prevPosition = m_position;
    m_age += 1.0;

    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 减速运动
    m_velocity *= m_friction;
    m_position += m_velocity;
    m_roll += 0.3;

    // 缩放动画：先增大后缩小
    f64 lifeRatio = m_age / m_maxAge;
    setSize(m_initialSize * (1.0 + lifeRatio * 0.5));

    // 透明度淡出
    if (lifeRatio > 0.5) {
        f64 fadeRatio = (lifeRatio - 0.5) * 2.0;
        setColor(glm::vec4(m_color.r, m_color.g, m_color.b, static_cast<f32>(1.0 - fadeRatio)));
    }
}

} // namespace mc::client::renderer::trident::particle::particles
