/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software be
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
 * @brief 试炼刷怪笼检测粒子
 *
 * 试炼刷怪笼激活时产生的橙黄色发光粒子。向上漂浮并淡出。
 */
class TrialSpawnerDetectionParticle : public Particle {
public:
    /**
     * @brief 构造试炼刷怪笼检测粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     */
    TrialSpawnerDetectionParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::PARTICLE_SHEET_LIT; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/trial_spawner_detection");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        return 0xF0;
    }

private:
    static constexpr f64 DEFAULT_LIFETIME = 20.0;
};

/**
 * @brief 试炼刷怪笼检测粒子（不祥变体）
 *
 * 试炼刷怪笼不祥激活时产生的蓝色-青色发光粒子。行为与 TrialSpawnerDetectionParticle
 * 相同但颜色不同。
 */
class TrialSpawnerDetectionOminousParticle : public Particle {
public:
    /**
     * @brief 构造试炼刷怪笼检测粒子（不祥变体）
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     */
    TrialSpawnerDetectionOminousParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::PARTICLE_SHEET_LIT; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/trial_spawner_detection_ominous");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        return 0xF0;
    }

private:
    static constexpr f64 DEFAULT_LIFETIME = 20.0;
};

} // namespace mc::client::renderer::trident::particle::particles
