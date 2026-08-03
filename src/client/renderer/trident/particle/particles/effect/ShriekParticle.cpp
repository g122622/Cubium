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

#include "ShriekParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

ShriekParticle::ShriekParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.1)
{
    setGravity(0.0);
    setSize(0.1 + m_random.nextFloat() * 0.03);
    m_initialSize = size();
    setFriction(0.96);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 10.0);

    // 深蓝色
    f32 b = 0.3f + m_random.nextFloat() * 0.2f;
    setColor(glm::vec4(0.15f, 0.1f, b, 0.0f)); // 起始 alpha=0，实现延迟出现效果

    // 轻微向上漂浮
    m_velocity.y += 0.005f;
}

std::unique_ptr<Particle> ShriekParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<ShriekParticle>(pos, velocity);
}

void ShriekParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 向上漂浮
    m_velocity.y += 0.002f;

    // 随机漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.002f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.002f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    f64 lifeRatio = m_age / m_maxAge;

    // 前 20% 生命周期淡入，之后淡出
    if (lifeRatio < 0.2) {
        // 淡入阶段
        m_color.a = static_cast<f32>(0.7 * (lifeRatio / 0.2));
    } else {
        // 淡出阶段
        m_color.a = static_cast<f32>(0.7 * (1.0 - lifeRatio));
    }
}

f64 ShriekParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
