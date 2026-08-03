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

#include "BipedModel.hpp"
#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model {

BipedModel::BipedModel()
    : AgeableModel(true, 16.0f, 0.0f, 2.0f, 2.0f, 24.0f)
{
    setTextureSize(64, 32);
    m_modelScale = 0.0f;
    m_yOffset = 0.0f;

    // 创建部件
    m_bipedHead = std::make_shared<ModelRenderer>("bipedHead");
    m_bipedHeadwear = std::make_shared<ModelRenderer>("bipedHeadwear");
    m_bipedBody = std::make_shared<ModelRenderer>("bipedBody");
    m_bipedRightArm = std::make_shared<ModelRenderer>("bipedRightArm");
    m_bipedLeftArm = std::make_shared<ModelRenderer>("bipedLeftArm");
    m_bipedRightLeg = std::make_shared<ModelRenderer>("bipedRightLeg");
    m_bipedLeftLeg = std::make_shared<ModelRenderer>("bipedLeftLeg");

    setupParts();

    // 添加到部件列表
    m_parts.push_back(m_bipedHead);
    m_parts.push_back(m_bipedHeadwear);
    m_parts.push_back(m_bipedBody);
    m_parts.push_back(m_bipedRightArm);
    m_parts.push_back(m_bipedLeftArm);
    m_parts.push_back(m_bipedRightLeg);
    m_parts.push_back(m_bipedLeftLeg);
}

BipedModel::BipedModel(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight)
    : AgeableModel(true, 16.0f, 0.0f, 2.0f, 2.0f, 24.0f)
{
    setTextureSize(textureWidth, textureHeight);
    m_modelScale = scale;
    m_yOffset = yOffset;

    // 创建部件
    m_bipedHead = std::make_shared<ModelRenderer>("bipedHead");
    m_bipedHeadwear = std::make_shared<ModelRenderer>("bipedHeadwear");
    m_bipedBody = std::make_shared<ModelRenderer>("bipedBody");
    m_bipedRightArm = std::make_shared<ModelRenderer>("bipedRightArm");
    m_bipedLeftArm = std::make_shared<ModelRenderer>("bipedLeftArm");
    m_bipedRightLeg = std::make_shared<ModelRenderer>("bipedRightLeg");
    m_bipedLeftLeg = std::make_shared<ModelRenderer>("bipedLeftLeg");

    setupParts();

    // 添加到部件列表
    m_parts.push_back(m_bipedHead);
    m_parts.push_back(m_bipedHeadwear);
    m_parts.push_back(m_bipedBody);
    m_parts.push_back(m_bipedRightArm);
    m_parts.push_back(m_bipedLeftArm);
    m_parts.push_back(m_bipedRightLeg);
    m_parts.push_back(m_bipedLeftLeg);
}

void BipedModel::setupParts()
{
    // 头部
    m_bipedHead->setTextureOffset(0, 0);
    m_bipedHead->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, m_modelScale);
    m_bipedHead->setRotationPoint(0.0f, 0.0f + m_yOffset, 0.0f);

    // 帽子层（膨胀 +0.5）
    m_bipedHeadwear->setTextureOffset(32, 0);
    m_bipedHeadwear->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, m_modelScale + 0.5f);
    m_bipedHeadwear->setRotationPoint(0.0f, 0.0f + m_yOffset, 0.0f);

    // 身体
    m_bipedBody->setTextureOffset(16, 16);
    m_bipedBody->addBox(-4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f, m_modelScale);
    m_bipedBody->setRotationPoint(0.0f, 0.0f + m_yOffset, 0.0f);

    // 右臂
    m_bipedRightArm->setTextureOffset(40, 16);
    m_bipedRightArm->addBox(-3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, m_modelScale);
    m_bipedRightArm->setRotationPoint(-5.0f, 2.0f + m_yOffset, 0.0f);

    // 左臂（镜像）
    m_bipedLeftArm->setTextureOffset(40, 16);
    m_bipedLeftArm->setMirror(true);
    m_bipedLeftArm->addBox(-1.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, m_modelScale);
    m_bipedLeftArm->setRotationPoint(5.0f, 2.0f + m_yOffset, 0.0f);

    // 右腿（注意 X 偏移是 -1.9 而非 -2.0）
    m_bipedRightLeg->setTextureOffset(0, 16);
    m_bipedRightLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, m_modelScale);
    m_bipedRightLeg->setRotationPoint(-1.9f, 12.0f + m_yOffset, 0.0f);

    // 左腿（镜像，注意 X 偏移是 1.9 而非 2.0）
    m_bipedLeftLeg->setTextureOffset(0, 16);
    m_bipedLeftLeg->setMirror(true);
    m_bipedLeftLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, m_modelScale);
    m_bipedLeftLeg->setRotationPoint(1.9f, 12.0f + m_yOffset, 0.0f);
}

std::vector<std::shared_ptr<ModelRenderer>> BipedModel::getHeadParts() const
{
    return {m_bipedHead};
}

std::vector<std::shared_ptr<ModelRenderer>> BipedModel::getBodyParts() const
{
    return {m_bipedBody, m_bipedRightArm, m_bipedLeftArm, m_bipedRightLeg, m_bipedLeftLeg, m_bipedHeadwear};
}

void BipedModel::render(f64 scale)
{
    AgeableModel::render(scale);
}

void BipedModel::setLivingAnimations(f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 /*partialTick*/)
{
    // 子类可重写此方法设置游泳动画等状态
}

void BipedModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 /*scale*/)
{
    // 鞘翅飞行状态：MC 1.21.11 HumanoidModel.setupAnim 仅检查 isFallFlying 布尔值，
    // 不再使用 fallFlyTicks（HumanoidRenderState 已移除该字段）。
    // MC 1.21.11 中鞘翅飞行起始时头部角度直接 snap 到 -π/4（无过渡 lerp），
    // 过渡 lerp 仅用于游泳（swimAmount > 0 路径）。
    // Cubium 已与此行为对齐，无需额外跟踪 fallFlyTicks 驱动头部动画。
    bool isElytraFlying = m_isFallFlying;
    bool isActuallySwimming = m_isActuallySwimming;

    // 头部旋转
    f32 headYawRad = static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0);
    f32 headPitchRad = static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0);

    // 鞘翅飞行或游泳时的头部角度
    if (isElytraFlying) {
        m_bipedHead->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0));
    } else if (m_swimAnimation > 0.0f) {
        if (isActuallySwimming) {
            m_bipedHead->setRotateAngleX(
                rotLerpRad(m_swimAnimation, m_bipedHead->rotateAngleX(), static_cast<f32>(-mc::math::PI_DOUBLE / 4.0)));
        } else {
            m_bipedHead->setRotateAngleX(rotLerpRad(m_swimAnimation, m_bipedHead->rotateAngleX(), headPitchRad));
        }
    } else {
        m_bipedHead->setRotateAngleX(headPitchRad);
    }
    m_bipedHead->setRotateAngleY(headYawRad);

    // 身体旋转
    m_bipedBody->setRotateAngleY(0.0f);

    // 重置手臂旋转点
    m_bipedRightArm->setRotationPointZ(0.0f);
    m_bipedRightArm->setRotationPointX(-5.0f);
    m_bipedLeftArm->setRotationPointZ(0.0f);
    m_bipedLeftArm->setRotationPointX(5.0f);

    // 速度因子：对应 MC 1.21.11 HumanoidRenderState.speedValue
    // 默认 1.0；鞘翅飞行时为 (deltaMovement.lengthSqr() / 0.2)^3，最终钳制到 [1.0, +∞)
    // 该值由渲染器在 setAngles 前通过 setSpeedValue() 推送。
    // 用作手臂/腿部摆动振幅的除数：值越大，摆动越慢（视觉上像风阻）。
    f32 f = m_speedValue;
    if (f < 1.0f) {
        f = 1.0f;
    }

    // 步态动画
    f32 limbSwingFloat = static_cast<f32>(limbSwing);
    f32 limbSwingAmountFloat = static_cast<f32>(limbSwingAmount);

    // 手臂摆动
    m_bipedRightArm->setRotateAngleX(static_cast<f32>(
        std::cos(limbSwingFloat * 0.6662f + mc::math::PI_DOUBLE) * 2.0 * limbSwingAmountFloat * 0.5 / f));
    m_bipedLeftArm->setRotateAngleX(
        static_cast<f32>(std::cos(limbSwingFloat * 0.6662f) * 2.0 * limbSwingAmountFloat * 0.5 / f));

    m_bipedRightArm->setRotateAngleZ(0.0f);
    m_bipedLeftArm->setRotateAngleZ(0.0f);

    // 腿部动画
    m_bipedRightLeg->setRotateAngleX(
        static_cast<f32>(std::cos(limbSwingFloat * 0.6662f) * 1.4 * limbSwingAmountFloat / f));
    m_bipedLeftLeg->setRotateAngleX(
        static_cast<f32>(std::cos(limbSwingFloat * 0.6662f + mc::math::PI_DOUBLE) * 1.4 * limbSwingAmountFloat / f));

    m_bipedRightLeg->setRotateAngleY(0.0f);
    m_bipedLeftLeg->setRotateAngleY(0.0f);
    m_bipedRightLeg->setRotateAngleZ(0.0f);
    m_bipedLeftLeg->setRotateAngleZ(0.0f);

    // 坐姿
    if (m_isSitting) {
        m_bipedRightArm->setRotateAngleX(
            m_bipedRightArm->rotateAngleX() + static_cast<f32>(-mc::math::PI_DOUBLE / 5.0));
        m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() + static_cast<f32>(-mc::math::PI_DOUBLE / 5.0));
        m_bipedRightLeg->setRotateAngleX(-1.4137167f);
        m_bipedRightLeg->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE / 10.0));
        m_bipedRightLeg->setRotateAngleZ(0.07853982f);
        m_bipedLeftLeg->setRotateAngleX(-1.4137167f);
        m_bipedLeftLeg->setRotateAngleY(static_cast<f32>(-mc::math::PI_DOUBLE / 10.0));
        m_bipedLeftLeg->setRotateAngleZ(-0.07853982f);
    }

    // 手臂姿态处理 - 根据主手和姿态类型决定处理顺序
    m_bipedRightArm->setRotateAngleY(0.0f);
    m_bipedLeftArm->setRotateAngleY(0.0f);

    // 判断是否需要双臂协调（弓箭、弩等姿态会影响另一只手臂的处理顺序）
    bool isRightHanded = (m_mainHand == HandSide::Right);
    bool needsCrossArmCoord = isRightHanded
        ? (m_leftArmPose == ArmPose::BowAndArrow || m_leftArmPose == ArmPose::CrossbowCharge ||
              m_leftArmPose == ArmPose::CrossbowHold)
        : (m_rightArmPose == ArmPose::BowAndArrow || m_rightArmPose == ArmPose::CrossbowCharge ||
              m_rightArmPose == ArmPose::CrossbowHold);

    if (isRightHanded != needsCrossArmCoord) {
        // 先左手，后右手
        handleLeftArmPose();
        handleRightArmPose();
    } else {
        // 先右手，后左手
        handleRightArmPose();
        handleLeftArmPose();
    }

    // 挥动动画
    handleSwingAnimation(ageInTicks);

    // 游泳动画
    if (m_swimAnimation > 0.0f) {
        handleSwimAnimation(limbSwing);
    }

    // 蹲伏
    if (m_isSneaking) {
        m_bipedBody->setRotateAngleX(0.5f);
        m_bipedRightArm->setRotateAngleX(m_bipedRightArm->rotateAngleX() + 0.4f);
        m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() + 0.4f);
        m_bipedRightLeg->setRotationPointZ(4.0f);
        m_bipedLeftLeg->setRotationPointZ(4.0f);
        m_bipedRightLeg->setRotationPointY(12.2f);
        m_bipedLeftLeg->setRotationPointY(12.2f);
        m_bipedHead->setRotationPointY(4.2f);
        m_bipedBody->setRotationPointY(3.2f);
        m_bipedLeftArm->setRotationPointY(5.2f);
        m_bipedRightArm->setRotationPointY(5.2f);
    } else {
        m_bipedBody->setRotateAngleX(0.0f);
        m_bipedRightLeg->setRotationPointZ(0.1f);
        m_bipedLeftLeg->setRotationPointZ(0.1f);
        m_bipedRightLeg->setRotationPointY(12.0f);
        m_bipedLeftLeg->setRotationPointY(12.0f);
        m_bipedHead->setRotationPointY(0.0f + m_yOffset);
        m_bipedBody->setRotationPointY(0.0f + m_yOffset);
        m_bipedLeftArm->setRotationPointY(2.0f + m_yOffset);
        m_bipedRightArm->setRotationPointY(2.0f + m_yOffset);
    }

    // 帽子跟随头部
    m_bipedHeadwear->copyModelAngles(*m_bipedHead);
}

void BipedModel::handleSwimAnimation(f64 limbSwing)
{
    f32 f1 = static_cast<f32>(std::fmod(limbSwing, 26.0));
    HandSide mainHand = getMainHand();
    f32 f2 = (mainHand == HandSide::Right && m_swingProgress > 0.0f) ? 0.0f : m_swimAnimation;
    f32 f3 = (mainHand == HandSide::Left && m_swingProgress > 0.0f) ? 0.0f : m_swimAnimation;

    if (f1 < 14.0f) {
        m_bipedLeftArm->setRotateAngleX(rotLerpRad(f3, m_bipedLeftArm->rotateAngleX(), 0.0f));
        m_bipedRightArm->setRotateAngleX(static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleX()));
        m_bipedLeftArm->setRotateAngleY(
            rotLerpRad(f3, m_bipedLeftArm->rotateAngleY(), static_cast<f32>(mc::math::PI_DOUBLE)));
        m_bipedRightArm->setRotateAngleY(
            static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleY() + f2 * mc::math::PI_DOUBLE));
        m_bipedLeftArm->setRotateAngleZ(rotLerpRad(f3,
            m_bipedLeftArm->rotateAngleZ(),
            static_cast<f32>(mc::math::PI_DOUBLE + 1.8707964 * getArmAngleSq(f1) / getArmAngleSq(14.0f))));
        m_bipedRightArm->setRotateAngleZ(static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleZ() +
            f2 * (mc::math::PI_DOUBLE - 1.8707964 * getArmAngleSq(f1) / getArmAngleSq(14.0f))));
    } else if (f1 >= 14.0f && f1 < 22.0f) {
        f32 f6 = (f1 - 14.0f) / 8.0f;
        m_bipedLeftArm->setRotateAngleX(
            rotLerpRad(f3, m_bipedLeftArm->rotateAngleX(), static_cast<f32>(mc::math::PI_DOUBLE / 2.0 * f6)));
        m_bipedRightArm->setRotateAngleX(
            static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleX() + f2 * mc::math::PI_DOUBLE / 2.0 * f6));
        m_bipedLeftArm->setRotateAngleY(
            rotLerpRad(f3, m_bipedLeftArm->rotateAngleY(), static_cast<f32>(mc::math::PI_DOUBLE)));
        m_bipedRightArm->setRotateAngleY(
            static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleY() + f2 * mc::math::PI_DOUBLE));
        m_bipedLeftArm->setRotateAngleZ(rotLerpRad(f3, m_bipedLeftArm->rotateAngleZ(), 5.012389f - 1.8707964f * f6));
        m_bipedRightArm->setRotateAngleZ(
            static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleZ() + f2 * (1.2707963f + 1.8707964f * f6)));
    } else if (f1 >= 22.0f && f1 < 26.0f) {
        f32 f4 = (f1 - 22.0f) / 4.0f;
        m_bipedLeftArm->setRotateAngleX(rotLerpRad(f3,
            m_bipedLeftArm->rotateAngleX(),
            static_cast<f32>(mc::math::PI_DOUBLE / 2.0 - mc::math::PI_DOUBLE / 2.0 * f4)));
        m_bipedRightArm->setRotateAngleX(static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleX() +
            f2 * (mc::math::PI_DOUBLE / 2.0 - mc::math::PI_DOUBLE / 2.0 * f4)));
        m_bipedLeftArm->setRotateAngleY(
            rotLerpRad(f3, m_bipedLeftArm->rotateAngleY(), static_cast<f32>(mc::math::PI_DOUBLE)));
        m_bipedRightArm->setRotateAngleY(
            static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleY() + f2 * mc::math::PI_DOUBLE));
        m_bipedLeftArm->setRotateAngleZ(
            rotLerpRad(f3, m_bipedLeftArm->rotateAngleZ(), static_cast<f32>(mc::math::PI_DOUBLE)));
        m_bipedRightArm->setRotateAngleZ(
            static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleZ() + f2 * mc::math::PI_DOUBLE));
    }

    // 腿部游泳动画
    f32 swimLegAngle1 = 0.3f * static_cast<f32>(std::cos(limbSwing * 0.33333334 + mc::math::PI_DOUBLE));
    f32 swimLegAngle2 = 0.3f * static_cast<f32>(std::cos(limbSwing * 0.33333334));
    m_bipedLeftLeg->setRotateAngleX(
        static_cast<f32>((1.0f - m_swimAnimation) * m_bipedLeftLeg->rotateAngleX() + m_swimAnimation * swimLegAngle1));
    m_bipedRightLeg->setRotateAngleX(
        static_cast<f32>((1.0f - m_swimAnimation) * m_bipedRightLeg->rotateAngleX() + m_swimAnimation * swimLegAngle2));
}

void BipedModel::handleRightArmPose()
{
    switch (m_rightArmPose) {
        case ArmPose::Empty:
            m_bipedRightArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::Block:
            m_bipedRightArm->setRotateAngleX(m_bipedRightArm->rotateAngleX() * 0.5f - 0.9424779f);
            m_bipedRightArm->setRotateAngleY(static_cast<f32>(-mc::math::PI_DOUBLE / 6.0));
            break;
        case ArmPose::Item:
            m_bipedRightArm->setRotateAngleX(
                m_bipedRightArm->rotateAngleX() * 0.5f - static_cast<f32>(mc::math::PI_DOUBLE / 10.0));
            m_bipedRightArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::ThrowSpear:
            m_bipedRightArm->setRotateAngleX(
                m_bipedRightArm->rotateAngleX() * 0.5f - static_cast<f32>(mc::math::PI_DOUBLE));
            m_bipedRightArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::BowAndArrow:
            m_bipedRightArm->setRotateAngleY(-0.1f + m_bipedHead->rotateAngleY());
            m_bipedLeftArm->setRotateAngleY(0.1f + m_bipedHead->rotateAngleY() + 0.4f);
            m_bipedRightArm->setRotateAngleX(
                static_cast<f32>(-mc::math::PI_DOUBLE / 2.0) + m_bipedHead->rotateAngleX());
            m_bipedLeftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0) + m_bipedHead->rotateAngleX());
            break;
        case ArmPose::CrossbowCharge:
            // 弩装填：双手协调姿态，由右手分支主导设置双手角度
            // 仅当右手姿态为 CrossbowCharge 时调用（避免与左手分支重复设置）
            handleCrossbowCharge(true);
            break;
        case ArmPose::CrossbowHold:
            // 弩持握：双手协调姿态，由右手分支主导设置双手角度
            handleCrossbowHold(true);
            break;
        case ArmPose::Spyglass: {
            // 望远镜：右臂贴近眼部前方，X 跟随头部 pitch（带 clamp），Y 为头部 yaw - π/12
            const f32 headPitch = m_bipedHead->rotateAngleX();
            const f32 headYaw = m_bipedHead->rotateAngleY();
            const f32 crouchOffset = m_isSneaking ? static_cast<f32>(mc::math::PI_DOUBLE / 12.0) : 0.0f;
            m_bipedRightArm->setRotateAngleX(std::clamp(headPitch - 1.9198622f - crouchOffset, -2.4f, 3.3f));
            m_bipedRightArm->setRotateAngleY(headYaw - static_cast<f32>(mc::math::PI_DOUBLE / 12.0));
            break;
        }
        case ArmPose::Brush:
            // 刷子：右臂半折，向前下方刷扫
            m_bipedRightArm->setRotateAngleX(
                m_bipedRightArm->rotateAngleX() * 0.5f - static_cast<f32>(mc::math::PI_DOUBLE / 5.0));
            m_bipedRightArm->setRotateAngleY(0.0f);
            break;
    }
}

void BipedModel::handleLeftArmPose()
{
    switch (m_leftArmPose) {
        case ArmPose::Empty:
            m_bipedLeftArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::Block:
            m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() * 0.5f - 0.9424779f);
            m_bipedLeftArm->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE / 6.0));
            break;
        case ArmPose::Item:
            m_bipedLeftArm->setRotateAngleX(
                m_bipedLeftArm->rotateAngleX() * 0.5f - static_cast<f32>(mc::math::PI_DOUBLE / 10.0));
            m_bipedLeftArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::ThrowSpear:
            m_bipedLeftArm->setRotateAngleX(
                m_bipedLeftArm->rotateAngleX() * 0.5f - static_cast<f32>(mc::math::PI_DOUBLE));
            m_bipedLeftArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::BowAndArrow:
            // 弓箭姿态：左手跟随头部方向（右手的关联已在 handleRightArmPose 中设置）
            m_bipedRightArm->setRotateAngleY(-0.1f + m_bipedHead->rotateAngleY() - 0.4f);
            m_bipedLeftArm->setRotateAngleY(0.1f + m_bipedHead->rotateAngleY());
            m_bipedRightArm->setRotateAngleX(
                static_cast<f32>(-mc::math::PI_DOUBLE / 2.0) + m_bipedHead->rotateAngleX());
            m_bipedLeftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0) + m_bipedHead->rotateAngleX());
            break;
        case ArmPose::CrossbowCharge:
            // 弩装填是双手协调姿态：仅当右手姿态不是 CrossbowCharge 时才由此分支主导
            // 否则双手已在 handleRightArmPose::CrossbowCharge 中被设置，此处跳过避免重复
            if (m_rightArmPose != ArmPose::CrossbowCharge) {
                handleCrossbowCharge(false);
            }
            break;
        case ArmPose::CrossbowHold:
            // 弩持握同理：仅当右手姿态不是 CrossbowHold 时由此分支主导
            if (m_rightArmPose != ArmPose::CrossbowHold) {
                handleCrossbowHold(false);
            }
            break;
        case ArmPose::Spyglass: {
            // 望远镜：左臂贴近眼部前方，X 跟随头部 pitch（带 clamp），Y 为头部 yaw + π/12
            const f32 headPitch = m_bipedHead->rotateAngleX();
            const f32 headYaw = m_bipedHead->rotateAngleY();
            const f32 crouchOffset = m_isSneaking ? static_cast<f32>(mc::math::PI_DOUBLE / 12.0) : 0.0f;
            m_bipedLeftArm->setRotateAngleX(std::clamp(headPitch - 1.9198622f - crouchOffset, -2.4f, 3.3f));
            m_bipedLeftArm->setRotateAngleY(headYaw + static_cast<f32>(mc::math::PI_DOUBLE / 12.0));
            break;
        }
        case ArmPose::Brush:
            // 刷子：左臂半折，向前下方刷扫
            m_bipedLeftArm->setRotateAngleX(
                m_bipedLeftArm->rotateAngleX() * 0.5f - static_cast<f32>(mc::math::PI_DOUBLE / 5.0));
            m_bipedLeftArm->setRotateAngleY(0.0f);
            break;
    }
}

void BipedModel::handleCrossbowCharge(bool isRightHanded)
{
    // 主手（持弩手）与副手（拉弦手）
    auto& mainArm = isRightHanded ? m_bipedRightArm : m_bipedLeftArm;
    auto& offArm = isRightHanded ? m_bipedLeftArm : m_bipedRightArm;

    // 主手角度：固定向前下方
    mainArm->setRotateAngleY(isRightHanded ? -0.8f : 0.8f);
    mainArm->setRotateAngleX(-0.97079635f);

    // 副手初始 X 角度与主手相同
    offArm->setRotateAngleX(mainArm->rotateAngleX());

    // 装填进度归一化到 [0, 1]，避免除零
    f32 progress = 0.0f;
    if (m_maxCrossbowChargeDuration > 0.0f) {
        const f32 clampedTicks = std::clamp(m_crossbowChargeTicks, 0.0f, m_maxCrossbowChargeDuration);
        progress = clampedTicks / m_maxCrossbowChargeDuration;
    }

    // 副手 Y 角度：从 0.4 lerp 到 0.85（带主手方向符号）
    const f32 sign = isRightHanded ? 1.0f : -1.0f;
    const f32 offArmY = (0.4f + (0.85f - 0.4f) * progress) * sign;
    offArm->setRotateAngleY(offArmY);

    // 副手 X 角度：从初始 -0.97079635 lerp 到 -PI/2
    const f32 targetX = static_cast<f32>(-mc::math::PI_DOUBLE / 2.0);
    const f32 offArmX = -0.97079635f + (targetX - (-0.97079635f)) * progress;
    offArm->setRotateAngleX(offArmX);
}

void BipedModel::handleCrossbowHold(bool isRightHanded)
{
    // 主手（持弩手）与副手（拉弦手）
    auto& mainArm = isRightHanded ? m_bipedRightArm : m_bipedLeftArm;
    auto& offArm = isRightHanded ? m_bipedLeftArm : m_bipedRightArm;

    const f32 headYaw = m_bipedHead->rotateAngleY();
    const f32 headPitch = m_bipedHead->rotateAngleX();
    const f32 sign = isRightHanded ? 1.0f : -1.0f;

    // 主手 Y: ±0.3 + headYaw；副手 Y: ∓0.6 + headYaw
    mainArm->setRotateAngleY((-0.3f * sign) + headYaw);
    offArm->setRotateAngleY((0.6f * sign) + headYaw);

    // 主手 X: -PI/2 + headPitch + 0.1；副手 X: -1.5 + headPitch
    mainArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0) + headPitch + 0.1f);
    offArm->setRotateAngleX(-1.5f + headPitch);
}

void BipedModel::handleSwingAnimation(f64 /*ageInTicks*/)
{
    if (m_swingProgress <= 0.0f) {
        return;
    }

    // 获取挥动的手臂
    HandSide mainHand = getMainHand();
    auto swingArm = (mainHand == HandSide::Left) ? m_bipedLeftArm : m_bipedRightArm;

    // 身体扭动
    f32 swingProgress = m_swingProgress;
    f32 bodyRotateY = static_cast<f32>(std::sin(std::sqrt(swingProgress) * mc::math::PI_DOUBLE * 2.0) * 0.2);
    if (mainHand == HandSide::Left) {
        bodyRotateY = -bodyRotateY;
    }
    m_bipedBody->setRotateAngleY(bodyRotateY);

    // 手臂位置调整
    m_bipedRightArm->setRotationPointZ(static_cast<f32>(std::sin(m_bipedBody->rotateAngleY()) * 5.0));
    m_bipedRightArm->setRotationPointX(static_cast<f32>(-std::cos(m_bipedBody->rotateAngleY()) * 5.0));
    m_bipedLeftArm->setRotationPointZ(static_cast<f32>(-std::sin(m_bipedBody->rotateAngleY()) * 5.0));
    m_bipedLeftArm->setRotationPointX(static_cast<f32>(std::cos(m_bipedBody->rotateAngleY()) * 5.0));

    m_bipedRightArm->setRotateAngleY(m_bipedRightArm->rotateAngleY() + m_bipedBody->rotateAngleY());
    m_bipedLeftArm->setRotateAngleY(m_bipedLeftArm->rotateAngleY() + m_bipedBody->rotateAngleY());
    m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() + m_bipedBody->rotateAngleY());

    // 挥动动画
    f32 f = 1.0f - swingProgress;
    f = f * f;
    f = f * f;
    f = 1.0f - f;
    f32 f1 = static_cast<f32>(std::sin(f * mc::math::PI_DOUBLE));
    f32 f2 =
        static_cast<f32>(std::sin(swingProgress * mc::math::PI_DOUBLE) * -(m_bipedHead->rotateAngleX() - 0.7f) * 0.75f);

    swingArm->setRotateAngleX(swingArm->rotateAngleX() - (f1 * 1.2f + f2));
    swingArm->setRotateAngleY(swingArm->rotateAngleY() + m_bipedBody->rotateAngleY() * 2.0f);
    swingArm->setRotateAngleZ(
        swingArm->rotateAngleZ() + static_cast<f32>(std::sin(swingProgress * mc::math::PI_DOUBLE) * -0.4f));
}

void BipedModel::setVisible(bool visible)
{
    m_bipedHead->setVisible(visible);
    m_bipedHeadwear->setVisible(visible);
    m_bipedBody->setVisible(visible);
    m_bipedRightArm->setVisible(visible);
    m_bipedLeftArm->setVisible(visible);
    m_bipedRightLeg->setVisible(visible);
    m_bipedLeftLeg->setVisible(visible);
}

void BipedModel::copyModelAttributesTo(BipedModel& target) const
{
    // 复制 EntityModel 的属性
    target.setSwingProgress(m_swingProgress);
    target.setSitting(m_isSitting);
    target.setChild(m_isChild);

    // 复制 BipedModel 特有属性
    target.m_leftArmPose = m_leftArmPose;
    target.m_rightArmPose = m_rightArmPose;
    target.m_isSneaking = m_isSneaking;
    target.m_swimAnimation = m_swimAnimation;
    target.m_mainHand = m_mainHand;
    // 保留历史遗留字段复制（m_elytraFlyingTicks 当前未被推送，永远为 0，
    // 但保留复制以避免未来重新启用时遗漏）。
    target.m_elytraFlyingTicks = m_elytraFlyingTicks;
    target.m_isFallFlying = m_isFallFlying;
    target.m_speedValue = m_speedValue;

    // 复制部件角度
    target.m_bipedHead->copyModelAngles(*m_bipedHead);
    target.m_bipedHeadwear->copyModelAngles(*m_bipedHeadwear);
    target.m_bipedBody->copyModelAngles(*m_bipedBody);
    target.m_bipedRightArm->copyModelAngles(*m_bipedRightArm);
    target.m_bipedLeftArm->copyModelAngles(*m_bipedLeftArm);
    target.m_bipedRightLeg->copyModelAngles(*m_bipedRightLeg);
    target.m_bipedLeftLeg->copyModelAngles(*m_bipedLeftLeg);
}

std::shared_ptr<ModelRenderer> BipedModel::getArmForSide(HandSide side)
{
    return (side == HandSide::Left) ? m_bipedLeftArm : m_bipedRightArm;
}

HandSide BipedModel::getMainHand() const
{
    // 如果正在挥动的手是主手，返回主手；否则返回副手
    if (m_swingingHand == m_mainHand) {
        return m_mainHand;
    }
    return (m_mainHand == HandSide::Left) ? HandSide::Right : HandSide::Left;
}

f32 BipedModel::rotLerpRad(f32 angle, f64 maxAngle, f64 target)
{
    f64 f = std::fmod(target - maxAngle, mc::math::PI_DOUBLE * 2.0);
    if (f < -mc::math::PI_DOUBLE) {
        f += mc::math::PI_DOUBLE * 2.0;
    }
    if (f >= mc::math::PI_DOUBLE) {
        f -= mc::math::PI_DOUBLE * 2.0;
    }
    return static_cast<f32>(maxAngle + static_cast<f64>(angle) * f);
}

f32 BipedModel::getArmAngleSq(f32 limbSwing)
{
    return -65.0f * limbSwing + limbSwing * limbSwing;
}

void BipedModel::translateHand(HandSide handSide, std::array<f64, 16>& outMatrix) const
{
    // 获取指定手侧的手臂，并构建其变换矩阵
    const auto& arm = (handSide == HandSide::Left) ? m_bipedLeftArm : m_bipedRightArm;

    if (!arm) {
        // 如果手臂不存在，返回单位矩阵
        outMatrix = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
        return;
    }

    // 获取手臂的变换矩阵
    arm->getTransformMatrix(outMatrix);
}

} // namespace mc::client::renderer::entity::model
