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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "DustPillarParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/particles/block/DiggingParticle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// DustPillarParticle
// ============================================================================

DustPillarParticle::DustPillarParticle(const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState)
    : DiggingParticle(pos, velocity, blockState)
{
    // 重写 DiggingParticle 的属性以匹配 MC Java DustPillarProvider 行为：
    // - 重力 1.0（DiggingParticle 默认 0.03，DustPillar 需要更重的重力形成先扬后抑的柱状效果）
    //   在本项目中，重力通过 velocity.y -= gravity * 0.04 计算，
    //   gravity=1.0 对应 MC Java 的 gravity=1.0F
    // - 生命周期 20-40 tick（DiggingParticle 默认基于 maxAge 公式）
    // - 颜色乘以 0.6 基础亮度（MC Java TerrainParticle 默认 rCol=gCol=bCol=0.6F）
    setGravity(1.0);
    setMaxAge(20.0 + m_random.nextInt(21));
    setColor(glm::vec4(m_color.r * 0.6f, m_color.g * 0.6f, m_color.b * 0.6f, m_color.a));

    // 重写速度以匹配 MC Java DustPillarProvider.setParticleSpeed() 行为：
    // - X/Z 速度替换为 nextGaussian() / 30.0（极低水平扩散，形成窄柱效果）
    // - Y 速度保留传入的原始 Y 分量并叠加 nextGaussian() / 2.0（先扬后抑的抛物线）
    f32 originalY = static_cast<f32>(m_velocity.y);
    setVelocity(glm::vec3(static_cast<f32>(m_random.nextGaussian()) / 30.0f,
        originalY + static_cast<f32>(m_random.nextGaussian()) / 2.0f,
        static_cast<f32>(m_random.nextGaussian()) / 30.0f));
}

std::unique_ptr<Particle> DustPillarParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 默认工厂方法：使用石头方块状态（不推荐，应使用 createWithBlock）
    static const BlockState* stoneState = nullptr;
    if (stoneState == nullptr) {
        if (auto* stone = VanillaBlocks::STONE; stone != nullptr) {
            stoneState = &stone->defaultState();
        }
    }
    if (stoneState != nullptr) {
        return std::make_unique<DustPillarParticle>(pos, velocity, *stoneState);
    }
    return nullptr;
}

std::unique_ptr<Particle> DustPillarParticle::createWithBlock(
    const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState)
{
    return std::make_unique<DustPillarParticle>(pos, velocity, blockState);
}

void DustPillarParticle::tick(mc::client::ClientWorld* world)
{
    // 使用 DiggingParticle 的 tick 逻辑（重力、摩擦、碰撞、淡出等）
    // DiggingParticle::tick 会应用 m_gravity * PARTICLE_GRAVITY_MULTIPLIER (0.04)
    // 我们设置的 gravity=1.0 使得 velocity.y -= 0.04/tick，匹配 MC Java 行为
    DiggingParticle::tick(world);
}

} // namespace mc::client::renderer::trident::particle::particles
