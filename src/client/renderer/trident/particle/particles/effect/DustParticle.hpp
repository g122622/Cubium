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
#include "common/util/assert/AssertAll.hpp"
#include <memory>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 灰尘粒子
 *
 * 可自定义颜色的灰尘粒子，红石粒子的泛化版本。
 * 发光、无重力、静止，颜色通过顶点色应用。
 */
class DustParticle : public Particle {
public:
    /**
     * @brief 构造灰尘粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param color 粒子颜色（RGBA）
     */
    DustParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color);

    /**
     * @brief 默认工厂方法
     *
     * 创建红色灰尘粒子（与红石粒子相同）。
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 带颜色工厂方法
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param world 客户端世界
     * @param color 粒子颜色
     * @return 粒子实例
     */
    static std::unique_ptr<Particle> createWithColor(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world, const glm::vec4& color);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/redstone");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        return 0xF0;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
};

/**
 * @brief 颜色过渡灰尘粒子
 *
 * 在生命周期内从一种颜色渐变到另一种颜色的灰尘粒子。
 * 发光、无重力、静止，颜色通过插值计算。
 */
class DustColorTransitionParticle : public Particle {
public:
    /**
     * @brief 构造颜色过渡灰尘粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param fromColor 起始颜色
     * @param toColor 目标颜色
     */
    DustColorTransitionParticle(
        const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& fromColor, const glm::vec4& toColor);

    /**
     * @brief 默认工厂方法
     *
     * 创建红到蓝颜色过渡的灰尘粒子。
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 带颜色工厂方法
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param world 客户端世界
     * @param fromColor 起始颜色
     * @param toColor 目标颜色
     * @return 粒子实例
     */
    static std::unique_ptr<Particle> createWithColors(const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world,
        const glm::vec4& fromColor,
        const glm::vec4& toColor);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/redstone");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        return 0xF0;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;

    /// 起始颜色
    glm::vec4 m_fromColor;

    /// 目标颜色
    glm::vec4 m_toColor;
};

} // namespace mc::client::renderer::trident::particle::particles
