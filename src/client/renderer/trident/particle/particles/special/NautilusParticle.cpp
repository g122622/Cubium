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

#include "NautilusParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

NautilusParticle::NautilusParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(0.0f);
    setSize(0.04 * (0.8 + m_random.nextFloat() * 0.4));

    // 鹦鹉螺粒子颜色：白色/淡蓝色
    f32 brightness = 0.8f + m_random.nextFloat() * 0.2f;
    setColor(glm::vec4(brightness, brightness, 1.0f, 1.0f));

    setFriction(0.95f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7 + m_random.nextFloat() * 0.6));
}

std::unique_ptr<Particle> NautilusParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<NautilusParticle>(pos, velocity);
}

void NautilusParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 粒子按照速度向量移动，速度逐渐衰减

    // 移动粒子
    m_position.x += m_velocity.x * 0.1f;
    m_position.y += m_velocity.y * 0.1f;
    m_position.z += m_velocity.z * 0.1f;

    // 速度衰减
    m_velocity.x *= 0.95f;
    m_velocity.y *= 0.95f;
    m_velocity.z *= 0.95f;

    // 旋转效果
    m_roll += 0.05;

    // 根据年龄淡出
    f64 ageRatio = m_age / m_maxAge;
    if (ageRatio > 0.5) {
        m_color.a = static_cast<f32>(1.0 - (ageRatio - 0.5) * 2.0);
    }
}

f64 NautilusParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);

    // 根据年龄缩放：开始时小，中间大，结束时淡出
    f64 ageRatio = m_age / m_maxAge;

    if (ageRatio < 0.3) {
        // 前期：从小变大
        return 0.5 + ageRatio / 0.3 * 0.5;
    } else if (ageRatio < 0.7) {
        // 中期：保持正常大小
        return 1.0;
    } else {
        // 后期：逐渐缩小
        return 1.0 - (ageRatio - 0.7) / 0.3 * 0.5;
    }
}

} // namespace mc::client::renderer::trident::particle::particles
