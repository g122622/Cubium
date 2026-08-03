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

#include "SculkChargeParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// SculkChargeParticle
// ============================================================================

SculkChargeParticle::SculkChargeParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.04)
{
    setGravity(0.0);
    setSize(0.04 + m_random.nextFloat() * 0.02);
    m_initialSize = size();
    setFriction(0.96);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 5.0);

    // 青色发光
    f32 g = 0.8f + m_random.nextFloat() * 0.2f;
    f32 b = 0.9f + m_random.nextFloat() * 0.1f;
    setColor(glm::vec4(0.1f, g, b, 0.9f));
}

std::unique_ptr<Particle> SculkChargeParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SculkChargeParticle>(pos, velocity);
}

void SculkChargeParticle::tick(mc::client::ClientWorld* world)
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
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.003f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.003f;

    // 轻微上升
    m_velocity.y += 0.001f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 旋转动画
    m_roll += 0.15;

    // 随年龄变大
    f64 lifeRatio = m_age / m_maxAge;
    f64 scale = 1.0 + lifeRatio * 0.5;
    setSize(m_initialSize * scale);

    // 70% 生命周期后淡出
    if (lifeRatio > 0.7) {
        f64 fadeProgress = (lifeRatio - 0.7) / 0.3;
        m_color.a = static_cast<f32>(0.9 * (1.0 - fadeProgress));
    }
}

f64 SculkChargeParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

// ============================================================================
// SculkChargePopParticle
// ============================================================================

SculkChargePopParticle::SculkChargePopParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.08)
{
    setGravity(0.0);
    setSize(0.08 + m_random.nextFloat() * 0.04);
    m_initialSize = size();
    setFriction(0.94);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 3.0);

    // 明亮青色
    f32 g = 0.9f + m_random.nextFloat() * 0.1f;
    setColor(glm::vec4(0.2f, g, 1.0f, 0.9f));
}

std::unique_ptr<Particle> SculkChargePopParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SculkChargePopParticle>(pos, velocity);
}

void SculkChargePopParticle::tick(mc::client::ClientWorld* world)
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
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.005f;
    m_velocity.y += (m_random.nextFloat() - 0.5f) * 0.005f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.005f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 快速旋转
    m_roll += 0.3;

    // 随年龄缩小
    f64 lifeRatio = m_age / m_maxAge;
    f64 scale = 1.0 - lifeRatio * 0.7;
    setSize(m_initialSize * scale);

    // 线性淡出
    m_color.a = static_cast<f32>(0.9 * (1.0 - lifeRatio));
}

f64 SculkChargePopParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
