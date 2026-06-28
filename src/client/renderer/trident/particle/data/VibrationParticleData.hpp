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

#include "ParticleData.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 振动粒子数据
 *
 * 携带振动粒子的目标位置和到达时间。
 * 用于幽匿感测体、幽匿尖啸体、监守者等振动监听器的振动粒子效果。
 * 粒子从源位置向目标位置飞行，飞行时间为 arrivalInTicks 个 tick。
 *
 */
class VibrationParticleData : public ParticleData {
public:
    /**
     * @brief 构造振动粒子数据
     *
     * @param targetPosition 粒子飞向的目标位置（世界坐标）
     * @param arrivalInTicks 粒子到达目标的剩余 tick 数
     */
    VibrationParticleData(const Vector3d& targetPosition, i32 arrivalInTicks);

    ~VibrationParticleData() override = default;

    // 允许拷贝
    VibrationParticleData(const VibrationParticleData&) = default;
    VibrationParticleData& operator=(const VibrationParticleData&) = default;

    // 允许移动
    VibrationParticleData(VibrationParticleData&&) noexcept = default;
    VibrationParticleData& operator=(VibrationParticleData&&) noexcept = default;

    // ========================================================================
    // ParticleData 接口实现
    // ========================================================================

    [[nodiscard]] ParticleTypeId getType() const override { return ParticleTypeId::Vibration; }
    [[nodiscard]] std::string getTypeName() const override;
    [[nodiscard]] std::string getParameters() const override;
    [[nodiscard]] std::unique_ptr<ParticleData> clone() const override;

    // ========================================================================
    // 振动粒子特有接口
    // ========================================================================

    /**
     * @brief 获取目标位置
     *
     * 粒子飞向的目标位置（世界坐标）。网络序列化时已将 PositionSource 解析为具体坐标，因此客户端不需要再解析。
     *
     * @return 目标位置
     */
    [[nodiscard]] const Vector3d& targetPosition() const noexcept { return m_targetPosition; }

    /**
     * @brief 获取到达目标的剩余 tick 数
     *
     * 粒子的生命周期为 arrivalInTicks 个 tick，每 tick 向目标插值移动。
     *
     * @return 剩余 tick 数
     */
    [[nodiscard]] i32 arrivalInTicks() const noexcept { return m_arrivalInTicks; }

private:
    Vector3d m_targetPosition; ///< 目标位置（世界坐标）
    i32 m_arrivalInTicks;      ///< 到达目标的剩余 tick 数
};

} // namespace mc::client::renderer::trident::particle::data
