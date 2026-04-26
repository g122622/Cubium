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
    // 创建一个由 7 个 box 组成的十字形结构
    m_main = std::make_shared<ModelRenderer>("main");
    m_main->setTextureOffset(0, 0);
    // 西面 (-X 方向)
    m_main->addBox(-4.0f, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, scale);
    // 下面 (-Y 方向)
    m_main->addBox(0.0f, -4.0f, 0.0f, 2.0f, 2.0f, 2.0f, scale);
    // 北面 (-Z 方向)
    m_main->addBox(0.0f, 0.0f, -4.0f, 2.0f, 2.0f, 2.0f, scale);
    // 中心
    m_main->addBox(0.0f, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, scale);
    // 东面 (+X 方向)
    m_main->addBox(2.0f, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, scale);
    // 上面 (+Y 方向)
    m_main->addBox(0.0f, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, scale);
    // 南面 (+Z 方向)
    m_main->addBox(0.0f, 0.0f, 2.0f, 2.0f, 2.0f, 2.0f, scale);
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
    // 参考 MC 1.16.5 EnderCrystalModel (EnderCrystalRenderer)
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

    // 基座: Java addBox(-6.0F, 0.0F, -6.0F, 12.0F, 4.0F, 12.0F)
    m_base = std::make_shared<ModelRenderer>("base");
    m_base->setTextureOffset(0, 16);
    m_base->addBox(-6.0f, 0.0f, -6.0f, 12.0f, 4.0f, 12.0f, scale);
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
    // 参考 MC 1.16.5 GenericHeadModel
    // Y 偏移为 -8.0F（不是 -4.0F）
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f);
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
    setTextureSize(64, 32);
    setupParts();
}

void EvokerFangsModel::setupParts() {
    // 参考 MC 1.16.5 EvokerFangsModel
    // 底座: 10x12x10，旋转点 (-5, 22, -5)
    m_base = std::make_shared<ModelRenderer>("base");
    m_base->setTextureOffset(0, 0);
    m_base->addBox(0.0f, 0.0f, 0.0f, 10.0f, 12.0f, 10.0f);
    m_base->setRotationPoint(-5.0f, 22.0f, -5.0f);
    m_parts.push_back(m_base);

    // 上颚: 4x14x8，旋转点 (1.5, 22, -4)
    m_upperJaw = std::make_shared<ModelRenderer>("upperJaw");
    m_upperJaw->setTextureOffset(40, 0);
    m_upperJaw->addBox(0.0f, 0.0f, 0.0f, 4.0f, 14.0f, 8.0f);
    m_upperJaw->setRotationPoint(1.5f, 22.0f, -4.0f);
    m_parts.push_back(m_upperJaw);

    // 下颚: 4x14x8，旋转点 (-1.5, 22, 4)
    m_lowerJaw = std::make_shared<ModelRenderer>("lowerJaw");
    m_lowerJaw->setTextureOffset(40, 0);
    m_lowerJaw->addBox(0.0f, 0.0f, 0.0f, 4.0f, 14.0f, 8.0f);
    m_lowerJaw->setRotationPoint(-1.5f, 22.0f, 4.0f);
    m_parts.push_back(m_lowerJaw);
}

void EvokerFangsModel::render(f64 scale) {
    EntityModel::render(scale);
}

void EvokerFangsModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                  f64 ageInTicks, f64 netHeadYaw,
                                  f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 EvokerFangsModel.setRotationAngles
    // f = limbSwing * 2.0F，限制在 [0, 1]
    // f = 1.0F - f * f * f
    f32 f = static_cast<f32>(limbSwing * 2.0);
    if (f > 1.0f) f = 1.0f;
    f = 1.0f - f * f * f;

    // 上颚 Z 轴旋转: PI - f * 0.35 * PI
    m_upperJaw->setRotateAngleZ(static_cast<f32>(PI - f * 0.35 * PI));
    // 下颚 Z 轴旋转: PI + f * 0.35 * PI，Y 轴旋转: PI
    m_lowerJaw->setRotateAngleZ(static_cast<f32>(PI + f * 0.35 * PI));
    m_lowerJaw->setRotateAngleY(static_cast<f32>(PI));

    // Y 位置动画
    f32 f1 = static_cast<f32>((limbSwing + std::sin(limbSwing * 2.7)) * 0.6 * 12.0);
    m_upperJaw->setRotationPointY(24.0f - f1);
    m_lowerJaw->setRotationPointY(24.0f - f1);
    m_base->setRotationPointY(24.0f - f1);

    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

} // namespace mc::client::renderer::entity::model::projectile
