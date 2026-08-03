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

#include "WolfModel.hpp"
#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model::animal {

WolfModel::WolfModel()
    : AgeableModel()
{
    setTextureSize(64, 32);

    const f32 f = 0.0f; // 缩放值
    const f32 f1 = 13.5f;

    // 头部
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setRotationPoint(-1.0f, f1, -7.0f);

    // 头部子部件
    m_headChild = std::make_shared<ModelRenderer>("headChild");
    m_headChild->setTextureOffset(0, 0);
    m_headChild->addBox(-2.0f, -3.0f, -2.0f, 6.0f, 6.0f, 4.0f, static_cast<f64>(f));
    m_head->addChild(m_headChild);

    // 左耳
    auto earLeft = m_headChild->createChild("earLeft");
    earLeft->setTextureOffset(16, 14);
    earLeft->addBox(-2.0f, -5.0f, 0.0f, 2.0f, 2.0f, 1.0f, static_cast<f64>(f));

    // 右耳
    auto earRight = m_headChild->createChild("earRight");
    earRight->setTextureOffset(16, 14);
    earRight->addBox(2.0f, -5.0f, 0.0f, 2.0f, 2.0f, 1.0f, static_cast<f64>(f));

    // 鼻子
    auto nose = m_headChild->createChild("nose");
    nose->setTextureOffset(0, 10);
    nose->addBox(-0.5f, 0.0f, -5.0f, 3.0f, 3.0f, 4.0f, static_cast<f64>(f));

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(18, 14);
    m_body->addBox(-3.0f, -2.0f, -3.0f, 6.0f, 9.0f, 6.0f, static_cast<f64>(f));
    m_body->setRotationPoint(0.0f, 14.0f, 2.0f);

    // 鬃毛
    m_mane = std::make_shared<ModelRenderer>("mane");
    m_mane->setTextureOffset(21, 0);
    m_mane->addBox(-3.0f, -3.0f, -3.0f, 8.0f, 6.0f, 7.0f, static_cast<f64>(f));
    m_mane->setRotationPoint(-1.0f, 14.0f, 2.0f);

    // 后右腿
    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureOffset(0, 18);
    m_legBackRight->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, static_cast<f64>(f));
    m_legBackRight->setRotationPoint(-2.5f, 16.0f, 7.0f);

    // 后左腿
    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureOffset(0, 18);
    m_legBackLeft->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, static_cast<f64>(f));
    m_legBackLeft->setRotationPoint(0.5f, 16.0f, 7.0f);

    // 前右腿
    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureOffset(0, 18);
    m_legFrontRight->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, static_cast<f64>(f));
    m_legFrontRight->setRotationPoint(-2.5f, 16.0f, -4.0f);

    // 前左腿
    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureOffset(0, 18);
    m_legFrontLeft->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, static_cast<f64>(f));
    m_legFrontLeft->setRotationPoint(0.5f, 16.0f, -4.0f);

    // 尾巴
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setRotationPoint(-1.0f, 12.0f, 8.0f);

    // 尾巴子部件
    m_tailChild = std::make_shared<ModelRenderer>("tailChild");
    m_tailChild->setTextureOffset(9, 18);
    m_tailChild->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, static_cast<f64>(f));
    m_tail->addChild(m_tailChild);

    // 添加所有部件到列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_mane);
    m_parts.push_back(m_legBackRight);
    m_parts.push_back(m_legBackLeft);
    m_parts.push_back(m_legFrontRight);
    m_parts.push_back(m_legFrontLeft);
    m_parts.push_back(m_tail);
}

std::vector<std::shared_ptr<ModelRenderer>> WolfModel::getHeadParts() const
{
    return {m_head};
}

std::vector<std::shared_ptr<ModelRenderer>> WolfModel::getBodyParts() const
{
    return {m_body, m_legBackRight, m_legBackLeft, m_legFrontRight, m_legFrontLeft, m_tail, m_mane};
}

void WolfModel::render(f64 scale)
{
    AgeableModel::render(scale);
}

void WolfModel::setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 /*partialTick*/)
{
    // 愤怒状态：尾巴不摇
    if (m_isAngry) {
        m_tail->setRotateAngleY(0.0f);
    } else {
        m_tail->setRotateAngleY(static_cast<f32>(std::cos(limbSwing * 0.6662) * 1.4 * limbSwingAmount));
    }

    if (m_isSitting) {
        // 坐下姿态：身体、鬃毛、尾巴和腿的位置
        m_mane->setRotationPoint(-1.0f, 16.0f, -3.0f);
        m_mane->setRotateAngleX(mc::math::TWO_PI / 5.0f); // 约 72 度
        m_mane->setRotateAngleY(0.0f);
        m_body->setRotationPoint(0.0f, 18.0f, 0.0f);
        m_body->setRotateAngleX(mc::math::QUARTER_PI); // 45 度
        m_tail->setRotationPoint(-1.0f, 21.0f, 6.0f);
        m_legBackRight->setRotationPoint(-2.5f, 22.7f, 2.0f);
        m_legBackRight->setRotateAngleX(mc::math::HALF_PI * 3.0f); // 270 度
        m_legBackLeft->setRotationPoint(0.5f, 22.7f, 2.0f);
        m_legBackLeft->setRotateAngleX(mc::math::HALF_PI * 3.0f);
        m_legFrontRight->setRotateAngleX(5.811947f); // 约 333 度
        m_legFrontRight->setRotationPoint(-2.49f, 17.0f, -4.0f);
        m_legFrontLeft->setRotateAngleX(5.811947f);
        m_legFrontLeft->setRotationPoint(0.51f, 17.0f, -4.0f);
    } else {
        // 正常站立姿态
        m_body->setRotationPoint(0.0f, 14.0f, 2.0f);
        m_body->setRotateAngleX(mc::math::HALF_PI); // 90 度
        m_mane->setRotationPoint(-1.0f, 14.0f, -3.0f);
        m_mane->setRotateAngleX(m_body->rotateAngleX());
        m_tail->setRotationPoint(-1.0f, 12.0f, 8.0f);
        m_legBackRight->setRotationPoint(-2.5f, 16.0f, 7.0f);
        m_legBackLeft->setRotationPoint(0.5f, 16.0f, 7.0f);
        m_legFrontRight->setRotationPoint(-2.5f, 16.0f, -4.0f);
        m_legFrontLeft->setRotationPoint(0.5f, 16.0f, -4.0f);

        // 腿部步态动画
        const f32 limbSwingFloat = static_cast<f32>(limbSwing);
        const f32 limbSwingAmountFloat = static_cast<f32>(limbSwingAmount);
        m_legBackRight->setRotateAngleX(
            static_cast<f32>(std::cos(limbSwingFloat * 0.6662) * 1.4 * limbSwingAmountFloat));
        m_legBackLeft->setRotateAngleX(
            static_cast<f32>(std::cos(limbSwingFloat * 0.6662 + mc::math::PI_DOUBLE) * 1.4 * limbSwingAmountFloat));
        m_legFrontRight->setRotateAngleX(
            static_cast<f32>(std::cos(limbSwingFloat * 0.6662 + mc::math::PI_DOUBLE) * 1.4 * limbSwingAmountFloat));
        m_legFrontLeft->setRotateAngleX(
            static_cast<f32>(std::cos(limbSwingFloat * 0.6662) * 1.4 * limbSwingAmountFloat));
    }

    // 摇晃动画（湿状态抖水）
    // 对应 MC 1.21.11 WolfModel.setupAnim() 第 132-135 行：
    //   realHead.zRot = headRollAngle + getBodyRollAngle(0.0F);
    //   upperBody.zRot = getBodyRollAngle(-0.08F);
    //   body.zRot = getBodyRollAngle(-0.16F);
    //   realTail.zRot = getBodyRollAngle(-0.2F);
    m_headChild->setRotateAngleZ(m_interestedAngle + _getBodyRollAngle(0.0f));
    m_mane->setRotateAngleZ(_getBodyRollAngle(-0.08f));
    m_body->setRotateAngleZ(_getBodyRollAngle(-0.16f));
    m_tailChild->setRotateAngleZ(_getBodyRollAngle(-0.2f));
}

void WolfModel::setAngles(
    f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 /*scale*/)
{
    // 大部分动画在 setLivingAnimations 中处理

    // 头部旋转
    m_head->setRotateAngleX(mc::math::toRadians(static_cast<f32>(headPitch)));
    m_head->setRotateAngleY(mc::math::toRadians(static_cast<f32>(netHeadYaw)));

    // 尾巴角度（根据狼的驯服状态和生命值计算，由 WolfRenderer 传入）
    m_tail->setRotateAngleX(m_tailRotation);
}

void WolfModel::setAnimState(
    bool isSitting, bool isAngry, bool isWet, f32 tailRotation, f32 shakeAnim, f32 interestedAngle)
{
    m_isSitting = isSitting;
    m_isAngry = isAngry;
    m_isWet = isWet;
    m_tailRotation = tailRotation;
    m_shakeAnim = shakeAnim;
    m_interestedAngle = interestedAngle;
}

f32 WolfModel::_getBodyRollAngle(f32 offset) const
{
    // 对应 MC 1.21.11 WolfRenderState.getBodyRollAngle():
    //   f = clamp((shakeAnim + offset) / 1.8F, 0, 1)
    //   return Mth.sin(f * PI) * Mth.sin(f * PI * 11) * 0.15F * PI
    f32 f = (m_shakeAnim + offset) / 1.8f;
    if (f < 0.0f) {
        f = 0.0f;
    } else if (f > 1.0f) {
        f = 1.0f;
    }
    return std::sin(f * math::PI) * std::sin(f * math::PI * 11.0f) * 0.15f * math::PI;
}

} // namespace mc::client::renderer::entity::model::animal
