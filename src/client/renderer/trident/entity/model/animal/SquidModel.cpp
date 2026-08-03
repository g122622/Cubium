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

#include "SquidModel.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <memory>
#include <string>

namespace mc::client::renderer::entity::model::animal {

namespace {
/// 触手数量
constexpr i32 TENTACLE_COUNT = 8;
} // namespace

SquidModel::SquidModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    _setupParts();
}

void SquidModel::_setupParts()
{
    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-6.0f, -8.0f, -6.0f, 12.0f, 16.0f, 12.0f);
    m_body->setRotationPoint(0.0f, 8.0f, 0.0f);
    m_parts.push_back(m_body);

    // 触手
    for (i32 i = 0; i < TENTACLE_COUNT; ++i) {
        m_tentacles[i] = std::make_shared<ModelRenderer>("tentacle" + std::to_string(i));
        m_tentacles[i]->setTextureOffset(48, 0);

        f64 d0 = static_cast<f64>(i) * mc::math::PI * 2.0 / static_cast<f64>(TENTACLE_COUNT);
        f32 x = static_cast<f32>(std::cos(d0) * 5.0);
        f32 z = static_cast<f32>(std::sin(d0) * 5.0);

        m_tentacles[i]->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 18.0f, 2.0f);
        m_tentacles[i]->setRotationPoint(x, 15.0f, z);

        f64 rotateY =
            static_cast<f64>(i) * mc::math::PI * -2.0 / static_cast<f64>(TENTACLE_COUNT) + (mc::math::PI / 2.0);
        m_tentacles[i]->setRotateAngleY(static_cast<f32>(rotateY));

        m_parts.push_back(m_tentacles[i]);
    }
}

void SquidModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void SquidModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 触手动画直接使用 ageInTicks 作为 X 旋转角度
    for (i32 i = 0; i < TENTACLE_COUNT; ++i) {
        m_tentacles[i]->setRotateAngleX(static_cast<f32>(ageInTicks));
    }

    // 身体不旋转
    MC_UNUSED(limbSwing);
    MC_UNUSED(limbSwingAmount);
    MC_UNUSED(netHeadYaw);
    MC_UNUSED(headPitch);
    MC_UNUSED(scale);
}

} // namespace mc::client::renderer::entity::model::animal
