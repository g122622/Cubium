#pragma once

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include <array>

namespace mc::client::renderer::entity::model::player {

/**
 * @brief 手部侧边
 * 参考 MC 1.16.5 HandSide
 */
enum class HandSide {
    Right = 0,  // 右手
    Left = 1    // 左手
};

/**
 * @brief 手臂姿态
 *
 * 参考 MC 1.16.5 BipedModel.ArmPose
 */
enum class ArmPose {
    Empty,          // 空手
    Item,           // 持有物品
    Block,          // 格挡
    BowAndArrow,    // 拉弓
    ThrowSpear,     // 投掷三叉戟
    CrossbowCharge, // 装填弩
    CrossbowHold    // 持有弩
};

/**
 * @brief 玩家模型
 *
 * 参考 MC 1.16.5 PlayerModel
 * 支持标准手臂和纤细手臂两种模式。
 */
class PlayerModel : public model::BipedModel {
public:
    /**
     * @brief 构造函数
     * @param scale 模型缩放
     * @param slimArms 是否使用纤细手臂
     */
    explicit PlayerModel(f64 scale = 0.0f, bool slimArms = false);
    ~PlayerModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;

    void setAngles(f64 limbSwing, f64 limbSwingAmount,
                   f64 ageInTicks, f64 netHeadYaw,
                   f64 headPitch, f64 scale) override;

    // ========== 手臂姿态 ==========

    /**
     * @brief 设置手臂姿态
     */
    void setArmPose(ArmPose leftArmPose, ArmPose rightArmPose);

    /**
     * @brief 设置左手姿态
     */
    void setLeftArmPose(ArmPose pose) { m_leftArmPose = pose; }

    /**
     * @brief 设置右手姿态
     */
    void setRightArmPose(ArmPose pose) { m_rightArmPose = pose; }

    // ========== 特殊状态 ==========

    /**
     * @brief 设置蹲伏状态
     */
    void setCrouching(bool crouching) { m_crouching = crouching; }

    /**
     * @brief 设置游泳状态
     */
    void setSwimming(bool swimming) { m_swimming = swimming; }

    /**
     * @brief 设置爬行状态
     */
    void setSprinting(bool sprinting) { m_sprinting = sprinting; }

    /**
     * @brief 获取是否使用纤细手臂
     */
    [[nodiscard]] bool hasSlimArms() const { return m_slimArms; }

    // ========== 外观部件 ==========

    /**
     * @brief 渲染斗篷
     */
    void renderCape(f64 scale = 1.0f / 16.0f);

    /**
     * @brief 渲染耳朵（Deadmau5 皮肤）
     */
    void renderEars(f64 scale = 1.0f / 16.0f);

    /**
     * @brief 设置所有部件可见性
     *  参考 MC 1.16.5 PlayerModel.setVisible
     */
    void setVisible(bool visible);

    /**
     * @brief 复制主部件角度到外观层
     */
    void copyAnglesToWear();

    /**
     * @brief 平移手部用于第一人称渲染
     * 参考 MC 1.16.5 PlayerModel.translateHand
     * 纤细手臂模式下需要偏移手臂位置
     * @param side 手部侧边（0=右，1=左）
     * @param matrixStack 矩阵栈（用于变换）
     */
    void translateHand(i32 side);

private:
    void setupSlimArms();
    void setupStandardArms();
    void setupWearParts();
    void setupCape();
    void setupEars();
    void animateArms(f64 limbSwing, f64 limbSwingAmount);
    void animateBow(f64 limbSwing);
    void animateCrossbowCharge();
    void animateCrossbowHold();
    void updateCapePosition(bool wearingChestplate, bool crouching);

    // 外观层部件
    std::shared_ptr<model::ModelRenderer> m_leftArmwear;   // 左袖外层
    std::shared_ptr<model::ModelRenderer> m_rightArmwear;  // 右袖外层
    std::shared_ptr<model::ModelRenderer> m_leftLegwear;   // 左裤腿外层
    std::shared_ptr<model::ModelRenderer> m_rightLegwear;  // 右裤腿外层
    std::shared_ptr<model::ModelRenderer> m_bodywear;      // 外套外层

    // 斗篷和耳朵
    std::shared_ptr<model::ModelRenderer> m_cape;          // 斗篷
    std::shared_ptr<model::ModelRenderer> m_ears;          // 耳朵（Deadmau5）

    // 手臂姿态
    ArmPose m_leftArmPose = ArmPose::Empty;
    ArmPose m_rightArmPose = ArmPose::Empty;

    // 状态
    bool m_slimArms = false;    // 纤细手臂
    bool m_crouching = false;   // 蹲伏
    bool m_swimming = false;    // 游泳
    bool m_sprinting = false;   // 疾跑
};

} // namespace mc::client::renderer::entity::model::player
