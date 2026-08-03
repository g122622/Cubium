/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software be
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

#include "TrialSpawnerParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// TrialSpawnerDetectionParticle
// ============================================================================

TrialSpawnerDetectionParticle::TrialSpawnerDetectionParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(0.0f);
    setSize(0.04f + m_random.nextFloat() * 0.02f);
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 5.0);

    // 橙黄色
    f32 r = 0.9f + m_random.nextFloat() * 0.1f;
    f32 g = 0.7f + m_random.nextFloat() * 0.2f;
    f32 b = 0.3f + m_random.nextFloat() * 0.1f;
    setColor(glm::vec4(r, g, b, 0.8f));
}

std::unique_ptr<Particle> TrialSpawnerDetectionParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<TrialSpawnerDetectionParticle>(pos, velocity);
}

void TrialSpawnerDetectionParticle::tick(mc::client::ClientWorld* world)
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
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.006f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.006f;

    // 向上漂浮
    m_velocity.y += 0.001f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 70% 生命周期后淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.7) {
        f64 fadeRatio = (lifeRatio - 0.7) / 0.3;
        m_color.a = static_cast<f32>(0.8 * (1.0 - fadeRatio));
    }
}

// ============================================================================
// TrialSpawnerDetectionOminousParticle
// ============================================================================

TrialSpawnerDetectionOminousParticle::TrialSpawnerDetectionOminousParticle(
    const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(0.0f);
    setSize(0.04f + m_random.nextFloat() * 0.02f);
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 5.0);

    // 蓝色-青色
    f32 r = 0.3f + m_random.nextFloat() * 0.2f;
    f32 g = 0.5f + m_random.nextFloat() * 0.3f;
    f32 b = 0.9f + m_random.nextFloat() * 0.1f;
    setColor(glm::vec4(r, g, b, 0.8f));
}

std::unique_ptr<Particle> TrialSpawnerDetectionOminousParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<TrialSpawnerDetectionOminousParticle>(pos, velocity);
}

void TrialSpawnerDetectionOminousParticle::tick(mc::client::ClientWorld* world)
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
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.006f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.006f;

    // 向上漂浮
    m_velocity.y += 0.001f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 70% 生命周期后淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.7) {
        f64 fadeRatio = (lifeRatio - 0.7) / 0.3;
        m_color.a = static_cast<f32>(0.8 * (1.0 - fadeRatio));
    }
}

} // namespace mc::client::renderer::trident::particle::particles
