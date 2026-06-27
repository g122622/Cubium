/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation to the rights
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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "DustPillarParticle.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

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
    // - 生命周期 20-40 tick（DiggingParticle 默认 16-24 tick）
    // - 颜色乘以 0.6 基础亮度（MC Java TerrainParticle 默认 rCol=gCol=bCol=0.6F）
    setGravity(1.0);
    setMaxAge(20.0 + m_random.nextInt(21));
    setColor(glm::vec4(m_color.r * 0.6f, m_color.g * 0.6f, m_color.b * 0.6f, m_color.a));
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
