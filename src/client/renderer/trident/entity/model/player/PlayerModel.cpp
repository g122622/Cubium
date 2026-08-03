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
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/PlayerModelPart.hpp"
#include <array>
#include <memory>

namespace mc::client::renderer::entity::model::player {

// ModelRenderer 在父命名空间 mc::client::renderer::entity::model 中
using mc::client::renderer::entity::model::ModelRenderer;

PlayerModel::PlayerModel(f64 scale, bool slimArms)
    : BipedModel()
    , m_slimArms(slimArms)
{
    // 玩家使用 64x64 纹理（包含外层）
    setTextureSize(64, 64);

    // 基类 BipedModel::setupParts 用 ModelRenderer 默认 64×32 固化了 7 个部件的
    // 盒子 UV，此处对每个基类部件补设 64×64 触发 ModelRenderer::setTextureSize 的
    // 回溯重算，把已固化 UV 按 64×64 重新归一化。手臂（arm）在 _setup*Arms 中
    // clearBoxes 重建，其重算无实际作用但保持字段一致。
    m_bipedHead->setTextureSize(64, 64);
    m_bipedHeadwear->setTextureSize(64, 64);
    m_bipedBody->setTextureSize(64, 64);
    m_bipedRightArm->setTextureSize(64, 64);
    m_bipedLeftArm->setTextureSize(64, 64);
    m_bipedRightLeg->setTextureSize(64, 64);
    m_bipedLeftLeg->setTextureSize(64, 64);

    // 基类 BipedModel 已设置基础部件
    // 这里需要根据手臂类型重新设置手臂尺寸
    if (m_slimArms) {
        _setupSlimArms();
    } else {
        _setupStandardArms();
    }

    // 设置外观层部件
    _setupWearParts();

    // 设置斗篷和耳朵
    _setupCape();
    _setupEars();
}

void PlayerModel::_setupStandardArms()
{
    // 标准手臂：左臂 4x12x4，纹理位置 (32, 48)，旋转点 (5, 2, 0)
    // clearBoxes 清除基类 BipedModel 用 (40,16) 建的盒子，避免与玩家左臂 (32,48)
    // 盒子共存导致双臂重叠渲染。
    if (m_leftArm) {
        m_leftArm->clearBoxes();
        m_leftArm->setTextureSize(64, 64);
        m_leftArm->setTextureOffset(32, 48);
        m_leftArm->addBox(-1.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0.0f);
        m_leftArm->setRotationPoint(5.0f, 2.0f, 0.0f);
    }

    // 右臂：4x12x4，纹理位置 (40, 16)，旋转点 (-5, 2, 0)
    if (m_rightArm) {
        m_rightArm->clearBoxes();
        m_rightArm->setTextureSize(64, 64);
        m_rightArm->setTextureOffset(40, 16);
        m_rightArm->addBox(-3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0.0f);
        m_rightArm->setRotationPoint(-5.0f, 2.0f, 0.0f);
    }

    // 左腿纹理修正为 (16, 48)，基类 BipedModel 使用的是 (0, 16)
    if (m_leftLeg) {
        m_leftLeg->setTextureSize(64, 64);
        m_leftLeg->setTextureOffset(16, 48);
        // 盒子和旋转点与基类相同，无需重新设置
    }
}

void PlayerModel::_setupSlimArms()
{
    // 纤细手臂：左臂 3x12x4，纹理位置 (32, 48)，旋转点 (5, 2.5, 0)
    // clearBoxes 清除基类盒子，避免双臂重叠（见 _setupStandardArms 注释）。
    if (m_leftArm) {
        m_leftArm->clearBoxes();
        m_leftArm->setTextureSize(64, 64);
        m_leftArm->setTextureOffset(32, 48);
        m_leftArm->addBox(-1.0f, -2.0f, -2.0f, 3.0f, 12.0f, 4.0f, 0.0f);
        m_leftArm->setRotationPoint(5.0f, 2.5f, 0.0f);
    }

    // 右臂：3x12x4，纹理位置 (40, 16)，旋转点 (-5, 2.5, 0)
    if (m_rightArm) {
        m_rightArm->clearBoxes();
        m_rightArm->setTextureSize(64, 64);
        m_rightArm->setTextureOffset(40, 16);
        m_rightArm->addBox(-2.0f, -2.0f, -2.0f, 3.0f, 12.0f, 4.0f, 0.0f);
        m_rightArm->setRotationPoint(-5.0f, 2.5f, 0.0f);
    }

    // 左腿纹理修正为 (16, 48)，基类 BipedModel 使用的是 (0, 16)
    if (m_leftLeg) {
        m_leftLeg->setTextureSize(64, 64);
        m_leftLeg->setTextureOffset(16, 48);
        // 盒子和旋转点与基类相同，无需重新设置
    }
}

void PlayerModel::_setupWearParts()
{
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
        m_rightArmwear->setRotationPoint(-5.0f, 2.5f, 10.0f);
    } else {
        m_rightArmwear = std::make_shared<ModelRenderer>("rightArmwear");
        m_rightArmwear->setTextureSize(64, 64);
        m_rightArmwear->setTextureOffset(40, 32);
        m_rightArmwear->addBox(-3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, 0.25f);
        m_rightArmwear->setRotationPoint(-5.0f, 2.0f, 10.0f);
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

void PlayerModel::_setupCape()
{
    // 斗篷：10x16x1，纹理尺寸 64x32，纹理位置 (0, 0)
    m_cape = std::make_shared<ModelRenderer>("cape");
    m_cape->setTextureSize(64, 32);
    m_cape->setTextureOffset(0, 0);
    m_cape->addBox(-5.0f, 0.0f, -1.0f, 10.0f, 16.0f, 1.0f, 0.0f);
    // 斗篷位置在运行时根据状态调整
    m_parts.push_back(m_cape);
}

void PlayerModel::_setupEars()
{
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
    _animateArms(limbSwing, limbSwingAmount);

    (void)ageInTicks; // 玩家不使用 ageInTicks 进行额外动画
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
    // 根据玩家皮肤部件位掩码设置所有外层皮肤部件的可见性
    setPartVisible(PlayerModelPart::Hat, (playerModelParts & getPlayerModelPartMask(PlayerModelPart::Hat)) != 0);
    setPartVisible(PlayerModelPart::Jacket, (playerModelParts & getPlayerModelPartMask(PlayerModelPart::Jacket)) != 0);
    setPartVisible(
        PlayerModelPart::LeftPantsLeg, (playerModelParts & getPlayerModelPartMask(PlayerModelPart::LeftPantsLeg)) != 0);
    setPartVisible(PlayerModelPart::RightPantsLeg,
        (playerModelParts & getPlayerModelPartMask(PlayerModelPart::RightPantsLeg)) != 0);
    setPartVisible(
        PlayerModelPart::LeftSleeve, (playerModelParts & getPlayerModelPartMask(PlayerModelPart::LeftSleeve)) != 0);
    setPartVisible(
        PlayerModelPart::RightSleeve, (playerModelParts & getPlayerModelPartMask(PlayerModelPart::RightSleeve)) != 0);
    // Cape 由 CapeLayer 单独处理，这里不设置
}

void PlayerModel::translateHand(HandSide handSide, std::array<f64, 16>& outMatrix) const
{
    // 参考 MC 1.21.11 PlayerModel.translateToHand：
    //   super.translateToHand(state, arm, poseStack);
    //   if (this.slim) {
    //       float f = 0.5F * (arm == HumanoidArm.RIGHT ? 1 : -1);
    //       modelpart.x += f;
    //       modelpart.translateAndRotate(poseStack);
    //       modelpart.x -= f;
    //   } else {
    //       modelpart.translateAndRotate(poseStack);
    //   }
    //
    // 纤细手臂宽度由 4 缩减为 3，手臂中心向身体中线方向偏移 0.5：
    //   右手（X=-5）→ X+0.5 → X=-4.5（向中线靠近）
    //   左手（X=+5）→ X-0.5 → X=+4.5（向中线靠近）
    //
    // 采用无副作用模式：临时修改 rotationPointX 获取矩阵后立即恢复。

    const auto& arm = (handSide == HandSide::Left) ? m_bipedLeftArm : m_bipedRightArm;
    if (!arm) {
        // 手臂不存在，返回单位矩阵
        outMatrix = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
        return;
    }

    if (m_slimArms) {
        // 纤细手臂：临时偏移 rotationPointX，获取矩阵后恢复
        const f32 offsetX = (handSide == HandSide::Right) ? 0.5f : -0.5f;
        const f32 originalX = arm->rotationPointX();
        arm->setRotationPointX(originalX + offsetX);
        arm->getTransformMatrix(outMatrix);
        arm->setRotationPointX(originalX);
    } else {
        // 标准手臂：直接获取变换矩阵
        arm->getTransformMatrix(outMatrix);
    }
}

void PlayerModel::copyAnglesToWear()
{
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

void PlayerModel::_animateArms(f64 limbSwing, f64 limbSwingAmount)
{
    // TODO: 手臂姿态动画尚未完整实现，当前仅由基类 BipedModel::setAngles 处理基本姿态
    // PlayerModel 需要额外处理弓箭、弩等需要头部关联的姿态

    (void)limbSwing;
    (void)limbSwingAmount;
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

void PlayerModel::renderRightArm(f64 scale)
{
    // 仅渲染右臂和右袖外层，用于第三人称视角手臂渲染

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

    // 重置手臂X轴旋转角度，确保手臂水平伸出
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
    // 仅渲染左臂和左袖外层，用于第三人称视角手臂渲染

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

    // 重置手臂X轴旋转角度，确保手臂水平伸出
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
