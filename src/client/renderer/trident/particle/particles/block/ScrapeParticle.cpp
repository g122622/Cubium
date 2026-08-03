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
 * LIABILITY, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ScrapeParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

ScrapeParticle::ScrapeParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.0)
{
    setGravity(DEFAULT_GRAVITY);
    // quadSize = 0.1 * (random * 0.5 + 0.2)
    setSize(0.1 * (0.2 + m_random.nextFloat() * 0.5));
    m_initialSize = size();

    // 随机选择两种氧化铜绿色之一
    if (m_random.nextFloat() < 0.5f) {
        // 铜绿色变体 1: (0.29, 0.58, 0.51)
        setColor(glm::vec4(0.29f, 0.58f, 0.51f, 1.0f));
    } else {
        // 铜绿色变体 2: (0.43, 0.77, 0.62)
        setColor(glm::vec4(0.43f, 0.77f, 0.62f, 1.0f));
    }

    // 速度缩放 dx*0.01, dy*0.01, dz*0.01
    m_velocity *= static_cast<f32>(VELOCITY_SCALE);

    setFriction(0.96);
    setHasPhysics(false);

    // lifetime = 10 + random.nextInt(30) (10~40 tick)
    setMaxAge(10.0 + m_random.nextInt(30));
}

std::unique_ptr<Particle> ScrapeParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<ScrapeParticle>(pos, velocity);
}

void ScrapeParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 轻微水平漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.004f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.004f;

    m_position += m_velocity;
    m_velocity *= static_cast<f32>(m_friction);

    // 根据生命周期缩小粒子
    f64 lifeRatio = m_age / m_maxAge;
    setSize(m_initialSize * (1.0 - lifeRatio * 0.5));

    // 生命周期后半段淡出
    if (lifeRatio > 0.6) {
        m_color.a = static_cast<f32>(1.0 - (lifeRatio - 0.6) / 0.4);
    }
}

f64 ScrapeParticle::getScale(f64 partialTick) const
{
    // 淡入效果：前 1/32 生命周期内从 0 渐变到 1.0
    f64 t = (m_age + partialTick) / m_maxAge;
    return mc::math::clamp(t * 32.0, 0.0, 1.0);
}

} // namespace mc::client::renderer::trident::particle::particles
