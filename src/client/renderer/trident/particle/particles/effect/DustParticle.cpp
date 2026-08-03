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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "DustParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// DustParticle
// ============================================================================

DustParticle::DustParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color)
    : Particle(pos, velocity)
{
    setGravity(0.0f);
    // 灰尘粒子大小基于颜色强度
    f32 intensity = (color.r + color.g + color.b) / 3.0f;
    setSize(0.01f + intensity * 0.05f);
    setFriction(1.0f); // 无摩擦
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 4.0);

    setColor(color);
}

std::unique_ptr<Particle> DustParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 默认红色，与红石粒子相同
    return std::make_unique<DustParticle>(pos, velocity, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
}

std::unique_ptr<Particle> DustParticle::createWithColor(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world, const glm::vec4& color)
{
    MC_UNUSED(world);
    return std::make_unique<DustParticle>(pos, velocity, color);
}

void DustParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 灰尘粒子静止不动
    m_position += m_velocity;

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.5f) {
        m_color.a = static_cast<f32>(1.0f - (lifeRatio - 0.5f) * 2.0f);
    }
}

f64 DustParticle::getScale(f64 partialTick) const
{
    // 淡入效果：前 1/32 生命周期内从 0 渐变到 1.0
    f64 t = (m_age + partialTick) / m_maxAge;
    return mc::math::clamp(t * 32.0, 0.0, 1.0);
}

// ============================================================================
// DustColorTransitionParticle
// ============================================================================

DustColorTransitionParticle::DustColorTransitionParticle(
    const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& fromColor, const glm::vec4& toColor)
    : Particle(pos, velocity)
    , m_fromColor(fromColor)
    , m_toColor(toColor)
{
    setGravity(0.0f);
    // 灰尘粒子大小基于起始颜色强度
    f32 intensity = (fromColor.r + fromColor.g + fromColor.b) / 3.0f;
    setSize(0.01f + intensity * 0.05f);
    setFriction(1.0f); // 无摩擦
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 4.0);

    setColor(fromColor);
}

std::unique_ptr<Particle> DustColorTransitionParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 默认红到蓝的过渡
    return std::make_unique<DustColorTransitionParticle>(
        pos, velocity, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
}

std::unique_ptr<Particle> DustColorTransitionParticle::createWithColors(const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world,
    const glm::vec4& fromColor,
    const glm::vec4& toColor)
{
    MC_UNUSED(world);
    return std::make_unique<DustColorTransitionParticle>(pos, velocity, fromColor, toColor);
}

void DustColorTransitionParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 灰尘粒子静止不动
    m_position += m_velocity;

    // 颜色插值
    f64 lifeRatio = m_age / m_maxAge;
    m_color.r = static_cast<f32>(m_fromColor.r + (m_toColor.r - m_fromColor.r) * lifeRatio);
    m_color.g = static_cast<f32>(m_fromColor.g + (m_toColor.g - m_fromColor.g) * lifeRatio);
    m_color.b = static_cast<f32>(m_fromColor.b + (m_toColor.b - m_fromColor.b) * lifeRatio);

    // 淡出
    if (lifeRatio > 0.5f) {
        m_color.a = static_cast<f32>(1.0f - (lifeRatio - 0.5f) * 2.0f);
    }
}

f64 DustColorTransitionParticle::getScale(f64 partialTick) const
{
    // 淡入效果：前 1/32 生命周期内从 0 渐变到 1.0
    f64 t = (m_age + partialTick) / m_maxAge;
    return mc::math::clamp(t * 32.0, 0.0, 1.0);
}

} // namespace mc::client::renderer::trident::particle::particles
