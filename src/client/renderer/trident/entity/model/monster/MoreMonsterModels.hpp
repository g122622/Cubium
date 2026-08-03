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
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc::client::renderer::entity::model::monster {

/**
 * @brief 灾厄村民手臂姿态
 */
enum class IllagerArmPose {
    Crossed,        // 交叉手臂
    Attacking,      // 攻击
    Spellcasting,   // 施法
    BowAndArrow,    // 拉弓
    CrossbowCharge, // 装填弩
    CrossbowHold,   // 持有弩
    Celebrating     // 庆祝
};

/**
 * @brief 灾厄村民模型
 */
class IllagerModel : public EntityModel {
public:
    IllagerModel();
    explicit IllagerModel(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight);
    ~IllagerModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    [[nodiscard]] std::shared_ptr<ModelRenderer> getHead() const { return m_head; }
    [[nodiscard]] std::shared_ptr<ModelRenderer> getHat() const { return m_hat; }

    /**
     * @brief 设置手臂姿态
     */
    void setArmPose(IllagerArmPose pose) { m_armPose = pose; }

    /**
     * @brief 设置主手是否为空
     */
    void setMainHandEmpty(bool empty) { m_mainHandEmpty = empty; }

protected:
    void setupParts(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight);

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_hat;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_arms;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;
    std::shared_ptr<ModelRenderer> m_rightArm;
    std::shared_ptr<ModelRenderer> m_leftArm;

    // 实体状态
    IllagerArmPose m_armPose = IllagerArmPose::Crossed;
    bool m_mainHandEmpty = true;
};

/**
 * @brief 恼鬼模型
 */
class VexModel : public BipedModel {
public:
    VexModel();
    ~VexModel() override = default;

    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    /**
     * @brief 设置充电状态
     * @param charging 是否处于充电状态
     */
    void setCharging(bool charging) { m_charging = charging; }

    /**
     * @brief 设置主手持物品是否为空
     * @param empty 主手是否为空
     */
    void setMainHandEmpty(bool empty) { m_mainHandEmpty = empty; }

private:
    std::shared_ptr<ModelRenderer> m_leftWing;
    std::shared_ptr<ModelRenderer> m_rightWing;

    // 实体状态
    bool m_charging = false;
    bool m_mainHandEmpty = true;
};

/**
 * @brief 铁傀儡模型
 */
class IronGolemModel : public EntityModel {
public:
    IronGolemModel();
    ~IronGolemModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    [[nodiscard]] std::shared_ptr<ModelRenderer> getRightArm() const { return m_rightArm; }

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_rightArm;
    std::shared_ptr<ModelRenderer> m_leftArm;
    std::shared_ptr<ModelRenderer> m_leftLeg;
    std::shared_ptr<ModelRenderer> m_rightLeg;
};

/**
 * @brief 雪傀儡模型
 */
class SnowGolemModel : public EntityModel {
public:
    SnowGolemModel();
    ~SnowGolemModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    [[nodiscard]] std::shared_ptr<ModelRenderer> getHead() const { return m_head; }

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_bottomBody;
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_rightHand;
    std::shared_ptr<ModelRenderer> m_leftHand;
};

/**
 * @brief 铜傀儡模型
 *
 * 对应 MC 1.21.11: net.minecraft.client.model.animal.golem.CopperGolemModel
 * 铜傀儡由 body（含 head 与双臂）与 left/right_leg 组成，纹理尺寸 64×64。
 *
 * 注意：MC 原版使用 KeyframeAnimation 系统驱动行走、空闲、宝箱交互等动画，
 * 本项目当前未实现关键帧动画系统，仅实现基础骨骼与 setAngles 中的简易动画。
 * TODO: 接入关键帧动画系统后，补充 CopperGolemAnimation 中定义的动画。
 */
class CopperGolemModel : public EntityModel {
public:
    CopperGolemModel();
    ~CopperGolemModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

    [[nodiscard]] std::shared_ptr<ModelRenderer> getHead() const { return m_head; }
    [[nodiscard]] std::shared_ptr<ModelRenderer> getRightArm() const { return m_rightArm; }
    [[nodiscard]] std::shared_ptr<ModelRenderer> getLeftArm() const { return m_leftArm; }

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_rightArm;
    std::shared_ptr<ModelRenderer> m_leftArm;
    std::shared_ptr<ModelRenderer> m_rightLeg;
    std::shared_ptr<ModelRenderer> m_leftLeg;
};

/**
 * @brief 蜜蜂模型
 */
class BeeModel : public EntityModel {
public:
    BeeModel();
    ~BeeModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_torso;
    std::shared_ptr<ModelRenderer> m_rightWing;
    std::shared_ptr<ModelRenderer> m_leftWing;
    std::shared_ptr<ModelRenderer> m_frontLegs;
    std::shared_ptr<ModelRenderer> m_middleLegs;
    std::shared_ptr<ModelRenderer> m_backLegs;
    std::shared_ptr<ModelRenderer> m_stinger;
    std::shared_ptr<ModelRenderer> m_leftAntenna;
    std::shared_ptr<ModelRenderer> m_rightAntenna;
};

/**
 * @brief 狐狸模型
 */
class FoxModel : public EntityModel {
public:
    FoxModel();
    ~FoxModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_rightEar;
    std::shared_ptr<ModelRenderer> m_leftEar;
    std::shared_ptr<ModelRenderer> m_snout;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_legBackRight;
    std::shared_ptr<ModelRenderer> m_legBackLeft;
    std::shared_ptr<ModelRenderer> m_legFrontRight;
    std::shared_ptr<ModelRenderer> m_legFrontLeft;
    std::shared_ptr<ModelRenderer> m_tail;
};

/**
 * @brief 熊猫模型
 */
class PandaModel : public EntityModel {
public:
    PandaModel();
    explicit PandaModel(i32 textureOffset, f32 scale);
    ~PandaModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts(i32 textureOffset, f32 scale);

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_legBackRight;
    std::shared_ptr<ModelRenderer> m_legBackLeft;
    std::shared_ptr<ModelRenderer> m_legFrontRight;
    std::shared_ptr<ModelRenderer> m_legFrontLeft;
};

/**
 * @brief 鹦鹉模型
 */
class ParrotModel : public EntityModel {
public:
    ParrotModel();
    ~ParrotModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_tail;
    std::shared_ptr<ModelRenderer> m_wingLeft;
    std::shared_ptr<ModelRenderer> m_wingRight;
    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_head2;
    std::shared_ptr<ModelRenderer> m_beak1;
    std::shared_ptr<ModelRenderer> m_beak2;
    std::shared_ptr<ModelRenderer> m_feather;
    std::shared_ptr<ModelRenderer> m_legLeft;
    std::shared_ptr<ModelRenderer> m_legRight;
};

/**
 * @brief 幻翼模型
 */
class PhantomModel : public EntityModel {
public:
    PhantomModel();
    ~PhantomModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_leftWingBody;
    std::shared_ptr<ModelRenderer> m_leftWing;
    std::shared_ptr<ModelRenderer> m_rightWingBody;
    std::shared_ptr<ModelRenderer> m_rightWing;
    std::shared_ptr<ModelRenderer> m_tail1;
    std::shared_ptr<ModelRenderer> m_tail2;
};

/**
 * @brief 劫掠兽模型
 */
class RavagerModel : public EntityModel {
public:
    RavagerModel();
    ~RavagerModel() override = default;

    void render(f64 scale = 1.0f / 16.0f) override;
    void setAngles(
        f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale) override;

private:
    void _setupParts();

    std::shared_ptr<ModelRenderer> m_head;
    std::shared_ptr<ModelRenderer> m_jaw;
    std::shared_ptr<ModelRenderer> m_body;
    std::shared_ptr<ModelRenderer> m_neck;
    std::shared_ptr<ModelRenderer> m_legBackRight;
    std::shared_ptr<ModelRenderer> m_legBackLeft;
    std::shared_ptr<ModelRenderer> m_legFrontRight;
    std::shared_ptr<ModelRenderer> m_legFrontLeft;
};

} // namespace mc::client::renderer::entity::model::monster
