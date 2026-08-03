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

#include "PigModel.hpp"
#include "client/renderer/trident/entity/model/base/QuadrupedModel.hpp"
#include "common/core/Types.hpp"

namespace mc::client::renderer::entity::model::animal {

PigModel::PigModel()
    : QuadrupedModel(6, 0.0f, false, 4.0f, 4.0f, 2.0f, 2.0f, 24.0f)
{
    // 在基础四足模型上追加猪鼻子
    m_head->setTextureOffset(16, 16).addBox(-2.0f, 0.0f, -9.0f, 4.0f, 3.0f, 1.0f);

    setTextureSize(64, 32);
}

void PigModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 调用基类动画
    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    (void)scale;
    (void)ageInTicks;
}

} // namespace mc::client::renderer::entity::model::animal
