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
#include <vector>
#include <glm/ext/vector_float3.hpp>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 雪花粒子
 *
 * 特性：
 * - 缓慢飘落
 * - 左右摇摆
 * - 随机大小和旋转
 *
 * 用法：
 * @code
 * auto snow = std::make_unique<SnowParticle>(
 *     glm::vec3(x, y, z),
 *     glm::vec3(0.0f, -0.5f, 0.0f)
 * );
 * particleManager.addParticle(std::move(snow));
 * @endcode
 */
class SnowParticle : public Particle {
public:
    /**
     * @brief 构造雪花粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     */
    SnowParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法：创建雪花粒子
     *
     * @param pos 位置
     * @param velocity 速度
     * @param world 客户端世界（可选）
     * @return 雪花粒子实例
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    void buildVertices(const glm::vec3& cameraPos,
        f64 partialTick,
        const ParticleTextureAtlas& atlas,
        std::vector<ParticleVertex>& outVertices) const override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/snowflake");
    }

private:
    static constexpr f64 SWING_AMPLITUDE = 0.1f;  ///< 摇摆振幅
    static constexpr f64 SWING_FREQUENCY = 0.05f; ///< 摇摆频率

    f64 m_swingPhase;     ///< 摇摆相位（随机初始值）
    f64 m_swingAmplitude; ///< 摇摆振幅（个体差异）
};

} // namespace mc::client::renderer::trident::particle::particles
