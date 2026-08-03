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

#include "CloudParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// CloudParticle
// ============================================================================

CloudParticle::CloudParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
{
    setGravity(0.0f);
    setSize(0.1f + m_random.nextFloat() * 0.05f);
    m_initialSize = size();
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 5.0);

    // 白色/灰白色
    f32 brightness = 0.9f + m_random.nextFloat() * 0.1f;
    setColor(glm::vec4(brightness, brightness, brightness, 0.8f));

    // 轻微向上
    m_velocity.y += m_random.nextFloat() * 0.02f;
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.02f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.02f;
}

std::unique_ptr<Particle> CloudParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<CloudParticle>(pos, velocity);
}

void CloudParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 随机漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.001f;
    m_velocity.y += 0.001f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.001f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄扩大
    f64 lifeRatio = m_age / m_maxAge;
    f64 expansion = 1.0f + lifeRatio * 2.0f;
    setSize(m_initialSize * expansion);

    // 淡出
    m_color.a = static_cast<f32>(0.8f * (1.0f - lifeRatio * 0.8f));
}

f64 CloudParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// BarrierParticle
// ============================================================================

BarrierParticle::BarrierParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(0.0f);
    setSize(0.5f); // 固定大小
    setFriction(1.0f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME);

    // 白色
    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // 不移动
    m_velocity = glm::vec3(0.0f);
}

std::unique_ptr<Particle> BarrierParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<BarrierParticle>(pos, velocity);
}

void BarrierParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 屏障粒子静止不动
    // 只是淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.8f) {
        m_color.a = static_cast<f32>(1.0f - (lifeRatio - 0.8f) / 0.2f);
    }
}

f64 BarrierParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// WaterWakeParticle
// ============================================================================

WaterWakeParticle::WaterWakeParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.05f)
{
    setGravity(0.0f);
    setSize(0.05f + m_random.nextFloat() * 0.02f);
    m_initialSize = size();
    setFriction(1.0f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 4.0);

    // 浅蓝/白色
    setColor(glm::vec4(0.8f, 0.9f, 1.0f, 0.6f));

    // 几乎不移动
    m_velocity *= 0.1f;
}

std::unique_ptr<Particle> WaterWakeParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<WaterWakeParticle>(pos, velocity);
}

void WaterWakeParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 缓慢扩大
    f64 lifeRatio = m_age / m_maxAge;
    f64 expansion = 1.0f + lifeRatio * 3.0f;
    setSize(m_initialSize * expansion);

    // 快速淡出
    m_color.a = static_cast<f32>(0.6f * (1.0f - lifeRatio));
}

f64 WaterWakeParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// DolphinParticle
// ============================================================================

DolphinParticle::DolphinParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.05f)
{
    setGravity(-0.005f); // 轻微向上
    setSize(0.03f + m_random.nextFloat() * 0.02f);
    m_initialSize = size();
    setFriction(0.98f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 5.0);

    // 浅蓝气泡色
    f32 blue = 0.8f + m_random.nextFloat() * 0.2f;
    setColor(glm::vec4(0.7f, 0.85f, blue, 0.7f));

    // 向上和前方运动
    m_velocity.y += 0.03f;
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.02f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.02f;
}

std::unique_ptr<Particle> DolphinParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<DolphinParticle>(pos, velocity);
}

void DolphinParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 向上漂浮
    m_velocity.y -= static_cast<f32>(m_gravity * 0.04);

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.7f * (1.0f - lifeRatio));
}

f64 DolphinParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

} // namespace mc::client::renderer::trident::particle::particles
