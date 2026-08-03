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
#include <memory>
#include <string>

namespace mc::client::renderer::trident::particle::data {

/**
 * @brief 振动粒子数据
 *
 * 携带振动粒子的目标位置来源和到达时间。
 * 用于幽匿感测体、幽匿尖啸体、监守者等振动监听器的振动粒子效果。
 * 粒子从源位置向目标位置飞行，飞行时间为 arrivalInTicks 个 tick。
 *
 * 目标来源有两种：
 * - **方块来源**：目标位置已解析为方块中心坐标，粒子飞行期间不再更新目标。
 *   对应 MC Java BlockPositionSource。
 * - **实体来源**：粒子持有实体 ID 和 Y 轴偏移，每 tick 通过 ClientWorld 重新解析
 *   实体当前位置，因此当目标实体移动时粒子会持续跟随。实体消失时粒子立即过期。
 *   对应 MC Java EntityPositionSource + VibrationSignalParticle.tick() 的行为。
 *
 */
class VibrationParticleData : public ParticleData {
public:
    /// 目标来源类型
    enum class TargetKind : u8 {
        Block = 0,  ///< 方块来源：m_targetPosition 已解析为方块中心坐标
        Entity = 1, ///< 实体来源：每 tick 通过 m_targetEntityId + m_yOffset 动态解析
    };

    /**
     * @brief 构造方块来源的振动粒子数据
     *
     * @param targetPosition 粒子飞向的目标位置（已解析为方块中心的世界坐标）
     * @param arrivalInTicks 粒子到达目标的剩余 tick 数
     */
    VibrationParticleData(const Vector3d& targetPosition, i32 arrivalInTicks);

    /**
     * @brief 构造实体来源的振动粒子数据
     *
     * 粒子持有实体 ID 和 Y 轴偏移，每 tick 由 VibrationSignalParticle 通过
     * ClientWorld.entityManager().getEntity(id) 重新解析实体当前位置并叠加 yOffset。
     *
     * @param targetEntityId 目标实体 ID
     * @param yOffset Y 轴偏移（如眼睛高度）
     * @param arrivalInTicks 粒子到达目标的剩余 tick 数
     */
    VibrationParticleData(EntityInstanceId targetEntityId, f32 yOffset, i32 arrivalInTicks);

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
     * @brief 获取目标来源类型
     */
    [[nodiscard]] TargetKind kind() const noexcept { return m_kind; }

    /**
     * @brief 是否为方块来源
     */
    [[nodiscard]] bool isBlockSource() const noexcept { return m_kind == TargetKind::Block; }

    /**
     * @brief 是否为实体来源
     */
    [[nodiscard]] bool isEntitySource() const noexcept { return m_kind == TargetKind::Entity; }

    /**
     * @brief 获取目标位置（方块来源）
     *
     * 仅当 isBlockSource() 为 true 时有效。
     * 粒子飞向的目标位置（已解析为方块中心的世界坐标）。
     *
     * @return 目标位置
     */
    [[nodiscard]] const Vector3d& targetPosition() const noexcept { return m_targetPosition; }

    /**
     * @brief 获取目标实体 ID（实体来源）
     *
     * 仅当 isEntitySource() 为 true 时有效。
     *
     * @return 目标实体 ID
     */
    [[nodiscard]] EntityInstanceId targetEntityId() const noexcept { return m_targetEntityId; }

    /**
     * @brief 获取 Y 轴偏移（实体来源）
     *
     * 仅当 isEntitySource() 为 true 时有效。
     *
     * @return Y 轴偏移
     */
    [[nodiscard]] f32 yOffset() const noexcept { return m_yOffset; }

    /**
     * @brief 获取到达目标的剩余 tick 数
     *
     * 粒子的生命周期为 arrivalInTicks 个 tick，每 tick 向目标插值移动。
     *
     * @return 剩余 tick 数
     */
    [[nodiscard]] i32 arrivalInTicks() const noexcept { return m_arrivalInTicks; }

private:
    TargetKind m_kind = TargetKind::Block;
    Vector3d m_targetPosition;                             ///< 方块来源的目标位置（世界坐标）
    EntityInstanceId m_targetEntityId = INVALID_ENTITY_ID; ///< 实体来源的目标实体 ID
    f32 m_yOffset = 0.0f;                                  ///< 实体来源的 Y 轴偏移
    i32 m_arrivalInTicks = 0;                              ///< 到达目标的剩余 tick 数
};

} // namespace mc::client::renderer::trident::particle::data
