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

#include "DripParticle.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/ParticleRenderType.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 滴水石水滴粒子
 *
 * 钟乳石滴水的专用粒子类型。与普通水滴粒子（DripWaterParticle）的区别：
 * - 落地时播放滴水石滴水音效（block.pointed_dripstone.drip_water）
 * - 落地后生成 Splash 粒子
 */
class DripstoneWaterDripParticle : public DripParticle {
public:
    DripstoneWaterDripParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法：创建滴水石水滴粒子（悬挂状态）
     */
    static std::unique_ptr<Particle> createDripping(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 工厂方法：创建滴水石下落水滴粒子
     */
    static std::unique_ptr<Particle> createFalling(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        if (m_dripState == DripState::Landed) {
            return ResourceLocation("minecraft:particle/splash");
        } else if (m_dripState == DripState::Falling) {
            return ResourceLocation("minecraft:particle/drip_fall");
        } else {
            return ResourceLocation("minecraft:particle/drip_hang");
        }
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override;

protected:
    void onLand(mc::client::ClientWorld* world) override;
};

/**
 * @brief 滴水石熔岩滴粒子
 *
 * 钟乳石滴熔岩的专用粒子类型。与普通熔岩滴粒子的区别：
 * - 落地时播放滴水石滴熔岩音效（block.pointed_dripstone.drip_lava）
 * - 落地后生成 LandingLava 粒子
 */
class DripstoneLavaDripParticle : public DripParticle {
public:
    DripstoneLavaDripParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法：创建滴水石熔岩滴粒子（悬挂状态）
     */
    static std::unique_ptr<Particle> createDripping(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 工厂方法：创建滴水石下落熔岩滴粒子
     */
    static std::unique_ptr<Particle> createFalling(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::PARTICLE_SHEET_LIT; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        if (m_dripState == DripState::Landed) {
            return ResourceLocation("minecraft:particle/drip_fall_lava");
        } else if (m_dripState == DripState::Falling) {
            return ResourceLocation("minecraft:particle/drip_fall");
        } else {
            return ResourceLocation("minecraft:particle/drip_hang");
        }
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override;

protected:
    void onLand(mc::client::ClientWorld* world) override;
};

} // namespace mc::client::renderer::trident::particle::particles
