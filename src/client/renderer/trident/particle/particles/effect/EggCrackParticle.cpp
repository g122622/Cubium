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

#include "EggCrackParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

EggCrackParticle::EggCrackParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.05)
{
    setGravity(0.0);
    setSize(0.05 + m_random.nextFloat() * 0.02);
    m_initialSize = size();
    setFriction(0.96);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 4.0);

    // 蛋白色/奶油色
    f32 r = 0.95f + m_random.nextFloat() * 0.05f;
    f32 g = 0.9f + m_random.nextFloat() * 0.05f;
    f32 b = 0.8f + m_random.nextFloat() * 0.1f;
    setColor(glm::vec4(r, g, b, 1.0f));

    // 初始速度添加随机分量
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.05f;
    m_velocity.y += m_random.nextFloat() * 0.03f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.05f;
}

std::unique_ptr<Particle> EggCrackParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<EggCrackParticle>(pos, velocity);
}

void EggCrackParticle::tick(mc::client::ClientWorld* world)
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
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.01f;
    m_velocity.y += (m_random.nextFloat() - 0.5f) * 0.01f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.01f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄缩小
    f64 lifeRatio = m_age / m_maxAge;
    f64 scale = 1.0 - lifeRatio * 0.5;
    setSize(m_initialSize * scale);

    // 线性淡出
    m_color.a = static_cast<f32>(1.0 - lifeRatio);
}

f64 EggCrackParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
