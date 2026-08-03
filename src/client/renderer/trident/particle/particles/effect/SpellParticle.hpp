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
#include <glm/ext/vector_float4.hpp>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 药水效果粒子基类
 *
 * 特性：
 * - 向上漂浮
 * - 无物理
 * - 半透明
 */
class SpellParticle : public Particle {
public:
    /**
     * @brief 构造药水粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param color 粒子颜色（ARGB）
     */
    SpellParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/spell");
    }

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
};

/**
 * @brief 瞬间药水效果粒子
 *
 * 与 SpellParticle 类似但颜色更亮。
 */
class InstantSpellParticle : public Particle {
public:
    InstantSpellParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/instant_spell");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        return 0xF0;
    }

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
};

/**
 * @brief 实体效果粒子
 *
 * 用于实体上的药水效果。MC 1.21.11 中 ambient_entity_effect 已被移除，
 * 其功能由此类通过低 alpha 值（环境模式）实现。
 * 环境模式通过构造时设置较低的初始 alpha 和更慢的漂浮速度来区分。
 */
class EntityEffectParticle : public Particle {
public:
    /**
     * @brief 构造实体效果粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param color 粒子颜色（ARGB）
     * @param ambient 是否为环境模式（信标效果，更透明、更慢）
     */
    EntityEffectParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color, bool ambient = false);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 创建环境模式的实体效果粒子
     *
     * 环境模式粒子的 alpha 值更低（约 15%，alpha=38/255≈0.149），
     * 漂浮速度更慢，生命周期更长。
     *
     * @param pos 初始位置
     * @param velocity 初始速度（RGB 编码在 xyz 分量中）
     * @param world 客户端世界
     * @return 粒子实例
     */
    static std::unique_ptr<Particle> createAmbient(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 从 RGBA 颜色创建实体效果粒子
     *
     * 用于粒子数据管线（EntityEffectParticleData）。直接使用 RGBA 颜色向量构造，
     * 不再从 velocity 中解码颜色。
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param world 客户端世界（未使用，保留以匹配工厂签名）
     * @param color 粒子颜色（RGBA，每个分量 0.0-1.0）
     * @return 粒子实例
     */
    static std::unique_ptr<Particle> createWithColor(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world, const glm::vec4& color);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/entity_effect");
    }

    /**
     * @brief 是否为环境模式
     *
     * 环境模式粒子更透明、漂浮更慢，用于信标等产生的 ambient 药水效果。
     */
    [[nodiscard]] bool isAmbient() const noexcept { return m_ambient; }

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
    static constexpr f64 AMBIENT_LIFETIME = 12.0;
    static constexpr f32 AMBIENT_ALPHA = 38.0f / 255.0f; ///< 环境模式 alpha = 38/255 ≈ 0.149

    bool m_ambient = false;
};

/**
 * @brief 巫师粒子
 *
 * 紫色的药水效果粒子。
 */
class WitchParticle : public Particle {
public:
    WitchParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/witch");
    }

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
};

} // namespace mc::client::renderer::trident::particle::particles
