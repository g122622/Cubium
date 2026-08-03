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

#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include <memory>
#include <glm/common.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

VaultConnectionParticle::VaultConnectionParticle(
    const glm::vec3& pos, const Vector3d& targetPosition, i32 arrivalInTicks)
    : Particle(pos, glm::vec3(0.0f))
    , m_targetPosition(targetPosition)
    , m_arrivalInTicks(arrivalInTicks)
{
    setGravity(0.0f);
    // quadSize = 1.5F * 0.1F * (random * 0.5F + 0.2F)
    setSize(1.5 * 0.1 * (0.2 + m_random.nextFloat() * 0.5));
    setHasPhysics(false);

    // 蓝白色 f = random*0.6+0.4, color = (0.9*f, 0.9*f, f)
    f32 f = m_random.nextFloat() * 0.6f + 0.4f;
    setColor(glm::vec4(0.9f * f, 0.9f * f, f, 1.0f));

    // 生命周期为到达时间
    setMaxAge(static_cast<f64>(arrivalInTicks));

    setFriction(1.0f);
}

std::unique_ptr<Particle> VaultConnectionParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // velocity 参数即为目标偏移 (targetPos = pos + velocity)
    Vector3d targetPos(static_cast<f64>(pos.x) + static_cast<f64>(velocity.x),
        static_cast<f64>(pos.y) + static_cast<f64>(velocity.y),
        static_cast<f64>(pos.z) + static_cast<f64>(velocity.z));
    // lifetime = 30 + random.nextInt(10) (30~39 tick)
    mc::math::Random rng(static_cast<u64>(pos.x * 3129871.0 + pos.y * 11613187.0 + pos.z * 4598127.0));
    i32 arrivalInTicks = 30 + rng.nextInt(10);
    return std::make_unique<VaultConnectionParticle>(pos, targetPos, arrivalInTicks);
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

    // 淡入(0→0.6) 再淡出(0.6→0)
    f64 ageRatio = m_age / m_maxAge;
    if (ageRatio < 0.25) {
        // 淡入：0 → 0.6
        m_color.a = static_cast<f32>(ageRatio / 0.25 * 0.6);
    } else {
        m_color.a = static_cast<f32>(0.6);
    }
    // 最后 75% 生命周期淡出
    if (ageRatio > 0.25) {
        f64 fadeRatio = (ageRatio - 0.25) / 0.75;
        m_color.a = static_cast<f32>(0.6 * (1.0 - fadeRatio));
    }
}

f64 VaultConnectionParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
