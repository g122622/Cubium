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

#include "NetherSporeParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// CrimsonSporeParticle
// ============================================================================

CrimsonSporeParticle::CrimsonSporeParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE + m_random.nextFloat() * SIZE_VARIATION);

    // 红紫色
    f32 r = static_cast<f32>(0.6 + m_random.nextFloat() * 0.2);
    f32 g = static_cast<f32>(0.1 + m_random.nextFloat() * 0.1);
    f32 b = static_cast<f32>(0.2 + m_random.nextFloat() * 0.1);
    setColor(glm::vec4(r, g, b, 1.0f));

    setFriction(FRICTION);
    setHasPhysics(false);

    // 生命周期 = 20 / (rand * 0.8 + 0.2)，范围 20-100 tick
    setMaxAge(DEFAULT_LIFETIME / (m_random.nextFloat() * 0.8 + 0.2));
}

std::unique_ptr<Particle> CrimsonSporeParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<CrimsonSporeParticle>(pos, velocity);
}

void CrimsonSporeParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 极慢漂移
    m_position += m_velocity;
    m_velocity *= static_cast<f32>(m_friction);
}

// ============================================================================
// WarpedSporeParticle
// ============================================================================

WarpedSporeParticle::WarpedSporeParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE + m_random.nextFloat() * SIZE_VARIATION);

    // 青绿色
    f32 r = static_cast<f32>(0.1 + m_random.nextFloat() * 0.1);
    f32 g = static_cast<f32>(0.4 + m_random.nextFloat() * 0.2);
    f32 b = static_cast<f32>(0.3 + m_random.nextFloat() * 0.2);
    setColor(glm::vec4(r, g, b, 1.0f));

    setFriction(FRICTION);
    setHasPhysics(false);

    // 生命周期 = 20 / (rand * 0.8 + 0.2)，范围 20-100 tick
    setMaxAge(DEFAULT_LIFETIME / (m_random.nextFloat() * 0.8 + 0.2));
}

std::unique_ptr<Particle> WarpedSporeParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<WarpedSporeParticle>(pos, velocity);
}

void WarpedSporeParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 极慢漂移
    m_position += m_velocity;
    m_velocity *= static_cast<f32>(m_friction);
}

} // namespace mc::client::renderer::trident::particle::particles
