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

#include "HorseModel.hpp"
#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model::animal {

namespace {
// 度数角度插值，在 [-180, 180) 范围内取最短路径
f32 rotLerp(f32 factor, f32 prevAngle, f32 curAngle)
{
    return prevAngle + factor * mc::math::wrapDegrees(curAngle - prevAngle);
}
} // namespace

HorseModel::HorseModel(f32 scale)
    : AgeableModel(true, 16.2f, 1.36f, 2.7272f, 2.0f, 20.0f)
    , m_scale(scale)
{
    setTextureSize(64, 64);

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 32);
    m_body->addBox(-5.0f, -8.0f, -17.0f, 10.0f, 10.0f, 22.0f, 0.05f + static_cast<f64>(scale));
    m_body->setRotationPoint(0.0f, 11.0f, 5.0f);

    // 头部
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 35);
    m_head->addBox(-2.05f, -6.0f, -2.0f, 4.0f, 12.0f, 7.0f, static_cast<f64>(scale));
    m_head->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 6.0));

    // 头部子部件 - 马鬃上部
    auto headTop = m_head->createChild("headTop");
    headTop->setTextureOffset(0, 13);
    headTop->addBox(-3.0f, -11.0f, -2.0f, 6.0f, 5.0f, 7.0f, static_cast<f64>(scale));

    // 马鬃
    auto mane = m_head->createChild("mane");
    mane->setTextureOffset(56, 36);
    mane->addBox(-1.0f, -11.0f, 5.01f, 2.0f, 16.0f, 2.0f, static_cast<f64>(scale));

    // 口鼻部
    auto muzzle = m_head->createChild("muzzle");
    muzzle->setTextureOffset(0, 25);
    muzzle->addBox(-2.0f, -11.0f, -7.0f, 4.0f, 5.0f, 5.0f, static_cast<f64>(scale));

    // 添加耳朵
    addEars(m_head);

    // 后右腿（成年体）
    m_backRightLeg = std::make_shared<ModelRenderer>("backRightLeg");
    m_backRightLeg->setTextureOffset(48, 21);
    m_backRightLeg->setMirror(true);
    m_backRightLeg->addBox(-3.0f, -1.01f, -1.0f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale));
    m_backRightLeg->setRotationPoint(4.0f, 14.0f, 7.0f);

    // 后左腿（成年体）
    m_backLeftLeg = std::make_shared<ModelRenderer>("backLeftLeg");
    m_backLeftLeg->setTextureOffset(48, 21);
    m_backLeftLeg->addBox(-1.0f, -1.01f, -1.0f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale));
    m_backLeftLeg->setRotationPoint(-4.0f, 14.0f, 7.0f);

    // 前右腿（成年体）
    m_frontRightLeg = std::make_shared<ModelRenderer>("frontRightLeg");
    m_frontRightLeg->setTextureOffset(48, 21);
    m_frontRightLeg->setMirror(true);
    m_frontRightLeg->addBox(-3.0f, -1.01f, -1.9f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale));
    m_frontRightLeg->setRotationPoint(4.0f, 6.0f, -12.0f);

    // 前左腿（成年体）
    m_frontLeftLeg = std::make_shared<ModelRenderer>("frontLeftLeg");
    m_frontLeftLeg->setTextureOffset(48, 21);
    m_frontLeftLeg->addBox(-1.0f, -1.01f, -1.9f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale));
    m_frontLeftLeg->setRotationPoint(-4.0f, 6.0f, -12.0f);

    // 幼体腿部
    constexpr f32 babyScale = 5.5f;

    // 后右腿（幼体）
    m_backRightLegBaby = std::make_shared<ModelRenderer>("backRightLegBaby");
    m_backRightLegBaby->setTextureOffset(48, 21);
    m_backRightLegBaby->setMirror(true);
    m_backRightLegBaby->addBox(-3.0f, -1.01f, -1.0f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale + babyScale));
    m_backRightLegBaby->setRotationPoint(4.0f, 14.0f, 7.0f);

    // 后左腿（幼体）
    m_backLeftLegBaby = std::make_shared<ModelRenderer>("backLeftLegBaby");
    m_backLeftLegBaby->setTextureOffset(48, 21);
    m_backLeftLegBaby->addBox(-1.0f, -1.01f, -1.0f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale + babyScale));
    m_backLeftLegBaby->setRotationPoint(-4.0f, 14.0f, 7.0f);

    // 前右腿（幼体）
    m_frontRightLegBaby = std::make_shared<ModelRenderer>("frontRightLegBaby");
    m_frontRightLegBaby->setTextureOffset(48, 21);
    m_frontRightLegBaby->setMirror(true);
    m_frontRightLegBaby->addBox(-3.0f, -1.01f, -1.9f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale + babyScale));
    m_frontRightLegBaby->setRotationPoint(4.0f, 6.0f, -12.0f);

    // 前左腿（幼体）
    m_frontLeftLegBaby = std::make_shared<ModelRenderer>("frontLeftLegBaby");
    m_frontLeftLegBaby->setTextureOffset(48, 21);
    m_frontLeftLegBaby->addBox(-1.0f, -1.01f, -1.9f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale + babyScale));
    m_frontLeftLegBaby->setRotationPoint(-4.0f, 6.0f, -12.0f);

    // 尾巴
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(42, 36);
    m_tail->addBox(-1.5f, 0.0f, 0.0f, 3.0f, 14.0f, 4.0f, static_cast<f64>(scale));
    m_tail->setRotationPoint(0.0f, -5.0f, 2.0f);
    m_tail->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 6.0));
    m_body->addChild(m_tail);

    // 鞍部件 - 身体上的鞍
    auto saddleBody = m_body->createChild("saddleBody");
    saddleBody->setTextureOffset(26, 0);
    saddleBody->addBox(-5.0f, -8.0f, -9.0f, 10.0f, 9.0f, 9.0f, 0.5f);
    m_saddleParts.push_back(saddleBody);

    // 鞍部件 - 右耳装饰
    auto saddleRightEar = m_head->createChild("saddleRightEar");
    saddleRightEar->setTextureOffset(29, 5);
    saddleRightEar->addBox(2.0f, -9.0f, -6.0f, 1.0f, 2.0f, 2.0f, static_cast<f64>(scale));
    m_saddleParts.push_back(saddleRightEar);

    // 鞍部件 - 左耳装饰
    auto saddleLeftEar = m_head->createChild("saddleLeftEar");
    saddleLeftEar->setTextureOffset(29, 5);
    saddleLeftEar->addBox(-3.0f, -9.0f, -6.0f, 1.0f, 2.0f, 2.0f, static_cast<f64>(scale));
    m_saddleParts.push_back(saddleLeftEar);

    // 鞍部件 - 额头装饰
    auto saddleForehead = m_head->createChild("saddleForehead");
    saddleForehead->setTextureOffset(1, 1);
    saddleForehead->addBox(-3.0f, -11.0f, -1.9f, 6.0f, 5.0f, 6.0f, 0.2f);
    m_saddleParts.push_back(saddleForehead);

    // 鞍部件 - 鼻装饰
    auto saddleNose = m_head->createChild("saddleNose");
    saddleNose->setTextureOffset(19, 0);
    saddleNose->addBox(-2.0f, -11.0f, -4.0f, 4.0f, 5.0f, 2.0f, 0.2f);
    m_saddleParts.push_back(saddleNose);

    // 骑乘部件（缰绳）
    // 右缰绳
    auto reinRight = m_head->createChild("reinRight");
    reinRight->setTextureOffset(32, 2);
    reinRight->addBox(3.1f, -6.0f, -8.0f, 0.0f, 3.0f, 16.0f, static_cast<f64>(scale));
    reinRight->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 6.0));
    m_ridingParts.push_back(reinRight);

    // 左缰绳
    auto reinLeft = m_head->createChild("reinLeft");
    reinLeft->setTextureOffset(32, 2);
    reinLeft->addBox(-3.1f, -6.0f, -8.0f, 0.0f, 3.0f, 16.0f, static_cast<f64>(scale));
    reinLeft->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 6.0));
    m_ridingParts.push_back(reinLeft);

    // 初始隐藏幼体腿部
    m_backRightLegBaby->setVisible(false);
    m_backLeftLegBaby->setVisible(false);
    m_frontRightLegBaby->setVisible(false);
    m_frontLeftLegBaby->setVisible(false);
}

void HorseModel::addEars(std::shared_ptr<ModelRenderer> head)
{
    // 右耳
    auto rightEar = head->createChild("rightEar");
    rightEar->setTextureOffset(19, 16);
    rightEar->addBox(0.55f, -13.0f, 4.0f, 2.0f, 3.0f, 1.0f, -0.001);

    // 左耳
    auto leftEar = head->createChild("leftEar");
    leftEar->setTextureOffset(19, 16);
    leftEar->addBox(-2.55f, -13.0f, 4.0f, 2.0f, 3.0f, 1.0f, -0.001);
}

std::vector<std::shared_ptr<ModelRenderer>> HorseModel::getHeadParts() const
{
    return {m_head};
}

std::vector<std::shared_ptr<ModelRenderer>> HorseModel::getBodyParts() const
{
    return {m_body,
        m_backRightLeg,
        m_backLeftLeg,
        m_frontRightLeg,
        m_frontLeftLeg,
        m_backRightLegBaby,
        m_backLeftLegBaby,
        m_frontRightLegBaby,
        m_frontLeftLegBaby};
}

void HorseModel::render(f64 scale)
{
    AgeableModel::render(scale);
}

void HorseModel::setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 partialTick)
{
    AgeableModel::setLivingAnimations(limbSwing, limbSwingAmount, partialTick);

    f32 f = rotLerp(static_cast<f32>(partialTick), m_prevRenderYawOffset, m_renderYawOffset);
    f32 f1 = rotLerp(static_cast<f32>(partialTick), m_prevRotationYawHead, m_rotationYawHead);
    f32 f2 = mc::math::lerp(m_prevRotationPitch, m_rotationPitch, static_cast<f32>(partialTick));
    f32 f3 = f1 - f;
    f32 f4 = f2 * static_cast<f32>(mc::math::PI_DOUBLE / 180.0);

    if (f3 > 20.0f) {
        f3 = 20.0f;
    }
    if (f3 < -20.0f) {
        f3 = -20.0f;
    }

    f32 limbSwingFloat = static_cast<f32>(limbSwing);
    f32 limbSwingAmountFloat = static_cast<f32>(limbSwingAmount);

    if (limbSwingAmountFloat > 0.2f) {
        f4 += static_cast<f32>(std::cos(limbSwingFloat * 0.4f) * 0.15f * limbSwingAmountFloat);
    }

    f32 f5 = m_grassEatingAmount;
    f32 f6 = m_rearingAmount;
    f32 f7 = 1.0f - f6;
    f32 f8 = m_mouthOpennessAngle;
    bool flag = m_tailCounter != 0;
    f32 f9 = static_cast<f32>(m_ticksExisted) + static_cast<f32>(partialTick);

    // 头部位置和旋转
    m_head->setRotationPointY(4.0f);
    m_head->setRotationPointZ(-12.0f);
    m_body->setRotateAngleX(0.0f);
    m_head->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 6.0) + f4);
    m_head->setRotateAngleY(f3 * static_cast<f32>(mc::math::PI_DOUBLE / 180.0));

    f32 f10 = m_inWater ? 0.2f : 1.0f;
    f32 f11 = static_cast<f32>(std::cos(f10 * limbSwingFloat * 0.6662f + static_cast<f64>(mc::math::PI_DOUBLE)));
    f32 f12 = f11 * 0.8f * limbSwingAmountFloat;
    f32 f13 = (1.0f - std::max(f6, f5)) *
        (static_cast<f32>(mc::math::PI_DOUBLE / 6.0) + f4 + f8 * static_cast<f32>(std::sin(f9)) * 0.05f);

    m_head->setRotateAngleX(f6 * (static_cast<f32>(mc::math::PI / 12.0f) + f4) +
        f5 * (static_cast<f32>(125.0f * mc::math::PI / 180.0) + static_cast<f32>(std::sin(f9)) * 0.05f) + f13);
    m_head->setRotateAngleY(
        f6 * f3 * static_cast<f32>(mc::math::PI_DOUBLE / 180.0) + (1.0f - std::max(f6, f5)) * m_head->rotateAngleY());
    m_head->setRotationPointY(f6 * -4.0f + f5 * 11.0f + (1.0f - std::max(f6, f5)) * m_head->rotationPointY());
    m_head->setRotationPointZ(f6 * -4.0f + f5 * -12.0f + (1.0f - std::max(f6, f5)) * m_head->rotationPointZ());
    m_body->setRotateAngleX(f6 * static_cast<f32>(-mc::math::PI_DOUBLE / 4.0) + f7 * m_body->rotateAngleX());

    f32 f14 = static_cast<f32>(mc::math::PI / 12.0f) * f6;
    f32 f15 = static_cast<f32>(std::cos(f9 * 0.6f + static_cast<f64>(mc::math::PI_DOUBLE)));

    // 前腿动画
    m_frontRightLeg->setRotationPointY(2.0f * f6 + 14.0f * f7);
    m_frontRightLeg->setRotationPointZ(-6.0f * f6 - 10.0f * f7);
    m_frontLeftLeg->setRotationPointY(m_frontRightLeg->rotationPointY());
    m_frontLeftLeg->setRotationPointZ(m_frontRightLeg->rotationPointZ());

    f32 f16 = (static_cast<f32>(-mc::math::PI_DOUBLE / 3.0) + f15) * f6 + f12 * f7;
    f32 f17 = (static_cast<f32>(-mc::math::PI_DOUBLE / 3.0) - f15) * f6 - f12 * f7;

    // 后腿动画
    m_backRightLeg->setRotateAngleX(f14 - f11 * 0.5f * limbSwingAmountFloat * f7);
    m_backLeftLeg->setRotateAngleX(f14 + f11 * 0.5f * limbSwingAmountFloat * f7);
    m_frontRightLeg->setRotateAngleX(f16);
    m_frontLeftLeg->setRotateAngleX(f17);

    // 尾巴动画
    m_tail->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 6.0) + limbSwingAmountFloat * 0.75f);
    m_tail->setRotationPointY(-5.0f + limbSwingAmountFloat);
    m_tail->setRotationPointZ(2.0f + limbSwingAmountFloat * 2.0f);
    if (flag) {
        m_tail->setRotateAngleY(static_cast<f32>(std::cos(f9 * 0.7f)));
    } else {
        m_tail->setRotateAngleY(0.0f);
    }

    // 幼体腿部同步
    m_backRightLegBaby->setRotationPointY(m_backRightLeg->rotationPointY());
    m_backRightLegBaby->setRotationPointZ(m_backRightLeg->rotationPointZ());
    m_backRightLegBaby->setRotateAngleX(m_backRightLeg->rotateAngleX());

    m_backLeftLegBaby->setRotationPointY(m_backLeftLeg->rotationPointY());
    m_backLeftLegBaby->setRotationPointZ(m_backLeftLeg->rotationPointZ());
    m_backLeftLegBaby->setRotateAngleX(m_backLeftLeg->rotateAngleX());

    m_frontRightLegBaby->setRotationPointY(m_frontRightLeg->rotationPointY());
    m_frontRightLegBaby->setRotationPointZ(m_frontRightLeg->rotationPointZ());
    m_frontRightLegBaby->setRotateAngleX(m_frontRightLeg->rotateAngleX());

    m_frontLeftLegBaby->setRotationPointY(m_frontLeftLeg->rotationPointY());
    m_frontLeftLegBaby->setRotationPointZ(m_frontLeftLeg->rotationPointZ());
    m_frontLeftLegBaby->setRotateAngleX(m_frontLeftLeg->rotateAngleX());

    // 幼体腿部可见性
    bool isChild = m_isChild;
    m_backRightLeg->setVisible(!isChild);
    m_backLeftLeg->setVisible(!isChild);
    m_frontRightLeg->setVisible(!isChild);
    m_frontLeftLeg->setVisible(!isChild);
    m_backRightLegBaby->setVisible(isChild);
    m_backLeftLegBaby->setVisible(isChild);
    m_frontRightLegBaby->setVisible(isChild);
    m_frontLeftLegBaby->setVisible(isChild);

    // 幼体时 Y 偏移
    if (isChild) {
        m_body->setRotationPointY(10.8f);
    } else {
        m_body->setRotationPointY(0.0f);
    }
}

void HorseModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 /*ageInTicks*/, f64 /*netHeadYaw*/, f64 /*headPitch*/, f64 /*scale*/)
{
    // 主要动画在 setLivingAnimations 中处理

    // 鞍部件可见性
    for (auto& part : m_saddleParts) {
        part->setVisible(m_saddled);
    }

    // 骑乘部件可见性（只有骑乘且有鞍时显示）
    for (auto& part : m_ridingParts) {
        part->setVisible(m_ridden && m_saddled);
    }
    // 注意：body.rotationPointY 在 setLivingAnimations 中根据是否幼体设置
}

} // namespace mc::client::renderer::entity::model::animal
