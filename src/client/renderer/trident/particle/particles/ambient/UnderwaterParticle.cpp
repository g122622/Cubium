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

#include "UnderwaterParticle.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::client::renderer::trident::particle::particles {

UnderwaterParticle::UnderwaterParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialAlpha(0.4f)
{
    mc::math::Random rng;

    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8f + rng.nextFloat() * 0.4f));
    m_initialAlpha = 0.2f + rng.nextFloat() * 0.3f;
    setColor(glm::vec4(0.6f, 0.8f, 1.0f, m_initialAlpha));

    setFriction(0.95f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7f + rng.nextFloat() * 0.6f));
}

std::unique_ptr<Particle> UnderwaterParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<UnderwaterParticle>(pos, velocity);
}

void UnderwaterParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 缓慢随机漂移
    mc::math::Random rng;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.002f;
    m_velocity.y += (rng.nextFloat() - 0.5f) * 0.002f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.002f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.6f) {
        m_color.a = m_initialAlpha * (1.0f - (lifeRatio - 0.6f) / 0.4f);
    }
}

} // namespace mc::client::renderer::trident::particle::particles
