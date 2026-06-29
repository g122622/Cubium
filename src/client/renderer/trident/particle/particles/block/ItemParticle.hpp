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

#pragma once

#include "client/renderer/trident/particle/Particle.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <memory>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 物品粒子（物品破碎、史莱姆、雪球、蛛网等）
 *
 * 物品破碎时产生的粒子效果，类似于 DiggingParticle 但用于物品纹理。
 * 目前使用占位纹理 "minecraft:particle/generic"，待 ItemModelCache 集成后
 * 将使用物品纹理图集渲染。
 *
 * Item、ItemSlime、ItemCobweb、ItemSnowball 均使用此类，
 * 仅通过不同的 ParticleTypeId 注册来区分。
 *
 * TODO: 粒子数据管线尚未支持ItemStack传递，当前 create() 工厂方法使用默认值/回退行为。
 * 待 ParticleFactory 签名扩展后，应通过 createWithItemStack() 方法传递真实数据。
 */
class ItemParticle : public Particle {
public:
    ItemParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::TERRAIN_SHEET; }

    /**
     * @brief 获取纹理位置
     *
     * 目前返回占位纹理，待 ItemModelCache 集成后使用物品纹理。
     * TODO: 集成 ItemModelCache 以支持物品纹理渲染
     */
    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/generic");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.03;
    static constexpr f64 DEFAULT_SIZE = 0.1;
    static constexpr f64 DEFAULT_LIFETIME = 20.0;
    static constexpr f64 FRICTION = 0.92;

    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
