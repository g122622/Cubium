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

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

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
    void setupParts();

    ArmPose m_rightArmPose = ArmPose::Empty;
    ArmPose m_leftArmPose = ArmPose::Empty;
    bool m_isAggressive = false;  // 是否处于攻击状态（影响手臂动画）
};

} // namespace mc::client::renderer::entity::model::monster
