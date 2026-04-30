#pragma once

#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "common/util/math/Vector3.hpp"
#include <functional>

namespace mc::client::renderer::entity::model {

// 前向声明
namespace entity {
    class LivingEntity;
}

/**
 * @brief 手的边（左/右）
 *
 * 参考 MC 1.16.5 HandSide
 */
enum class HandSide {
    Left,
    Right
};

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
 * 参考 MC 1.16.5 BipedModel - 继承自 AgeableModel
 */
class BipedModel : public AgeableModel {
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
     * @brief 设置生物动画状态（每帧调用）
     *
     * 参考 MC 1.16.5 BipedModel.setLivingAnimations
     * 用于设置游泳动画等状态
     */
    void setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick) override;

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

    /**
     * @brief 设置主手
     */
    void setMainHand(HandSide hand) { m_mainHand = hand; }

    /**
     * @brief 设置挥动的手
     */
    void setSwingingHand(HandSide hand) { m_swingingHand = hand; }

    /**
     * @brief 设置鞘翅飞行时间
     */
    void setElytraFlyingTicks(i32 ticks) { m_elytraFlyingTicks = ticks; }

    /**
     * @brief 设置是否真正游泳
     */
    void setActuallySwimming(bool swimming) { m_isActuallySwimming = swimming; }

    /**
     * @brief 设置所有部件可见性
     */
    void setVisible(bool visible);

    /**
     * @brief 复制模型属性到另一个模型
     */
    void copyModelAttributesTo(BipedModel& target) const;

    /**
     * @brief 获取指定边的手臂
     */
    std::shared_ptr<ModelRenderer> getArmForSide(HandSide side);

    /**
     * @brief 获取头部模型
     */
    std::shared_ptr<ModelRenderer> getModelHead() { return m_bipedHead; }

protected:
    /**
     * @brief 设置模型部件
     */
    virtual void setupParts();

    /**
     * @brief 获取头部部件（AgeableModel 接口）
     */
    std::vector<std::shared_ptr<ModelRenderer>> getHeadParts() const override;

    /**
     * @brief 获取身体部件（AgeableModel 接口）
     */
    std::vector<std::shared_ptr<ModelRenderer>> getBodyParts() const override;

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
     * @brief 处理游泳动画
     */
    virtual void handleSwimAnimation(f64 limbSwing);

    /**
     * @brief 角度插值（弧度）
     */
    static f32 rotLerpRad(f32 angle, f64 maxAngle, f64 target);

    /**
     * @brief 获取手臂角度平方
     */
    static f32 getArmAngleSq(f32 limbSwing);

    /**
     * @brief 获取主手
     */
    HandSide getMainHand() const;

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
    HandSide m_mainHand = HandSide::Right;
    HandSide m_swingingHand = HandSide::Right;
    i32 m_elytraFlyingTicks = 0;
    bool m_isActuallySwimming = false;
};

} // namespace mc::client::renderer::entity::model
