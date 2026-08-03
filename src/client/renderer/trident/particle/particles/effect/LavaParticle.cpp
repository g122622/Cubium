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

#include "LavaParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

LavaParticle::LavaParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8f + m_random.nextFloat() * 0.4f));

    // 熔岩颜色：橙红色
    f32 colorVar = m_random.nextFloat() * 0.2f;
    setColor(glm::vec4(1.0f, 0.3f + colorVar, 0.0f, 1.0f));

    setFriction(0.98f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.8f + m_random.nextFloat() * 0.4f));
}

std::unique_ptr<Particle> LavaParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<LavaParticle>(pos, velocity);
}

void LavaParticle::tick(mc::client::ClientWorld* world)
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

    // 随机水平漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.002f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.002f;

    m_position += m_velocity;
    m_velocity.x *= m_friction;
    m_velocity.z *= m_friction;

    // 随年龄淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.6) {
        m_color.a = 1.0f - static_cast<f32>((lifeRatio - 0.6) / 0.4);
    }
}

f64 LavaParticle::getScale(f64 partialTick) const
{
    // 二次收缩：粒子从完整尺寸缩小到零
    f64 t = (m_age + partialTick) / m_maxAge;
    return 1.0 - t * t;
}

} // namespace mc::client::renderer::trident::particle::particles
