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
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/PlayerModelPart.hpp"
#include <array>
#include <memory>

namespace mc::client::renderer::entity::model::player {

/**
 * @brief 手的边（左/右）
 *
 * 与基类 BipedModel::HandSide 等价，此处保留以便旧代码兼容
 */
using HandSide = mc::client::renderer::entity::model::HandSide;

/**
 * @brief 手臂姿态枚举别名
 *
 * 直接复用基类 BipedModel 的 ArmPose 枚举，避免命名空间分裂和字段隐藏问题。
 * 9 种姿态：Empty, Item, Block, BowAndArrow, ThrowSpear, CrossbowCharge, CrossbowHold,
 * Spyglass, Brush
 */
using ArmPose = mc::client::renderer::entity::model::ArmPose;

/**
 * @brief 玩家模型
 *
 * 支持标准手臂和纤细手臂两种模式。
 *
 * 手臂姿态直接由基类 BipedModel 的 m_leftArmPose/m_rightArmPose 字段驱动，
 * 子类不再重新定义同名字段，避免 setArmPose 与 handleRightArmPose/handleLeftArmPose
 * 之间出现字段隐藏问题。
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
     * @brief 设置双手姿态
     *
     * 直接转发到基类 BipedModel 的 m_leftArmPose/m_rightArmPose 字段，
     * 由 BipedModel::setAngles → handleRightArmPose/handleLeftArmPose 消费。
     */
    void setArmPose(ArmPose leftArmPose, ArmPose rightArmPose)
    {
        m_leftArmPose = leftArmPose;
        m_rightArmPose = rightArmPose;
    }

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
     * @brief 设置疾跑状态
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
     */
    void setVisible(bool visible);

    // ========== 部件可见性控制 ==========

    /**
     * @brief 设置指定皮肤部件的可见性
     *
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
     * @brief 平移手部用于手持物品渲染
     *
     * 重写基类 BipedModel::translateHand 以支持纤细手臂偏移。
     *
     * 纤细手臂模式下，手臂宽度由 4 缩减为 3，手臂中心需要向身体中线方向
     * 偏移 0.5 个模型单位（右手 X+0.5、左手 X-0.5），以保持手持物品
     * 视觉上仍位于手臂中心。
     *
     * 实现采用无副作用模式：临时修改手臂 rotationPointX 获取变换矩阵后
     * 立即恢复原值，参考 MC 1.21.11 PlayerModel.translateToHand。
     *
     * @param handSide 手侧（左手或右手）
     * @param outMatrix 输出变换矩阵（4x4，行主序）
     */
    void translateHand(HandSide handSide, std::array<f64, 16>& outMatrix) const override;

private:
    void _setupSlimArms();
    void _setupStandardArms();
    void _setupWearParts();
    void _setupCape();
    void _setupEars();
    void _animateArms(f64 limbSwing, f64 limbSwingAmount);

    // 外观层部件
    std::shared_ptr<model::ModelRenderer> m_leftArmwear;  // 左袖外层
    std::shared_ptr<model::ModelRenderer> m_rightArmwear; // 右袖外层
    std::shared_ptr<model::ModelRenderer> m_leftLegwear;  // 左裤腿外层
    std::shared_ptr<model::ModelRenderer> m_rightLegwear; // 右裤腿外层
    std::shared_ptr<model::ModelRenderer> m_bodywear;     // 外套外层

    // 斗篷和耳朵
    std::shared_ptr<model::ModelRenderer> m_cape; // 斗篷
    std::shared_ptr<model::ModelRenderer> m_ears; // 耳朵（Deadmau5）

    // 状态
    bool m_slimArms = false;  // 纤细手臂
    bool m_crouching = false; // 蹲伏
    bool m_swimming = false;  // 游泳
    bool m_sprinting = false; // 疾跑
};

} // namespace mc::client::renderer::entity::model::player
