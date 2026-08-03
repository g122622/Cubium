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

#include "CowModel.hpp"
#include "client/renderer/trident/entity/model/base/QuadrupedModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <memory>

namespace mc::client::renderer::entity::model::animal {

CowModel::CowModel()
    : QuadrupedModel(12, 0.0f, false, 10.0f, 4.0f, 2.0f, 2.0f, 24.0f)
{
    // 重建头、身（腿在 QuadrupedModel 中已创建，需调整位置）
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->addBox(-4.0f, -4.0f, -6.0f, 8.0f, 8.0f, 6.0f);
    m_head->setRotationPoint(0.0f, 4.0f, -8.0f);
    // 牛角
    m_head->setTextureOffset(22, 0).addBox(-5.0f, -5.0f, -4.0f, 1.0f, 3.0f, 1.0f);
    m_head->setTextureOffset(22, 0).addBox(4.0f, -5.0f, -4.0f, 1.0f, 3.0f, 1.0f);

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(18, 4).addBox(-6.0f, -10.0f, -7.0f, 12.0f, 18.0f, 10.0f);
    m_body->setRotationPoint(0.0f, 5.0f, 2.0f);
    m_body->setTextureOffset(52, 0).addBox(-2.0f, 2.0f, -8.0f, 4.0f, 6.0f, 1.0f);

    // 腿部：QuadrupedModel 已创建，但需要调整位置；牛腿高度 12，所以 Y = 24 - 12 = 12
    m_legBackRight->setRotationPoint(-4.0f, 12.0f, 7.0f);
    m_legBackLeft->setRotationPoint(4.0f, 12.0f, 7.0f);
    m_legFrontRight->setRotationPoint(-4.0f, 12.0f, -6.0f);
    m_legFrontLeft->setRotationPoint(4.0f, 12.0f, -6.0f);

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

void CowModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    (void)scale;
    (void)ageInTicks;
}

} // namespace mc::client::renderer::entity::model::animal
