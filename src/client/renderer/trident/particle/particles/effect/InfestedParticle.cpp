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
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "InfestedParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

InfestedParticle::InfestedParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE + m_random.nextFloat() * SIZE_VARIATION)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(m_initialSize);
    setFriction(FRICTION);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * LIFETIME_VARIATION);

    // 暗灰绿色
    f32 r = static_cast<f32>(0.3 + m_random.nextFloat() * 0.1);
    f32 g = static_cast<f32>(0.3 + m_random.nextFloat() * 0.1);
    f32 b = static_cast<f32>(0.25 + m_random.nextFloat() * 0.1);
    setColor(glm::vec4(r, g, b, 0.6f));
}

std::unique_ptr<Particle> InfestedParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<InfestedParticle>(pos, velocity);
}

void InfestedParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 随机漂移
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.01f;
    m_velocity.y += 0.001f; // 轻微上升
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.01f;

    // 摩擦衰减
    m_velocity *= static_cast<f32>(m_friction);

    m_position += m_velocity;

    // 生命周期比例
    f64 lifeRatio = m_age / m_maxAge;

    // 膨胀：1 + lifeRatio * 1.5
    setSize(m_initialSize * (1.0 + lifeRatio * 1.5));

    // 透明度淡出：0.6 * (1 - lifeRatio)
    m_color.a = static_cast<f32>(0.6 * (1.0 - lifeRatio));
}

f64 InfestedParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    // size 已在 tick() 中更新，这里返回 1.0
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
