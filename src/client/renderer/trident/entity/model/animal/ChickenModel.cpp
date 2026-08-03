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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ChickenModel.hpp"
#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model::animal {

ChickenModel::ChickenModel()
    : AgeableModel()
{
    setTextureSize(64, 32);
    _setupParts();
}

void ChickenModel::_setupParts()
{
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-2.0f, -6.0f, -2.0f, 4.0f, 6.0f, 3.0f);
    m_head->setRotationPoint(0.0f, 15.0f, -4.0f);

    // 喙（bill）
    m_beak = std::make_shared<ModelRenderer>("beak");
    m_beak->setTextureOffset(14, 0);
    m_beak->addBox(-2.0f, -4.0f, -4.0f, 4.0f, 2.0f, 2.0f);
    m_beak->setRotationPoint(0.0f, 15.0f, -4.0f);

    // 肉垂（chin）
    m_wattle = std::make_shared<ModelRenderer>("wattle");
    m_wattle->setTextureOffset(14, 4);
    m_wattle->addBox(-1.0f, -2.0f, -3.0f, 2.0f, 2.0f, 2.0f);
    m_wattle->setRotationPoint(0.0f, 15.0f, -4.0f);

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 9);
    m_body->addBox(-3.0f, -4.0f, -3.0f, 6.0f, 8.0f, 6.0f);
    m_body->setRotationPoint(0.0f, 16.0f, 0.0f);

    // 右翼
    m_rightWing = std::make_shared<ModelRenderer>("rightWing");
    m_rightWing->setTextureOffset(24, 13);
    m_rightWing->addBox(0.0f, 0.0f, -3.0f, 1.0f, 4.0f, 6.0f);
    m_rightWing->setRotationPoint(-4.0f, 13.0f, 0.0f);

    // 左翼
    m_leftWing = std::make_shared<ModelRenderer>("leftWing");
    m_leftWing->setTextureOffset(24, 13);
    m_leftWing->addBox(-1.0f, 0.0f, -3.0f, 1.0f, 4.0f, 6.0f);
    m_leftWing->setRotationPoint(4.0f, 13.0f, 0.0f);

    // 右腿
    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_rightLeg->setTextureOffset(26, 0);
    m_rightLeg->addBox(-1.0f, 0.0f, -3.0f, 3.0f, 5.0f, 3.0f);
    m_rightLeg->setRotationPoint(-2.0f, 19.0f, 1.0f);

    // 左腿
    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");
    m_leftLeg->setTextureOffset(26, 0);
    m_leftLeg->addBox(-1.0f, 0.0f, -3.0f, 3.0f, 5.0f, 3.0f);
    m_leftLeg->setRotationPoint(1.0f, 19.0f, 1.0f);

    // 添加到部件列表 - 注意：AgeableModel 会通过 getHeadParts/getBodyParts 处理
    m_parts.push_back(m_head);
    m_parts.push_back(m_beak);
    m_parts.push_back(m_wattle);
    m_parts.push_back(m_body);
    m_parts.push_back(m_rightWing);
    m_parts.push_back(m_leftWing);
    m_parts.push_back(m_rightLeg);
    m_parts.push_back(m_leftLeg);
}

std::vector<std::shared_ptr<ModelRenderer>> ChickenModel::getHeadParts() const
{
    std::vector<std::shared_ptr<ModelRenderer>> parts;
    parts.push_back(m_head);
    parts.push_back(m_beak);
    parts.push_back(m_wattle);
    return parts;
}

std::vector<std::shared_ptr<ModelRenderer>> ChickenModel::getBodyParts() const
{
    std::vector<std::shared_ptr<ModelRenderer>> parts;
    parts.push_back(m_body);
    parts.push_back(m_rightWing);
    parts.push_back(m_leftWing);
    parts.push_back(m_rightLeg);
    parts.push_back(m_leftLeg);
    return parts;
}

void ChickenModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void ChickenModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 头部旋转
    m_head->setRotateAngleX(static_cast<f32>(mc::math::toRadians(static_cast<f32>(headPitch))));
    m_head->setRotateAngleY(static_cast<f32>(mc::math::toRadians(static_cast<f32>(netHeadYaw))));

    // 喙、肉垂跟随头部
    m_beak->setRotateAngleX(m_head->rotateAngleX());
    m_beak->setRotateAngleY(m_head->rotateAngleY());
    m_wattle->setRotateAngleX(m_head->rotateAngleX());
    m_wattle->setRotateAngleY(m_head->rotateAngleY());

    // 身体基础姿态（水平）
    m_body->setRotateAngleX(static_cast<f32>(mc::math::PI * 0.5));

    // 步态动画
    const f64 walkAngle = limbSwing * 0.6662f;
    const f64 walkAmount = limbSwingAmount * 1.4f;

    // 腿部动画
    m_rightLeg->setRotateAngleX(static_cast<f32>(std::cos(walkAngle) * walkAmount));
    m_leftLeg->setRotateAngleX(static_cast<f32>(std::cos(walkAngle + mc::math::PI) * walkAmount));

    // 翅膀动画（按年龄tick摆动）
    m_rightWing->setRotateAngleZ(static_cast<f32>(ageInTicks));
    m_leftWing->setRotateAngleZ(static_cast<f32>(-ageInTicks));

    (void)scale;
}

} // namespace mc::client::renderer::entity::model::animal
