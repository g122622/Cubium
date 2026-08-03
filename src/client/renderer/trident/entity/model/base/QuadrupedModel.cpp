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

#include "QuadrupedModel.hpp"
#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model {

QuadrupedModel::QuadrupedModel()
    : AgeableModel() // 使用 AgeableModel 默认构造函数
    , m_legHeight(6)
    , m_scale(0.0f)
{
    // 创建部件
    m_head = std::make_shared<ModelRenderer>("head");
    m_body = std::make_shared<ModelRenderer>("body");
    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");

    setupParts();

    // 添加到部件列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_legFrontRight);
    m_parts.push_back(m_legFrontLeft);
    m_parts.push_back(m_legBackRight);
    m_parts.push_back(m_legBackLeft);
}

QuadrupedModel::QuadrupedModel(i32 legHeight,
    f32 scale,
    bool isChildHeadScaled,
    f32 childHeadOffsetY,
    f32 childHeadOffsetZ,
    f32 childHeadScale,
    f32 childBodyScale,
    f32 childBodyOffsetY)
    : AgeableModel(
          isChildHeadScaled, childHeadOffsetY, childHeadOffsetZ, childHeadScale, childBodyScale, childBodyOffsetY)
    , m_legHeight(legHeight)
    , m_scale(scale)
{
    // 创建部件
    m_head = std::make_shared<ModelRenderer>("head");
    m_body = std::make_shared<ModelRenderer>("body");
    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");

    setupParts();

    // 添加到部件列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_legFrontRight);
    m_parts.push_back(m_legFrontLeft);
    m_parts.push_back(m_legBackRight);
    m_parts.push_back(m_legBackLeft);
}

void QuadrupedModel::setupParts()
{
    // 头部
    // 旋转点 Y = 18 - legHeight
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -4.0f, -8.0f, 8.0f, 8.0f, 8.0f, m_scale);
    m_head->setRotationPoint(0.0f, static_cast<f32>(18 - m_legHeight), -6.0f);

    // 身体
    // 旋转点 Y = 17 - legHeight
    m_body->setTextureOffset(28, 8);
    m_body->addBox(-5.0f, -10.0f, -7.0f, 10.0f, 16.0f, 8.0f, m_scale);
    m_body->setRotationPoint(0.0f, static_cast<f32>(17 - m_legHeight), 2.0f);

    // 后右腿
    // 旋转点 Y = 24 - legHeight
    m_legBackRight->setTextureOffset(0, 16);
    m_legBackRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, static_cast<f32>(m_legHeight), 4.0f, m_scale);
    m_legBackRight->setRotationPoint(-3.0f, static_cast<f32>(24 - m_legHeight), 7.0f);

    // 后左腿
    m_legBackLeft->setTextureOffset(0, 16);
    m_legBackLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, static_cast<f32>(m_legHeight), 4.0f, m_scale);
    m_legBackLeft->setRotationPoint(3.0f, static_cast<f32>(24 - m_legHeight), 7.0f);

    // 前右腿
    m_legFrontRight->setTextureOffset(0, 16);
    m_legFrontRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, static_cast<f32>(m_legHeight), 4.0f, m_scale);
    m_legFrontRight->setRotationPoint(-3.0f, static_cast<f32>(24 - m_legHeight), -5.0f);

    // 前左腿
    m_legFrontLeft->setTextureOffset(0, 16);
    m_legFrontLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, static_cast<f32>(m_legHeight), 4.0f, m_scale);
    m_legFrontLeft->setRotationPoint(3.0f, static_cast<f32>(24 - m_legHeight), -5.0f);
}

std::vector<std::shared_ptr<ModelRenderer>> QuadrupedModel::getHeadParts() const
{
    return {m_head};
}

std::vector<std::shared_ptr<ModelRenderer>> QuadrupedModel::getBodyParts() const
{
    return {m_body, m_legBackRight, m_legBackLeft, m_legFrontRight, m_legFrontLeft};
}

void QuadrupedModel::render(f64 scale)
{
    AgeableModel::render(scale);
}

void QuadrupedModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 /*ageInTicks*/, f64 netHeadYaw, f64 headPitch, f64 /*scale*/)
{
    // 头部旋转
    m_head->setRotateAngleX(static_cast<f32>(math::toRadians(static_cast<f32>(headPitch))));
    m_head->setRotateAngleY(static_cast<f32>(math::toRadians(static_cast<f32>(netHeadYaw))));

    // 身体默认姿态
    m_body->setRotateAngleX(math::HALF_PI);

    // 步态动画
    const f64 walkAngle = limbSwing * 0.6662;
    const f64 walkAmount = limbSwingAmount * 1.4;

    m_legBackRight->setRotateAngleX(static_cast<f32>(std::cos(walkAngle) * walkAmount));
    m_legBackLeft->setRotateAngleX(static_cast<f32>(std::cos(walkAngle + math::PI) * walkAmount));
    m_legFrontRight->setRotateAngleX(static_cast<f32>(std::cos(walkAngle + math::PI) * walkAmount));
    m_legFrontLeft->setRotateAngleX(static_cast<f32>(std::cos(walkAngle) * walkAmount));
}

} // namespace mc::client::renderer::entity::model
