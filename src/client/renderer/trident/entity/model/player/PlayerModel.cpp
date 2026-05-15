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
#include "common/util/math/MathConstants.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::player {

// ModelRenderer 在父命名空间 mc::client::renderer::entity::model 中
using mc::client::renderer::entity::model::ModelRenderer;

namespace {
constexpr f64 DEG_TO_RAD = mc::math::PI_DOUBLE / 180.0;
}

PlayerModel::PlayerModel(f64 scale, bool slimArms)
    : BipedModel()
    , m_slimArms(slimArms)
{
    // 参考 MC 1.16.5 PlayerModel
    // 玩家使用 64x64 纹理（包含外层）
    setTextureSize(64, 64);

    // 基类 BipedModel 已设置基础部件
    // 这里需要根据手臂类型重新设置手臂尺寸
    if (m_slimArms) {
        setupSlimArms();
    } else {
        setupStandardArms();
    }

    // 设置外观层部件
    setupWearParts();

    // 设置斗篷和耳朵
    setupCape();
    setupEars();
}

void PlayerModel::setupStandardArms()
{
    // 参考 MC 1.16.5 PlayerModel 构造函数（标准手臂）
    // 左臂：4x12x4，纹理位置 (32, 48)，旋转点 (5, 2, 0)
    if (m_leftArm) {
        m_leftArm->setTextureSize(64, 64);
        m_leftArm->setTextureOffset(32, 48);
        m_leftArm->addBox(-1.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0.0f);
        m_leftArm->setRotationPoint(5.0f, 2.0f, 0.0f);
    }

    // 右臂：4x12x4，纹理位置 (40, 16)，旋转点 (-5, 2, 0)
    if (m_rightArm) {
        m_rightArm->setTextureSize(64, 64);
        m_rightArm->setTextureOffset(40, 16);
        m_rightArm->addBox(-3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0.0f);
        m_rightArm->setRotationPoint(-5.0f, 2.0f, 0.0f);
    }

    // 左腿：Java 原版 PlayerModel 重新创建左腿，纹理 (16, 48)
    // 基类 BipedModel 使用的是 (0, 16)，需要修正
    if (m_leftLeg) {
        m_leftLeg->setTextureSize(64, 64);
        m_leftLeg->setTextureOffset(16, 48);
        // 盒子和旋转点与基类相同，无需重新设置
    }
}

void PlayerModel::setupSlimArms()
{
    // 参考 MC 1.16.5 PlayerModel 构造函数（纤细手臂）
    // 左臂：3x12x4，纹理位置 (32, 48)，旋转点 (5, 2.5, 0)
    if (m_leftArm) {
        m_leftArm->setTextureSize(64, 64);
        m_leftArm->setTextureOffset(32, 48);
        m_leftArm->addBox(-1.0f, -2.0f, -2.0f, 3.0f, 12.0f, 4.0f, 0.0f);
        m_leftArm->setRotationPoint(5.0f, 2.5f, 0.0f);
    }

    // 右臂：3x12x4，纹理位置 (40, 16)，旋转点 (-5, 2.5, 0)
    if (m_rightArm) {
        m_rightArm->setTextureSize(64, 64);
        m_rightArm->setTextureOffset(40, 16);
        m_rightArm->addBox(-2.0f, -2.0f, -2.0f, 3.0f, 12.0f, 4.0f, 0.0f);
        m_rightArm->setRotationPoint(-5.0f, 2.5f, 0.0f);
    }

    // 左腿：Java 原版 PlayerModel 重新创建左腿，纹理 (16, 48)
    // 基类 BipedModel 使用的是 (0, 16)，需要修正
    if (m_leftLeg) {
        m_leftLeg->setTextureSize(64, 64);
        m_leftLeg->setTextureOffset(16, 48);
        // 盒子和旋转点与基类相同，无需重新设置
    }
}

void PlayerModel::setupWearParts()
{
    // 参考 MC 1.16.5 PlayerModel 构造函数
    // 外观层部件比主部件大 0.25（膨胀）

    // 左臂外层
    if (m_slimArms) {
        m_leftArmwear = std::make_shared<ModelRenderer>("leftArmwear");
        m_leftArmwear->setTextureSize(64, 64);
        m_leftArmwear->setTextureOffset(48, 48);
        m_leftArmwear->addBox(-1.0f, -2.0f, -2.0f, 3.0f, 12.0f, 4.0f, 0.25f);
        m_leftArmwear->setRotationPoint(5.0f, 2.5f, 0.0f);
    } else {
        m_leftArmwear = std::make_shared<ModelRenderer>("leftArmwear");
        m_leftArmwear->setTextureSize(64, 64);
        m_leftArmwear->setTextureOffset(48, 48);
        m_leftArmwear->addBox(-1.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0.25f);
        m_leftArmwear->setRotationPoint(5.0f, 2.0f, 0.0f);
    }
    m_parts.push_back(m_leftArmwear);

    // 右臂外层
    if (m_slimArms) {
        m_rightArmwear = std::make_shared<ModelRenderer>("rightArmwear");
        m_rightArmwear->setTextureSize(64, 64);
        m_rightArmwear->setTextureOffset(40, 32);
        m_rightArmwear->addBox(-2.0f, -2.0f, -2.0f, 3.0f, 12.0f, 4.0f, 0.25f);
        m_rightArmwear->setRotationPoint(-5.0f, 2.5f, 10.0f); // 注意：MC 原版这里 Z=10.0F
    } else {
        m_rightArmwear = std::make_shared<ModelRenderer>("rightArmwear");
        m_rightArmwear->setTextureSize(64, 64);
        m_rightArmwear->setTextureOffset(40, 32);
        m_rightArmwear->addBox(-3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0.25f);
        m_rightArmwear->setRotationPoint(-5.0f, 2.0f, 10.0f); // 注意：MC 原版这里 Z=10.0F
    }
    m_parts.push_back(m_rightArmwear);

    // 左腿外层：纹理 (0, 48)
    m_leftLegwear = std::make_shared<ModelRenderer>("leftLegwear");
    m_leftLegwear->setTextureSize(64, 64);
    m_leftLegwear->setTextureOffset(0, 48);
    m_leftLegwear->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0.25f);
    m_leftLegwear->setRotationPoint(1.9f, 12.0f, 0.0f);
    m_parts.push_back(m_leftLegwear);

    // 右腿外层：纹理 (0, 32)
    m_rightLegwear = std::make_shared<ModelRenderer>("rightLegwear");
    m_rightLegwear->setTextureSize(64, 64);
    m_rightLegwear->setTextureOffset(0, 32);
    m_rightLegwear->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0.25f);
    m_rightLegwear->setRotationPoint(-1.9f, 12.0f, 0.0f);
    m_parts.push_back(m_rightLegwear);

    // 身体外层：纹理 (16, 32)
    m_bodywear = std::make_shared<ModelRenderer>("bodywear");
    m_bodywear->setTextureSize(64, 64);
    m_bodywear->setTextureOffset(16, 32);
    m_bodywear->addBox(-4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f, 0.25f);
    m_bodywear->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_bodywear);
}

void PlayerModel::setupCape()
{
    // 参考 MC 1.16.5 PlayerModel 构造函数
    // 斗篷：10x16x1，纹理尺寸 64x32，纹理位置 (0, 0)
    m_cape = std::make_shared<ModelRenderer>("cape");
    m_cape->setTextureSize(64, 32);
    m_cape->setTextureOffset(0, 0);
    m_cape->addBox(-5.0f, 0.0f, -1.0f, 10.0f, 16.0f, 1.0f, 0.0f);
    // 斗篷位置在运行时根据状态调整
    m_parts.push_back(m_cape);
}

void PlayerModel::setupEars()
{
    // 参考 MC 1.16.5 PlayerModel 构造函数
    // Deadmau5 耳朵：6x6x1，纹理位置 (24, 0)
    m_ears = std::make_shared<ModelRenderer>("ears");
    m_ears->setTextureSize(64, 64);
    m_ears->setTextureOffset(24, 0);
    m_ears->addBox(-3.0f, -6.0f, -1.0f, 6.0f, 6.0f, 1.0f, 0.0f);
    m_parts.push_back(m_ears);
}

void PlayerModel::render(f64 scale)
{
    // 渲染基础部件
    BipedModel::render(scale);
}

void PlayerModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 调用基类设置基础动画
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 复制角度到外观层
    copyAnglesToWear();

    // 处理手臂姿态
    animateArms(limbSwing, limbSwingAmount);

    (void)ageInTicks; // 玩家不使用 ageInTicks 进行额外动画
}

void PlayerModel::setArmPose(ArmPose leftArmPose, ArmPose rightArmPose)
{
    m_leftArmPose = leftArmPose;
    m_rightArmPose = rightArmPose;
}

void PlayerModel::setVisible(bool visible)
{
    BipedModel::setVisible(visible);

    if (m_leftArmwear) m_leftArmwear->setVisible(visible);
    if (m_rightArmwear) m_rightArmwear->setVisible(visible);
    if (m_leftLegwear) m_leftLegwear->setVisible(visible);
    if (m_rightLegwear) m_rightLegwear->setVisible(visible);
    if (m_bodywear) m_bodywear->setVisible(visible);
    if (m_cape) m_cape->setVisible(visible);
    if (m_ears) m_ears->setVisible(visible);
}

void PlayerModel::setPartVisible(PlayerModelPart part, bool visible)
{
    // 参考 MC 1.16.5 PlayerRenderer.setModelVisibilities
    // 根据 PlayerModelPart 设置对应模型部件的可见性
    switch (part) {
        case PlayerModelPart::Cape:
            if (m_cape) m_cape->setVisible(visible);
            break;
        case PlayerModelPart::Jacket:
            if (m_bodywear) m_bodywear->setVisible(visible);
            break;
        case PlayerModelPart::LeftSleeve:
            if (m_leftArmwear) m_leftArmwear->setVisible(visible);
            break;
        case PlayerModelPart::RightSleeve:
            if (m_rightArmwear) m_rightArmwear->setVisible(visible);
            break;
        case PlayerModelPart::LeftPantsLeg:
            if (m_leftLegwear) m_leftLegwear->setVisible(visible);
            break;
        case PlayerModelPart::RightPantsLeg:
            if (m_rightLegwear) m_rightLegwear->setVisible(visible);
            break;
        case PlayerModelPart::Hat:
            // Hat 是头部外层，在基类 BipedModel 中是 m_headwear
            if (m_headwear) m_headwear->setVisible(visible);
            break;
    }
}

bool PlayerModel::isPartVisible(PlayerModelPart part) const
{
    // 参考 MC 1.16.5 PlayerModel 部件可见性检查
    switch (part) {
        case PlayerModelPart::Cape:
            return m_cape ? m_cape->isVisible() : false;
        case PlayerModelPart::Jacket:
            return m_bodywear ? m_bodywear->isVisible() : false;
        case PlayerModelPart::LeftSleeve:
            return m_leftArmwear ? m_leftArmwear->isVisible() : false;
        case PlayerModelPart::RightSleeve:
            return m_rightArmwear ? m_rightArmwear->isVisible() : false;
        case PlayerModelPart::LeftPantsLeg:
            return m_leftLegwear ? m_leftLegwear->isVisible() : false;
        case PlayerModelPart::RightPantsLeg:
            return m_rightLegwear ? m_rightLegwear->isVisible() : false;
        case PlayerModelPart::Hat:
            return m_headwear ? m_headwear->isVisible() : false;
    }
    return false;
}

void PlayerModel::setModelVisibilitiesFromFlags(u8 playerModelParts)
{
    // 参考 MC 1.16.5 PlayerRenderer.setModelVisibilities
    // 根据玩家皮肤部件位掩码设置所有外层皮肤部件的可见性
    setPartVisible(PlayerModelPart::Hat, (playerModelParts & getPlayerModelPartMask(PlayerModelPart::Hat)) != 0);
    setPartVisible(PlayerModelPart::Jacket, (playerModelParts & getPlayerModelPartMask(PlayerModelPart::Jacket)) != 0);
    setPartVisible(PlayerModelPart::LeftPantsLeg, (playerModelParts & getPlayerModelPartMask(PlayerModelPart::LeftPantsLeg)) != 0);
    setPartVisible(PlayerModelPart::RightPantsLeg, (playerModelParts & getPlayerModelPartMask(PlayerModelPart::RightPantsLeg)) != 0);
    setPartVisible(PlayerModelPart::LeftSleeve, (playerModelParts & getPlayerModelPartMask(PlayerModelPart::LeftSleeve)) != 0);
    setPartVisible(PlayerModelPart::RightSleeve, (playerModelParts & getPlayerModelPartMask(PlayerModelPart::RightSleeve)) != 0);
    // Cape 由 CapeLayer 单独处理，这里不设置
}

void PlayerModel::translateHand(i32 side)
{
    // 参考 MC 1.16.5 PlayerModel.translateHand
    // 纤细手臂模式下需要偏移手臂位置
    ModelRenderer* arm = nullptr;
    f32 offset = 0.0f;

    if (side == static_cast<i32>(HandSide::Right)) {
        arm = m_rightArm.get();
        if (m_slimArms) {
            offset = -0.5f; // 纤细右手向左偏移
        }
    } else {
        arm = m_leftArm.get();
        if (m_slimArms) {
            offset = 0.5f; // 纤细左手向右偏移
        }
    }

    if (arm) {
        // 应用偏移
        arm->setRotationPointX(arm->rotationPointX() + offset);
        // 调用渲染后需要恢复偏移（调用者负责）
    }
}

void PlayerModel::copyAnglesToWear()
{
    // 参考 MC 1.16.5 PlayerModel.setRotationAngles
    // 复制主部件角度到外观层
    if (m_leftLeg && m_leftLegwear) {
        m_leftLegwear->copyModelAngles(*m_leftLeg);
    }
    if (m_rightLeg && m_rightLegwear) {
        m_rightLegwear->copyModelAngles(*m_rightLeg);
    }
    if (m_leftArm && m_leftArmwear) {
        m_leftArmwear->copyModelAngles(*m_leftArm);
    }
    if (m_rightArm && m_rightArmwear) {
        m_rightArmwear->copyModelAngles(*m_rightArm);
    }
    if (m_body && m_bodywear) {
        m_bodywear->copyModelAngles(*m_body);
    }
}

void PlayerModel::animateArms(f64 limbSwing, f64 limbSwingAmount)
{
    // 参考 MC 1.16.5 BipedModel.func_241654_b_ 和 func_241655_c_
    // 手臂姿态处理已在基类 BipedModel::setAngles 中完成
    // 这里只需要处理玩家特有的动画

    // 基类 BipedModel 已经处理了大部分手臂姿态
    // PlayerModel 只需要额外处理弓箭、弩等需要头部关联的姿态

    // 注意：BowAndArrow 姿态需要关联头部角度，这已在基类中处理

    (void)limbSwing;
    (void)limbSwingAmount;
}

void PlayerModel::animateBow(f64 limbSwing)
{
    // 参考 MC 1.16.5 BipedModel 弓姿态
    // 右手持弓，左手拉弦
    if (m_rightArm) {
        m_rightArm->setRotateAngleY(-0.1f);
        m_rightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
    }
    if (m_leftArm) {
        m_leftArm->setRotateAngleY(0.1f + 0.4f);
        m_leftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
    }
    (void)limbSwing; // 弓姿态不依赖 limbSwing
}

void PlayerModel::animateCrossbowCharge()
{
    // 参考 MC 1.16.5 BipedModel 弩装填姿态
    if (m_rightArm) {
        m_rightArm->setRotateAngleY(-0.8f);
        m_rightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
    }
    if (m_leftArm) {
        m_leftArm->setRotateAngleY(0.8f);
        m_leftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
    }
}

void PlayerModel::animateCrossbowHold()
{
    // 参考 MC 1.16.5 BipedModel 弩持有姿态
    if (m_rightArm) {
        m_rightArm->setRotateAngleY(-0.3f);
        m_rightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
    }
    if (m_leftArm) {
        m_leftArm->setRotateAngleY(0.6f);
        m_leftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
    }
}

void PlayerModel::renderCape(f64 scale)
{
    if (m_cape) {
        m_cape->render(scale);
    }
}

void PlayerModel::renderEars(f64 scale)
{
    if (m_ears && m_head) {
        // 耳朵跟随头部角度
        m_ears->copyModelAngles(*m_head);
        m_ears->setRotationPointX(0.0f);
        m_ears->setRotationPointY(0.0f);
        m_ears->render(scale);
    }
}

void PlayerModel::updateCapePosition(bool wearingChestplate, bool crouching)
{
    // 参考 MC 1.16.5 PlayerModel.setRotationAngles
    // 根据胸甲和蹲伏状态调整斗篷位置
    if (m_cape) {
        if (!wearingChestplate) {
            if (crouching) {
                m_cape->setRotationPointZ(1.4f);
                m_cape->setRotationPointY(1.85f);
            } else {
                m_cape->setRotationPointZ(0.0f);
                m_cape->setRotationPointY(0.0f);
            }
        } else if (crouching) {
            m_cape->setRotationPointZ(0.3f);
            m_cape->setRotationPointY(0.8f);
        } else {
            m_cape->setRotationPointZ(-1.1f);
            m_cape->setRotationPointY(-0.85f);
        }
    }
}

void PlayerModel::renderRightArm(f64 scale)
{
    // 参考 MC 1.16.5 PlayerRenderer.renderRightArm
    // 仅渲染右臂和右袖外层
    // 这是用于第三人称视角的手臂渲染

    // 保存当前可见性状态
    const bool rightArmVisible = m_rightArm ? m_rightArm->isVisible() : true;
    const bool rightArmwearVisible = m_rightArmwear ? m_rightArmwear->isVisible() : true;

    // 隐藏所有部件
    setVisible(false);

    // 仅显示右臂和右袖
    if (m_rightArm) {
        m_rightArm->setVisible(true);
    }
    if (m_rightArmwear) {
        m_rightArmwear->setVisible(true);
    }

    // 重置手臂X轴旋转角度（MC 1.16.5: rendererArmIn.rotateAngleX = 0.0F）
    // 这确保手臂水平伸出
    if (m_rightArm) {
        m_rightArm->setRotateAngleX(0.0f);
    }
    if (m_rightArmwear) {
        m_rightArmwear->setRotateAngleX(0.0f);
    }

    // 渲染右臂（内层皮肤）
    if (m_rightArm) {
        m_rightArm->render(scale);
    }

    // 渲染右袖（外层皮肤）
    if (m_rightArmwear) {
        m_rightArmwear->render(scale);
    }

    // 恢复原始可见性状态
    if (m_rightArm) {
        m_rightArm->setVisible(rightArmVisible);
    }
    if (m_rightArmwear) {
        m_rightArmwear->setVisible(rightArmwearVisible);
    }
}

void PlayerModel::renderLeftArm(f64 scale)
{
    // 参考 MC 1.16.5 PlayerRenderer.renderLeftArm
    // 仅渲染左臂和左袖外层
    // 这是用于第三人称视角的手臂渲染

    // 保存当前可见性状态
    const bool leftArmVisible = m_leftArm ? m_leftArm->isVisible() : true;
    const bool leftArmwearVisible = m_leftArmwear ? m_leftArmwear->isVisible() : true;

    // 隐藏所有部件
    setVisible(false);

    // 仅显示左臂和左袖
    if (m_leftArm) {
        m_leftArm->setVisible(true);
    }
    if (m_leftArmwear) {
        m_leftArmwear->setVisible(true);
    }

    // 重置手臂X轴旋转角度（MC 1.16.5: rendererArmIn.rotateAngleX = 0.0F）
    // 这确保手臂水平伸出
    if (m_leftArm) {
        m_leftArm->setRotateAngleX(0.0f);
    }
    if (m_leftArmwear) {
        m_leftArmwear->setRotateAngleX(0.0f);
    }

    // 渲染左臂（内层皮肤）
    if (m_leftArm) {
        m_leftArm->render(scale);
    }

    // 渲染左袖（外层皮肤）
    if (m_leftArmwear) {
        m_leftArmwear->render(scale);
    }

    // 恢复原始可见性状态
    if (m_leftArm) {
        m_leftArm->setVisible(leftArmVisible);
    }
    if (m_leftArmwear) {
        m_leftArmwear->setVisible(leftArmwearVisible);
    }
}

} // namespace mc::client::renderer::entity::model::player
