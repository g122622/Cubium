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

#include "ArmPose.hpp"
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc::client::renderer {

// 导入需要的类型
using entity::model::BipedModel;
using entity::model::ModelRenderer;

/**
 * @brief 玩家模型
 *
 * 扩展 BipedModel，添加玩家特有的模型部件和动画。
 *
 * 玩家模型包含：
 * - 头部（带第二层装饰：帽子）
 * - 身体（带第二层装饰：外套）
 * - 手臂（带第二层装饰：袖子）
 * - 腿部（带第二层装饰：裤腿）
 *
 * 手臂姿态支持：
 * - Empty: 空手，手臂自然下垂
 * - Item: 持有物品，手臂略微前伸
 * - Block: 格挡（盾牌）
 * - BowAndArrow: 拉弓
 * - ThrowSpear: 投掷三叉戟
 * - CrossbowCharge: 装填弩
 * - CrossbowHold: 持有已装填的弩
 * - EatOrDrink: 吃食物/喝药水
 */
class PlayerModel : public BipedModel {
public:
    /**
     * @brief 构造玩家模型
     * @param smallArms 是否使用细手臂（Alex 模型）
     */
    explicit PlayerModel(bool smallArms = false);
    ~PlayerModel() override = default;

    /**
     * @brief 渲染模型
     * @param scale 缩放因子
     */
    void render(f64 scale = 1.0f / 16.0f) override;

    /**
     * @brief 设置动画参数
     * @param limbSwing 步态动画周期（弧度）
     * @param limbSwingAmount 步态动画强度
     * @param ageInTicks 年龄tick（用于空闲动画）
     * @param netHeadYaw 头部偏航角（相对于身体，度）
     * @param headPitch 头部俯仰角（度）
     * @param scale 缩放因子
     */
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    // ========== 玩家特有部件访问器 ==========

    /**
     * @brief 获取右臂
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> rightArm() const { return m_rightArm; }

    /**
     * @brief 获取左臂
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> leftArm() const { return m_leftArm; }

    /**
     * @brief 获取右臂袖子（外层）
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> rightArmWear() const { return m_rightArmWear; }

    /**
     * @brief 获取左臂袖子（外层）
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> leftArmWear() const { return m_leftArmWear; }

    /**
     * @brief 获取右腿
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> rightLeg() const { return m_rightLeg; }

    /**
     * @brief 获取左腿
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> leftLeg() const { return m_leftLeg; }

    /**
     * @brief 获取右腿裤腿（外层）
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> rightLegWear() const { return m_rightLegWear; }

    /**
     * @brief 获取左腿裤腿（外层）
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> leftLegWear() const { return m_leftLegWear; }

    /**
     * @brief 获取身体外套（外层）
     */
    [[nodiscard]] std::shared_ptr<ModelRenderer> bodyWear() const { return m_bodyWear; }

    // ========== 手臂姿态设置 ==========

    /**
     * @brief 设置右手手臂姿态
     */
    void setRightArmPose(ArmPose pose) { m_rightArmPose = pose; }

    /**
     * @brief 设置左手手臂姿态
     */
    void setLeftArmPose(ArmPose pose) { m_leftArmPose = pose; }

    /**
     * @brief 获取右手手臂姿态
     */
    [[nodiscard]] ArmPose rightArmPose() const { return m_rightArmPose; }

    /**
     * @brief 获取左手手臂姿态
     */
    [[nodiscard]] ArmPose leftArmPose() const { return m_leftArmPose; }

    // ========== 状态设置 ==========

    /**
     * @brief 设置是否正在潜行
     */
    void setSneaking(bool sneaking) { m_sneaking = sneaking; }

    /**
     * @brief 设置是否正在游泳
     */
    void setSwimming(bool swimming) { m_swimming = swimming; }

    /**
     * @brief 设置是否正在使用鞘翅飞行
     */
    void setFallFlying(bool fallFlying) { m_fallFlying = fallFlying; }

    /**
     * @brief 设置所有部件的可见性
     */
    void setVisible(bool visible);

    // ========== 手臂渲染（用于第一人称） ==========

    /**
     * @brief 渲染右手臂
     *
     * 仅渲染右手臂和袖子，用于第一人称手部渲染。
     *
     * @param scale 缩放因子
     */
    void renderRightArm(f64 scale = 1.0f / 16.0f);

    /**
     * @brief 渲染左手臂
     *
     * 仅渲染左手臂和袖子，用于第一人称手部渲染。
     *
     * @param scale 缩放因子
     */
    void renderLeftArm(f64 scale = 1.0f / 16.0f);

    /**
     * @brief 设置是否使用细手臂（Alex 模型）
     */
    void setSmallArms(bool smallArms);

    /**
     * @brief 获取是否使用细手臂
     */
    [[nodiscard]] bool isSmallArms() const { return m_smallArms; }

protected:
    /**
     * @brief 设置模型部件
     */
    void setupParts() override;

    /**
     * @brief 根据手臂姿态设置手臂角度
     *
     * 这是 protected 方法，子类可以重写以自定义手臂动画。
     */
    void setupArmAngles(f64 limbSwing, f64 limbSwingAmount);

    /**
     * @brief 设置潜行姿态
     */
    void setupSneakingAngles();

    /**
     * @brief 设置游泳姿态
     */
    void setupSwimmingAngles();

    /**
     * @brief 设置鞘翅飞行姿态
     */
    void setupFallFlyingAngles();

private:
    // 纹理尺寸（玩家皮肤为 64x64）
    static constexpr i32 PLAYER_TEXTURE_WIDTH = 64;
    static constexpr i32 PLAYER_TEXTURE_HEIGHT = 64;

    // 外层部件（第二层皮肤）
    std::shared_ptr<ModelRenderer> m_headWear;
    std::shared_ptr<ModelRenderer> m_bodyWear;
    std::shared_ptr<ModelRenderer> m_rightArmWear;
    std::shared_ptr<ModelRenderer> m_leftArmWear;
    std::shared_ptr<ModelRenderer> m_rightLegWear;
    std::shared_ptr<ModelRenderer> m_leftLegWear;

    // 手臂姿态
    ArmPose m_rightArmPose = ArmPose::Empty;
    ArmPose m_leftArmPose = ArmPose::Empty;

    // 状态标志
    bool m_sneaking = false;
    bool m_swimming = false;
    bool m_fallFlying = false;
    bool m_smallArms = false;

    // 重写父类部件以使用正确类型
    using BipedModel::m_body;
    using BipedModel::m_head;
    using BipedModel::m_leftArm;
    using BipedModel::m_leftLeg;
    using BipedModel::m_parts;
    using BipedModel::m_rightArm;
    using BipedModel::m_rightLeg;
};

} // namespace mc::client::renderer
