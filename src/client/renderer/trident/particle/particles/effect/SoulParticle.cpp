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

#include "SoulParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// SoulFireFlameParticle
// ============================================================================

SoulFireFlameParticle::SoulFireFlameParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.04f)
{
    setGravity(0.0f);
    setSize(0.04f * (0.6f + m_random.nextFloat() * 0.4f));
    m_initialSize = size();
    setFriction(0.95f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.8f + m_random.nextFloat() * 0.4f));

    // 蓝色火焰颜色
    f32 blueIntensity = 0.8f + m_random.nextFloat() * 0.2f;
    setColor(glm::vec4(0.2f, 0.4f + m_random.nextFloat() * 0.2f, blueIntensity, 1.0f));
}

std::unique_ptr<Particle> SoulFireFlameParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SoulFireFlameParticle>(pos, velocity);
}

void SoulFireFlameParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 向上飘动并随机摇摆
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.01f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.01f;
    m_velocity.y += 0.002f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄缩小
    f64 lifeRatio = m_age / m_maxAge;
    setSize(m_initialSize * (1.0f - lifeRatio * 0.5f));

    // 淡出
    if (lifeRatio > 0.5f) {
        m_color.a = static_cast<f32>(1.0f - (lifeRatio - 0.5f) * 2.0f);
    }
}

f64 SoulFireFlameParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// SoulParticle
// ============================================================================

SoulParticle::SoulParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
{
    setGravity(0.0f);
    setSize(0.1f + m_random.nextFloat() * 0.03f);
    m_initialSize = size();
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 4.0);

    // 蓝色灵魂颜色
    f32 blue = 0.7f + m_random.nextFloat() * 0.3f;
    setColor(glm::vec4(0.3f, 0.5f, blue, 0.8f));

    // 轻微向上漂浮
    m_velocity.y += 0.01f;
}

std::unique_ptr<Particle> SoulParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SoulParticle>(pos, velocity);
}

void SoulParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 向上漂浮并随机漂移
    m_velocity.y += 0.002f;
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.002f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.002f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄淡出
    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.8f * (1.0f - lifeRatio));
}

f64 SoulParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

} // namespace mc::client::renderer::trident::particle::particles
