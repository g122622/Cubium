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

#include "SheepModel.hpp"
#include "client/renderer/trident/entity/model/base/QuadrupedModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <memory>

namespace mc::client::renderer::entity::model::animal {

SheepModel::SheepModel()
    : QuadrupedModel(12, 0.0f, false, 8.0f, 4.0f, 2.0f, 2.0f, 24.0f)
{
    // 重建头、身
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-3.0f, -4.0f, -6.0f, 6.0f, 6.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 6.0f, -8.0f);

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(28, 8).addBox(-4.0f, -10.0f, -7.0f, 8.0f, 16.0f, 6.0f);
    m_body->setRotationPoint(0.0f, 5.0f, 2.0f);

    // 腿部使用 QuadrupedModel 默认位置（羊不需要调整腿部位置）
    // 羊腿高度 12，所以 Y = 24 - 12 = 12

    // 更新部件列表
    m_parts.clear();
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_legFrontRight);
    m_parts.push_back(m_legFrontLeft);
    m_parts.push_back(m_legBackRight);
    m_parts.push_back(m_legBackLeft);

    setTextureSize(64, 32);
}

void SheepModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 保存 headPitch 以便在吃草动画中使用
    m_headPitch = headPitch;

    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 计算吃草动画的头部姿态：head.xRot = headEatAngleScale
    const f32 headEatAngle = getHeadEatAngleScale(m_partialTick);
    m_head->setRotateAngleX(static_cast<f64>(headEatAngle));

    (void)scale;
    (void)ageInTicks;
}

void SheepModel::setLivingAnimations(f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 partialTick)
{
    m_partialTick = static_cast<f32>(partialTick);

    // head.y = head.y + headEatPositionScale * 9.0F * ageScale
    // 这里默认头部 Y 旋转点为 6.0，ageScale 默认为 1.0
    const f32 headEatPosScale = getHeadEatPositionScale(m_partialTick);
    m_head->setRotationPointY(6.0 + static_cast<f64>(headEatPosScale) * 9.0);
}

f32 SheepModel::getHeadEatPositionScale(f32 partialTick) const
{
    if (m_eatAnimationTimer <= 0) {
        return 0.0f;
    }
    if (m_eatAnimationTimer >= 4 && m_eatAnimationTimer <= 36) {
        return 1.0f;
    }
    if (m_eatAnimationTimer < 4) {
        // 头部逐渐低下的过渡阶段
        return (static_cast<f32>(m_eatAnimationTimer) - partialTick) / 4.0f;
    }
    // eatAnimationTimer > 36: 头部逐渐抬起的恢复阶段
    return -(static_cast<f32>(m_eatAnimationTimer) - EAT_ANIMATION_DURATION - partialTick) / 4.0f;
}

f32 SheepModel::getHeadEatAngleScale(f32 partialTick) const
{
    if (m_eatAnimationTimer > 4 && m_eatAnimationTimer <= 36) {
        // 头部保持低位并左右摆动
        const f32 f = (static_cast<f32>(m_eatAnimationTimer) - 4.0f - partialTick) / 32.0f;
        return static_cast<f32>(mc::math::PI / 5.0) + 0.21991149f * std::sin(static_cast<f64>(f) * 28.7);
    }
    if (m_eatAnimationTimer > 0) {
        // 头部低头但不摆动（过渡阶段）
        return static_cast<f32>(mc::math::PI / 5.0);
    }
    // 正常姿态：使用头部俯仰角
    return static_cast<f32>(m_headPitch * (mc::math::PI / 180.0));
}

} // namespace mc::client::renderer::entity::model::animal
