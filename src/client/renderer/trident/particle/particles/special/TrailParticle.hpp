/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. In NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN NO EVENT SHALL THE
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
#include "common/util/math/Vector3.hpp"
#include <memory>
#include <glm/ext/vector_float3.hpp>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 轨迹粒子
 *
 * 从起始位置飞向目标位置的定向粒子效果，带有自定义颜色。
 * 用于诡异橡树眼球花和嘎枝心声方块的视觉反馈。
 *
 * 特性：
 * - 不透明渲染、全亮度光照
 * - 固定尺寸 0.26
 * - 颜色来自外部传入，每通道随机微调 ±12.5%
 * - 向目标位置加速飞行（指数缓动插值）
 * - alpha 始终为 1.0（不透明、不淡出）
 *
 * 数据管线：通过 TrailParticleData 传递目标位置、颜色和持续时间。
 * 当无 ParticleData 时，create() 回退到默认行为（velocity 作为目标偏移，白色，10 tick）。
 * 数据工厂已在 ParticleFactories.cpp 中注册。
 */
class TrailParticle : public Particle {
public:
    /**
     * @brief 构造轨迹粒子
     *
     * @param pos 起始位置
     * @param targetPosition 目标位置（飞向的终点）
     * @param color ARGB 颜色值
     * @param durationInTicks 飞行持续 tick 数（即粒子生命周期）
     */
    TrailParticle(const glm::vec3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks);

    /**
     * @brief 标准工厂方法（用于 ParticleRegistry 注册）
     *
     * velocity 参数作为目标偏移（targetPos = pos + velocity）。
     * 使用默认白色和默认持续时间。
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 带目标位置的工厂方法
     *
     * @param pos 起始位置
     * @param targetPosition 目标位置
     * @param color ARGB 颜色值
     * @param durationInTicks 飞行持续 tick 数
     */
    static std::unique_ptr<Particle> createWithTarget(
        const glm::vec3& pos, const Vector3d& targetPosition, u32 color, i32 durationInTicks);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/trail");
    }

    /**
     * @brief 获取固定高亮度
     *
     * 轨迹粒子是发光粒子，使用最大亮度。
     */
    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        return FULL_BRIGHTNESS;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    /// 最大亮度光照值（skyLight=15, blockLight=15 的组合值）
    static constexpr u32 FULL_BRIGHTNESS = 15728880;

    Vector3d m_targetPosition; ///< 目标位置（世界坐标）
    i32 m_durationInTicks;     ///< 飞行持续 tick 数（生命周期）
};

} // namespace mc::client::renderer::trident::particle::particles
