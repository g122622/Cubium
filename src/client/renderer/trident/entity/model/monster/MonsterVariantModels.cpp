#include "MonsterVariantModels.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

// ==================== ZombieVillagerModel ====================

ZombieVillagerModel::ZombieVillagerModel()
    : BipedModel()
{
    setupParts(0.0f, false);
}

ZombieVillagerModel::ZombieVillagerModel(f32 scale, bool slim)
    : BipedModel()
{
    setupParts(scale, slim);
}

void ZombieVillagerModel::setupParts(f32 scale, bool slim)
{
    // 参考 MC 1.16.5 ZombieVillagerModel
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
    setupParts(0.0f, 0.0f, 64, 64);
}

DrownedModel::DrownedModel(f32 scale, bool slim)
    : BipedModel()
{
    (void)slim;
    setupParts(scale, 0.0f, 64, 64);
}

void DrownedModel::setupParts(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight)
{
    // 参考 MC 1.16.5 DrownedModel
    (void)textureWidth;
    (void)textureHeight;

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

    // 溺尸游泳动画
    // 如果需要游泳动画，在这里添加
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
    // 参考 MC 1.16.5 CaveSpiderRenderer.preRenderCallback()
    // matrixStack.scale(0.7F, 0.7F, 0.7F);
    SpiderModel::render(scale * 0.7);
}

// ==================== GiantModel ====================

GiantModel::GiantModel()
    : BipedModel()
{
    // 巨人与僵尸模型相同，只是渲染时缩放更大
}

} // namespace mc::client::renderer::entity::model::monster
