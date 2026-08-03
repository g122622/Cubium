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

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "common/core/Types.hpp"

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 手臂姿态枚举别名
 *
 * 直接复用基类 BipedModel 的 ArmPose 枚举，避免命名空间分裂和字段隐藏问题。
 * 对应 MC 1.21.11 HumanoidModel.ArmPose，9 种姿态：
 * Empty, Item, Block, BowAndArrow, ThrowSpear, CrossbowCharge, CrossbowHold,
 * Spyglass, Brush
 *
 * @see model::ArmPose
 */
using ArmPose = mc::client::renderer::entity::model::ArmPose;

/**
 * @brief 骷髅模型
 *
 * 骷髅是双足生物，手臂向前伸，手臂和腿更细。
 *
 * 手臂姿态直接由基类 BipedModel 的 m_leftArmPose/m_rightArmPose 字段驱动，
 * 子类不再重新定义同名字段，避免 setRightArmPose/setLeftArmPose 与
 * handleRightArmPose/handleLeftArmPose 之间出现字段隐藏问题。弩姿态
 * (CrossbowCharge/CrossbowHold) 由基类 BipedModel::setAngles →
 * handleRightArmPose/handleLeftArmPose → handleCrossbowCharge/handleCrossbowHold
 * 完整处理，子类无需重复实现。骷髅特有的空手攻击动画（isAggressive）在
 * SkeletonModel::setAngles 中于基类 setAngles 之后覆盖写入。
 *
 * @see BipedModel::handleCrossbowCharge
 * @see BipedModel::handleCrossbowHold
 */
class SkeletonModel : public model::BipedModel {
public:
    SkeletonModel();
    ~SkeletonModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置右手手臂姿态
     *
     * 直接转发到基类 BipedModel 的 m_rightArmPose 字段，
     * 由 BipedModel::setAngles → handleRightArmPose 消费。
     */
    void setRightArmPose(ArmPose pose) { m_rightArmPose = pose; }

    /**
     * @brief 设置左手手臂姿态
     *
     * 直接转发到基类 BipedModel 的 m_leftArmPose 字段，
     * 由 BipedModel::setAngles → handleLeftArmPose 消费。
     */
    void setLeftArmPose(ArmPose pose) { m_leftArmPose = pose; }

    /**
     * @brief 设置是否处于攻击状态
     *
     * 对应 MC 1.21.11 SkeletonRenderState.isAggressive。
     * 当 isAggressive=true 且未持弓时，SkeletonModel::setAngles 会覆盖
     * 基类设置的手臂角度，呈现空手挥击动画。
     */
    void setAggressive(bool aggressive) { m_isAggressive = aggressive; }

    /**
     * @brief 是否处于攻击状态
     */
    [[nodiscard]] bool isAggressive() const { return m_isAggressive; }

protected:
    void setupParts() override;

    bool m_isAggressive = false; // 是否处于攻击状态（影响手臂动画）
};

} // namespace mc::client::renderer::entity::model::monster
