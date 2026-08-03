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
 * @brief 爆炸粒子
 *
 * 特性：
 * - 发光粒子（最大亮度）
 * - 快速扩大
 * - 快速淡出
 *
 * 用法：
 * @code
 * auto explosion = std::make_unique<ExplosionParticle>(
 *     glm::vec3(x, y, z),
 *     glm::vec3(0.0f)
 * );
 * particleManager.addParticle(std::move(explosion));
 * @endcode
 */
class ExplosionParticle : public Particle {
public:
    /**
     * @brief 构造爆炸粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     */
    ExplosionParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::PARTICLE_SHEET_LIT; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/explosion");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        return 0xF0;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 6.0; // 约 0.3 秒

    f64 m_initialSize;
};

/**
 * @brief 大型爆炸粒子
 *
 * 特性：
 * - 生命周期 6-9 tick
 * - 发光粒子（最大亮度 15728880）
 * - 根据年龄选择纹理帧（动画）
 * - 无运动
 * - 缩放随 xSpeed 参数变化
 */
class LargeExplosionParticle : public Particle {
public:
    LargeExplosionParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::PARTICLE_SHEET_LIT; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override;

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        return 15728880;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
};

} // namespace mc::client::renderer::trident::particle::particles
