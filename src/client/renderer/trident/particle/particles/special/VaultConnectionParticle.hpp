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
 * @brief 宝库连接粒子
 *
 * 从宝库位置飞向目标位置的定向粒子效果，类似于 VibrationSignalParticle。
 * FlyTowardsPositionParticle.VaultConnectionProvider 的行为：
 * - SimpleParticleType，速度参数即为目标偏移（targetPos = spawnPos + velocity）
 * - 发光蓝白色 (0.9*f, 0.9*f, f)，f = random*0.6+0.4
 * - 生命周期 30~39 tick
 * - 无重力、无物理碰撞
 * - 指数缓动向目标位置飞行
 * - 尺寸：1.5 * 0.1 * (random*0.5+0.2)
 * - 透明度：先淡入 0→0.6，后淡出 0.6→0
 *
 * 数据管线：MC Java 中 vault_connection 是 SimpleParticleType，速度参数即为目标偏移
 * （targetPos = spawnPos + velocity），因此标准 create() 工厂方法已满足网络层需求
 * （velocity 字段传递目标偏移）。编程方式创建（如通过 addParticleWithData）可使用
 * VibrationParticleData 传递目标位置和到达时间，数据工厂已在 ParticleFactories.cpp 中注册。
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
     * MC Java 中 vault_connection 是 SimpleParticleType，
     * velocity 参数表示目标偏移：targetPos = pos + velocity
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
        return ResourceLocation("minecraft:particle/vault_connection");
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

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    /// 最大亮度光照值（skyLight=15, blockLight=15 的组合值）
    static constexpr u32 FULL_BRIGHTNESS = 15728880;

    Vector3d m_targetPosition; ///< 目标位置（世界坐标）
    i32 m_arrivalInTicks;      ///< 到达目标的总 tick 数（生命周期）
};

} // namespace mc::client::renderer::trident::particle::particles
