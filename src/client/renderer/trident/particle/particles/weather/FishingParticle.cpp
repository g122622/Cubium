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

#include "FishingParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

FishingParticle::FishingParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    // 钓鱼粒子向下移动
    setGravity(0.0); // 无重力
    setSize(DEFAULT_SIZE * (0.5f + m_random.nextFloat() * 0.5f));
    setColor(glm::vec4(0.8f, 0.9f, 1.0f, 0.6f)); // 淡蓝色半透明

    setFriction(0.98f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.5f + m_random.nextFloat()));
}

std::unique_ptr<Particle> FishingParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<FishingParticle>(pos, velocity);
}

void FishingParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 向下移动
    m_position += m_velocity;

    // 应用摩擦
    m_velocity.x *= m_friction;
    m_velocity.y *= m_friction;
    m_velocity.z *= m_friction;

    // 淡出效果
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.3f) {
        m_color.a = 0.6f * (1.0f - (lifeRatio - 0.3f) / 0.7f);
    }
}

} // namespace mc::client::renderer::trident::particle::particles
