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

#include "ParticleData.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include <cstdint>
#include <memory>
#include <string>

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 轨迹粒子数据
 *
 * 携带轨迹粒子的目标位置、颜色和持续时间。
 * 对应 MC Java 的 TrailParticleOption（target, color, duration）。
 * 用于洞穴探途者试炼Vault等场景的轨迹粒子效果。
 *
 * 粒子从源位置向目标位置飞行，带有自定义 ARGB 颜色，持续指定 tick 数。
 */
class TrailParticleData : public ParticleData {
public:
    /**
     * @brief 构造轨迹粒子数据
     *
     * @param targetPosition 粒子飞向的目标位置（世界坐标）
     * @param color 粒子颜色（ARGB 格式，如 0xFFFFFFFF 为白色）
     * @param durationInTicks 粒子飞行持续时间（tick 数）
     */
    TrailParticleData(const Vector3d& targetPosition, u32 color, i32 durationInTicks);

    ~TrailParticleData() override = default;

    // 允许拷贝
    TrailParticleData(const TrailParticleData&) = default;
    TrailParticleData& operator=(const TrailParticleData&) = default;

    // 允许移动
    TrailParticleData(TrailParticleData&&) noexcept = default;
    TrailParticleData& operator=(TrailParticleData&&) noexcept = default;

    // ========================================================================
    // ParticleData 接口实现
    // ========================================================================

    [[nodiscard]] ParticleTypeId getType() const override { return ParticleTypeId::Trail; }
    [[nodiscard]] std::string getTypeName() const override;
    [[nodiscard]] std::string getParameters() const override;
    [[nodiscard]] std::unique_ptr<ParticleData> clone() const override;

    // ========================================================================
    // 轨迹粒子特有接口
    // ========================================================================

    /**
     * @brief 获取目标位置
     *
     * 粒子飞向的目标位置（世界坐标）。
     *
     * @return 目标位置
     */
    [[nodiscard]] const Vector3d& targetPosition() const noexcept { return m_targetPosition; }

    /**
     * @brief 获取粒子颜色
     *
     * ARGB 格式颜色值，如 0xFFFFFFFF 为白色。
     *
     * @return ARGB 颜色
     */
    [[nodiscard]] u32 color() const noexcept { return m_color; }

    /**
     * @brief 获取飞行持续时间
     *
     * @return 持续时间（tick 数）
     */
    [[nodiscard]] i32 durationInTicks() const noexcept { return m_durationInTicks; }

private:
    Vector3d m_targetPosition; ///< 目标位置（世界坐标）
    u32 m_color;               ///< ARGB 颜色
    i32 m_durationInTicks;     ///< 飞行持续时间（tick 数）
};

} // namespace mc::client::renderer::trident::particle::data
