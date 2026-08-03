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
 * @brief 绯红孢子粒子
 *
 * 绯红森林中漂浮的红紫色孢子，非常小的尺寸，缓慢漂浮。
 * 类似于 SuspendedTownParticle 但带有红紫色。
 */
class CrimsonSporeParticle : public Particle {
public:
    CrimsonSporeParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/crimson_spore");
    }

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.0;
    static constexpr f64 DEFAULT_SIZE = 0.02;
    static constexpr f64 SIZE_VARIATION = 0.005;
    static constexpr f64 DEFAULT_LIFETIME = 20.0;
    static constexpr f64 FRICTION = 0.99;
};

/**
 * @brief 诡异孢子粒子
 *
 * 诡异森林中漂浮的青绿色孢子，非常小的尺寸，缓慢漂浮。
 * 与 CrimsonSporeParticle 共享逻辑但颜色不同。
 */
class WarpedSporeParticle : public Particle {
public:
    WarpedSporeParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/warped_spore");
    }

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.0;
    static constexpr f64 DEFAULT_SIZE = 0.02;
    static constexpr f64 SIZE_VARIATION = 0.005;
    static constexpr f64 DEFAULT_LIFETIME = 20.0;
    static constexpr f64 FRICTION = 0.99;
};

} // namespace mc::client::renderer::trident::particle::particles
