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

#include "VillagerParticle.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// AngryVillagerParticle
// ============================================================================

AngryVillagerParticle::AngryVillagerParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
{
    mc::math::Random rng;

    setGravity(0.0f);
    setSize(0.05f + rng.nextFloat() * 0.03f);
    m_initialSize = size();
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 4.0);

    // 灰色/深灰色
    f32 gray = 0.3f + rng.nextFloat() * 0.2f;
    setColor(glm::vec4(gray, gray, gray, 0.8f));

    // 轻微向上漂浮
    m_velocity.y += 0.01f;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.02f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.02f;
}

std::unique_ptr<Particle> AngryVillagerParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<AngryVillagerParticle>(pos, velocity);
}

void AngryVillagerParticle::tick(mc::client::ClientWorld* world)
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
    mc::math::Random rng;
    m_velocity.y += 0.003f;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.002f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.002f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.8f * (1.0f - lifeRatio));
}

f64 AngryVillagerParticle::getScale(f64 partialTick) const
{
    // 淡入效果：粒子从零尺寸快速增大到完整尺寸
    // 前 1/32 生命周期内从 0 渐变到 m_initialSize，之后保持 m_initialSize
    f64 t = (m_age + partialTick) / m_maxAge;
    return m_initialSize * mc::math::clamp(t * 32.0, 0.0, 1.0);
}

// ============================================================================
// HappyVillagerParticle
// ============================================================================

HappyVillagerParticle::HappyVillagerParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    mc::math::Random rng;

    setGravity(0.0f);
    setSize(0.05f + rng.nextFloat() * 0.03f);
    setFriction(0.95f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 5.0);

    // 绿色
    f32 green = 0.7f + rng.nextFloat() * 0.3f;
    setColor(glm::vec4(0.2f, green, 0.2f, 0.9f));

    // 向上漂浮
    m_velocity.y += 0.015f;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.03f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.03f;
}

std::unique_ptr<Particle> HappyVillagerParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<HappyVillagerParticle>(pos, velocity);
}

void HappyVillagerParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    mc::math::Random rng;
    m_velocity.y += 0.005f;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.003f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.003f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.9f * (1.0f - lifeRatio));
}

// ============================================================================
// SneezeParticle
// ============================================================================

SneezeParticle::SneezeParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.05f)
{
    mc::math::Random rng;

    setGravity(DEFAULT_GRAVITY);
    setSize(0.05f + rng.nextFloat() * 0.02f);
    m_initialSize = size();
    setFriction(0.98f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 5.0);

    // 淡绿色
    f32 green = 0.8f + rng.nextFloat() * 0.2f;
    setColor(glm::vec4(0.6f, green, 0.5f, 0.8f));

    // 向前喷射
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.1f;
    m_velocity.y += rng.nextFloat() * 0.05f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.1f;
}

std::unique_ptr<Particle> SneezeParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SneezeParticle>(pos, velocity);
}

void SneezeParticle::tick(mc::client::ClientWorld* world)
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

    m_position += m_velocity;
    m_velocity *= m_friction;

    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.8f * (1.0f - lifeRatio));
}

f64 SneezeParticle::getScale(f64 partialTick) const
{
    // 淡入效果：粒子从零尺寸快速增大到完整尺寸
    // 前 1/32 生命周期内从 0 渐变到 m_initialSize，之后保持 m_initialSize
    f64 t = (m_age + partialTick) / m_maxAge;
    return m_initialSize * mc::math::clamp(t * 32.0, 0.0, 1.0);
}

} // namespace mc::client::renderer::trident::particle::particles
