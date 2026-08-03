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

#include "PoofParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

PoofParticle::PoofParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
{
    setGravity(0.0f);
    setSize(0.1f + m_random.nextFloat() * 0.02f);
    m_initialSize = size();
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 8.0);

    // 灰白色烟雾
    f32 gray = 0.7f + m_random.nextFloat() * 0.3f;
    setColor(glm::vec4(gray, gray, gray, 1.0f));

    // 初始速度添加随机分量
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.1f;
    m_velocity.y += m_random.nextFloat() * 0.05f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.1f;
}

std::unique_ptr<Particle> PoofParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<PoofParticle>(pos, velocity);
}

void PoofParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 轻微随机运动
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.005f;
    m_velocity.y += 0.001f; // 轻微上升
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.005f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄缩小并淡出
    f64 lifeRatio = m_age / m_maxAge;
    setSize(m_initialSize * (1.0f - lifeRatio * 0.75f));

    // 淡出
    m_color.a = static_cast<f32>(1.0f - lifeRatio);
}

f64 PoofParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
