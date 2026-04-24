#include "MonsterVariantModels.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

namespace {
    constexpr f64 PI = 3.14159265359;
}

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

void ZombieVillagerModel::setupParts(f32 scale, bool slim) {
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
        m_head->addBox(-1.0f, -3.0f, -6.0f, 2.0f, 4.0f, 2.0f, scale);  // 鼻子

        m_villagerNose = std::make_shared<ModelRenderer>("villagerNose");
        m_villagerNose->setTextureOffset(30, 47);
        m_villagerNose->addBox(-8.0f, -8.0f, -6.0f, 16.0f, 16.0f, 1.0f, scale);
        m_villagerNose->setRotateAngleX(static_cast<f32>(-PI / 2.0));

        m_body->setTextureOffset(16, 20);
        m_body->addBox(-4.0f, 0.0f, -3.0f, 8.0f, 12.0f, 6.0f, scale);
        m_body->setTextureOffset(0, 38);
        m_body->addBox(-4.0f, 0.0f, -3.0f, 8.0f, 18.0f, 6.0f, scale + 0.05f);
    }
}

void ZombieVillagerModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                     f64 ageInTicks, f64 netHeadYaw,
                                     f64 headPitch, f64 scale) {
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);
}

void ZombieVillagerModel::setHeadVisible(bool visible) {
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

void DrownedModel::setupParts(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight) {
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

void DrownedModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                              f64 ageInTicks, f64 netHeadYaw,
                              f64 headPitch, f64 scale) {
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
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void CaveSpiderModel::setupParts() {
    // 参考 MC 1.16.5 SpiderModel
    // 洞穴蜘蛛比普通蜘蛛小

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(32, 4);
    m_head->addBox(-4.0f, -4.0f, -8.0f, 8.0f, 8.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 15.0f, -3.0f);
    m_parts.push_back(m_head);

    m_neck = std::make_shared<ModelRenderer>("neck");
    m_neck->setTextureOffset(0, 0);
    m_neck->addBox(-3.0f, -3.0f, -3.0f, 6.0f, 6.0f, 6.0f);
    m_neck->setRotationPoint(0.0f, 15.0f, 0.0f);
    m_parts.push_back(m_neck);

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 12);
    m_body->addBox(-5.0f, -4.0f, -6.0f, 10.0f, 8.0f, 12.0f);
    m_body->setRotationPoint(0.0f, 15.0f, 9.0f);
    m_parts.push_back(m_body);

    // 8条腿
    for (i32 i = 0; i < 8; ++i) {
        m_legs[i] = std::make_shared<ModelRenderer>("leg" + std::to_string(i));
        m_legs[i]->setTextureOffset(18, 0);
        m_legs[i]->addBox(-15.0f, -1.0f, -1.0f, 15.0f, 2.0f, 2.0f);

        f32 angle = static_cast<f32>(i * PI / 4.0);
        f32 x = static_cast<f32>(std::cos(angle) * 5.0);
        f32 z = static_cast<f32>(std::sin(angle) * 5.0);

        m_legs[i]->setRotationPoint(x, 15.0f, z);
        m_legs[i]->setRotateAngleY(angle);
        m_parts.push_back(m_legs[i]);
    }
}

void CaveSpiderModel::render(f64 scale) {
    EntityModel::render(scale);
}

void CaveSpiderModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                 f64 ageInTicks, f64 netHeadYaw,
                                 f64 headPitch, f64 scale) {
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    // 腿的动画
    for (i32 i = 0; i < 8; ++i) {
        f32 phase = static_cast<f32>(i * PI / 4.0);
        f32 legSwing = static_cast<f32>(std::cos(limbSwing * 0.6662 + phase) * limbSwingAmount * 0.3);
        m_legs[i]->setRotateAngleZ(legSwing);
    }

    (void)ageInTicks;
}

// ==================== GiantModel ====================

GiantModel::GiantModel()
    : BipedModel()
{
    // 巨人与僵尸模型相同，只是渲染时缩放更大
}

} // namespace mc::client::renderer::entity::model::monster
