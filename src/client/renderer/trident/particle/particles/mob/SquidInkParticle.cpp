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

#include "SquidInkParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// SquidInkParticle
// ============================================================================

SquidInkParticle::SquidInkParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(0.0);
    setSize(DEFAULT_SIZE + m_random.nextFloat() * 0.05);

    // 深蓝黑色墨汁
    f32 r = 0.05f + m_random.nextFloat() * 0.1f;
    f32 g = 0.05f + m_random.nextFloat() * 0.1f;
    f32 b = 0.1f + m_random.nextFloat() * 0.15f;
    setColor(glm::vec4(r, g, b, 0.8f));

    setFriction(0.96);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 5.0);
}

std::unique_ptr<Particle> SquidInkParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SquidInkParticle>(pos, velocity);
}

void SquidInkParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 随机漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.01f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.01f;

    // 微弱向上漂浮
    m_velocity.y += 0.001f;

    // 摩擦衰减
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.y *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    m_position += m_velocity;

    // 随生命周期膨胀和淡出
    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.8f * (1.0f - lifeRatio));
}

f64 SquidInkParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    // 膨胀效果：从 1.0 扩大到 1.0 + lifeRatio * 2.0
    f64 lifeRatio = m_age / m_maxAge;
    return 1.0 + lifeRatio * 2.0;
}

// ============================================================================
// GlowSquidInkParticle
// ============================================================================

GlowSquidInkParticle::GlowSquidInkParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(0.0);
    setSize(DEFAULT_SIZE + m_random.nextFloat() * 0.05);

    // 明亮青蓝色荧光墨汁
    f32 r = 0.2f + m_random.nextFloat() * 0.2f;
    f32 g = 0.6f + m_random.nextFloat() * 0.3f;
    f32 b = 0.8f + m_random.nextFloat() * 0.2f;
    setColor(glm::vec4(r, g, b, 0.8f));

    setFriction(0.96);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 5.0);
}

std::unique_ptr<Particle> GlowSquidInkParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<GlowSquidInkParticle>(pos, velocity);
}

void GlowSquidInkParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 随机漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.01f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.01f;

    // 微弱向上漂浮
    m_velocity.y += 0.001f;

    // 摩擦衰减
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.y *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    m_position += m_velocity;

    // 随生命周期膨胀和淡出
    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.8f * (1.0f - lifeRatio));
}

f64 GlowSquidInkParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    // 膨胀效果：从 1.0 扩大到 1.0 + lifeRatio * 2.0
    f64 lifeRatio = m_age / m_maxAge;
    return 1.0 + lifeRatio * 2.0;
}

} // namespace mc::client::renderer::trident::particle::particles
