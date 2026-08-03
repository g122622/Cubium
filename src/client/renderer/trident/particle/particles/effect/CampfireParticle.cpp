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

#include "CampfireParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

CampfireParticle::CampfireParticle(const glm::vec3& pos, const glm::vec3& velocity, CampfireType type)
    : Particle(pos, velocity)
    , m_campfireType(type)
{
    // 尺寸 0.25 x 0.25，缩放 3 倍
    setSize(static_cast<f32>(BASE_SIZE));
    setBoundingBox(static_cast<f32>(BASE_SIZE), static_cast<f32>(BASE_SIZE));

    // 生命周期随机：Cozy: 80 + rand(50)，Signal: 280 + rand(50)
    if (type == CampfireType::Signal) {
        setMaxAge(280.0 + m_random.nextInt(50));
        m_initialAlpha = 0.95;
    } else {
        setMaxAge(80.0 + m_random.nextInt(50));
        m_initialAlpha = 0.9;
    }

    // 非常小的"重力"（实际使粒子缓慢上升）
    setGravity(GRAVITY);
    setFriction(1.0);     // 无摩擦
    setHasPhysics(false); // 不做碰撞检测

    // Y速度增加随机量
    m_velocity.y += m_random.nextFloat() / 500.0f;

    // 初始颜色为灰色，alpha 根据类型设置
    setColor(glm::vec4(0.2f, 0.2f, 0.2f, static_cast<f32>(m_initialAlpha)));
}

std::unique_ptr<Particle> CampfireParticle::createCozy(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<CampfireParticle>(pos, velocity, CampfireType::Cozy);
}

std::unique_ptr<Particle> CampfireParticle::createSignal(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<CampfireParticle>(pos, velocity, CampfireType::Signal);
}

void CampfireParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    // 保存上一帧位置
    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    // 生命周期递增
    m_age += 1.0f;

    // 检查是否应该过期
    if (m_age >= m_maxAge || m_color.a <= 0.0f) {
        setExpired();
        return;
    }

    // 随机水平漂移
    m_velocity.x += (m_random.nextFloat() / 5000.0f) * (m_random.nextBoolean() ? 1.0f : -1.0f);
    m_velocity.z += (m_random.nextFloat() / 5000.0f) * (m_random.nextBoolean() ? 1.0f : -1.0f);

    // 应用重力（负值，向上）
    m_velocity.y -= static_cast<f32>(m_gravity);

    // 移动粒子
    m_position += m_velocity;

    // 最后 60 tick 淡出
    if (m_age >= m_maxAge - 60.0 && m_color.a > 0.01f) {
        m_color.a -= 0.015f;
    }
}

f64 CampfireParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    // 缩放倍数为 3.0
    return SCALE_MULTIPLIER;
}

} // namespace mc::client::renderer::trident::particle::particles
