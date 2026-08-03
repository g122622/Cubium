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

#include "PlayerModel.hpp"
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/firstperson/ArmPose.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <memory>

namespace mc::client::renderer {

// ============================================================================
// PlayerModel 实现
// ============================================================================

PlayerModel::PlayerModel(bool smallArms)
    : BipedModel()
    , m_smallArms(smallArms)
{
    setTextureSize(PLAYER_TEXTURE_WIDTH, PLAYER_TEXTURE_HEIGHT);
    setupParts();
}

void PlayerModel::setupParts()
{
    // 清除父类创建的部件
    m_parts.clear();

    // 创建新部件
    m_head = std::make_shared<ModelRenderer>("head");
    m_headWear = std::make_shared<ModelRenderer>("headWear");
    m_body = std::make_shared<ModelRenderer>("body");
    m_bodyWear = std::make_shared<ModelRenderer>("bodyWear");
    m_rightArm = std::make_shared<ModelRenderer>("rightArm");
    m_leftArm = std::make_shared<ModelRenderer>("leftArm");
    m_rightArmWear = std::make_shared<ModelRenderer>("rightArmWear");
    m_leftArmWear = std::make_shared<ModelRenderer>("leftArmWear");
    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");
    m_rightLegWear = std::make_shared<ModelRenderer>("rightLegWear");
    m_leftLegWear = std::make_shared<ModelRenderer>("leftLegWear");

    // 设置纹理尺寸
    m_head->setTextureSize(PLAYER_TEXTURE_WIDTH, PLAYER_TEXTURE_HEIGHT);
    m_headWear->setTextureSize(PLAYER_TEXTURE_WIDTH, PLAYER_TEXTURE_HEIGHT);
    m_body->setTextureSize(PLAYER_TEXTURE_WIDTH, PLAYER_TEXTURE_HEIGHT);
    m_bodyWear->setTextureSize(PLAYER_TEXTURE_WIDTH, PLAYER_TEXTURE_HEIGHT);
    m_rightArm->setTextureSize(PLAYER_TEXTURE_WIDTH, PLAYER_TEXTURE_HEIGHT);
    m_leftArm->setTextureSize(PLAYER_TEXTURE_WIDTH, PLAYER_TEXTURE_HEIGHT);
    m_rightArmWear->setTextureSize(PLAYER_TEXTURE_WIDTH, PLAYER_TEXTURE_HEIGHT);
    m_leftArmWear->setTextureSize(PLAYER_TEXTURE_WIDTH, PLAYER_TEXTURE_HEIGHT);
    m_rightLeg->setTextureSize(PLAYER_TEXTURE_WIDTH, PLAYER_TEXTURE_HEIGHT);
    m_leftLeg->setTextureSize(PLAYER_TEXTURE_WIDTH, PLAYER_TEXTURE_HEIGHT);
    m_rightLegWear->setTextureSize(PLAYER_TEXTURE_WIDTH, PLAYER_TEXTURE_HEIGHT);
    m_leftLegWear->setTextureSize(PLAYER_TEXTURE_WIDTH, PLAYER_TEXTURE_HEIGHT);

    // ========== 头部 ==========
    // 内层皮肤: 纹理位置 (0, 0), 尺寸 8x8x8
    m_head->addBox(0, 0, -4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 外层皮肤（帽子）: 纹理位置 (32, 0), 尺寸 8x8x8, 膨胀 0.5
    m_headWear->addBox(0, 0, -4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, 0.5f);
    m_headWear->setRotationPoint(0.0f, 0.0f, 0.0f);

    // ========== 身体 ==========
    // 内层皮肤: 纹理位置 (16, 16), 尺寸 8x12x4
    m_body->addBox(0, 16, -4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f);
    m_body->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 外层皮肤（外套）: 纹理位置 (16, 32), 尺寸 8x12x4, 膨胀 0.25
    m_bodyWear->addBox(0, 16, -4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f, 0.25f);
    m_bodyWear->setRotationPoint(0.0f, 0.0f, 0.0f);

    // ========== 手臂 ==========
    // 手臂尺寸根据 smallArms 设置决定
    // Steve (标准): 4x12x4
    // Alex (细手臂): 3x12x4

    const f64 armWidth = m_smallArms ? 3.0f : 4.0f;
    const f64 armOffset = m_smallArms ? 1.0f : 0.0f; // 细手臂需要居中偏移

    // 右臂内层: 纹理位置 (40, 16)
    m_rightArm->addBox(40, 16, -armWidth + armOffset, -2.0f, -2.0f, armWidth, 12.0f, 4.0f);
    m_rightArm->setRotationPoint(-5.0f, 2.0f, 0.0f);
    m_rightArm->setMirror(true); // 右臂需要镜像

    // 右臂外层（袖子）: 纹理位置 (40, 32), 膨胀 0.25
    m_rightArmWear->addBox(40, 32, -armWidth + armOffset, -2.0f, -2.0f, armWidth, 12.0f, 4.0f, 0.25f);
    m_rightArmWear->setRotationPoint(-5.0f, 2.0f, 0.0f);
    m_rightArmWear->setMirror(true);

    // 左臂内层: 纹理位置 (32, 48) - 注意左臂纹理在第二层
    m_leftArm->addBox(32, 48, -armOffset, -2.0f, -2.0f, armWidth, 12.0f, 4.0f);
    m_leftArm->setRotationPoint(5.0f, 2.0f, 0.0f);

    // 左臂外层（袖子）: 纹理位置 (48, 48), 膨胀 0.25
    m_leftArmWear->addBox(48, 48, -armOffset, -2.0f, -2.0f, armWidth, 12.0f, 4.0f, 0.25f);
    m_leftArmWear->setRotationPoint(5.0f, 2.0f, 0.0f);

    // ========== 腿部 ==========
    // 右腿内层: 纹理位置 (0, 16)
    m_rightLeg->addBox(0, 16, -2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_rightLeg->setRotationPoint(-2.0f, 12.0f, 0.0f);
    m_rightLeg->setMirror(true);

    // 右腿外层（裤腿）: 纹理位置 (0, 32), 膨胀 0.25
    m_rightLegWear->addBox(0, 32, -2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0.25f);
    m_rightLegWear->setRotationPoint(-2.0f, 12.0f, 0.0f);
    m_rightLegWear->setMirror(true);

    // 左腿内层: 纹理位置 (16, 48)
    m_leftLeg->addBox(16, 48, -2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_leftLeg->setRotationPoint(2.0f, 12.0f, 0.0f);

    // 左腿外层（裤腿）: 纹理位置 (0, 48), 膨胀 0.25
    m_leftLegWear->addBox(0, 48, -2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0.25f);
    m_leftLegWear->setRotationPoint(2.0f, 12.0f, 0.0f);

    // 添加到部件列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_headWear);
    m_parts.push_back(m_body);
    m_parts.push_back(m_bodyWear);
    m_parts.push_back(m_rightArm);
    m_parts.push_back(m_leftArm);
    m_parts.push_back(m_rightArmWear);
    m_parts.push_back(m_leftArmWear);
    m_parts.push_back(m_rightLeg);
    m_parts.push_back(m_leftLeg);
    m_parts.push_back(m_rightLegWear);
    m_parts.push_back(m_leftLegWear);
}

void PlayerModel::render(f64 scale)
{
    // 先渲染身体和腿部
    m_body->render(scale);
    m_bodyWear->render(scale);
    m_rightLeg->render(scale);
    m_leftLeg->render(scale);
    m_rightLegWear->render(scale);
    m_leftLegWear->render(scale);

    // 渲染手臂
    m_rightArm->render(scale);
    m_leftArm->render(scale);
    m_rightArmWear->render(scale);
    m_leftArmWear->render(scale);

    // 最后渲染头部（确保头部在最前面）
    m_head->render(scale);
    m_headWear->render(scale);
}

void PlayerModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 /*scale*/)
{
    // 头部旋转
    m_head->setRotateAngleX(math::toRadians(static_cast<f32>(headPitch)));
    m_head->setRotateAngleY(math::toRadians(static_cast<f32>(netHeadYaw)));

    // 帽子跟随头部
    m_headWear->setRotateAngleX(m_head->rotateAngleX());
    m_headWear->setRotateAngleY(m_head->rotateAngleY());
    m_headWear->setRotateAngleZ(m_head->rotateAngleZ());

    // 身体姿态
    if (m_fallFlying) {
        setupFallFlyingAngles();
    } else if (m_swimming) {
        setupSwimmingAngles();
    } else if (m_sneaking) {
        setupSneakingAngles();
    }

    // 步态动画
    const f64 walkAngle = limbSwing;
    const f64 walkAmount = limbSwingAmount;

    // 腿部摆动
    m_rightLeg->setRotateAngleX(std::cos(walkAngle) * 1.4f * walkAmount);
    m_leftLeg->setRotateAngleX(-std::cos(walkAngle) * 1.4f * walkAmount);
    m_rightLegWear->setRotateAngleX(m_rightLeg->rotateAngleX());
    m_leftLegWear->setRotateAngleX(m_leftLeg->rotateAngleX());

    // 手臂动画
    setupArmAngles(limbSwing, limbSwingAmount);
}

void PlayerModel::setupArmAngles(f64 limbSwing, f64 limbSwingAmount)
{
    // 基础手臂摆动
    const f64 swingAngle = limbSwing;
    const f64 swingAmount = limbSwingAmount;

    // 根据手臂姿态设置角度
    // 右臂
    switch (m_rightArmPose) {
        case ArmPose::Empty:
            // 空手：自然下垂摆动
            m_rightArm->setRotateAngleX(std::cos(swingAngle) * 2.0f * swingAmount);
            m_rightArm->setRotateAngleY(0.0f);
            m_rightArm->setRotateAngleZ(0.0f);
            break;

        case ArmPose::Item:
            // 持有物品：前伸
            m_rightArm->setRotateAngleX(math::toRadians(-45.0f));
            m_rightArm->setRotateAngleY(0.0f);
            m_rightArm->setRotateAngleZ(0.0f);
            break;

        case ArmPose::Block:
            // 格挡（盾牌）
            m_rightArm->setRotateAngleX(math::toRadians(-54.0f));
            m_rightArm->setRotateAngleY(math::toRadians(-30.0f));
            m_rightArm->setRotateAngleZ(0.0f);
            break;

        case ArmPose::BowAndArrow:
            // 拉弓
            m_rightArm->setRotateAngleX(math::toRadians(-45.0f));
            m_rightArm->setRotateAngleY(math::toRadians(-45.0f));
            m_rightArm->setRotateAngleZ(0.0f);
            break;

        case ArmPose::ThrowSpear:
            // 投掷三叉戟
            m_rightArm->setRotateAngleX(math::toRadians(-135.0f));
            m_rightArm->setRotateAngleY(0.0f);
            m_rightArm->setRotateAngleZ(0.0f);
            break;

        case ArmPose::CrossbowCharge:
        case ArmPose::CrossbowHold:
            // 装填弩 / 持有弩
            m_rightArm->setRotateAngleX(math::toRadians(-45.0f));
            m_rightArm->setRotateAngleY(math::toRadians(-20.0f));
            m_rightArm->setRotateAngleZ(0.0f);
            break;

        case ArmPose::EatOrDrink:
            // 吃食物/喝药水
            m_rightArm->setRotateAngleX(math::toRadians(-50.0f));
            m_rightArm->setRotateAngleY(math::toRadians(10.0f));
            m_rightArm->setRotateAngleZ(0.0f);
            break;

        default:
            m_rightArm->setRotateAngleX(std::cos(swingAngle) * 2.0f * swingAmount);
            break;
    }

    // 左臂
    switch (m_leftArmPose) {
        case ArmPose::Empty:
            m_leftArm->setRotateAngleX(-std::cos(swingAngle) * 2.0f * swingAmount);
            m_leftArm->setRotateAngleY(0.0f);
            m_leftArm->setRotateAngleZ(0.0f);
            break;

        case ArmPose::Item:
            m_leftArm->setRotateAngleX(math::toRadians(-45.0f));
            m_leftArm->setRotateAngleY(0.0f);
            m_leftArm->setRotateAngleZ(0.0f);
            break;

        case ArmPose::Block:
            m_leftArm->setRotateAngleX(math::toRadians(-54.0f));
            m_leftArm->setRotateAngleY(math::toRadians(30.0f));
            m_leftArm->setRotateAngleZ(0.0f);
            break;

        case ArmPose::BowAndArrow:
            // 拉弓时左臂也参与
            m_leftArm->setRotateAngleX(math::toRadians(-45.0f));
            m_leftArm->setRotateAngleY(math::toRadians(45.0f));
            m_leftArm->setRotateAngleZ(0.0f);
            break;

        case ArmPose::EatOrDrink:
            m_leftArm->setRotateAngleX(math::toRadians(-50.0f));
            m_leftArm->setRotateAngleY(math::toRadians(-10.0f));
            m_leftArm->setRotateAngleZ(0.0f);
            break;

        default:
            m_leftArm->setRotateAngleX(-std::cos(swingAngle) * 2.0f * swingAmount);
            break;
    }

    // 袖子跟随手臂
    m_rightArmWear->setRotateAngleX(m_rightArm->rotateAngleX());
    m_rightArmWear->setRotateAngleY(m_rightArm->rotateAngleY());
    m_rightArmWear->setRotateAngleZ(m_rightArm->rotateAngleZ());

    m_leftArmWear->setRotateAngleX(m_leftArm->rotateAngleX());
    m_leftArmWear->setRotateAngleY(m_leftArm->rotateAngleY());
    m_leftArmWear->setRotateAngleZ(m_leftArm->rotateAngleZ());

    // 潜行时调整手臂
    if (m_sneaking) {
        // 潜行时手臂略微前伸
        m_rightArm->setRotateAngleX(m_rightArm->rotateAngleX() - math::toRadians(15.0f));
        m_leftArm->setRotateAngleX(m_leftArm->rotateAngleX() - math::toRadians(15.0f));
    }
}

void PlayerModel::setupSneakingAngles()
{
    // 潜行时身体前倾
    m_body->setRotateAngleX(math::toRadians(29.0f));
    m_bodyWear->setRotateAngleX(math::toRadians(29.0f));

    // 腿部向后
    m_rightLeg->setRotationPoint(-2.0f, 14.0f, 2.0f);
    m_leftLeg->setRotationPoint(2.0f, 14.0f, 2.0f);
    m_rightLegWear->setRotationPoint(-2.0f, 14.0f, 2.0f);
    m_leftLegWear->setRotationPoint(2.0f, 14.0f, 2.0f);
}

void PlayerModel::setupSwimmingAngles()
{
    // 游泳时身体水平
    m_body->setRotateAngleX(math::HALF_PI);
    m_bodyWear->setRotateAngleX(math::HALF_PI);

    // 头部抬起
    m_head->setRotateAngleX(m_head->rotateAngleX() + math::toRadians(45.0f));
}

void PlayerModel::setupFallFlyingAngles()
{
    // 鞘翅飞行时身体水平，手臂向后
    m_body->setRotateAngleX(math::QUARTER_PI);
    m_bodyWear->setRotateAngleX(math::QUARTER_PI);

    // 手臂向后伸
    m_rightArm->setRotateAngleX(math::PI);
    m_leftArm->setRotateAngleX(math::PI);
    m_rightArmWear->setRotateAngleX(math::PI);
    m_leftArmWear->setRotateAngleX(math::PI);
}

void PlayerModel::setVisible(bool visible)
{
    m_head->setVisible(visible);
    m_headWear->setVisible(visible);
    m_body->setVisible(visible);
    m_bodyWear->setVisible(visible);
    m_rightArm->setVisible(visible);
    m_leftArm->setVisible(visible);
    m_rightArmWear->setVisible(visible);
    m_leftArmWear->setVisible(visible);
    m_rightLeg->setVisible(visible);
    m_leftLeg->setVisible(visible);
    m_rightLegWear->setVisible(visible);
    m_leftLegWear->setVisible(visible);
}

void PlayerModel::renderRightArm(f64 scale)
{
    m_rightArm->render(scale);
    m_rightArmWear->render(scale);
}

void PlayerModel::renderLeftArm(f64 scale)
{
    m_leftArm->render(scale);
    m_leftArmWear->render(scale);
}

void PlayerModel::setSmallArms(bool smallArms)
{
    if (m_smallArms != smallArms) {
        m_smallArms = smallArms;
        setupParts(); // 重建模型
    }
}

} // namespace mc::client::renderer
