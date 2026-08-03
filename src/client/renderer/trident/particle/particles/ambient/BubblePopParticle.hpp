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
 * @brief 气泡破裂粒子
 *
 * 当气泡粒子（BubbleParticle）离开水面时生成，模拟气泡破裂的效果。
 * 对应 Minecraft 原版的 BubblePopParticle。
 *
 * 特性：
 * - 生命周期 4 tick（极短）
 * - 微弱重力 0.008
 * - 有碰撞检测
 * - 5 帧动画纹理（bubble_pop_0 ~ bubble_pop_4）
 * - 渲染类型：OPAQUE
 * - 白色不透明
 *
 * 与 BubbleParticle 的关系：
 * - BubbleParticle 在 tick 中检测到离开水面时，通过 emitCallback
 *   生成 BubblePop 粒子，然后自身过期。
 */
class BubblePopParticle : public Particle {
public:
    /**
     * @brief 构造气泡破裂粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度（直接使用，不做额外随机偏移）
     */
    BubblePopParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    /**
     * @brief 根据粒子年龄返回对应的动画帧纹理
     *
     * BubblePop 使用5帧动画纹理（bubble_pop_0 ~ bubble_pop_4），
     * 根据生命周期进度选择当前帧。
     */
    [[nodiscard]] ResourceLocation getTextureLocation() const override;

private:
    /// 动画帧数
    static constexpr i32 FRAME_COUNT = 5;

    /// 生命周期（ticks）
    static constexpr f64 DEFAULT_LIFETIME = 4.0;

    /// 重力加速度
    static constexpr f64 BUBBLE_POP_GRAVITY = 0.008;
};

} // namespace mc::client::renderer::trident::particle::particles
