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

#include "GlowParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

GlowParticle::GlowParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE + m_random.nextFloat() * 0.02);
    m_initialSize = size();

    // 暖色发光
    setColor(glm::vec4(0.6f + m_random.nextFloat() * 0.3f,
        0.8f + m_random.nextFloat() * 0.2f,
        0.5f + m_random.nextFloat() * 0.3f,
        0.7f));

    setFriction(0.96);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 5.0);
}

std::unique_ptr<Particle> GlowParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<GlowParticle>(pos, velocity);
}

void GlowParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 随机水平漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.004f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.004f;

    // 轻微向上
    m_velocity.y += 0.001f;

    m_position += m_velocity;
    m_velocity *= static_cast<f32>(m_friction);

    // 根据生命周期缩小粒子
    f64 lifeRatio = m_age / m_maxAge;
    setSize(m_initialSize * (1.0 - lifeRatio * 0.5));

    // 生命周期 60% 后淡出
    if (lifeRatio > 0.6) {
        m_color.a = static_cast<f32>(0.7 * (1.0 - (lifeRatio - 0.6) / 0.4));
    }
}

f64 GlowParticle::getScale(f64 partialTick) const
{
    // 淡入效果：粒子从零尺寸快速增大到完整尺寸
    f64 t = (m_age + partialTick) / m_maxAge;
    return mc::math::clamp(t * 32.0, 0.0, 1.0);
}

} // namespace mc::client::renderer::trident::particle::particles
