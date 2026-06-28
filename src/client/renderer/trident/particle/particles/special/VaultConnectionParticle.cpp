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

#include "VaultConnectionParticle.hpp"

#include <glm/glm.hpp>

namespace mc::client::renderer::trident::particle::particles {

VaultConnectionParticle::VaultConnectionParticle(
    const glm::vec3& pos, const Vector3d& targetPosition, i32 arrivalInTicks)
    : Particle(pos, glm::vec3(0.0f))
    , m_targetPosition(targetPosition)
    , m_arrivalInTicks(arrivalInTicks)
{
    setGravity(0.0f);
    setSize(0.04);
    setHasPhysics(false);

    // 橙金色
    setColor(glm::vec4(0.9f, 0.7f, 0.3f, 1.0f));

    // 生命周期为到达时间
    setMaxAge(static_cast<f64>(arrivalInTicks));

    setFriction(1.0f);
}

std::unique_ptr<Particle> VaultConnectionParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    MC_UNUSED(velocity);
    // 默认工厂：创建一个向正上方飞行 60 tick 的宝库连接粒子
    Vector3d targetPos(pos.x, pos.y + 8.0, pos.z);
    return std::make_unique<VaultConnectionParticle>(pos, targetPos, 60);
}

std::unique_ptr<Particle> VaultConnectionParticle::createWithTarget(
    const glm::vec3& pos, const Vector3d& targetPosition, i32 arrivalInTicks)
{
    return std::make_unique<VaultConnectionParticle>(pos, targetPosition, arrivalInTicks);
}

void VaultConnectionParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 计算剩余 tick 数
    i32 remainingTicks = static_cast<i32>(m_maxAge - m_age);
    if (remainingTicks <= 0) {
        setExpired();
        return;
    }

    // 向目标位置插值移动（与 VibrationSignalParticle 相同的缓动逻辑）
    f64 lerpFactor = 1.0 / static_cast<f64>(remainingTicks);
    m_position.x = static_cast<f32>(glm::mix(static_cast<f64>(m_position.x), m_targetPosition.x, lerpFactor));
    m_position.y = static_cast<f32>(glm::mix(static_cast<f64>(m_position.y), m_targetPosition.y, lerpFactor));
    m_position.z = static_cast<f32>(glm::mix(static_cast<f64>(m_position.z), m_targetPosition.z, lerpFactor));

    // 旋转效果
    m_roll += 0.05;

    // 70% 生命周期后开始淡出
    f64 ageRatio = m_age / m_maxAge;
    if (ageRatio > 0.7) {
        m_color.a = static_cast<f32>(1.0 - (ageRatio - 0.7) / 0.3);
    }
}

} // namespace mc::client::renderer::trident::particle::particles
