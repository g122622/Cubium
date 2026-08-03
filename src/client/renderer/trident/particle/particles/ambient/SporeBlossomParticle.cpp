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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SporeBlossomParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// FallingSporeBlossomParticle
// ============================================================================

FallingSporeBlossomParticle::FallingSporeBlossomParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8 + m_random.nextFloat() * 0.4));
    m_initialAlpha = 0.3 + m_random.nextFloat() * 0.4;
    setColor(glm::vec4(0.32f, 0.50f, 0.22f, static_cast<f32>(m_initialAlpha)));

    setFriction(0.96);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7 + m_random.nextFloat() * 0.6));
}

std::unique_ptr<Particle> FallingSporeBlossomParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<FallingSporeBlossomParticle>(pos, velocity);
}

void FallingSporeBlossomParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 缓慢水平漂移
    m_velocity.x += static_cast<f32>((m_random.nextFloat() - 0.5) * DRIFT_STRENGTH);
    m_velocity.z += static_cast<f32>((m_random.nextFloat() - 0.5) * DRIFT_STRENGTH);

    // 应用重力（下落）
    m_velocity.y -= static_cast<f32>(m_gravity);

    m_position += m_velocity;
    m_velocity *= static_cast<f32>(m_friction);

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > FADE_START_RATIO) {
        m_color.a = static_cast<f32>(m_initialAlpha * (1.0 - (lifeRatio - FADE_START_RATIO) / FADE_RANGE));
    }
}

// ============================================================================
// SporeBlossomAirParticle
// ============================================================================

SporeBlossomAirParticle::SporeBlossomAirParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8 + m_random.nextFloat() * 0.4));
    m_initialAlpha = 0.2 + m_random.nextFloat() * 0.3;
    // 初始 alpha 为 0，渐入
    setColor(glm::vec4(0.32f, 0.50f, 0.22f, 0.0f));

    setFriction(0.95);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7 + m_random.nextFloat() * 0.6));
}

std::unique_ptr<Particle> SporeBlossomAirParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SporeBlossomAirParticle>(pos, velocity);
}

void SporeBlossomAirParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 缓慢随机漂移
    m_velocity.x += static_cast<f32>((m_random.nextFloat() - 0.5) * DRIFT_STRENGTH);
    m_velocity.y += static_cast<f32>((m_random.nextFloat() - 0.5) * DRIFT_STRENGTH);
    m_velocity.z += static_cast<f32>((m_random.nextFloat() - 0.5) * DRIFT_STRENGTH);

    m_position += m_velocity;
    m_velocity *= static_cast<f32>(m_friction);

    f64 lifeRatio = m_age / m_maxAge;

    // 渐入
    if (lifeRatio <= FADEIN_END_RATIO) {
        m_color.a = static_cast<f32>(m_initialAlpha * (lifeRatio / FADEIN_END_RATIO));
    }
    // 淡出
    else if (lifeRatio > FADE_START_RATIO) {
        m_color.a = static_cast<f32>(m_initialAlpha * (1.0 - (lifeRatio - FADE_START_RATIO) / FADE_RANGE));
    }
    // 中间保持初始 alpha
    else {
        m_color.a = static_cast<f32>(m_initialAlpha);
    }
}

} // namespace mc::client::renderer::trident::particle::particles
