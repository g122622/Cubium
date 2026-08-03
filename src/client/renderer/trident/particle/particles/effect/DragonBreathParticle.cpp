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

#include "DragonBreathParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// DragonBreathParticle
// ============================================================================

DragonBreathParticle::DragonBreathParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
    , m_hasLanded(false)
{
    setGravity(0.0f);
    setSize(0.1f + m_random.nextFloat() * 0.05f);
    m_initialSize = size();
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 10.0);

    // 紫色龙息颜色
    f32 purple = 0.8f + m_random.nextFloat() * 0.2f;
    setColor(glm::vec4(purple * 0.6f, 0.0f, purple, 1.0f));

    // 添加随机初始速度
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.1f;
    m_velocity.y += m_random.nextFloat() * 0.1f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.1f;
}

std::unique_ptr<Particle> DragonBreathParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<DragonBreathParticle>(pos, velocity);
}

void DragonBreathParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 随机漂浮运动
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.01f;
    m_velocity.y += (m_random.nextFloat() - 0.5f) * 0.01f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.01f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄淡出
    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(1.0f - lifeRatio * lifeRatio);

    // 轻微缩小
    setSize(m_initialSize * (1.0f - lifeRatio * 0.3f));
}

f64 DragonBreathParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// EndRodParticle
// ============================================================================

EndRodParticle::EndRodParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.02f)
    , m_brightness(0.95f)
{
    setGravity(-5e-4f); // 轻微向上浮动
    setSize(0.02f + m_random.nextFloat() * 0.01f);
    m_initialSize = size();
    setFriction(0.98f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 12.0);

    // 白色/淡黄色
    m_brightness = 0.95f + m_random.nextFloat() * 0.05f;
    setColor(glm::vec4(m_brightness, m_brightness, 1.0f, 1.0f));

    // 向上运动
    m_velocity.y -= 0.02f; // 负重力让粒子向上
}

std::unique_ptr<Particle> EndRodParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<EndRodParticle>(pos, velocity);
}

void EndRodParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用向上的"重力"
    m_velocity.y -= static_cast<f32>(m_gravity * 0.04);

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 颜色渐变淡出
    f64 lifeRatio = m_age / m_maxAge;
    f64 fade = 1.0f - lifeRatio;
    m_color.a = static_cast<f32>(fade);
    m_color.r = static_cast<f32>(m_brightness * fade);
    m_color.g = static_cast<f32>(m_brightness * fade);
}

f64 EndRodParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// SweepAttackParticle
// ============================================================================

SweepAttackParticle::SweepAttackParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_scaleMultiplier(1.0)
{
    // 固定生命周期 4 tick
    setMaxAge(4.0);

    // 随机灰白色
    f32 gray = m_random.nextFloat() * 0.6f + 0.4f;
    setColor(glm::vec4(gray, gray, gray, 1.0f));

    m_scaleMultiplier = 1.0 - velocity.x * 0.5;

    // 无重力、无摩擦、无碰撞
    setGravity(0.0f);
    setFriction(1.0f);
    setHasPhysics(false);

    // 无运动
    m_velocity = glm::vec3(0.0f);
}

std::unique_ptr<Particle> SweepAttackParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SweepAttackParticle>(pos, velocity);
}

void SweepAttackParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 无运动，仅更新纹理帧
}

ResourceLocation SweepAttackParticle::getTextureLocation() const
{
    // 根据年龄选择纹理帧（4帧）
    i32 frame = static_cast<i32>(m_age);
    frame = std::min(frame, 3);
    return ResourceLocation("minecraft:particle/sweep_" + std::to_string(frame));
}

f64 SweepAttackParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return m_scaleMultiplier;
}

} // namespace mc::client::renderer::trident::particle::particles
