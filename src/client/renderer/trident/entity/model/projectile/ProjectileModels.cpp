#include "ProjectileModels.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::projectile {

namespace {
    constexpr f64 PI = 3.14159265359;
}

// ==================== ShulkerBulletModel ====================

ShulkerBulletModel::ShulkerBulletModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void ShulkerBulletModel::setupParts() {
    // 参考 MC 1.16.5 ShulkerBulletModel
    m_bullet = std::make_shared<ModelRenderer>("bullet");
    m_bullet->setTextureOffset(0, 0);
    m_bullet->addBox(-4.0f, -4.0f, -1.0f, 8.0f, 8.0f, 2.0f);
    m_bullet->setTextureOffset(0, 10);
    m_bullet->addBox(-1.0f, -4.0f, -4.0f, 2.0f, 8.0f, 8.0f);
    m_bullet->setTextureOffset(20, 0);
    m_bullet->addBox(-4.0f, -1.0f, -4.0f, 8.0f, 2.0f, 8.0f);
    m_bullet->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_bullet);
}

void ShulkerBulletModel::render(f64 scale) {
    EntityModel::render(scale);
}

void ShulkerBulletModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                    f64 ageInTicks, f64 netHeadYaw,
                                    f64 headPitch, f64 scale) {
    m_bullet->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_bullet->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)ageInTicks;
    (void)scale;
}

// ==================== LlamaSpitModel ====================

LlamaSpitModel::LlamaSpitModel()
    : EntityModel()
{
    setTextureSize(8, 8);
    setupParts(0.0f);
}

LlamaSpitModel::LlamaSpitModel(f32 scale)
    : EntityModel()
{
    setTextureSize(8, 8);
    setupParts(scale);
}

void LlamaSpitModel::setupParts(f32 scale) {
    // 参考 MC 1.16.5 LlamaSpitModel
    m_main = std::make_shared<ModelRenderer>("main");
    m_main->setTextureOffset(0, 0);
    // 创建一个小的立方体
    m_main->addBox(-1.0f, -1.0f, -1.0f, 2.0f, 2.0f, 2.0f, scale);
    m_main->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_main);
}

void LlamaSpitModel::render(f64 scale) {
    EntityModel::render(scale);
}

void LlamaSpitModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                f64 ageInTicks, f64 netHeadYaw,
                                f64 headPitch, f64 scale) {
    // 没有动画
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== EnderCrystalModel ====================

EnderCrystalModel::EnderCrystalModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts(0.0f);
}

EnderCrystalModel::EnderCrystalModel(f32 scale)
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts(scale);
}

void EnderCrystalModel::setupParts(f32 scale) {
    // 参考 MC 1.16.5 EnderCrystalModel
    // 核心立方体
    m_cube = std::make_shared<ModelRenderer>("cube");
    m_cube->setTextureOffset(32, 0);
    m_cube->addBox(-4.0f, -4.0f, -4.0f, 8.0f, 8.0f, 8.0f, scale);
    m_cube->setRotationPoint(0.0f, 8.0f, 0.0f);
    m_parts.push_back(m_cube);

    // 玻璃外壳
    m_glass = std::make_shared<ModelRenderer>("glass");
    m_glass->setTextureOffset(0, 0);
    m_glass->addBox(-4.0f, -4.0f, -4.0f, 8.0f, 8.0f, 8.0f, scale);
    m_glass->setRotationPoint(0.0f, 8.0f, 0.0f);
    m_parts.push_back(m_glass);

    // 基座
    m_base = std::make_shared<ModelRenderer>("base");
    m_base->setTextureOffset(0, 16);
    m_base->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 4.0f, 4.0f, scale);
    m_base->setRotationPoint(0.0f, 8.0f, 0.0f);
    m_parts.push_back(m_base);
}

void EnderCrystalModel::render(f64 scale) {
    EntityModel::render(scale);
}

void EnderCrystalModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                   f64 ageInTicks, f64 netHeadYaw,
                                   f64 headPitch, f64 scale) {
    // 末影水晶旋转动画
    m_cube->setRotateAngleY(static_cast<f32>(ageInTicks * 0.1));
    m_glass->setRotateAngleY(static_cast<f32>(ageInTicks * 0.05));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== SpectralArrowModel ====================

SpectralArrowModel::SpectralArrowModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    setupParts();
}

void SpectralArrowModel::setupParts() {
    m_arrow = std::make_shared<ModelRenderer>("arrow");
    m_arrow->setTextureOffset(0, 0);
    m_arrow->addBox(0.0f, -0.5f, -0.5f, 16.0f, 1.0f, 1.0f);
    m_arrow->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_arrow);
}

void SpectralArrowModel::render(f64 scale) {
    EntityModel::render(scale);
}

void SpectralArrowModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                    f64 ageInTicks, f64 netHeadYaw,
                                    f64 headPitch, f64 scale) {
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== WitherSkullModel ====================

WitherSkullModel::WitherSkullModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void WitherSkullModel::setupParts() {
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -4.0f, -4.0f, 8.0f, 8.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_head);
}

void WitherSkullModel::render(f64 scale) {
    EntityModel::render(scale);
}

void WitherSkullModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                   f64 ageInTicks, f64 netHeadYaw,
                                   f64 headPitch, f64 scale) {
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)ageInTicks;
    (void)scale;
}

// ==================== DragonFireballModel ====================

DragonFireballModel::DragonFireballModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void DragonFireballModel::setupParts() {
    m_core = std::make_shared<ModelRenderer>("core");
    m_core->setTextureOffset(0, 0);
    m_core->addBox(-4.0f, -4.0f, -4.0f, 8.0f, 8.0f, 8.0f);
    m_core->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_core);

    m_outer = std::make_shared<ModelRenderer>("outer");
    m_outer->setTextureOffset(0, 16);
    m_outer->addBox(-6.0f, -6.0f, -6.0f, 12.0f, 12.0f, 12.0f);
    m_outer->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_outer);
}

void DragonFireballModel::render(f64 scale) {
    EntityModel::render(scale);
}

void DragonFireballModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                     f64 ageInTicks, f64 netHeadYaw,
                                     f64 headPitch, f64 scale) {
    m_core->setRotateAngleY(static_cast<f32>(ageInTicks * 0.1));
    m_outer->setRotateAngleY(static_cast<f32>(-ageInTicks * 0.05));
    m_outer->setRotateAngleX(static_cast<f32>(ageInTicks * 0.03));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== EvokerFangsModel ====================

EvokerFangsModel::EvokerFangsModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    setupParts();
}

void EvokerFangsModel::setupParts() {
    // 4个尖牙
    for (i32 i = 0; i < 4; ++i) {
        m_fangs[i] = std::make_shared<ModelRenderer>("fang" + std::to_string(i));
        m_fangs[i]->setTextureOffset(0, 0);
        m_fangs[i]->addBox(-2.5f, 0.0f, -0.5f, 5.0f, 8.0f, 1.0f);
        m_fangs[i]->setRotationPoint(0.0f, 16.0f, 0.0f);
        m_parts.push_back(m_fangs[i]);
    }
}

void EvokerFangsModel::render(f64 scale) {
    EntityModel::render(scale);
}

void EvokerFangsModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                  f64 ageInTicks, f64 netHeadYaw,
                                  f64 headPitch, f64 scale) {
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

} // namespace mc::client::renderer::entity::model::projectile
