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

#include "MonsterVariantModels.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathConstants.hpp"

namespace mc::client::renderer::entity::model::monster {

// ==================== ZombieVillagerModel ====================

ZombieVillagerModel::ZombieVillagerModel()
    : BipedModel()
{
    _setupParts(0.0f, false);
}

ZombieVillagerModel::ZombieVillagerModel(f32 scale, bool slim)
    : BipedModel()
{
    _setupParts(scale, slim);
}

void ZombieVillagerModel::_setupParts(f32 scale, bool slim)
{
    if (slim) {
        m_head->setTextureOffset(0, 0);
        m_head->addBox(-4.0f, -10.0f, -4.0f, 8.0f, 8.0f, 8.0f, scale);
        m_body->setTextureOffset(16, 16);
        m_body->addBox(-4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f, scale + 0.1f);
    } else {
        m_head->setTextureOffset(0, 0);
        m_head->addBox(-4.0f, -10.0f, -4.0f, 8.0f, 10.0f, 8.0f, scale);
        m_head->setTextureOffset(24, 0);
        m_head->addBox(-1.0f, -3.0f, -6.0f, 2.0f, 4.0f, 2.0f, scale); // 鼻子

        m_villagerNose = std::make_shared<ModelRenderer>("villagerNose");
        m_villagerNose->setTextureOffset(30, 47);
        m_villagerNose->addBox(-8.0f, -8.0f, -6.0f, 16.0f, 16.0f, 1.0f, scale);
        m_villagerNose->setRotateAngleX(static_cast<f32>(-mc::math::PI / 2.0));

        m_body->setTextureOffset(16, 20);
        m_body->addBox(-4.0f, 0.0f, -3.0f, 8.0f, 12.0f, 6.0f, scale);
        m_body->setTextureOffset(0, 38);
        m_body->addBox(-4.0f, 0.0f, -3.0f, 8.0f, 18.0f, 6.0f, scale + 0.05f);
    }
}

void ZombieVillagerModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);
}

void ZombieVillagerModel::setHeadVisible(bool visible)
{
    m_head->setVisible(visible);
    m_headwear->setVisible(visible);
    if (m_villagerNose) {
        m_villagerNose->setVisible(visible);
    }
}

// ==================== DrownedModel ====================

DrownedModel::DrownedModel()
    : BipedModel()
{
    _setupParts(0.0f, 0.0f, 64, 64);
}

DrownedModel::DrownedModel(f32 scale, bool slim)
    : BipedModel()
{
    MC_UNUSED(slim);
    _setupParts(scale, 0.0f, 64, 64);
}

void DrownedModel::_setupParts(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight)
{
    MC_UNUSED(textureWidth);
    MC_UNUSED(textureHeight);

    // 溺尸有特殊的手臂和腿
    m_rightArm->setTextureOffset(32, 48);
    m_rightArm->addBox(-3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, scale);
    m_rightArm->setRotationPoint(-5.0f, 2.0f + yOffset, 0.0f);

    m_rightLeg->setTextureOffset(16, 48);
    m_rightLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, scale);
    m_rightLeg->setRotationPoint(-1.9f, 12.0f + yOffset, 0.0f);
}

void DrownedModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // TODO: 溺尸游泳动画未实现
}

// ==================== StrayModel ====================

StrayModel::StrayModel()
    : BipedModel()
{
    // 流浪者与骷髅结构相同，只是纹理不同
}

// ==================== HuskModel ====================

HuskModel::HuskModel()
    : BipedModel()
{
    // 尸壳与僵尸结构相同，只是纹理不同
}

// ==================== CaveSpiderModel ====================

CaveSpiderModel::CaveSpiderModel()
    : SpiderModel() // 继承 SpiderModel，模型结构完全相同
{
    // 洞穴蜘蛛模型与普通蜘蛛完全相同
    // 区别在于渲染时缩放 0.7 倍（在 render() 中处理）
}

void CaveSpiderModel::render(f64 scale)
{
    SpiderModel::render(scale * getScaleFactor());
}

// ==================== GiantModel ====================

GiantModel::GiantModel()
    : BipedModel()
{
    // 巨人与僵尸模型相同，只是渲染时缩放更大
}

} // namespace mc::client::renderer::entity::model::monster
