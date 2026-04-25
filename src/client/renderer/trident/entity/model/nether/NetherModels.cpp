#include "NetherModels.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::nether {

namespace {
    constexpr f64 PI = 3.14159265359;
}

// ==================== GhastModel ====================

GhastModel::GhastModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void GhastModel::setupParts() {
    // 参考 MC 1.16.5 GhastModel
    // 身体尺寸: 16x16x16，中心在原点
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-8.0f, -8.0f, -8.0f, 16.0f, 16.0f, 16.0f);
    m_body->setRotationPoint(0.0f, 17.6f, 0.0f);  // Java: rotationPointY = 17.6F
    m_parts.push_back(m_body);

    // 9 条触手
    // Java 触手位置计算:
    // float f = (((float)(i % 3) - (float)(i / 3 % 2) * 0.5F + 0.25F) / 2.0F * 2.0F - 1.0F) * 5.0F;
    // float f1 = ((float)(i / 3) / 2.0F * 2.0F - 1.0F) * 5.0F;
    // 触手长度: random.nextInt(7) + 8  -> 8 到 14

    // 使用固定种子生成触手长度（与 Java 原版一致）
    mc::math::Random rng(1660);

    for (i32 i = 0; i < 9; ++i) {
        m_tentacles[i] = std::make_shared<ModelRenderer>("tentacle" + std::to_string(i));
        m_tentacles[i]->setTextureOffset(0, 0);

        // 随机长度 8-14（Java: random.nextInt(7) + 8）
        i32 length = rng.nextInt(8, 14);

        // Java 公式计算 X 和 Z 位置
        f32 f = (((static_cast<f32>(i % 3) - static_cast<f32>(i / 3 % 2) * 0.5f + 0.25f) / 2.0f * 2.0f - 1.0f) * 5.0f);
        f32 f1 = ((static_cast<f32>(i / 3) / 2.0f * 2.0f - 1.0f) * 5.0f);

        m_tentacles[i]->addBox(-1.0f, 0.0f, -1.0f, 2.0f, static_cast<f32>(length), 2.0f);
        m_tentacles[i]->setRotationPoint(f, 24.6f, f1);  // Java: rotationPointY = 24.6F
        m_parts.push_back(m_tentacles[i]);
    }
}

void GhastModel::render(f64 scale) {
    EntityModel::render(scale);
}

void GhastModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                            f64 ageInTicks, f64 netHeadYaw,
                            f64 headPitch, f64 scale) {
    // 身体跟随头部旋转
    m_body->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_body->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    // Java 触手动画: rotateAngleX = 0.2F * MathHelper.sin(ageInTicks * 0.3F + (float)i) + 0.4F
    for (i32 i = 0; i < 9; ++i) {
        f32 angle = static_cast<f32>(0.2 * std::sin(ageInTicks * 0.3 + static_cast<f64>(i)) + 0.4);
        m_tentacles[i]->setRotateAngleX(angle);
    }

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)scale;
}

// ==================== MagmaCubeModel ====================

MagmaCubeModel::MagmaCubeModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

MagmaCubeModel::MagmaCubeModel(i32 size)
    : EntityModel()
    , m_size(size)
{
    setTextureSize(64, 32);
    setupParts();
}

void MagmaCubeModel::setupParts() {
    // 核心
    m_core = std::make_shared<ModelRenderer>("core");
    m_core->setTextureOffset(0, 16);
    m_core->addBox(-2.0f, -2.0f, -2.0f, 4.0f, 4.0f, 4.0f);
    m_core->setRotationPoint(0.0f, 17.0f, 0.0f);
    m_parts.push_back(m_core);

    // 8 个外层分段
    for (i32 i = 0; i < 8; ++i) {
        m_segments[i] = std::make_shared<ModelRenderer>("segment" + std::to_string(i));
        m_segments[i]->setTextureOffset(16, 0);
        m_segments[i]->addBox(-4.0f, -4.0f, -4.0f, 8.0f, 8.0f, 8.0f);
        m_segments[i]->setRotationPoint(0.0f, 17.0f, 0.0f);
        m_parts.push_back(m_segments[i]);
    }
}

void MagmaCubeModel::render(f64 scale) {
    EntityModel::render(scale);
}

void MagmaCubeModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                f64 ageInTicks, f64 netHeadYaw,
                                f64 headPitch, f64 scale) {
    // 外层分段旋转动画
    for (i32 i = 0; i < 8; ++i) {
        f32 phase = static_cast<f32>(i * PI / 4.0);
        m_segments[i]->setRotateAngleY(static_cast<f32>(std::sin(ageInTicks * 0.1 + phase) * 0.1));
        m_segments[i]->setRotateAngleX(static_cast<f32>(std::cos(ageInTicks * 0.1 + phase) * 0.1));
    }

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== PiglinModel ====================

PiglinModel::PiglinModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    setupParts();
}

void PiglinModel::setupParts() {
    // 头部
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_head);

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(16, 16);
    m_body->addBox(-4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f);
    m_body->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_body);

    // 右臂
    m_rightArm = std::make_shared<ModelRenderer>("rightArm");
    m_rightArm->setTextureOffset(32, 16);
    m_rightArm->addBox(-2.0f, -2.0f, -1.0f, 3.0f, 10.0f, 2.0f);
    m_rightArm->setRotationPoint(-5.0f, 2.0f, 0.0f);
    m_parts.push_back(m_rightArm);

    // 左臂
    m_leftArm = std::make_shared<ModelRenderer>("leftArm");
    m_leftArm->setTextureOffset(32, 16);
    m_leftArm->addBox(-1.0f, -2.0f, -1.0f, 3.0f, 10.0f, 2.0f);
    m_leftArm->setRotationPoint(5.0f, 2.0f, 0.0f);
    m_parts.push_back(m_leftArm);

    // 右腿
    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_rightLeg->setTextureOffset(0, 16);
    m_rightLeg->addBox(-1.0f, 0.0f, -1.0f, 3.0f, 12.0f, 2.0f);
    m_rightLeg->setRotationPoint(-2.0f, 12.0f, 0.0f);
    m_parts.push_back(m_rightLeg);

    // 左腿
    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");
    m_leftLeg->setTextureOffset(0, 16);
    m_leftLeg->addBox(-2.0f, 0.0f, -1.0f, 3.0f, 12.0f, 2.0f);
    m_leftLeg->setRotationPoint(2.0f, 12.0f, 0.0f);
    m_parts.push_back(m_leftLeg);
}

void PiglinModel::render(f64 scale) {
    EntityModel::render(scale);
}

void PiglinModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                             f64 ageInTicks, f64 netHeadYaw,
                             f64 headPitch, f64 scale) {
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    // 步态动画
    f32 legSwing = static_cast<f32>(std::cos(limbSwing * 0.6662) * limbSwingAmount);
    m_rightLeg->setRotateAngleX(legSwing);
    m_leftLeg->setRotateAngleX(-legSwing);

    m_rightArm->setRotateAngleX(-legSwing * 0.5f);
    m_leftArm->setRotateAngleX(legSwing * 0.5f);

    (void)ageInTicks;
    (void)scale;
}

// ==================== BoarModel ====================

BoarModel::BoarModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void BoarModel::setupParts() {
    // 参考 MC 1.16.5 BoarModel
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-6.0f, -8.0f, -6.0f, 12.0f, 8.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 6.0f, -12.0f);
    m_parts.push_back(m_head);

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(32, 0);
    m_body->addBox(-8.0f, -10.0f, -8.0f, 16.0f, 12.0f, 16.0f);
    m_body->setRotationPoint(0.0f, 8.0f, 2.0f);
    m_parts.push_back(m_body);

    m_rightFrontLeg = std::make_shared<ModelRenderer>("rightFrontLeg");
    m_rightFrontLeg->setTextureOffset(0, 16);
    m_rightFrontLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 8.0f, 4.0f);
    m_rightFrontLeg->setRotationPoint(-5.0f, 16.0f, -6.0f);
    m_parts.push_back(m_rightFrontLeg);

    m_leftFrontLeg = std::make_shared<ModelRenderer>("leftFrontLeg");
    m_leftFrontLeg->setTextureOffset(0, 16);
    m_leftFrontLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 8.0f, 4.0f);
    m_leftFrontLeg->setRotationPoint(5.0f, 16.0f, -6.0f);
    m_parts.push_back(m_leftFrontLeg);

    m_rightBackLeg = std::make_shared<ModelRenderer>("rightBackLeg");
    m_rightBackLeg->setTextureOffset(0, 16);
    m_rightBackLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 8.0f, 4.0f);
    m_rightBackLeg->setRotationPoint(-5.0f, 16.0f, 8.0f);
    m_parts.push_back(m_rightBackLeg);

    m_leftBackLeg = std::make_shared<ModelRenderer>("leftBackLeg");
    m_leftBackLeg->setTextureOffset(0, 16);
    m_leftBackLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 8.0f, 4.0f);
    m_leftBackLeg->setRotationPoint(5.0f, 16.0f, 8.0f);
    m_parts.push_back(m_leftBackLeg);
}

void BoarModel::render(f64 scale) {
    EntityModel::render(scale);
}

void BoarModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                           f64 ageInTicks, f64 netHeadYaw,
                           f64 headPitch, f64 scale) {
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    f32 legSwing = static_cast<f32>(std::cos(limbSwing * 0.6662) * limbSwingAmount * 0.8);
    m_rightFrontLeg->setRotateAngleX(legSwing);
    m_leftFrontLeg->setRotateAngleX(-legSwing);
    m_rightBackLeg->setRotateAngleX(-legSwing);
    m_leftBackLeg->setRotateAngleX(legSwing);

    (void)ageInTicks;
    (void)scale;
}

// ==================== StriderModel ====================

StriderModel::StriderModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    setupParts();
}

void StriderModel::setupParts() {
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-4.0f, -8.0f, -8.0f, 8.0f, 10.0f, 16.0f);
    m_body->setRotationPoint(0.0f, 18.0f, 0.0f);
    m_parts.push_back(m_body);

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 26);
    m_head->addBox(-2.0f, -4.0f, -2.0f, 4.0f, 4.0f, 4.0f);
    m_head->setRotationPoint(0.0f, 10.0f, -8.0f);
    m_parts.push_back(m_head);

    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_rightLeg->setTextureOffset(48, 0);
    m_rightLeg->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 16.0f, 2.0f);
    m_rightLeg->setRotationPoint(-2.0f, 18.0f, 4.0f);
    m_parts.push_back(m_rightLeg);

    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");
    m_leftLeg->setTextureOffset(48, 0);
    m_leftLeg->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 16.0f, 2.0f);
    m_leftLeg->setRotationPoint(2.0f, 18.0f, 4.0f);
    m_parts.push_back(m_leftLeg);
}

void StriderModel::render(f64 scale) {
    EntityModel::render(scale);
}

void StriderModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                              f64 ageInTicks, f64 netHeadYaw,
                              f64 headPitch, f64 scale) {
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    f32 legSwing = static_cast<f32>(std::cos(limbSwing * 0.5) * limbSwingAmount * 0.3);
    m_rightLeg->setRotateAngleX(legSwing);
    m_leftLeg->setRotateAngleX(-legSwing);

    (void)ageInTicks;
    (void)scale;
}

} // namespace mc::client::renderer::entity::model::nether
