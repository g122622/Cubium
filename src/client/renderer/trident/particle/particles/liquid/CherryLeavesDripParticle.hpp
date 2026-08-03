/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
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
 * @brief 樱花树叶悬挂粒子（滴落状态）
 *
 * 樱花树叶花瓣从树上悬挂滴落的效果。
 * 悬挂阶段无重力，70% 生命周期后转为下落状态。
 */
class DrippingCherryLeavesParticle : public Particle {
public:
    DrippingCherryLeavesParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/cherry");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.0;
    static constexpr f64 DEFAULT_SIZE = 0.06;
    static constexpr f64 SIZE_VARIATION = 0.02;
    static constexpr f64 DEFAULT_LIFETIME = 40.0;
    static constexpr f64 LIFETIME_VARIATION = 10.0;
    static constexpr f64 FRICTION = 0.98;
    static constexpr f64 FALLING_GRAVITY = 0.01;

    bool m_isFalling = false;
    f64 m_initialSize;
};

/**
 * @brief 樱花树叶下落粒子
 *
 * 樱花花瓣飘落效果，受微弱重力影响，带有正弦摆动。
 * 有碰撞检测，落地后消失。
 */
class FallingCherryLeavesParticle : public Particle {
public:
    FallingCherryLeavesParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/cherry");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.003;
    static constexpr f64 DEFAULT_SIZE = 0.06;
    static constexpr f64 SIZE_VARIATION = 0.02;
    static constexpr f64 DEFAULT_LIFETIME = 30.0;
    static constexpr f64 LIFETIME_VARIATION = 10.0;
    static constexpr f64 FRICTION = 0.97;

    f64 m_initialSize;
};

/**
 * @brief 樱花树叶落地粒子
 *
 * 樱花花瓣落地后静止的效果，仅淡出消失。
 */
class LandingCherryLeavesParticle : public Particle {
public:
    LandingCherryLeavesParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/cherry");
    }

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.0;
    static constexpr f64 DEFAULT_SIZE = 0.06;
    static constexpr f64 SIZE_VARIATION = 0.02;
    static constexpr f64 DEFAULT_LIFETIME = 20.0;
    static constexpr f64 LIFETIME_VARIATION = 5.0;
};

} // namespace mc::client::renderer::trident::particle::particles
