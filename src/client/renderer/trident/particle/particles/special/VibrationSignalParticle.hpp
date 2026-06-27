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
#include "common/util/math/Vector3.hpp"
#include <memory>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 振动信号粒子
 *
 * 从振动源位置飞向目标位置的定向粒子效果。
 * 用于幽匿感测体、幽匿尖啸体、监守者等振动监听器接收到振动信号时的视觉反馈。
 *
 * 粒子以指数缓动向目标位置飞行，飞行过程中带有轻微的正弦摆动效果，
 * 始终面向摄像机，使用半透明渲染和全亮度光照。
 *
 * 参考: net.minecraft.client.particle.VibrationSignalParticle
 */
class VibrationSignalParticle : public Particle {
public:
    /**
     * @brief 构造振动信号粒子
     *
     * @param pos 起始位置
     * @param targetPosition 目标位置（飞向的终点）
     * @param arrivalInTicks 到达目标位置的 tick 数（即粒子生命周期）
     */
    VibrationSignalParticle(const glm::vec3& pos, const Vector3d& targetPosition, i32 arrivalInTicks);

    /**
     * @brief 工厂函数（标准粒子工厂，用于 ParticleRegistry 注册）
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 带目标位置的工厂函数
     *
     * 用于从 VibrationParticleData 创建振动信号粒子。
     * velocity 参数在此工厂中不使用，粒子的运动完全由目标位置驱动。
     *
     * @param pos 起始位置
     * @param targetPosition 目标位置
     * @param arrivalInTicks 到达目标的 tick 数
     */
    static std::unique_ptr<Particle> createWithTarget(
        const glm::vec3& pos, const Vector3d& targetPosition, i32 arrivalInTicks);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::PARTICLE_SHEET_LIT; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/vibration");
    }

    /**
     * @brief 获取固定高亮度
     *
     * 振动粒子是发光粒子，使用最大亮度。
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
    i32 m_arrivalInTicks;      ///< 到达目标的总 tick 数（生命周期）
};

} // namespace mc::client::renderer::trident::particle::particles
