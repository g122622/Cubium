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

#include "AquaticModels.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <memory>

namespace mc::client::renderer::entity::model::aquatic {

// ==================== CodModel ====================

CodModel::CodModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    _setupParts();
}

void CodModel::_setupParts()
{
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-1.0f, -2.0f, 0.0f, 2.0f, 4.0f, 7.0f);
    m_body->setRotationPoint(0.0f, 22.0f, 0.0f);

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(11, 0);
    m_head->addBox(-1.0f, -2.0f, -3.0f, 2.0f, 4.0f, 3.0f);
    m_head->setRotationPoint(0.0f, 22.0f, 0.0f);

    m_headFront = std::make_shared<ModelRenderer>("headFront");
    m_headFront->setTextureOffset(0, 0);
    m_headFront->addBox(-1.0f, -2.0f, -1.0f, 2.0f, 3.0f, 1.0f);
    m_headFront->setRotationPoint(0.0f, 22.0f, -3.0f);

    m_finRight = std::make_shared<ModelRenderer>("finRight");
    m_finRight->setTextureOffset(22, 1);
    m_finRight->addBox(-2.0f, 0.0f, -1.0f, 2.0f, 0.0f, 2.0f);
    m_finRight->setRotationPoint(-1.0f, 23.0f, 0.0f);
    m_finRight->setRotateAngleZ(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0));

    m_finLeft = std::make_shared<ModelRenderer>("finLeft");
    m_finLeft->setTextureOffset(22, 4);
    m_finLeft->addBox(0.0f, 0.0f, -1.0f, 2.0f, 0.0f, 2.0f);
    m_finLeft->setRotationPoint(1.0f, 23.0f, 0.0f);
    m_finLeft->setRotateAngleZ(static_cast<f32>(mc::math::PI_DOUBLE / 4.0));

    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(22, 3);
    m_tail->addBox(0.0f, -2.0f, 0.0f, 0.0f, 4.0f, 4.0f);
    m_tail->setRotationPoint(0.0f, 22.0f, 7.0f);

    m_finTop = std::make_shared<ModelRenderer>("finTop");
    m_finTop->setTextureOffset(20, -6);
    m_finTop->addBox(0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 6.0f);
    m_finTop->setRotationPoint(0.0f, 20.0f, 0.0f);

    // 添加到部件列表
    m_parts.push_back(m_body);
    m_parts.push_back(m_head);
    m_parts.push_back(m_headFront);
    m_parts.push_back(m_finRight);
    m_parts.push_back(m_finLeft);
    m_parts.push_back(m_tail);
    m_parts.push_back(m_finTop);
}

void CodModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void CodModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    f32 f = m_isInWater ? 1.0f : 1.5f;
    m_tail->setRotateAngleY(-f * 0.45f * static_cast<f32>(std::sin(0.6 * ageInTicks)));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== SalmonModel ====================

SalmonModel::SalmonModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    _setupParts();
}

void SalmonModel::_setupParts()
{
    m_bodyFront = std::make_shared<ModelRenderer>("bodyFront");
    m_bodyFront->setTextureOffset(0, 0);
    m_bodyFront->addBox(-1.5f, -2.5f, 0.0f, 3.0f, 5.0f, 8.0f);
    m_bodyFront->setRotationPoint(0.0f, 20.0f, 0.0f);

    m_bodyRear = std::make_shared<ModelRenderer>("bodyRear");
    m_bodyRear->setTextureOffset(0, 13);
    m_bodyRear->addBox(-1.5f, -2.5f, 0.0f, 3.0f, 5.0f, 8.0f);
    m_bodyRear->setRotationPoint(0.0f, 20.0f, 8.0f);

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(22, 0);
    m_head->addBox(-1.0f, -2.0f, -3.0f, 2.0f, 4.0f, 3.0f);
    m_head->setRotationPoint(0.0f, 20.0f, 0.0f);

    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(20, 10);
    m_tail->addBox(0.0f, -2.5f, 0.0f, 0.0f, 5.0f, 6.0f);
    m_tail->setRotationPoint(0.0f, 0.0f, 8.0f);
    m_bodyRear->addChild(m_tail);

    m_dorsalFin = std::make_shared<ModelRenderer>("dorsalFin");
    m_dorsalFin->setTextureOffset(2, 1);
    m_dorsalFin->addBox(0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 3.0f);
    m_dorsalFin->setRotationPoint(0.0f, -4.5f, 5.0f);
    m_bodyFront->addChild(m_dorsalFin);

    m_ventralFin = std::make_shared<ModelRenderer>("ventralFin");
    m_ventralFin->setTextureOffset(0, 2);
    m_ventralFin->addBox(0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 4.0f);
    m_ventralFin->setRotationPoint(0.0f, -4.5f, -1.0f);
    m_bodyRear->addChild(m_ventralFin);

    m_finRight = std::make_shared<ModelRenderer>("finRight");
    m_finRight->setTextureOffset(-4, 0);
    m_finRight->addBox(-2.0f, 0.0f, 0.0f, 2.0f, 0.0f, 2.0f);
    m_finRight->setRotationPoint(-1.5f, 21.5f, 0.0f);
    m_finRight->setRotateAngleZ(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0));

    m_finLeft = std::make_shared<ModelRenderer>("finLeft");
    m_finLeft->setTextureOffset(0, 0);
    m_finLeft->addBox(0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 2.0f);
    m_finLeft->setRotationPoint(1.5f, 21.5f, 0.0f);
    m_finLeft->setRotateAngleZ(static_cast<f32>(mc::math::PI_DOUBLE / 4.0));

    // 子部件会跟随父部件渲染，不需要单独添加
    m_parts.push_back(m_bodyFront);
    m_parts.push_back(m_bodyRear);
    m_parts.push_back(m_head);
    m_parts.push_back(m_finRight);
    m_parts.push_back(m_finLeft);
}

void SalmonModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void SalmonModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    f32 f = m_isInWater ? 1.0f : 1.3f;
    f32 f1 = m_isInWater ? 1.0f : 1.7f;
    m_bodyRear->setRotateAngleY(-f * 0.25f * static_cast<f32>(std::sin(f1 * 0.6 * ageInTicks)));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== DolphinModel ====================

DolphinModel::DolphinModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    _setupParts();
}

void DolphinModel::_setupParts()
{
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(22, 0);
    m_body->addBox(-4.0f, -7.0f, 0.0f, 8.0f, 7.0f, 13.0f);
    m_body->setRotationPoint(0.0f, 22.0f, -5.0f);

    // 背鳍
    m_dorsalFin = std::make_shared<ModelRenderer>("dorsalFin");
    m_dorsalFin->setTextureOffset(51, 0);
    m_dorsalFin->addBox(-0.5f, 0.0f, 8.0f, 1.0f, 4.0f, 5.0f);
    m_dorsalFin->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 3.0));
    m_body->addChild(m_dorsalFin);

    // 尾巴
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(0, 19);
    m_tail->addBox(-2.0f, -2.5f, 0.0f, 4.0f, 5.0f, 11.0f);
    m_tail->setRotationPoint(0.0f, -2.5f, 11.0f);
    m_tail->setRotateAngleX(-0.10471976f);
    m_body->addChild(m_tail);

    // 尾鳍
    m_tailFin = std::make_shared<ModelRenderer>("tailFin");
    m_tailFin->setTextureOffset(19, 20);
    m_tailFin->addBox(-5.0f, -0.5f, 0.0f, 10.0f, 1.0f, 6.0f);
    m_tailFin->setRotationPoint(0.0f, 0.0f, 9.0f);
    m_tail->addChild(m_tailFin);

    // 右鳍
    m_finRight = std::make_shared<ModelRenderer>("finRight");
    m_finRight->setTextureOffset(48, 20);
    m_finRight->setMirror(true);
    m_finRight->addBox(-0.5f, -4.0f, 0.0f, 1.0f, 4.0f, 7.0f);
    m_finRight->setRotationPoint(2.0f, -2.0f, 4.0f);
    m_finRight->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 3.0));
    m_finRight->setRotateAngleZ(2.0943952f);
    m_body->addChild(m_finRight);

    // 左鳍
    m_finLeft = std::make_shared<ModelRenderer>("finLeft");
    m_finLeft->setTextureOffset(48, 20);
    m_finLeft->addBox(-0.5f, -4.0f, 0.0f, 1.0f, 4.0f, 7.0f);
    m_finLeft->setRotationPoint(-2.0f, -2.0f, 4.0f);
    m_finLeft->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 3.0));
    m_finLeft->setRotateAngleZ(-2.0943952f);
    m_body->addChild(m_finLeft);

    // 头部
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -3.0f, -3.0f, 8.0f, 7.0f, 6.0f);
    m_head->setRotationPoint(0.0f, -4.0f, -3.0f);
    m_body->addChild(m_head);

    // 鼻子
    m_nose = std::make_shared<ModelRenderer>("nose");
    m_nose->setTextureOffset(0, 13);
    m_nose->addBox(-1.0f, 2.0f, -7.0f, 2.0f, 2.0f, 4.0f);
    m_head->addChild(m_nose);

    // 只有 body 需要添加到 m_parts（其他都是子部件）
    m_parts.push_back(m_body);
}

void DolphinModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void DolphinModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    m_body->setRotateAngleX(mc::math::toRadians(static_cast<f32>(headPitch)));
    m_body->setRotateAngleY(mc::math::toRadians(static_cast<f32>(netHeadYaw)));

    // 对应 MC 1.21.11 DolphinModel.setupAnim：
    //   if (renderState.isMoving) {
    //       float wave = Mth.cos(ageInTicks * 0.3F);
    //       body.xRot += -0.05F - 0.05F * wave;
    //       tail.xRot = -0.1F * wave;
    //       tailFin.xRot = -0.2F * wave;
    //   } else {
    //       tail.xRot = -0.10471976F;  // 即 -PI/30，静态尾鳍基础角度
    //       // tailFin 保持初始 0
    //   }
    // isMoving 由 DolphinRenderer 从 deltaMovement.horizontalDistanceSqr() > 1.0E-7 计算，
    // 本项目中由 EntityRendererManager::_applyDolphinMotionState 推送到 m_motionMagnitude。
    if (m_motionMagnitude > MOTION_THRESHOLD) {
        f32 wave = static_cast<f32>(std::cos(ageInTicks * 0.3));
        m_body->setRotateAngleX(m_body->rotateAngleX() + (-0.05f - 0.05f * wave));
        m_tail->setRotateAngleX(-0.1f * wave);
        m_tailFin->setRotateAngleX(-0.2f * wave);
    } else {
        // 不移动时恢复静态角度（-PI/30 ≈ -0.10471976）
        m_tail->setRotateAngleX(-0.10471976f);
        m_tailFin->setRotateAngleX(0.0f);
    }

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)scale;
}

// ==================== TurtleModel ====================

TurtleModel::TurtleModel()
    : EntityModel()
{
    setTextureSize(128, 64);
    _setupParts(0.0f);
}

TurtleModel::TurtleModel(f32 scale)
    : EntityModel()
{
    setTextureSize(128, 64);
    _setupParts(scale);
}

void TurtleModel::_setupParts(f32 scale)
{
    // 头部
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(3, 0);
    m_head->addBox(-3.0f, -1.0f, -3.0f, 6.0f, 5.0f, 6.0f, scale);
    m_head->setRotationPoint(0.0f, 19.0f, -10.0f);
    m_parts.push_back(m_head);

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(7, 37);
    m_body->addBox(-9.5f, 3.0f, -10.0f, 19.0f, 20.0f, 6.0f, scale);
    m_body->setTextureOffset(31, 1);
    m_body->addBox(-5.5f, 3.0f, -13.0f, 11.0f, 18.0f, 3.0f, scale);
    m_body->setRotationPoint(0.0f, 11.0f, -10.0f);
    m_parts.push_back(m_body);

    // 怀孕腹部
    m_pregnant = std::make_shared<ModelRenderer>("pregnant");
    m_pregnant->setTextureOffset(70, 33);
    m_pregnant->addBox(-4.5f, 3.0f, -14.0f, 9.0f, 18.0f, 1.0f, scale);
    m_pregnant->setRotationPoint(0.0f, 11.0f, -10.0f);
    m_pregnant->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 2.0));
    m_pregnant->setVisible(false); // 默认隐藏
    m_parts.push_back(m_pregnant);

    // 右后腿
    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureOffset(1, 23);
    m_legBackRight->addBox(-2.0f, 0.0f, 0.0f, 4.0f, 1.0f, 10.0f, scale);
    m_legBackRight->setRotationPoint(-3.5f, 22.0f, 11.0f);
    m_parts.push_back(m_legBackRight);

    // 左后腿
    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureOffset(1, 12);
    m_legBackLeft->addBox(-2.0f, 0.0f, 0.0f, 4.0f, 1.0f, 10.0f, scale);
    m_legBackLeft->setRotationPoint(3.5f, 22.0f, 11.0f);
    m_parts.push_back(m_legBackLeft);

    // 右前腿
    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureOffset(27, 30);
    m_legFrontRight->addBox(-13.0f, 0.0f, -2.0f, 13.0f, 1.0f, 5.0f, scale);
    m_legFrontRight->setRotationPoint(-5.0f, 21.0f, -4.0f);
    m_parts.push_back(m_legFrontRight);

    // 左前腿
    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureOffset(27, 24);
    m_legFrontLeft->addBox(0.0f, 0.0f, -2.0f, 13.0f, 1.0f, 5.0f, scale);
    m_legFrontLeft->setRotationPoint(5.0f, 21.0f, -4.0f);
    m_parts.push_back(m_legFrontLeft);
}

void TurtleModel::render(f64 scale)
{
    // 如果有蛋且不是幼体，渲染怀孕腹部前先下移
    m_pregnant->setVisible(m_hasEgg && !m_isChild);
    EntityModel::render(scale);
}

void TurtleModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 后腿 X 轴旋转
    m_legBackRight->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 * 0.6) * 0.5 * limbSwingAmount));
    m_legBackLeft->setRotateAngleX(
        static_cast<f32>(std::cos(limbSwing * 0.6662 * 0.6 + mc::math::PI_DOUBLE) * 0.5 * limbSwingAmount));

    // 前腿 Z 轴旋转（与后腿相反）
    m_legFrontRight->setRotateAngleZ(
        static_cast<f32>(std::cos(limbSwing * 0.6662 * 0.6 + mc::math::PI_DOUBLE) * 0.5 * limbSwingAmount));
    m_legFrontLeft->setRotateAngleZ(static_cast<f32>(std::cos(limbSwing * 0.6662 * 0.6) * 0.5 * limbSwingAmount));

    // 前腿 X 和 Y 轴旋转归零
    m_legFrontRight->setRotateAngleX(0.0f);
    m_legFrontLeft->setRotateAngleX(0.0f);
    m_legFrontRight->setRotateAngleY(0.0f);
    m_legFrontLeft->setRotateAngleY(0.0f);
    m_legBackRight->setRotateAngleY(0.0f);
    m_legBackLeft->setRotateAngleY(0.0f);

    // 怀孕腹部 X 轴旋转
    m_pregnant->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 2.0));

    // 如果不在水中且在地面，执行爬行动画
    if (!m_isInWater && m_isOnGround) {
        f32 f = m_isDigging ? 4.0f : 1.0f;
        f32 f1 = m_isDigging ? 2.0f : 1.0f;

        // 前腿 Y 轴旋转
        m_legFrontRight->setRotateAngleY(
            static_cast<f32>(std::cos(f * limbSwing * 5.0 + mc::math::PI_DOUBLE) * 8.0 * limbSwingAmount * f1));
        m_legFrontLeft->setRotateAngleY(static_cast<f32>(std::cos(f * limbSwing * 5.0) * 8.0 * limbSwingAmount * f1));
        m_legFrontRight->setRotateAngleZ(0.0f);
        m_legFrontLeft->setRotateAngleZ(0.0f);

        // 后腿 Y 轴旋转
        m_legBackRight->setRotateAngleY(
            static_cast<f32>(std::cos(limbSwing * 5.0 + mc::math::PI_DOUBLE) * 3.0 * limbSwingAmount));
        m_legBackLeft->setRotateAngleY(static_cast<f32>(std::cos(limbSwing * 5.0) * 3.0 * limbSwingAmount));
        m_legBackRight->setRotateAngleX(0.0f);
        m_legBackLeft->setRotateAngleX(0.0f);
    }

    // 头部旋转
    m_head->setRotateAngleY(mc::math::toRadians(static_cast<f32>(netHeadYaw)));
    m_head->setRotateAngleX(mc::math::toRadians(static_cast<f32>(headPitch)));

    (void)ageInTicks;
    (void)scale;
}

// ==================== AbstractTropicalFishModel ====================

void AbstractTropicalFishModel::render(f64 scale)
{
    EntityModel::render(scale);
}

// ==================== TropicalFishAModel ====================

TropicalFishAModel::TropicalFishAModel(f32 scale)
    : AbstractTropicalFishModel()
{
    setTextureSize(32, 32);
    _setupParts(scale);
    // 添加部件到列表
    m_parts.push_back(m_body);
    m_parts.push_back(m_tail);
    m_parts.push_back(m_finRight);
    m_parts.push_back(m_finLeft);
    m_parts.push_back(m_finTop);
}

void TropicalFishAModel::_setupParts(f32 scale)
{
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-1.0f, -1.5f, -3.0f, 2.0f, 3.0f, 6.0f, scale);
    m_body->setRotationPoint(0.0f, 22.0f, 0.0f);

    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(22, -6);
    m_tail->addBox(0.0f, -1.5f, 0.0f, 0.0f, 3.0f, 6.0f, scale);
    m_tail->setRotationPoint(0.0f, 22.0f, 3.0f);

    m_finRight = std::make_shared<ModelRenderer>("finRight");
    m_finRight->setTextureOffset(2, 16);
    m_finRight->addBox(-2.0f, -1.0f, 0.0f, 2.0f, 2.0f, 0.0f, scale);
    m_finRight->setRotationPoint(-1.0f, 22.5f, 0.0f);
    m_finRight->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE / 4.0));

    m_finLeft = std::make_shared<ModelRenderer>("finLeft");
    m_finLeft->setTextureOffset(2, 12);
    m_finLeft->addBox(0.0f, -1.0f, 0.0f, 2.0f, 2.0f, 0.0f, scale);
    m_finLeft->setRotationPoint(1.0f, 22.5f, 0.0f);
    m_finLeft->setRotateAngleY(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0));

    m_finTop = std::make_shared<ModelRenderer>("finTop");
    m_finTop->setTextureOffset(10, -5);
    m_finTop->addBox(0.0f, -3.0f, 0.0f, 0.0f, 3.0f, 6.0f, scale);
    m_finTop->setRotationPoint(0.0f, 20.5f, -3.0f);
}

void TropicalFishAModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    f32 f = m_isInWater ? 1.0f : 1.5f;
    m_tail->setRotateAngleY(-f * 0.45f * static_cast<f32>(std::sin(0.6 * ageInTicks)));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== TropicalFishBModel ====================

TropicalFishBModel::TropicalFishBModel(f32 scale)
    : AbstractTropicalFishModel()
{
    setTextureSize(32, 32);
    _setupParts(scale);
    // 添加部件到列表
    m_parts.push_back(m_body);
    m_parts.push_back(m_tail);
    m_parts.push_back(m_finRight);
    m_parts.push_back(m_finLeft);
    m_parts.push_back(m_finTop);
    m_parts.push_back(m_finBottom);
}

void TropicalFishBModel::_setupParts(f32 scale)
{
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 20);
    m_body->addBox(-1.0f, -3.0f, -3.0f, 2.0f, 6.0f, 6.0f, scale);
    m_body->setRotationPoint(0.0f, 19.0f, 0.0f);

    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(21, 16);
    m_tail->addBox(0.0f, -3.0f, 0.0f, 0.0f, 6.0f, 5.0f, scale);
    m_tail->setRotationPoint(0.0f, 19.0f, 3.0f);

    m_finRight = std::make_shared<ModelRenderer>("finRight");
    m_finRight->setTextureOffset(2, 16);
    m_finRight->addBox(-2.0f, 0.0f, 0.0f, 2.0f, 2.0f, 0.0f, scale);
    m_finRight->setRotationPoint(-1.0f, 20.0f, 0.0f);
    m_finRight->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE / 4.0));

    m_finLeft = std::make_shared<ModelRenderer>("finLeft");
    m_finLeft->setTextureOffset(2, 12);
    m_finLeft->addBox(0.0f, 0.0f, 0.0f, 2.0f, 2.0f, 0.0f, scale);
    m_finLeft->setRotationPoint(1.0f, 20.0f, 0.0f);
    m_finLeft->setRotateAngleY(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0));

    m_finTop = std::make_shared<ModelRenderer>("finTop");
    m_finTop->setTextureOffset(20, 11);
    m_finTop->addBox(0.0f, -4.0f, 0.0f, 0.0f, 4.0f, 6.0f, scale);
    m_finTop->setRotationPoint(0.0f, 16.0f, -3.0f);

    m_finBottom = std::make_shared<ModelRenderer>("finBottom");
    m_finBottom->setTextureOffset(20, 21);
    m_finBottom->addBox(0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 6.0f, scale);
    m_finBottom->setRotationPoint(0.0f, 22.0f, -3.0f);
}

void TropicalFishBModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    f32 f = m_isInWater ? 1.0f : 1.5f;
    m_tail->setRotateAngleY(-f * 0.45f * static_cast<f32>(std::sin(0.6 * ageInTicks)));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ============================================================================
// AxolotlModel
// ============================================================================

AxolotlModel::AxolotlModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    _setupParts();
}

void AxolotlModel::_setupParts()
{
    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-4.0, -3.0, -7.0, 8.0, 6.0, 14.0);
    m_body->setRotationPoint(0.0, 20.0, 1.0);

    // 头部（身体子部件）
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 20);
    m_head->addBox(-4.0, -3.0, -5.0, 8.0, 5.0, 5.0);
    m_head->setRotationPoint(0.0, -1.0, -7.0);
    m_body->addChild(m_head);

    // 尾巴
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(44, 0);
    m_tail->addBox(-2.0, -2.0, 0.0, 4.0, 4.0, 7.0);
    m_tail->setRotationPoint(0.0, 0.0, 7.0);
    m_body->addChild(m_tail);

    // 左后腿
    m_leftHindLeg = std::make_shared<ModelRenderer>("leftHindLeg");
    m_leftHindLeg->setTextureOffset(30, 20);
    m_leftHindLeg->addBox(-1.0, 0.0, -1.0, 2.0, 4.0, 2.0);
    m_leftHindLeg->setRotationPoint(2.0, 3.0, 4.0);
    m_leftHindLeg->setMirror(true);
    m_body->addChild(m_leftHindLeg);

    // 右后腿
    m_rightHindLeg = std::make_shared<ModelRenderer>("rightHindLeg");
    m_rightHindLeg->setTextureOffset(30, 20);
    m_rightHindLeg->addBox(-1.0, 0.0, -1.0, 2.0, 4.0, 2.0);
    m_rightHindLeg->setRotationPoint(-2.0, 3.0, 4.0);
    m_body->addChild(m_rightHindLeg);

    // 左前腿
    m_leftFrontLeg = std::make_shared<ModelRenderer>("leftFrontLeg");
    m_leftFrontLeg->setTextureOffset(30, 20);
    m_leftFrontLeg->addBox(-1.0, 0.0, -1.0, 2.0, 4.0, 2.0);
    m_leftFrontLeg->setRotationPoint(2.0, 3.0, -4.0);
    m_leftFrontLeg->setMirror(true);
    m_body->addChild(m_leftFrontLeg);

    // 右前腿
    m_rightFrontLeg = std::make_shared<ModelRenderer>("rightFrontLeg");
    m_rightFrontLeg->setTextureOffset(30, 20);
    m_rightFrontLeg->addBox(-1.0, 0.0, -1.0, 2.0, 4.0, 2.0);
    m_rightFrontLeg->setRotationPoint(-2.0, 3.0, -4.0);
    m_body->addChild(m_rightFrontLeg);

    // 顶部鳃（头部子部件）
    m_topGills = std::make_shared<ModelRenderer>("topGills");
    m_topGills->setTextureOffset(0, 30);
    m_topGills->addBox(-3.0, -4.0, -2.0, 6.0, 1.0, 4.0);
    m_topGills->setRotationPoint(0.0, -3.0, -2.0);
    m_head->addChild(m_topGills);

    // 左侧鳃（头部子部件）
    m_leftGills = std::make_shared<ModelRenderer>("leftGills");
    m_leftGills->setTextureOffset(20, 30);
    m_leftGills->addBox(0.0, -2.0, -2.0, 1.0, 4.0, 4.0);
    m_leftGills->setRotationPoint(4.0, -1.0, -2.0);
    m_leftGills->setMirror(true);
    m_head->addChild(m_leftGills);

    // 右侧鳃（头部子部件）
    m_rightGills = std::make_shared<ModelRenderer>("rightGills");
    m_rightGills->setTextureOffset(20, 30);
    m_rightGills->addBox(-1.0, -2.0, -2.0, 1.0, 4.0, 4.0);
    m_rightGills->setRotationPoint(-4.0, -1.0, -2.0);
    m_head->addChild(m_rightGills);

    // 添加所有部件到渲染列表
    m_parts.push_back(m_body);
}

void AxolotlModel::render(f64 scale)
{
    // 幼体缩放
    f64 actualScale = scale;
    if (m_isChild) {
        actualScale = scale * 0.5;
    }
    EntityModel::render(actualScale);
}

void AxolotlModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 头部旋转
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw) * 0.017453292f);
    m_head->setRotateAngleX(static_cast<f32>(headPitch) * 0.017453292f);

    if (m_isPlayingDead) {
        // 装死时翻转
        m_body->setRotateAngleZ(1.5708f); // 90度
        m_tail->setRotateAngleY(0.0f);
        m_leftHindLeg->setRotateAngleX(0.0f);
        m_rightHindLeg->setRotateAngleX(0.0f);
        m_leftFrontLeg->setRotateAngleX(0.0f);
        m_rightFrontLeg->setRotateAngleX(0.0f);
    } else if (m_isInWater) {
        // 水中游泳动画
        f32 swimSpeed = 0.45f;
        f32 swimFreq = static_cast<f32>(std::sin(0.6 * ageInTicks));
        m_tail->setRotateAngleY(-swimSpeed * swimFreq);

        // 鳃的摆动
        m_topGills->setRotateAngleZ(0.05f * static_cast<f32>(std::sin(1.2 * ageInTicks)));
        m_leftGills->setRotateAngleZ(0.05f * static_cast<f32>(std::sin(1.2 * ageInTicks)));
        m_rightGills->setRotateAngleZ(-0.05f * static_cast<f32>(std::sin(1.2 * ageInTicks)));

        // 腿向后伸展
        m_leftHindLeg->setRotateAngleX(-0.5f);
        m_rightHindLeg->setRotateAngleX(-0.5f);
        m_leftFrontLeg->setRotateAngleX(-0.5f);
        m_rightFrontLeg->setRotateAngleX(-0.5f);

        m_body->setRotateAngleZ(0.0f);
    } else {
        // 陆地行走动画
        f32 walkSpeed = static_cast<f32>(limbSwing);
        f32 walkAmount = static_cast<f32>(limbSwingAmount);

        m_tail->setRotateAngleY(-0.15f * walkAmount * static_cast<f32>(std::sin(walkSpeed * 0.5)));

        // 腿部行走摆动
        // 对侧腿相位差 PI（对应 MC 1.21.11 AxolotlModel.setupAnim 陆地分支中
        // 使用 Mth.PI 对左右侧、前后腿做反相摆动）
        const f32 legPhaseOffset = static_cast<f32>(mc::math::PI);
        m_leftHindLeg->setRotateAngleX(walkAmount * 0.6f * static_cast<f32>(std::sin(walkSpeed)));
        m_rightHindLeg->setRotateAngleX(walkAmount * 0.6f * static_cast<f32>(std::sin(walkSpeed + legPhaseOffset)));
        m_leftFrontLeg->setRotateAngleX(walkAmount * 0.6f * static_cast<f32>(std::sin(walkSpeed + legPhaseOffset)));
        m_rightFrontLeg->setRotateAngleX(walkAmount * 0.6f * static_cast<f32>(std::sin(walkSpeed)));

        m_body->setRotateAngleZ(0.0f);
    }
}

} // namespace mc::client::renderer::entity::model::aquatic
