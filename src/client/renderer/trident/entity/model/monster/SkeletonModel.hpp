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

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 骷髅模型
 *
 * 参考 MC 1.16.5 SkeletonModel
 * 骷髅是双足生物，手臂向前伸，手臂和腿更细。
 */
class SkeletonModel : public model::BipedModel {
public:
    SkeletonModel();
    ~SkeletonModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 手臂姿态
     */
    enum class ArmPose {
        Empty,          // 空手
        BowAndArrow,    // 拉弓
        ThrowSpear,     // 投掷三叉戟
        CrossbowCharge, // 装填弩
        CrossbowHold    // 持有弩
    };

    /**
     * @brief 设置右手手臂姿态
     */
    void setRightArmPose(ArmPose pose) { m_rightArmPose = pose; }

    /**
     * @brief 设置左手手臂姿态
     */
    void setLeftArmPose(ArmPose pose) { m_leftArmPose = pose; }

    /**
     * @brief 设置是否处于攻击状态
     */
    void setAggressive(bool aggressive) { m_isAggressive = aggressive; }

    /**
     * @brief 是否处于攻击状态
     */
    [[nodiscard]] bool isAggressive() const { return m_isAggressive; }

protected:
    void setupParts() override;

    ArmPose m_rightArmPose = ArmPose::Empty;
    ArmPose m_leftArmPose = ArmPose::Empty;
    bool m_isAggressive = false; // 是否处于攻击状态（影响手臂动画）
};

} // namespace mc::client::renderer::entity::model::monster
