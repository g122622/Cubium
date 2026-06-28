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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
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
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 宝库连接粒子
 *
 * 从宝库位置飞向目标位置的定向粒子效果，类似于 VibrationSignalParticle。
 * 用于宝库解锁时的连接光束视觉反馈。
 * 使用指数缓动向目标飞行，并带有轻微旋转和淡出效果。
 */
class VaultConnectionParticle : public Particle {
public:
    /**
     * @brief 构造宝库连接粒子
     *
     * @param pos 起始位置
     * @param targetPosition 目标位置（飞向的终点）
     * @param arrivalInTicks 到达目标位置的 tick 数（即粒子生命周期）
     */
    VaultConnectionParticle(const glm::vec3& pos, const Vector3d& targetPosition, i32 arrivalInTicks);

    /**
     * @brief 标准工厂方法（用于 ParticleRegistry 注册）
     *
     * 默认创建一个向正上方 8 格飞行 60 tick 的粒子。
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 带目标位置的工厂方法
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
     * 宝库连接粒子是发光粒子，使用最大亮度。
     */
    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        return FULL_BRIGHTNESS;
    }

private:
    /// 最大亮度光照值（skyLight=15, blockLight=15 的组合值）
    static constexpr u32 FULL_BRIGHTNESS = 15728880;

    Vector3d m_targetPosition; ///< 目标位置（世界坐标）
    i32 m_arrivalInTicks;      ///< 到达目标的总 tick 数（生命周期）
};

} // namespace mc::client::renderer::trident::particle::particles
