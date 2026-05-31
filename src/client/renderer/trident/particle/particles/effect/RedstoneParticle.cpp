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

#include "RedstoneParticle.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// RedstoneParticle
// ============================================================================

RedstoneParticle::RedstoneParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
{
    mc::math::Random rng;

    setGravity(0.0f);
    // 红石粒子大小基于颜色强度
    f32 intensity = (color.r + color.g + color.b) / 3.0f;
    setSize(0.01f + intensity * 0.05f);
    m_initialSize = size();
    setFriction(1.0f); // 无摩擦
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 4.0);

    // 设置颜色
    setColor(color);
}

std::unique_ptr<Particle> RedstoneParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 默认红色
    return std::make_unique<RedstoneParticle>(pos, velocity, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
}

void RedstoneParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 红石粒子静止不动
    m_position += m_velocity;

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.5f) {
        m_color.a = static_cast<f32>(1.0f - (lifeRatio - 0.5f) * 2.0f);
    }
}

f64 RedstoneParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// EnchantParticle
// ============================================================================

EnchantParticle::EnchantParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.02f)
{
    mc::math::Random rng;

    setGravity(0.0f);
    setSize(0.02f + rng.nextFloat() * 0.01f);
    m_initialSize = size();
    setFriction(0.95f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 10.0);

    // 紫色附魔颜色
    f32 purple = 0.7f + rng.nextFloat() * 0.3f;
    setColor(glm::vec4(purple, 0.0f, purple * 1.2f, 1.0f));

    // 向上运动
    m_velocity.y += 0.02f + rng.nextFloat() * 0.02f;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.05f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.05f;
}

std::unique_ptr<Particle> EnchantParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<EnchantParticle>(pos, velocity);
}

void EnchantParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 曲线运动
    mc::math::Random rng;
    f64 lifeRatio = m_age / m_maxAge;

    // 随机摆动
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.01f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.01f;

    // 继续向上
    m_velocity.y *= 0.98f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 淡出
    m_color.a = static_cast<f32>(1.0f - lifeRatio * 0.5f);
}

f64 EnchantParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// FallingDustParticle
// ============================================================================

FallingDustParticle::FallingDustParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
{
    mc::math::Random rng;

    setGravity(DEFAULT_GRAVITY);
    setSize(0.05f + rng.nextFloat() * 0.02f);
    m_initialSize = size();
    setFriction(0.98f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 10.0);

    setColor(color);

    // 设置旋转
    setRoll(rng.nextFloat() * mc::math::PI_DOUBLE * 2.0);
}

std::unique_ptr<Particle> FallingDustParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 默认灰色
    return std::make_unique<FallingDustParticle>(pos, velocity, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
}

void FallingDustParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= static_cast<f32>(m_gravity * 0.04);

    // 随机水平漂移
    mc::math::Random rng;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.002f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.002f;

    m_position += m_velocity;
    m_velocity.x *= m_friction;
    m_velocity.z *= m_friction;

    // 旋转
    m_roll += 0.1f;

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.7f) {
        m_color.a = static_cast<f32>(1.0f - (lifeRatio - 0.7f) / 0.3f);
    }
}

f64 FallingDustParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

} // namespace mc::client::renderer::trident::particle::particles
