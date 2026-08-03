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

#include "AshParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// AshParticle
// ============================================================================

AshParticle::AshParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE + m_random.nextFloat() * 0.01);

    // 灰橙色
    setColor(glm::vec4(0.6f + m_random.nextFloat() * 0.3f,
        0.4f + m_random.nextFloat() * 0.2f,
        0.2f + m_random.nextFloat() * 0.1f,
        1.0f));

    setFriction(0.96);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 10.0);
}

std::unique_ptr<Particle> AshParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<AshParticle>(pos, velocity);
}

void AshParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= static_cast<f32>(m_gravity * 0.04);

    // 随机水平漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.004f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.004f;

    m_position += m_velocity;
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.y *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // 生命周期 70% 后淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.7) {
        m_color.a = static_cast<f32>(1.0 - (lifeRatio - 0.7) / 0.3);
    }
}

f64 AshParticle::getScale(f64 partialTick) const
{
    // 淡入效果：粒子从零尺寸快速增大到完整尺寸
    f64 t = (m_age + partialTick) / m_maxAge;
    return mc::math::clamp(t * 32.0, 0.0, 1.0);
}

// ============================================================================
// WhiteAshParticle
// ============================================================================

WhiteAshParticle::WhiteAshParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE + m_random.nextFloat() * 0.01);

    // 白灰色
    f32 gray = 0.8f + m_random.nextFloat() * 0.2f;
    setColor(glm::vec4(gray, gray, gray, 0.9f));

    setFriction(0.97);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 10.0);
}

std::unique_ptr<Particle> WhiteAshParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<WhiteAshParticle>(pos, velocity);
}

void WhiteAshParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= static_cast<f32>(m_gravity * 0.04);

    // 随机水平漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.002f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.002f;

    m_position += m_velocity;
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.y *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // 生命周期 70% 后淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.7) {
        m_color.a = static_cast<f32>(0.9 * (1.0 - (lifeRatio - 0.7) / 0.3));
    }
}

f64 WhiteAshParticle::getScale(f64 partialTick) const
{
    // 淡入效果：粒子从零尺寸快速增大到完整尺寸
    f64 t = (m_age + partialTick) / m_maxAge;
    return mc::math::clamp(t * 32.0, 0.0, 1.0);
}

} // namespace mc::client::renderer::trident::particle::particles
