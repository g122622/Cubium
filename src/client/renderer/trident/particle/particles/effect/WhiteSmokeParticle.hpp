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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/ParticleRenderType.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 白色烟雾粒子
 *
 * 与 SmokeParticle 类似，但使用偏白灰色调，用于：
 * - 合成器发射物品时的方向性白烟 (WorldEvents::SHOOT_WHITE_SMOKE)
 * - 干涸恶魂方块的环境粒子
 *
 * 继承自 Particle 基类，物理参数：friction=0.96, gravity=-0.1, hasPhysics=true
 * 颜色为白灰色 (R=0.729, G=0.694, B=0.761)，与普通灰色烟雾区分
 */
class WhiteSmokeParticle : public Particle {
public:
    WhiteSmokeParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/smoke");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    /// 重力参数 (-0.1，负值使粒子向上漂移)
    static constexpr f64 DEFAULT_GRAVITY = -0.1;
    /// 基础尺寸
    static constexpr f64 DEFAULT_SIZE = 0.1;
    /// 默认生命周期帧数
    static constexpr f64 DEFAULT_LIFETIME = 8.0;

    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
