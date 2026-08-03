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

#include "ExplosionParticle.hpp"
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
// ExplosionParticle
// ============================================================================

ExplosionParticle::ExplosionParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(1.0)
{
    setGravity(0.0f);
    setSize(1.0f + m_random.nextFloat() * 0.5f);
    m_initialSize = size();
    setFriction(1.0f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 4.0);

    // 爆炸颜色：亮白色/黄色
    setColor(glm::vec4(1.0f, 0.9f + m_random.nextFloat() * 0.1f, 0.7f + m_random.nextFloat() * 0.3f, 1.0f));
}

std::unique_ptr<Particle> ExplosionParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<ExplosionParticle>(pos, velocity);
}

void ExplosionParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 爆炸粒子静止并扩大
    m_position += m_velocity;

    // 随年龄快速扩大
    f64 lifeRatio = m_age / m_maxAge;
    f64 expansion = 1.0 + lifeRatio * 3.0;
    setSize(m_initialSize * expansion);

    // 快速淡出
    m_color.a = static_cast<f32>(1.0 - lifeRatio * lifeRatio);
}

f64 ExplosionParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

// ============================================================================
// LargeExplosionParticle
// ============================================================================

LargeExplosionParticle::LargeExplosionParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setMaxAge(6.0 + m_random.nextInt(4));

    f32 gray = m_random.nextFloat() * 0.6f + 0.4f;
    setColor(glm::vec4(gray, gray, gray, 1.0f));

    // 缩放随 xSpeed 参数变化
    f64 initialSize = 2.0 * (1.0 - velocity.x * 0.5);

    setGravity(0.0f);
    setSize(static_cast<f32>(initialSize));
    setFriction(1.0f);
    setHasPhysics(false);

    // 无运动
    m_velocity = glm::vec3(0.0f);
}

std::unique_ptr<Particle> LargeExplosionParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<LargeExplosionParticle>(pos, velocity);
}

void LargeExplosionParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 仅更新纹理帧，无运动
}

ResourceLocation LargeExplosionParticle::getTextureLocation() const
{
    i32 frame = static_cast<i32>((m_age / m_maxAge) * 4.0);
    frame = std::min(frame, 3);
    return ResourceLocation("minecraft:particle/explosion_" + std::to_string(frame));
}

f64 LargeExplosionParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
