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

#include "PortalParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

PortalParticle::PortalParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_startX(pos.x)
    , m_startZ(pos.z)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8 + static_cast<f64>(m_random.nextFloat()) * 0.4));

    // 传送门颜色：紫色
    f64 purple = 0.6 + static_cast<f64>(m_random.nextFloat()) * 0.4;
    setColor(glm::vec4(0.4f, 0.1f, static_cast<f32>(purple), 0.8f));

    setFriction(0.95);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7 + static_cast<f64>(m_random.nextFloat()) * 0.6));
}

std::unique_ptr<Particle> PortalParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<PortalParticle>(pos, velocity);
}

void PortalParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 传送门粒子向下飘落，带有水平摆动
    f64 ageRatio = m_age / m_maxAge;

    // 水平摆动
    f64 swing = std::sin(ageRatio * mc::math::PI * 4.0) * 0.05;
    m_position.x = static_cast<f32>(m_startX + swing);
    m_position.z = static_cast<f32>(m_startZ + std::cos(ageRatio * mc::math::PI * 4.0) * 0.05);

    // 向下移动
    m_position.y += m_velocity.y;

    // 旋转
    m_roll += 0.1;

    // 淡出
    if (ageRatio > 0.5) {
        m_color.a = static_cast<f32>(0.8 * (1.0 - (ageRatio - 0.5) * 2.0));
    }
}

// ============================================================================
// ReversePortalParticle
// ============================================================================

ReversePortalParticle::ReversePortalParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_startX(pos.x)
    , m_startZ(pos.z)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8 + static_cast<f64>(m_random.nextFloat()) * 0.4));

    // 反向传送门颜色：绿色
    f64 green = 0.6 + static_cast<f64>(m_random.nextFloat()) * 0.4;
    setColor(glm::vec4(0.1f, static_cast<f32>(green), 0.4f + m_random.nextFloat() * 0.2f, 0.8f));

    setFriction(0.95);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7 + static_cast<f64>(m_random.nextFloat()) * 0.6));
}

std::unique_ptr<Particle> ReversePortalParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<ReversePortalParticle>(pos, velocity);
}

void ReversePortalParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 反向传送门粒子向下飘落，带有水平摆动
    f64 ageRatio = m_age / m_maxAge;

    // 水平摆动
    f64 swing = std::sin(ageRatio * mc::math::PI * 4.0) * 0.05;
    m_position.x = static_cast<f32>(m_startX + swing);
    m_position.z = static_cast<f32>(m_startZ + std::cos(ageRatio * mc::math::PI * 4.0) * 0.05);

    // 向下移动
    m_position.y += m_velocity.y;

    // 反向旋转
    m_roll -= 0.1;

    // 淡出
    if (ageRatio > 0.5) {
        m_color.a = static_cast<f32>(0.8 * (1.0 - (ageRatio - 0.5) * 2.0));
    }
}

} // namespace mc::client::renderer::trident::particle::particles
