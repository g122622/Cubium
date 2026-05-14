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

#include "../../Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 鹦鹉螺粒子
 *
 * 参考 MC 1.16.5 NautilusParticle
 *
 * 特性：
 * - 发光粒子（最大亮度）
 * - 无物理碰撞
 * - 向目标位置移动
 * - 用于潮涌核心效果
 *
 * 用法：
 * - 位置：粒子起始位置
 * - 速度：目标偏移（速度向量表示粒子飞向的目标方向）
 */
class NautilusParticle : public Particle {
public:
    /**
     * @brief 构造鹦鹉螺粒子
     *
     * @param pos 起始位置
     * @param velocity 目标方向/速度
     */
    NautilusParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂函数
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::PARTICLE_SHEET_LIT; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/nautilus");
    }

    /**
     * @brief 获取固定高亮度
     *
     * MC 1.16.5: 鹦鹉螺粒子是发光粒子，使用最大亮度
     */
    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        // 固定高亮度 15728880 (blockLight=15, skyLight=15)
        return 15728880;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 60.0; // 约 3 秒
    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
