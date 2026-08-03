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
 * @brief 灰烬粒子
 *
 * 特性：
 * - 灰橙色发光粒子
 * - 受重力影响缓慢下落
 * - 随机水平漂移
 * - 淡入淡出
 */
class AshParticle : public Particle {
public:
    AshParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::PARTICLE_SHEET_LIT; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/ash");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        return 0xF0;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.003;
    static constexpr f64 DEFAULT_SIZE = 0.02;
    static constexpr f64 DEFAULT_LIFETIME = 20.0;
};

/**
 * @brief 白色灰烬粒子
 *
 * 特性：
 * - 白灰色发光粒子
 * - 受重力影响缓慢下落
 * - 随机水平漂移
 * - 淡入淡出
 */
class WhiteAshParticle : public Particle {
public:
    WhiteAshParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::PARTICLE_SHEET_LIT; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/white_ash");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        return 0xF0;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.002;
    static constexpr f64 DEFAULT_SIZE = 0.02;
    static constexpr f64 DEFAULT_LIFETIME = 20.0;
};

} // namespace mc::client::renderer::trident::particle::particles
