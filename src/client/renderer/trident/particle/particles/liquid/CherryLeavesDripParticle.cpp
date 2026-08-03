/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CherryLeavesDripParticle.hpp"
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
// DrippingCherryLeavesParticle
// ============================================================================

DrippingCherryLeavesParticle::DrippingCherryLeavesParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE + m_random.nextFloat() * SIZE_VARIATION)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(m_initialSize);

    // 粉色
    f32 r = static_cast<f32>(0.9 + m_random.nextFloat() * 0.1);
    f32 g = static_cast<f32>(0.6 + m_random.nextFloat() * 0.3);
    f32 b = static_cast<f32>(0.7 + m_random.nextFloat() * 0.2);
    setColor(glm::vec4(r, g, b, 0.9f));

    setFriction(FRICTION);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * LIFETIME_VARIATION);
}

std::unique_ptr<Particle> DrippingCherryLeavesParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<DrippingCherryLeavesParticle>(pos, velocity);
}

void DrippingCherryLeavesParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    f64 lifeRatio = m_age / m_maxAge;

    if (!m_isFalling) {
        // 悬挂状态：缓慢漂移，无重力
        m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.002f;
        m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.002f;
        m_velocity *= static_cast<f32>(m_friction);
        m_position += m_velocity;

        // 70% 生命周期后转为下落状态
        if (lifeRatio >= 0.7) {
            m_isFalling = true;
            setGravity(FALLING_GRAVITY);
            setHasPhysics(true);
        }
    } else {
        // 下落状态：应用重力
        m_velocity.y -= static_cast<f32>(m_gravity * mc::physics::PARTICLE_GRAVITY_MULTIPLIER);
        m_velocity.x *= static_cast<f32>(m_friction);
        m_velocity.z *= static_cast<f32>(m_friction);

        if (m_hasPhysics) {
            move(world, m_velocity);
        } else {
            m_position += m_velocity;
        }

        // 地面摩擦
        if (onGround()) {
            m_velocity.x *= 0.7f;
            m_velocity.z *= 0.7f;
        }
    }

    // 80% 生命周期后淡出
    if (lifeRatio > 0.8) {
        m_color.a = static_cast<f32>(0.9 * (1.0 - (lifeRatio - 0.8) / 0.2));
    }
}

f64 DrippingCherryLeavesParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

// ============================================================================
// FallingCherryLeavesParticle
// ============================================================================

FallingCherryLeavesParticle::FallingCherryLeavesParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE + m_random.nextFloat() * SIZE_VARIATION)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(m_initialSize);

    // 粉色
    f32 r = static_cast<f32>(0.9 + m_random.nextFloat() * 0.1);
    f32 g = static_cast<f32>(0.6 + m_random.nextFloat() * 0.3);
    f32 b = static_cast<f32>(0.7 + m_random.nextFloat() * 0.2);
    setColor(glm::vec4(r, g, b, 0.9f));

    setFriction(FRICTION);
    setHasPhysics(true);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * LIFETIME_VARIATION);
}

std::unique_ptr<Particle> FallingCherryLeavesParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<FallingCherryLeavesParticle>(pos, velocity);
}

void FallingCherryLeavesParticle::tick(mc::client::ClientWorld* world)
{
    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= static_cast<f32>(m_gravity * mc::physics::PARTICLE_GRAVITY_MULTIPLIER);

    // 正弦摆动（水平方向周期性偏移）
    f64 swingPhase = m_age * 0.1;
    m_velocity.x += static_cast<f32>(std::sin(swingPhase) * 0.005);

    // 旋转
    m_roll += 0.05;

    // 摩擦衰减
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // 移动并碰撞
    if (m_hasPhysics) {
        move(world, m_velocity);
    } else {
        m_position += m_velocity;
    }

    // 地面摩擦
    if (onGround()) {
        m_velocity.x *= 0.7f;
        m_velocity.z *= 0.7f;
    }

    // 80% 生命周期后淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.8) {
        m_color.a = static_cast<f32>(0.9 * (1.0 - (lifeRatio - 0.8) / 0.2));
    }
}

f64 FallingCherryLeavesParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

// ============================================================================
// LandingCherryLeavesParticle
// ============================================================================

LandingCherryLeavesParticle::LandingCherryLeavesParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE + m_random.nextFloat() * SIZE_VARIATION);

    // 粉色（略低透明度）
    f32 r = static_cast<f32>(0.9 + m_random.nextFloat() * 0.1);
    f32 g = static_cast<f32>(0.6 + m_random.nextFloat() * 0.3);
    f32 b = static_cast<f32>(0.7 + m_random.nextFloat() * 0.2);
    setColor(glm::vec4(r, g, b, 0.7f));

    setFriction(1.0);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * LIFETIME_VARIATION);
}

std::unique_ptr<Particle> LandingCherryLeavesParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<LandingCherryLeavesParticle>(pos, velocity);
}

void LandingCherryLeavesParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 静止状态，仅淡出
    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.7 * (1.0 - lifeRatio));
}

} // namespace mc::client::renderer::trident::particle::particles
