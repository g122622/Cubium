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
 * @brief 守卫者外观粒子
 *
 * 远古守卫者出现的诅咒效果粒子。深蓝灰色，向上漂浮并淡出。
 */
class ElderGuardianParticle : public Particle {
public:
    /**
     * @brief 构造守卫者外观粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     */
    ElderGuardianParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法
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
        return ResourceLocation("minecraft:particle/entity_effect");
    }

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
};

} // namespace mc::client::renderer::trident::particle::particles
