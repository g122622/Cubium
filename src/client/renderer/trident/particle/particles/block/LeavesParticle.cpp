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

#include "LeavesParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <cmath>
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// CherryLeavesParticle
// ============================================================================

CherryLeavesParticle::CherryLeavesParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.1)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(0.1 + m_random.nextFloat() * 0.05);
    m_initialSize = size();
    setFriction(0.97);
    setHasPhysics(true);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 20.0);

    // 粉色花瓣颜色
    f32 r = 0.9f + m_random.nextFloat() * 0.1f;
    f32 g = 0.6f + m_random.nextFloat() * 0.3f;
    f32 b = 0.7f + m_random.nextFloat() * 0.2f;
    setColor(glm::vec4(r, g, b, 0.9f));
}

std::unique_ptr<Particle> CherryLeavesParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<CherryLeavesParticle>(pos, velocity);
}

void CherryLeavesParticle::tick(mc::client::ClientWorld* world)
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
    m_velocity.y -= static_cast<f32>(m_gravity * mc::physics::PARTICLE_GRAVITY_MULTIPLIER);

    // 随机水平漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.005f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.005f;

    // 正弦摆动
    m_velocity.x += static_cast<f32>(std::sin(m_age * 0.1) * 0.02);

    // 旋转
    m_roll += 0.05;

    // 移动并碰撞
    move(world, m_velocity);

    // 地面摩擦
    if (onGround()) {
        m_velocity.x *= 0.7f;
        m_velocity.z *= 0.7f;
    }

    // 80% 生命周期后淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.8) {
        f64 fadeProgress = (lifeRatio - 0.8) / 0.2;
        m_color.a = static_cast<f32>(0.9 * (1.0 - fadeProgress));
    }
}

f64 CherryLeavesParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

// ============================================================================
// PaleOakLeavesParticle
// ============================================================================

PaleOakLeavesParticle::PaleOakLeavesParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.1)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(0.1 + m_random.nextFloat() * 0.05);
    m_initialSize = size();
    setFriction(0.97);
    setHasPhysics(true);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 20.0);

    // 灰绿色叶片颜色
    f32 r = 0.7f + m_random.nextFloat() * 0.1f;
    f32 g = 0.75f + m_random.nextFloat() * 0.1f;
    f32 b = 0.6f + m_random.nextFloat() * 0.1f;
    setColor(glm::vec4(r, g, b, 0.9f));
}

std::unique_ptr<Particle> PaleOakLeavesParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<PaleOakLeavesParticle>(pos, velocity);
}

void PaleOakLeavesParticle::tick(mc::client::ClientWorld* world)
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
    m_velocity.y -= static_cast<f32>(m_gravity * mc::physics::PARTICLE_GRAVITY_MULTIPLIER);

    // 随机水平漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.005f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.005f;

    // 较弱的正弦摆动
    m_velocity.x += static_cast<f32>(std::sin(m_age * 0.08) * 0.01);

    // 旋转
    m_roll += 0.05;

    // 移动并碰撞
    move(world, m_velocity);

    // 地面摩擦
    if (onGround()) {
        m_velocity.x *= 0.7f;
        m_velocity.z *= 0.7f;
    }

    // 80% 生命周期后淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.8) {
        f64 fadeProgress = (lifeRatio - 0.8) / 0.2;
        m_color.a = static_cast<f32>(0.9 * (1.0 - fadeProgress));
    }
}

f64 PaleOakLeavesParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

// ============================================================================
// TintedLeavesParticle
// ============================================================================

TintedLeavesParticle::TintedLeavesParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.1)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(0.1 + m_random.nextFloat() * 0.05);
    m_initialSize = size();
    setFriction(0.97);
    setHasPhysics(true);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 20.0);

    // 默认绿色（MC 中使用生物群系着色）
    f32 r = 0.4f + m_random.nextFloat() * 0.2f;
    f32 g = 0.6f + m_random.nextFloat() * 0.2f;
    f32 b = 0.2f + m_random.nextFloat() * 0.1f;
    setColor(glm::vec4(r, g, b, 1.0f));
}

std::unique_ptr<Particle> TintedLeavesParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<TintedLeavesParticle>(pos, velocity);
}

void TintedLeavesParticle::tick(mc::client::ClientWorld* world)
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
    m_velocity.y -= static_cast<f32>(m_gravity * mc::physics::PARTICLE_GRAVITY_MULTIPLIER);

    // 随机水平漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.005f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.005f;

    // 正弦摆动
    m_velocity.x += static_cast<f32>(std::sin(m_age * 0.1) * 0.02);

    // 旋转
    m_roll += 0.05;

    // 移动并碰撞
    move(world, m_velocity);

    // 地面摩擦
    if (onGround()) {
        m_velocity.x *= 0.7f;
        m_velocity.z *= 0.7f;
    }

    // 80% 生命周期后淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.8) {
        f64 fadeProgress = (lifeRatio - 0.8) / 0.2;
        m_color.a = static_cast<f32>(1.0 - fadeProgress);
    }
}

f64 TintedLeavesParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
