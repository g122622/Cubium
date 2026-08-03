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
 * THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "PolarBearModel.hpp"
#include "client/renderer/trident/entity/model/base/QuadrupedModel.hpp"
#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model::animal {

PolarBearModel::PolarBearModel()
    : QuadrupedModel(12, 0.0f, true, 16.0f, 4.0f, 2.25f, 2.0f, 24.0f)
{
    m_textureWidth = 128;
    m_textureHeight = 64;

    setupParts();

    // 清空部件列表并重新添加（因为基类 setupParts() 已经添加过）
    m_parts.clear();
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_legFrontRight);
    m_parts.push_back(m_legFrontLeft);
    m_parts.push_back(m_legBackRight);
    m_parts.push_back(m_legBackLeft);
}

void PolarBearModel::setupParts()
{
    // 注意：鼻子、耳朵、下身体都是直接 addBox 到同一个 ModelRenderer 上，
    // 而不是作为子部件。

    // ==================== 头部 ====================
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureSize(m_textureWidth, m_textureHeight);
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-3.5f, -3.0f, -3.0f, 7.0f, 7.0f, 7.0f, 0.0f);
    m_head->setRotationPoint(0.0f, 10.0f, -16.0f);

    // 鼻子
    m_head->setTextureOffset(0, 44);
    m_head->addBox(-2.5f, 1.0f, -6.0f, 5.0f, 3.0f, 3.0f, 0.0f);

    // 左耳
    m_head->setTextureOffset(26, 0);
    m_head->addBox(-4.5f, -4.0f, -1.0f, 2.0f, 2.0f, 1.0f, 0.0f);

    // 右耳（镜像）
    m_head->setTextureOffset(26, 0);
    m_head->setMirror(true);
    m_head->addBox(2.5f, -4.0f, -1.0f, 2.0f, 2.0f, 1.0f, 0.0f);
    m_head->setMirror(false);

    // ==================== 身体 ====================
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureSize(m_textureWidth, m_textureHeight);
    m_body->setTextureOffset(0, 19);
    m_body->addBox(-5.0f, -13.0f, -7.0f, 14.0f, 14.0f, 11.0f, 0.0f);
    m_body->setTextureOffset(39, 0);
    m_body->addBox(-4.0f, -25.0f, -7.0f, 12.0f, 12.0f, 10.0f, 0.0f);
    m_body->setRotationPoint(-2.0f, 9.0f, 12.0f);

    // ==================== 后腿 ====================

    // 右后腿
    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureSize(m_textureWidth, m_textureHeight);
    m_legBackRight->setTextureOffset(50, 22);
    m_legBackRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 10.0f, 8.0f, 0.0f);
    m_legBackRight->setRotationPoint(-3.5f, 14.0f, 6.0f);

    // 左后腿
    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureSize(m_textureWidth, m_textureHeight);
    m_legBackLeft->setTextureOffset(50, 22);
    m_legBackLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 10.0f, 8.0f, 0.0f);
    m_legBackLeft->setRotationPoint(3.5f, 14.0f, 6.0f);

    // ==================== 前腿 ====================

    // 右前腿
    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureSize(m_textureWidth, m_textureHeight);
    m_legFrontRight->setTextureOffset(50, 40);
    m_legFrontRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 10.0f, 6.0f, 0.0f);
    m_legFrontRight->setRotationPoint(-2.5f, 14.0f, -7.0f);

    // 左前腿
    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureSize(m_textureWidth, m_textureHeight);
    m_legFrontLeft->setTextureOffset(50, 40);
    m_legFrontLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 10.0f, 6.0f, 0.0f);
    m_legFrontLeft->setRotationPoint(2.5f, 14.0f, -7.0f);

    // ==================== 位置调整 ====================
    m_legBackRight->setRotationPointX(m_legBackRight->rotationPointX() - 1.0);
    m_legBackLeft->setRotationPointX(m_legBackLeft->rotationPointX() + 1.0);
    m_legFrontRight->setRotationPointX(m_legFrontRight->rotationPointX() - 1.0);
    m_legFrontLeft->setRotationPointX(m_legFrontLeft->rotationPointX() + 1.0);
    m_legFrontRight->setRotationPointZ(m_legFrontRight->rotationPointZ() - 1.0);
    m_legFrontLeft->setRotationPointZ(m_legFrontLeft->rotationPointZ() - 1.0);
}

std::vector<std::shared_ptr<ModelRenderer>> PolarBearModel::getHeadParts() const
{
    return {m_head};
}

std::vector<std::shared_ptr<ModelRenderer>> PolarBearModel::getBodyParts() const
{
    return {m_body, m_legFrontRight, m_legFrontLeft, m_legBackRight, m_legBackLeft};
}

void PolarBearModel::render(f64 scale)
{
    AgeableModel::render(scale);
}

void PolarBearModel::setStandingProgress(f32 standingProgress)
{
    m_standingProgress = standingProgress;
}

void PolarBearModel::setLivingAnimations(f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 /*partialTick*/)
{
    // 站立动画在 setAngles 中处理，此处无需额外操作
}

void PolarBearModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 先调用基类方法设置基本动画
    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // ==================== 站立动画 ====================
    const f32 f1 = m_standingProgress * m_standingProgress; // 站立因子（平方）
    const f32 f2 = 1.0f - f1;                               // 反向因子

    // 身体动画
    m_body->setRotateAngleX(static_cast<f32>(math::PI / 2.0) - f1 * static_cast<f32>(math::PI) * 0.35f);
    m_body->setRotationPointY(9.0f * f2 + 11.0f * f1);

    // 右前腿动画
    m_legFrontRight->setRotationPointY(14.0f * f2 - 6.0f * f1);
    m_legFrontRight->setRotationPointZ(-8.0f * f2 - 4.0f * f1);
    m_legFrontRight->setRotateAngleX(m_legFrontRight->rotateAngleX() - f1 * static_cast<f32>(math::PI) * 0.45f);

    // 左前腿同步
    m_legFrontLeft->setRotationPointY(m_legFrontRight->rotationPointY());
    m_legFrontLeft->setRotationPointZ(m_legFrontRight->rotationPointZ());
    m_legFrontLeft->setRotateAngleX(m_legFrontLeft->rotateAngleX() - f1 * static_cast<f32>(math::PI) * 0.45f);

    // 头部动画（幼体和成年不同）
    if (m_isChild) {
        m_head->setRotationPointY(10.0f * f2 - 9.0f * f1);
        m_head->setRotationPointZ(-16.0f * f2 - 7.0f * f1);
    } else {
        m_head->setRotationPointY(10.0f * f2 - 14.0f * f1);
        m_head->setRotationPointZ(-16.0f * f2 - 3.0f * f1);
    }

    // 头部X旋转增加（抬头）
    m_head->setRotateAngleX(m_head->rotateAngleX() + f1 * static_cast<f32>(math::PI) * 0.15f);

    (void)scale;
    (void)ageInTicks;
}

} // namespace mc::client::renderer::entity::model::animal
