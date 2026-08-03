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

#include <memory>
#include <glm/ext/vector_float3.hpp>

#include "client/renderer/trident/particle/Particle.hpp"
#include "client/renderer/trident/particle/ParticleRenderType.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 营火烟雾粒子
 *
 * 特性：
 * - 两种变体：普通烟雾（约80tick）和信号烟雾（约280tick）
 * - 向上缓慢飘动，带有水平随机漂移
 * - 生命周期结束前60tick开始淡出
 */
class CampfireParticle : public Particle {
public:
    /**
     * @brief 营火烟雾类型
     */
    enum class CampfireType {
        Cozy,  ///< 普通营火烟雾（约80tick，alpha=0.9）
        Signal ///< 信号营火烟雾（约280tick，alpha=0.95）
    };

    CampfireParticle(const glm::vec3& pos, const glm::vec3& velocity, CampfireType type);

    /**
     * @brief 工厂方法：创建普通营火烟雾
     */
    static std::unique_ptr<Particle> createCozy(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 工厂方法：创建信号营火烟雾
     */
    static std::unique_ptr<Particle> createSignal(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/campfire_smoke");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 GRAVITY = 3.0e-6;       ///< 非常小的"重力"（实际向上）
    static constexpr f64 BASE_SIZE = 0.25;       ///< 基础尺寸
    static constexpr f64 SCALE_MULTIPLIER = 3.0; ///< 缩放倍数

    CampfireType m_campfireType;
    f64 m_initialAlpha;
};

} // namespace mc::client::renderer::trident::particle::particles
