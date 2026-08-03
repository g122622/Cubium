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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SuspendedTownParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

SuspendedTownParticle::SuspendedTownParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(DEFAULT_GRAVITY);

    // this.setSize(0.02F, 0.02F) + this.quadSize *= (random * 0.6F + 0.5F)
    setSize(DEFAULT_SIZE * (0.5 + m_random.nextFloat() * 0.6));

    // 灰色 r/g/b = random * 0.1 + 0.2, alpha = 1.0
    f32 grey = static_cast<f32>(m_random.nextFloat() * 0.1 + 0.2);
    setColor(glm::vec4(grey, grey, grey, 1.0f));

    // velocity *= 0.02
    m_velocity *= static_cast<f32>(VELOCITY_SCALE);

    setFriction(FRICTION);
    setHasPhysics(false);

    // lifetime = (int)(20.0 / (random * 0.8 + 0.2))
    // 范围: 20 / 1.0 = 20 tick 到 20 / 0.2 = 100 tick
    setMaxAge(DEFAULT_LIFETIME / (m_random.nextFloat() * 0.8 + 0.2));
}

std::unique_ptr<Particle> SuspendedTownParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SuspendedTownParticle>(pos, velocity);
}

void SuspendedTownParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 简单位移 + 摩擦衰减
    // this.move(this.xd, this.yd, this.zd) — 简单位移，无碰撞
    // this.xd *= 0.99; this.yd *= 0.99; this.zd *= 0.99;
    m_position += m_velocity;
    m_velocity *= static_cast<f32>(m_friction);
}

} // namespace mc::client::renderer::trident::particle::particles
