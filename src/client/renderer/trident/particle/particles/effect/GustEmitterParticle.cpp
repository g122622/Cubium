/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software or
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

#include "GustEmitterParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// GustEmitterLargeParticle
// ============================================================================

GustEmitterLargeParticle::GustEmitterLargeParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(0.0f);
    setSize(0.0f);
    setFriction(1.0f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 2.0);
}

std::unique_ptr<Particle> GustEmitterLargeParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<GustEmitterLargeParticle>(pos, velocity);
}

void GustEmitterLargeParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 每 tick 发射 2~3 个 GustParticle 子粒子
    if (emitCallback()) {
        i32 count = 2 + m_random.nextInt(2); // [2, 3]
        for (i32 i = 0; i < count; ++i) {
            // 随机速度
            glm::vec3 vel((m_random.nextFloat() - 0.5f) * 0.1f,
                (m_random.nextFloat() - 0.5f) * 0.1f,
                (m_random.nextFloat() - 0.5f) * 0.1f);
            emitCallback()(ParticleTypeId::Gust, m_position, vel);
        }
    }
}

// ============================================================================
// GustEmitterSmallParticle
// ============================================================================

GustEmitterSmallParticle::GustEmitterSmallParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    setGravity(0.0f);
    setSize(0.0f);
    setFriction(1.0f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + m_random.nextFloat() * 1.0);
}

std::unique_ptr<Particle> GustEmitterSmallParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<GustEmitterSmallParticle>(pos, velocity);
}

void GustEmitterSmallParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 每 tick 发射 1~2 个 SmallGustParticle 子粒子
    if (emitCallback()) {
        i32 count = 1 + m_random.nextInt(2); // [1, 2]
        for (i32 i = 0; i < count; ++i) {
            // 随机速度
            glm::vec3 vel((m_random.nextFloat() - 0.5f) * 0.08f,
                (m_random.nextFloat() - 0.5f) * 0.08f,
                (m_random.nextFloat() - 0.5f) * 0.08f);
            emitCallback()(ParticleTypeId::SmallGust, m_position, vel);
        }
    }
}

} // namespace mc::client::renderer::trident::particle::particles
