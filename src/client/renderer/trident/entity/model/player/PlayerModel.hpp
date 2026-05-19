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
#include "common/entity/entities/player/PlayerModelPart.hpp"
#include <array>

namespace mc::client::renderer::entity::model::player {

/**
 * @brief 手部侧边
 * 参考 MC 1.16.5 HandSide
 */
enum class HandSide {
    Right = 0, // 右手
    Left = 1   // 左手
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

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

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

    // ========== 部件可见性控制 ==========

    /**
     * @brief 设置指定皮肤部件的可见性
     *
     * 参考 MC 1.16.5 PlayerRenderer.setModelVisibilities
     * 根据 PlayerModelPart 设置对应模型部件的可见性：
     * - Cape: 斗篷 (m_cape)
     * - Jacket: 外套外层 (m_bodywear)
     * - LeftSleeve: 左袖外层 (m_leftArmwear)
     * - RightSleeve: 右袖外层 (m_rightArmwear)
     * - LeftPantsLeg: 左裤腿外层 (m_leftLegwear)
     * - RightPantsLeg: 右裤腿外层 (m_rightLegwear)
     * - Hat: 帽子/头部外层 (m_headwear)
     *
     * @param part 要设置的皮肤部件
     * @param visible 是否可见
     */
    void setPartVisible(PlayerModelPart part, bool visible);

    /**
     * @brief 获取指定皮肤部件是否可见
     *
     * @param part 要查询的皮肤部件
     * @return 是否可见
     */
    [[nodiscard]] bool isPartVisible(PlayerModelPart part) const;

    /**
     * @brief 根据玩家设置批量设置所有部件可见性
     *
     * 参考 MC 1.16.5 PlayerRenderer.setModelVisibilities
     * 根据 playerModelParts 位掩码设置所有外层皮肤部件的可见性。
     * 注意：此方法不会改变基础部件（内层皮肤）的可见性。
     *
     * @param playerModelParts 玩家皮肤部件位掩码
     */
    void setModelVisibilitiesFromFlags(u8 playerModelParts);

    /**
     * @brief 复制主部件角度到外观层
     */
    void copyAnglesToWear();

    // ========== 手臂渲染（用于第三人称视角） ==========

    /**
     * @brief 渲染右手臂（仅手臂和袖子）
     *
     * 仅渲染右手臂和右袖外层，用于第三人称视角手臂渲染。
     * 参考 MC 1.16.5 PlayerRenderer.renderRightArm
     *
     * 此方法会：
     * 1. 隐藏所有其他部件
     * 2. 仅显示右臂和右袖
     * 3. 渲染右臂和右袖
     * 4. 恢复原始可见性状态
     *
     * @param scale 缩放因子，默认 1/16
     */
    void renderRightArm(f64 scale = 1.0f / 16.0f);

    /**
     * @brief 渲染左手臂（仅手臂和袖子）
     *
     * 仅渲染左手臂和左袖外层，用于第三人称视角手臂渲染。
     * 参考 MC 1.16.5 PlayerRenderer.renderLeftArm
     *
     * 此方法会：
     * 1. 隐藏所有其他部件
     * 2. 仅显示左臂和左袖
     * 3. 渲染左臂和左袖
     * 4. 恢复原始可见性状态
     *
     * @param scale 缩放因子，默认 1/16
     */
    void renderLeftArm(f64 scale = 1.0f / 16.0f);

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
    std::shared_ptr<model::ModelRenderer> m_leftArmwear;  // 左袖外层
    std::shared_ptr<model::ModelRenderer> m_rightArmwear; // 右袖外层
    std::shared_ptr<model::ModelRenderer> m_leftLegwear;  // 左裤腿外层
    std::shared_ptr<model::ModelRenderer> m_rightLegwear; // 右裤腿外层
    std::shared_ptr<model::ModelRenderer> m_bodywear;     // 外套外层

    // 斗篷和耳朵
    std::shared_ptr<model::ModelRenderer> m_cape; // 斗篷
    std::shared_ptr<model::ModelRenderer> m_ears; // 耳朵（Deadmau5）

    // 手臂姿态
    ArmPose m_leftArmPose = ArmPose::Empty;
    ArmPose m_rightArmPose = ArmPose::Empty;

    // 状态
    bool m_slimArms = false;  // 纤细手臂
    bool m_crouching = false; // 蹲伏
    bool m_swimming = false;  // 游泳
    bool m_sprinting = false; // 疾跑
};

} // namespace mc::client::renderer::entity::model::player
