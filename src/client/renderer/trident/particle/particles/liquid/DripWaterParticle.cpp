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

#include "DripWaterParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/particles/liquid/DripParticle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

DripWaterParticle::DripWaterParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : DripParticle(pos, velocity, DripType::Water)
{
    setSize(0.01f + m_random.nextFloat() * 0.01f);

    setColor(glm::vec4(0.7f, 0.7f, 1.0f, 0.8f));

    setMaxAge(40.0f);
}

std::unique_ptr<Particle> DripWaterParticle::createDripping(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<DripWaterParticle>(pos, velocity);
}

std::unique_ptr<Particle> DripWaterParticle::createFalling(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripWaterParticle>(pos, velocity);
    particle->m_dripState = DripState::Falling;
    particle->m_dripProgress = 1.0;
    particle->setGravity(0.06f);
    return particle;
}

std::unique_ptr<Particle> DripWaterParticle::createLanding(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    auto particle = std::make_unique<DripWaterParticle>(pos, velocity);
    particle->m_dripState = DripState::Landed;
    particle->setMaxAge(16.0f);
    particle->setSize(0.04f);
    return particle;
}

u32 DripWaterParticle::getLightColor(mc::client::ClientWorld* world) const
{
    return Particle::getLightColor(world);
}

void DripWaterParticle::onLand(mc::client::ClientWorld* world)
{
    // 父类 DripParticle::onLand() 已通过 emitCallback 生成落地粒子
    DripParticle::onLand(world);
}

} // namespace mc::client::renderer::trident::particle::particles
