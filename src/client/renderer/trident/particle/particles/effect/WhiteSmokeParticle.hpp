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
#include "common/util/assert/AssertAll.hpp"
#include <memory>

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
 * 参考: net.minecraft.client.particle.WhiteSmokeParticle
 * 继承关系: WhiteSmokeParticle 与 SmokeParticle 均继承自 BaseAshSmokeParticle (MC Java)
 * 区别: WhiteSmokeParticle 在构造函数中覆盖了颜色为白灰色 (R=0.729, G=0.694, B=0.761)
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
    /// 重力参数，与 MC Java BaseAshSmokeParticle 一致 (-0.1)
    static constexpr f64 DEFAULT_GRAVITY = -0.1f;
    /// 基础尺寸
    static constexpr f64 DEFAULT_SIZE = 0.1f;
    /// 默认生命周期帧数 (MC Java: 8 / (random * 0.8 + 0.2) * scale, scale=1.0)
    static constexpr f64 DEFAULT_LIFETIME = 8.0f;

    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
