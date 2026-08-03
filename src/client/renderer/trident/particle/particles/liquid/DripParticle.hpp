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
 * @brief 液体滴落粒子基类
 *
 * 用于实现水滴、熔岩滴、蜂蜜滴等液体滴落效果。
 *
 * 状态机：
 * 1. Hanging（悬挂）：在方块下方缓慢积累变大
 * 2. Falling（下落）：积累满后下落，受重力影响
 * 3. Landed（落地）：触地后短暂存在
 */
class DripParticle : public Particle {
public:
    /**
     * @brief 滴落状态
     */
    enum class DripState {
        Hanging, ///< 悬挂积累中
        Falling, ///< 下落中
        Landed   ///< 已落地
    };

    /**
     * @brief 滴落类型
     */
    enum class DripType {
        Water,       ///< 水
        Lava,        ///< 熔岩
        Honey,       ///< 蜂蜜
        ObsidianTear ///< 哭泣黑曜石眼泪
    };

    DripParticle(const glm::vec3& pos, const glm::vec3& velocity, DripType type);

    /**
     * @brief 工厂方法：创建熔岩滴落粒子（悬挂状态）
     */
    static std::unique_ptr<Particle> createDrippingLava(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 工厂方法：创建熔岩下落粒子
     */
    static std::unique_ptr<Particle> createFallingLava(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 工厂方法：创建熔岩落地粒子
     */
    static std::unique_ptr<Particle> createLandingLava(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 工厂方法：创建蜂蜜滴落粒子（悬挂状态）
     */
    static std::unique_ptr<Particle> createDrippingHoney(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 工厂方法：创建蜂蜜下落粒子
     */
    static std::unique_ptr<Particle> createFallingHoney(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 工厂方法：创建蜂蜜落地粒子
     */
    static std::unique_ptr<Particle> createLandingHoney(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 工厂方法：创建黑曜石眼泪滴落粒子（悬挂状态）
     */
    static std::unique_ptr<Particle> createDrippingObsidianTear(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 工厂方法：创建黑曜石眼泪下落粒子
     */
    static std::unique_ptr<Particle> createFallingObsidianTear(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 工厂方法：创建黑曜石眼泪落地粒子
     */
    static std::unique_ptr<Particle> createLandingObsidianTear(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        // 熔岩滴是发光粒子
        return m_type == DripType::Lava ? ParticleRenderType::PARTICLE_SHEET_LIT
                                        : ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override;

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override;

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

    [[nodiscard]] DripState dripState() const { return m_dripState; }
    [[nodiscard]] f64 dripProgress() const { return m_dripProgress; }

protected:
    /**
     * @brief 悬挂更新逻辑
     *
     * 缓慢积累，积累满后转 Falling
     */
    virtual void tickHanging(mc::client::ClientWorld* world);

    /**
     * @brief 下落更新逻辑
     *
     * 应用重力，检测与方块碰撞
     */
    virtual void tickFalling(mc::client::ClientWorld* world);

    /**
     * @brief 落地处理
     *
     * 根据类型生成不同效果
     */
    virtual void onLand(mc::client::ClientWorld* world);

    DripState m_dripState = DripState::Hanging;
    DripType m_type;
    f64 m_dripProgress = 0.0; ///< 悬挂积累进度 (0-1)
    glm::vec3 m_hangPosition; ///< 悬挂位置
};

} // namespace mc::client::renderer::trident::particle::particles
