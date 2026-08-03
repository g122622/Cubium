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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CritParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// CritParticle
// ============================================================================

CritParticle::CritParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8f + m_random.nextFloat() * 0.4f));
    m_initialSize = size();

    // 暴击颜色：淡黄色
    setColor(glm::vec4(1.0f, 0.9f, 0.5f, 1.0f));

    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7f + m_random.nextFloat() * 0.6f));
}

std::unique_ptr<Particle> CritParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<CritParticle>(pos, velocity);
}

void CritParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 暴击粒子保持速度，略有阻力
    m_position += m_velocity;
    m_velocity *= m_friction;

    // 旋转
    m_roll += 0.3f;

    // 随年龄变大
    f64 lifeRatio = m_age / m_maxAge;
    setSize(m_initialSize * (1.0f + lifeRatio * 0.5f));

    // 淡出
    if (lifeRatio > 0.5f) {
        m_color.a = 1.0f - (lifeRatio - 0.5f) * 2.0f;
    }
}

// ============================================================================
// EnchantedHitParticle
// ============================================================================

EnchantedHitParticle::EnchantedHitParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8f + m_random.nextFloat() * 0.4f));
    m_initialSize = size();

    // 附魔暴击颜色：紫蓝色
    // 基础颜色与暴击粒子相同，但红、绿色通道减弱，产生紫蓝色调
    f32 r = (m_random.nextFloat() * 0.3f + 0.6f) * 0.3f;
    f32 g = (m_random.nextFloat() * 0.3f + 0.6f) * 0.8f;
    f32 b = m_random.nextFloat() * 0.3f + 0.6f;
    setColor(glm::vec4(r, g, b, 1.0f));

    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7f + m_random.nextFloat() * 0.6f));
}

std::unique_ptr<Particle> EnchantedHitParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<EnchantedHitParticle>(pos, velocity);
}

void EnchantedHitParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 附魔暴击粒子保持速度，略有阻力
    m_position += m_velocity;
    m_velocity *= m_friction;

    // 旋转
    m_roll += 0.3f;

    // 随年龄变大
    f64 lifeRatio = m_age / m_maxAge;
    setSize(m_initialSize * (1.0f + lifeRatio * 0.5f));

    // 淡出
    if (lifeRatio > 0.5f) {
        m_color.a = 1.0f - (lifeRatio - 0.5f) * 2.0f;
    }
}

} // namespace mc::client::renderer::trident::particle::particles
