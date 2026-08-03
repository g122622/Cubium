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
 * @brief 大型风爆发射器粒子
 *
 * 不可见的发射器粒子，在生命周期内每 tick 发射 2~3 个 GustParticle
 * 子粒子来创建风爆效果。
 */
class GustEmitterLargeParticle : public Particle {
public:
    /**
     * @brief 构造大型风爆发射器
     *
     * @param pos 位置
     * @param velocity 初始速度
     */
    GustEmitterLargeParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::NO_RENDER; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/gust");
    }

private:
    static constexpr f64 DEFAULT_LIFETIME = 3.0;
};

/**
 * @brief 小型风爆发射器粒子
 *
 * 不可见的发射器粒子，在生命周期内每 tick 发射 1~2 个 SmallGustParticle
 * 子粒子来创建小型风爆效果。
 */
class GustEmitterSmallParticle : public Particle {
public:
    /**
     * @brief 构造小型风爆发射器
     *
     * @param pos 位置
     * @param velocity 初始速度
     */
    GustEmitterSmallParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::NO_RENDER; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/small_gust");
    }

private:
    static constexpr f64 DEFAULT_LIFETIME = 2.0;
};

} // namespace mc::client::renderer::trident::particle::particles
