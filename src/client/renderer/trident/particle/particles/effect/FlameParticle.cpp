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

#include "FlameParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

FlameParticle::FlameParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE)
{
    // 速度缩放后加上随机偏移
    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE);
    m_initialSize = size();

    // 火焰颜色：橙黄色
    f64 colorVariation = m_random.nextFloat() * 0.2f;
    setColor(glm::vec4(1.0f, 0.6f + static_cast<f32>(colorVariation), 0.1f, 1.0f));

    setFriction(0.96f);
    setHasPhysics(false);

    // 生命周期带随机浮动
    setMaxAge(DEFAULT_LIFETIME * (0.8f + m_random.nextFloat() * 0.4f));
}

std::unique_ptr<Particle> FlameParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<FlameParticle>(pos, velocity);
}

void FlameParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    // 保存上一帧位置（用于插值）
    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    // 生命周期递增
    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 速度衰减
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.y *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // 火焰粒子不做碰撞检测，直接移动
    m_position += m_velocity;

    // 根据生命周期缩小粒子
    f64 lifeRatio = m_age / m_maxAge;
    f64 scale = 1.0f - lifeRatio * lifeRatio * 0.5f;
    setSize(m_initialSize * scale);

    // 生命周期后半段淡出
    if (lifeRatio > 0.5f) {
        m_color.a = static_cast<f32>(1.0f - (lifeRatio - 0.5f) * 2.0f);
    }
}

f64 FlameParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    // size 已在 tick() 中根据生命周期更新，这里返回 1.0
    return 1.0f;
}

} // namespace mc::client::renderer::trident::particle::particles
