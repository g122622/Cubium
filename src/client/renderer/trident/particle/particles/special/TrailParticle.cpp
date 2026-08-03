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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. In NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "TrailParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>
#include <glm/common.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

TrailParticle::TrailParticle(const glm::vec3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks)
    : Particle(pos, glm::vec3(0.0f))
    , m_targetPosition(targetPosition)
    , m_durationInTicks(durationInTicks)
{
    setGravity(0.0f);
    // 固定尺寸 0.26
    setSize(0.26);
    setHasPhysics(false);

    // 从 ARGB 颜色提取各通道，每通道随机微调 ±12.5%
    f32 a = static_cast<f32>((color >> 24) & 0xFF) / 255.0f;
    f32 r = static_cast<f32>((color >> 16) & 0xFF) / 255.0f;
    f32 g = static_cast<f32>((color >> 8) & 0xFF) / 255.0f;
    f32 b = static_cast<f32>(color & 0xFF) / 255.0f;

    // 每通道乘以 [0.875, 1.125] 的随机因子
    r *= 0.875f + m_random.nextFloat() * 0.25f;
    g *= 0.875f + m_random.nextFloat() * 0.25f;
    b *= 0.875f + m_random.nextFloat() * 0.25f;

    setColor(glm::vec4(r, g, b, a));

    // 生命周期为飞行持续时间
    setMaxAge(static_cast<f64>(durationInTicks));

    setFriction(1.0f);
}

std::unique_ptr<Particle> TrailParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 默认工厂：velocity 作为目标偏移（与 VaultConnectionParticle 相同的约定）
    // 默认白色 (0xFFFFFFFF)，默认持续时间 10 tick
    Vector3d targetPos(static_cast<f64>(pos.x) + static_cast<f64>(velocity.x),
        static_cast<f64>(pos.y) + static_cast<f64>(velocity.y),
        static_cast<f64>(pos.z) + static_cast<f64>(velocity.z));
    constexpr u32 DEFAULT_COLOR = 0xFFFFFFFF; // 白色 ARGB
    constexpr i32 DEFAULT_DURATION = 10;
    return std::make_unique<TrailParticle>(pos, targetPos, DEFAULT_COLOR, DEFAULT_DURATION);
}

std::unique_ptr<Particle> TrailParticle::createWithTarget(
    const glm::vec3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks)
{
    return std::make_unique<TrailParticle>(pos, targetPosition, color, durationInTicks);
}

void TrailParticle::tick(mc::client::ClientWorld* world)
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

    // 向目标位置插值移动（与 VaultConnectionParticle/VibrationSignalParticle 相同的缓动逻辑）
    f64 lerpFactor = 1.0 / static_cast<f64>(remainingTicks);
    m_position.x = static_cast<f32>(glm::mix(static_cast<f64>(m_position.x), m_targetPosition.x, lerpFactor));
    m_position.y = static_cast<f32>(glm::mix(static_cast<f64>(m_position.y), m_targetPosition.y, lerpFactor));
    m_position.z = static_cast<f32>(glm::mix(static_cast<f64>(m_position.z), m_targetPosition.z, lerpFactor));
}

f64 TrailParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

} // namespace mc::client::renderer::trident::particle::particles
