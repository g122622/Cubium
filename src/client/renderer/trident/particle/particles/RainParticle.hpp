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
#include <memory>
#include <glm/ext/vector_float3.hpp>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 雨滴粒子
 *
 * 特性：
 * - 快速下落
 * - 碰撞地面时消失或产生溅射效果
 * - 雨滴大小和速度随机变化
 *
 * 用法：
 * @code
 * auto rain = std::make_unique<RainParticle>(
 *     glm::vec3(x, y, z),
 *     glm::vec3(0.0f, -3.0f, 0.0f)
 * );
 * particleManager.addParticle(std::move(rain));
 * @endcode
 */
class RainParticle : public Particle {
public:
    /**
     * @brief 构造雨滴粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     */
    RainParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法：创建雨滴粒子
     *
     * @param pos 位置
     * @param velocity 速度
     * @param world 客户端世界（可选）
     * @return 雨滴粒子实例
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/rain");
    }

private:
    static constexpr f64 DEFAULT_SIZE = 0.01;      ///< 雨滴大小
    static constexpr f64 TERMINAL_VELOCITY = -3.0; ///< 终端速度
};

} // namespace mc::client::renderer::trident::particle::particles
