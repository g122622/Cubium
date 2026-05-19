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

#include "BlazeModel.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

namespace {
// 烟雾棒的旋转偏移（每根棒间隔30度）
constexpr f64 ROD_ANGLE_OFFSET = mc::math::PI_DOUBLE / 6.0;
// 棒的浮动偏移角度增量
constexpr f64 ROD_FLOAT_SPEED = 0.5;
} // namespace

BlazeModel::BlazeModel()
{
    setTextureSize(64, 64);
    setupParts();
}

void BlazeModel::setupParts()
{
    // 参考 MC 1.16.5 BlazeModel
    // 头部（主体）- Java原版没有调用setRotationPoint，默认是(0,0,0)
    m_head = std::make_shared<model::ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -4.0f, -4.0f, 8.0f, 8.0f, 8.0f, 0.0);
    // Java原版不设置rotationPoint，默认为(0,0,0)
    m_parts.push_back(m_head);

    // 烟雾棒（12根）
    // 参考 MC 1.16.5：棒围绕头部排列，随机浮动
    // 棒尺寸：2x8x2（MC 1.16.5 使用 addBox(0.0F, 0.0F, 0.0F, 2.0F, 8.0F, 2.0F)）
    for (i32 i = 0; i < SMOKE_ROD_COUNT; ++i) {
        auto& rod = m_smokeRods[i];
        rod = std::make_shared<model::ModelRenderer>("smokeRod" + std::to_string(i));

        // 棒尺寸：2x8x2 (MC 1.16.5)
        rod->setTextureOffset(0, 16);
        rod->addBox(0.0f, 0.0f, 0.0f, 2.0f, 8.0f, 2.0f, 0.0);

        // 初始位置在头部周围（setAngles 会动态更新）
        rod->setRotationPoint(0.0f, 0.0f, 0.0f);
        m_parts.push_back(rod);
    }
}

void BlazeModel::render(f64 scale)
{
    // 渲染头部
    if (m_head) {
        m_head->render(scale);
    }

    // 渲染烟雾棒
    for (auto& rod : m_smokeRods) {
        if (rod) {
            rod->render(scale);
        }
    }
}

void BlazeModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 /*scale*/)
{
    m_ageInTicks = ageInTicks;
    (void)limbSwing;
    (void)limbSwingAmount;

    // 参考 MC 1.16.5 BlazeModel.setRotationAngles
    // 头部旋转
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0));

    // 烟雾棒动画 - 分三层，每层有不同的半径和Y偏移
    // 参考 MC 1.16.5:
    // 第1层 (0-3): 半径9，Y=-2 + cos((i*2 + ageInTicks) * 0.25)
    // 第2层 (4-7): 半径7，Y=2 + cos((i*2 + ageInTicks) * 0.25)
    // 第3层 (8-11): 半径5，Y=11 + cos((i*1.5 + ageInTicks) * 0.5)

    // 第一层：棒 0-3
    f64 f = ageInTicks * mc::math::PI_DOUBLE * -0.1;
    for (i32 i = 0; i < 4; ++i) {
        auto& rod = m_smokeRods[i];
        if (!rod) continue;

        // Y坐标：-2 + cos((i*2 + ageInTicks) * 0.25)
        f32 y = -2.0f + static_cast<f32>(std::cos((i * 2.0 + ageInTicks) * 0.25));
        rod->setRotationPointY(y);

        // X/Z 坐标围绕头部旋转
        f32 x = static_cast<f32>(std::cos(f) * 9.0);
        f32 z = static_cast<f32>(std::sin(f) * 9.0);
        rod->setRotationPointX(x);
        rod->setRotationPointZ(z);

        ++f;
    }

    // 第二层：棒 4-7
    f = mc::math::PI_DOUBLE / 4.0 + ageInTicks * mc::math::PI_DOUBLE * 0.03;
    for (i32 i = 4; i < 8; ++i) {
        auto& rod = m_smokeRods[i];
        if (!rod) continue;

        // Y坐标：2 + cos((i*2 + ageInTicks) * 0.25)
        f32 y = 2.0f + static_cast<f32>(std::cos((i * 2.0 + ageInTicks) * 0.25));
        rod->setRotationPointY(y);

        // X/Z 坐标围绕头部旋转
        f32 x = static_cast<f32>(std::cos(f) * 7.0);
        f32 z = static_cast<f32>(std::sin(f) * 7.0);
        rod->setRotationPointX(x);
        rod->setRotationPointZ(z);

        ++f;
    }

    // 第三层：棒 8-11
    f = 0.47123894 + ageInTicks * mc::math::PI_DOUBLE * -0.05;
    for (i32 i = 8; i < 12; ++i) {
        auto& rod = m_smokeRods[i];
        if (!rod) continue;

        // Y坐标：11 + cos((i*1.5 + ageInTicks) * 0.5)
        f32 y = 11.0f + static_cast<f32>(std::cos((i * 1.5 + ageInTicks) * 0.5));
        rod->setRotationPointY(y);

        // X/Z 坐标围绕头部旋转
        f32 x = static_cast<f32>(std::cos(f) * 5.0);
        f32 z = static_cast<f32>(std::sin(f) * 5.0);
        rod->setRotationPointX(x);
        rod->setRotationPointZ(z);

        ++f;
    }
}

} // namespace mc::client::renderer::entity::model::monster
