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
 * @brief 孢子花掉落粒子
 *
 * 从孢子花正下方掉落的绿色孢子粒子。
 * 缓慢下落，带有轻微的水平漂移，逐渐淡出。
 *
 * 特性：
 * - 微弱重力（缓慢下落）
 * - 轻微水平漂移
 * - 半透明绿色调
 * - 生命周期结束后淡出
 */
class FallingSporeBlossomParticle : public Particle {
public:
    FallingSporeBlossomParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/spore_blossom");
    }

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.01;  ///< 微弱重力，缓慢下落
    static constexpr f64 DEFAULT_SIZE = 0.04;     ///< 粒子大小
    static constexpr f64 DEFAULT_LIFETIME = 60.0; ///< 默认生命周期 (ticks)
    static constexpr f64 DRIFT_STRENGTH = 0.003;  ///< 水平漂移强度
    static constexpr f64 FADE_START_RATIO = 0.6;  ///< 开始淡出的生命比例
    static constexpr f64 FADE_RANGE = 0.4;        ///< 淡出区间长度

    f64 m_initialAlpha;
};

/**
 * @brief 孢子花空气粒子
 *
 * 在孢子花周围21x10x21区域内漂浮的环境粒子。
 * 无重力，缓慢随机漂移，半透明绿色调。
 *
 * 特性：
 * - 无重力（漂浮）
 * - 缓慢随机漂移
 * - 半透明绿色调
 * - 长生命周期，渐入渐出
 */
class SporeBlossomAirParticle : public Particle {
public:
    SporeBlossomAirParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/spore_blossom_air");
    }

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.0;    ///< 无重力
    static constexpr f64 DEFAULT_SIZE = 0.03;      ///< 粒子大小
    static constexpr f64 DEFAULT_LIFETIME = 100.0; ///< 默认生命周期 (ticks)
    static constexpr f64 DRIFT_STRENGTH = 0.002;   ///< 漂移强度
    static constexpr f64 FADE_START_RATIO = 0.5;   ///< 开始淡出的生命比例
    static constexpr f64 FADE_RANGE = 0.5;         ///< 淡出区间长度
    static constexpr f64 FADEIN_END_RATIO = 0.1;   ///< 淡入结束的生命比例

    f64 m_initialAlpha;
};

} // namespace mc::client::renderer::trident::particle::particles
