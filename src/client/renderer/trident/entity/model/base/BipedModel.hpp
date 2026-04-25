#pragma once

#include "client/renderer/trident/entity/model/core/EntityModel.hpp"

namespace mc::client::renderer::entity::model {

/**
 * @brief 手臂姿态枚举
 *
 * 参考 MC 1.16.5 BipedModel.ArmPose
 */
enum class ArmPose {
    Empty,
    Item,
    Block,
    BowAndArrow,
    ThrowSpear,
    CrossbowCharge,
    CrossbowHold
};

/**
 * @brief 双足动物模型基类
 *
 * 用于玩家、僵尸、骷髅等双足生物的模型基类。
 * 参考 MC 1.16.5 BipedModel
 */
class BipedModel : public EntityModel {
public:
    BipedModel();
    /**
     * @brief 带参数的构造函数
     * @param scale 模型膨胀值
     * @param yOffset Y轴偏移
     * @param textureWidth 纹理宽度
     * @param textureHeight 纹理高度
     */
    BipedModel(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight);
    ~BipedModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    /**
     * @brief 设置是否蹲伏
     */
    void setSneaking(bool sneaking) { m_isSneaking = sneaking; }

    /**
     * @brief 设置是否坐着
     */
    void setSitting(bool sitting) { m_isSitting = sitting; }

    /**
     * @brief 设置游泳动画进度
     */
    void setSwimAnimation(f32 animation) { m_swimAnimation = animation; }

    /**
     * @brief 设置挥动进度
     */
    void setSwingProgress(f32 progress) { m_swingProgress = progress; }

    /**
     * @brief 设置左手姿态
     */
    void setLeftArmPose(ArmPose pose) { m_leftArmPose = pose; }

    /**
     * @brief 设置右手姿态
     */
    void setRightArmPose(ArmPose pose) { m_rightArmPose = pose; }

protected:
    /**
     * @brief 设置模型部件
     */
    virtual void setupParts();

    /**
     * @brief 处理右手姿态
     */
    virtual void handleRightArmPose();

    /**
     * @brief 处理左手姿态
     */
    virtual void handleLeftArmPose();

    /**
     * @brief 处理挥动动画
     * @param ageInTicks 年龄 ticks
     */
    virtual void handleSwingAnimation(f64 ageInTicks);

    /**
     * @brief 角度插值（弧度）
     */
    static f32 rotLerpRad(f32 angle, f32 maxAngle, f32 target);

    // 模型部件
    std::shared_ptr<ModelRenderer> m_bipedHead;
    std::shared_ptr<ModelRenderer> m_bipedHeadwear;
    std::shared_ptr<ModelRenderer> m_bipedBody;
    std::shared_ptr<ModelRenderer> m_bipedRightArm;
    std::shared_ptr<ModelRenderer> m_bipedLeftArm;
    std::shared_ptr<ModelRenderer> m_bipedRightLeg;
    std::shared_ptr<ModelRenderer> m_bipedLeftLeg;

    // 兼容性别名
    std::shared_ptr<ModelRenderer>& m_head = m_bipedHead;
    std::shared_ptr<ModelRenderer>& m_headwear = m_bipedHeadwear;
    std::shared_ptr<ModelRenderer>& m_body = m_bipedBody;
    std::shared_ptr<ModelRenderer>& m_rightArm = m_bipedRightArm;
    std::shared_ptr<ModelRenderer>& m_leftArm = m_bipedLeftArm;
    std::shared_ptr<ModelRenderer>& m_rightLeg = m_bipedRightLeg;
    std::shared_ptr<ModelRenderer>& m_leftLeg = m_bipedLeftLeg;

    // 模型参数
    f32 m_modelScale = 0.0f;
    f32 m_yOffset = 0.0f;

    // 状态
    bool m_isSneaking = false;
    bool m_isSitting = false;
    f32 m_swimAnimation = 0.0f;
    f32 m_swingProgress = 0.0f;
    ArmPose m_leftArmPose = ArmPose::Empty;
    ArmPose m_rightArmPose = ArmPose::Empty;
};

} // namespace mc::client::renderer::entity::model
