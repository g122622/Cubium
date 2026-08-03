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

#include "VillagerModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <memory>

namespace mc::client::renderer::entity::model::animal {

VillagerModel::VillagerModel(f32 scale)
    : VillagerModel(scale, 64, 64)
{}

VillagerModel::VillagerModel(f32 scale, i32 textureWidth, i32 textureHeight)
{
    setTextureSize(textureWidth, textureHeight);

    // 头部
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -10.0f, -4.0f, 8.0f, 10.0f, 8.0f, static_cast<f64>(scale));
    m_head->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 帽子（覆盖在头上）
    m_hat = std::make_shared<ModelRenderer>("hat");
    m_hat->setTextureOffset(32, 0);
    m_hat->addBox(-4.0f, -10.0f, -4.0f, 8.0f, 10.0f, 8.0f, static_cast<f64>(scale + 0.5f));
    m_hat->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_head->addChild(m_hat);

    // 帽檐
    m_hatBrim = std::make_shared<ModelRenderer>("hatBrim");
    m_hatBrim->setTextureOffset(30, 47);
    m_hatBrim->addBox(-8.0f, -8.0f, -6.0f, 16.0f, 16.0f, 1.0f, static_cast<f64>(scale));
    m_hatBrim->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_hatBrim->setRotateAngleX(-mc::math::HALF_PI);
    m_hat->addChild(m_hatBrim);

    // 鼻子
    m_nose = std::make_shared<ModelRenderer>("nose");
    m_nose->setTextureOffset(24, 0);
    m_nose->addBox(-1.0f, -1.0f, -6.0f, 2.0f, 4.0f, 2.0f, static_cast<f64>(scale));
    m_nose->setRotationPoint(0.0f, -2.0f, 0.0f);
    m_head->addChild(m_nose);

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(16, 20);
    m_body->addBox(-4.0f, 0.0f, -3.0f, 8.0f, 12.0f, 6.0f, static_cast<f64>(scale));
    m_body->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 衣服（覆盖在身体上）
    m_clothing = std::make_shared<ModelRenderer>("clothing");
    m_clothing->setTextureOffset(0, 38);
    m_clothing->addBox(-4.0f, 0.0f, -3.0f, 8.0f, 18.0f, 6.0f, static_cast<f64>(scale + 0.5f));
    m_clothing->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_body->addChild(m_clothing);

    // 手臂（交叉在胸前）
    m_arms = std::make_shared<ModelRenderer>("arms");
    m_arms->setTextureOffset(44, 22);
    m_arms->addBox(-8.0f, -2.0f, -2.0f, 4.0f, 8.0f, 4.0f, static_cast<f64>(scale)); // 左臂
    // 右臂需要 mirror=true
    m_arms->setTextureOffset(44, 22);
    m_arms->addBox(4.0f, -2.0f, -2.0f, 4.0f, 8.0f, 4.0f, true, static_cast<f64>(scale)); // 右臂，mirror=true
    m_arms->setTextureOffset(40, 38);
    m_arms->addBox(-4.0f, 2.0f, -2.0f, 8.0f, 4.0f, 4.0f, static_cast<f64>(scale)); // 连接部分
    m_arms->setRotationPoint(0.0f, 2.0f, 0.0f);

    // 右腿
    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_rightLeg->setTextureOffset(0, 22);
    m_rightLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, static_cast<f64>(scale));
    m_rightLeg->setRotationPoint(-2.0f, 12.0f, 0.0f);

    // 左腿 - 需要 mirror=true
    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");
    m_leftLeg->setTextureOffset(0, 22);
    m_leftLeg->setMirror(true);
    m_leftLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, static_cast<f64>(scale));
    m_leftLeg->setRotationPoint(2.0f, 12.0f, 0.0f);

    // 添加所有部件到列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_rightLeg);
    m_parts.push_back(m_leftLeg);
    m_parts.push_back(m_arms);
}

void VillagerModel::render(f64 scale)
{
    for (auto& part : m_parts) {
        part->render(scale);
    }
}

void VillagerModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 /*scale*/)
{
    // 头部旋转
    m_head->setRotateAngleY(mc::math::toRadians(static_cast<f32>(netHeadYaw)));
    m_head->setRotateAngleX(mc::math::toRadians(static_cast<f32>(headPitch)));

    // 摇头动画（交易不满意时）
    if (m_shakingHead) {
        m_head->setRotateAngleZ(static_cast<f32>(0.3 * std::sin(0.45 * ageInTicks)));
        m_head->setRotateAngleX(0.4f);
    } else {
        m_head->setRotateAngleZ(0.0f);
    }

    // 手臂位置
    m_arms->setRotationPointY(3.0f);
    m_arms->setRotationPointZ(-1.0f);
    m_arms->setRotateAngleX(-0.75f);

    // 腿部动画
    m_rightLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662) * 1.4 * limbSwingAmount * 0.5));
    m_leftLeg->setRotateAngleX(
        static_cast<f32>(std::cos(limbSwing * 0.6662 + mc::math::PI_DOUBLE) * 1.4 * limbSwingAmount * 0.5));
    m_rightLeg->setRotateAngleY(0.0f);
    m_leftLeg->setRotateAngleY(0.0f);
}

} // namespace mc::client::renderer::entity::model::animal
