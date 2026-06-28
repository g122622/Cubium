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

#include "CopperFireFlameParticle.hpp"

namespace mc::client::renderer::trident::particle::particles {

CopperFireFlameParticle::CopperFireFlameParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.04)
{
    setGravity(0.0);
    setSize(0.04 * (0.6 + m_random.nextFloat() * 0.4));
    m_initialSize = size();

    // 铜橙色火焰颜色
    setColor(glm::vec4(0.9f + m_random.nextFloat() * 0.1f,
        0.5f + m_random.nextFloat() * 0.2f,
        0.2f + m_random.nextFloat() * 0.1f,
        1.0f));

    setFriction(0.95);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.8 + m_random.nextFloat() * 0.4));
}

std::unique_ptr<Particle> CopperFireFlameParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<CopperFireFlameParticle>(pos, velocity);
}

void CopperFireFlameParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 随机水平摇摆
    m_velocity.x += (m_random.nextFloat() - 0.5f) * 0.02f;
    m_velocity.z += (m_random.nextFloat() - 0.5f) * 0.02f;

    // 向上飘动
    m_velocity.y += 0.002f;

    m_position += m_velocity;
    m_velocity *= static_cast<f32>(m_friction);

    // 根据生命周期缩小粒子
    f64 lifeRatio = m_age / m_maxAge;
    setSize(m_initialSize * (1.0 - lifeRatio * 0.5));

    // 生命周期 50% 后淡出
    if (lifeRatio > 0.5) {
        m_color.a = static_cast<f32>(1.0 - (lifeRatio - 0.5) * 2.0);
    }
}

f64 CopperFireFlameParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
