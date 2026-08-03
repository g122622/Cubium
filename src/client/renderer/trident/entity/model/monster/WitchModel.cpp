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

#include "WitchModel.hpp"
#include "client/renderer/trident/entity/model/animal/VillagerModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <memory>

namespace mc::client::renderer::entity::model::monster {

WitchModel::WitchModel()
    : VillagerModel(0.0f, 64, 128) // 女巫纹理 64x128
{
    // ====== 隐藏村民默认帽子（女巫有自己的帽子） ======
    setHatVisible(false);

    // ====== 女巫帽子（分层结构，覆盖在头部上方） ======
    // 帽檐（hat）: 女巫标志性的宽帽檐
    m_witchHat = std::make_shared<ModelRenderer>("witchHat");
    m_witchHat->setTextureOffset(0, 64);
    m_witchHat->addBox(0.0f, 0.0f, 0.0f, 10.0f, 2.0f, 10.0f);
    m_witchHat->setRotationPoint(-5.0f, -10.03125f, -5.0f);
    m_head->addChild(m_witchHat);

    // 帽身下层（hat2）: 帽子的主体部分
    m_hat2 = std::make_shared<ModelRenderer>("hat2");
    m_hat2->setTextureOffset(0, 76);
    m_hat2->addBox(0.0f, 0.0f, 0.0f, 7.0f, 4.0f, 7.0f);
    m_hat2->setRotationPoint(1.75f, -4.0f, 2.0f);
    m_hat2->setRotateAngleX(-0.05235988f); // ~-3 度
    m_hat2->setRotateAngleZ(0.02617994f);  // ~1.5 度
    m_witchHat->addChild(m_hat2);

    // 帽身中层（hat3）: 帽子中段，倾斜度更大
    m_hat3 = std::make_shared<ModelRenderer>("hat3");
    m_hat3->setTextureOffset(0, 87);
    m_hat3->addBox(0.0f, 0.0f, 0.0f, 4.0f, 4.0f, 4.0f);
    m_hat3->setRotationPoint(1.75f, -4.0f, 2.0f);
    m_hat3->setRotateAngleX(-0.10471976f); // ~-6 度
    m_hat3->setRotateAngleZ(0.05235988f);  // ~3 度
    m_hat2->addChild(m_hat3);

    // 帽尖（hat4）: 帽子的尖端，带有微小的膨胀效果
    m_hat4 = std::make_shared<ModelRenderer>("hat4");
    m_hat4->setTextureOffset(0, 95);
    m_hat4->addBox(0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f, 0.25); // CubeDeformation(0.25)
    m_hat4->setRotationPoint(1.75f, -2.0f, 2.0f);
    m_hat4->setRotateAngleX(static_cast<f32>(-mc::math::PI / 15.0)); // -12 度
    m_hat4->setRotateAngleZ(0.10471976f);                            // ~6 度
    m_hat3->addChild(m_hat4);

    // ====== 鼻子上的痣（mole） ======
    m_mole = std::make_shared<ModelRenderer>("mole");
    m_mole->setTextureOffset(0, 0);
    m_mole->addBox(0.0f, 3.0f, -6.75f, 1.0f, 1.0f, 1.0f, -0.25); // CubeDeformation(-0.25)
    m_mole->setRotationPoint(0.0f, -2.0f, 0.0f);
    m_nose->addChild(m_mole);

    // 添加女巫特有部件到渲染列表
    m_parts.push_back(m_witchHat);
    m_parts.push_back(m_hat2);
    m_parts.push_back(m_hat3);
    m_parts.push_back(m_hat4);
    m_parts.push_back(m_mole);
}

void WitchModel::render(f64 scale)
{
    VillagerModel::render(scale);
}

void WitchModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 调用父类设置基本动画（头部旋转、腿部摆动等）
    VillagerModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // ====== 女巫特有的鼻子动画 ======
    // 鼻子根据 entityId 取模产生不同的摆动频率，使每个女巫的鼻子摆动略有不同
    // MC原版: frequency = 0.01F * (entityId % 10)，频率范围 [0.0, 0.09]
    const f32 noseFrequency = 0.01f * static_cast<f32>(m_entityId % 10);
    m_nose->setRotateAngleX(static_cast<f32>(std::sin(ageInTicks * noseFrequency) * 4.5 * (mc::math::PI / 180.0)));
    m_nose->setRotateAngleZ(static_cast<f32>(std::cos(ageInTicks * noseFrequency) * 2.5 * (mc::math::PI / 180.0)));

    // 当持有物品（喝药水）时，鼻子上扬并前伸
    if (m_holdingItem) {
        m_nose->setRotationPoint(0.0f, 1.0f, -1.5f);
        m_nose->setRotateAngleX(-0.9f);
    } else {
        m_nose->setRotationPoint(0.0f, -2.0f, 0.0f);
    }
}

} // namespace mc::client::renderer::entity::model::monster
