/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject of the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies of substantial portions of the Software.
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
 * @brief 物品拾取粒子
 *
 * 拾取物品时产生的小型向上烟雾效果。
 * 微弱负重力使粒子缓慢上升，快速淡出并缩小。
 *
 * TODO: 粒子数据管线尚未支持实体渲染数据传递，当前 create() 工厂方法使用默认值/回退行为。
 * 待 ParticleFactory 签名扩展后，应通过 createWithEntity() 方法传递真实数据。
 */
class ItemPickupParticle : public Particle {
public:
    ItemPickupParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/poof");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_GRAVITY = -0.01;
    static constexpr f64 DEFAULT_SIZE = 0.05;
    static constexpr f64 SIZE_VARIATION = 0.02;
    static constexpr f64 DEFAULT_LIFETIME = 10.0;
    static constexpr f64 LIFETIME_VARIATION = 5.0;
    static constexpr f64 FRICTION = 0.94;

    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
