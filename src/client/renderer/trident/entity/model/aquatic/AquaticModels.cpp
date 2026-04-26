#include "AquaticModels.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::aquatic {

namespace {
    constexpr f64 PI = 3.14159265359;
}

// ==================== CodModel ====================
// 参考 MC 1.16.5 CodModel

CodModel::CodModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    setupParts();
}

void CodModel::setupParts() {
    // 参考 MC 1.16.5 CodModel 构造函数
    // body: (-1, -2, 0) 到 (1, 2, 7), 纹理 (0, 0), 旋转点 (0, 22, 0)
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-1.0f, -2.0f, 0.0f, 2.0f, 4.0f, 7.0f);
    m_body->setRotationPoint(0.0f, 22.0f, 0.0f);

    // head: 纹理 (11, 0), (-1, -2, -3) 到 (1, 2, 0), 旋转点 (0, 22, 0)
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(11, 0);
    m_head->addBox(-1.0f, -2.0f, -3.0f, 2.0f, 4.0f, 3.0f);
    m_head->setRotationPoint(0.0f, 22.0f, 0.0f);

    // headFront: 纹理 (0, 0), (-1, -2, -1) 到 (1, 1, 0), 旋转点 (0, 22, -3)
    m_headFront = std::make_shared<ModelRenderer>("headFront");
    m_headFront->setTextureOffset(0, 0);
    m_headFront->addBox(-1.0f, -2.0f, -1.0f, 2.0f, 3.0f, 1.0f);
    m_headFront->setRotationPoint(0.0f, 22.0f, -3.0f);

    // finRight: 纹理 (22, 1), (-2, 0, -1) 到 (0, 0, 1), 旋转点 (-1, 23, 0)
    m_finRight = std::make_shared<ModelRenderer>("finRight");
    m_finRight->setTextureOffset(22, 1);
    m_finRight->addBox(-2.0f, 0.0f, -1.0f, 2.0f, 0.0f, 2.0f);
    m_finRight->setRotationPoint(-1.0f, 23.0f, 0.0f);
    m_finRight->setRotateAngleZ(static_cast<f32>(-PI / 4.0));

    // finLeft: 纹理 (22, 4), (0, 0, -1) 到 (2, 0, 1), 旋转点 (1, 23, 0)
    m_finLeft = std::make_shared<ModelRenderer>("finLeft");
    m_finLeft->setTextureOffset(22, 4);
    m_finLeft->addBox(0.0f, 0.0f, -1.0f, 2.0f, 0.0f, 2.0f);
    m_finLeft->setRotationPoint(1.0f, 23.0f, 0.0f);
    m_finLeft->setRotateAngleZ(static_cast<f32>(PI / 4.0));

    // tail: 纹理 (22, 3), (0, -2, 0) 到 (0, 2, 4), 旋转点 (0, 22, 7)
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(22, 3);
    m_tail->addBox(0.0f, -2.0f, 0.0f, 0.0f, 4.0f, 4.0f);
    m_tail->setRotationPoint(0.0f, 22.0f, 7.0f);

    // finTop: 纹理 (20, -6), (0, -1, -1) 到 (0, 0, 5), 旋转点 (0, 20, 0)
    m_finTop = std::make_shared<ModelRenderer>("finTop");
    m_finTop->setTextureOffset(20, -6);
    m_finTop->addBox(0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 6.0f);
    m_finTop->setRotationPoint(0.0f, 20.0f, 0.0f);

    // 添加到部件列表
    m_parts.push_back(m_body);
    m_parts.push_back(m_head);
    m_parts.push_back(m_headFront);
    m_parts.push_back(m_finRight);
    m_parts.push_back(m_finLeft);
    m_parts.push_back(m_tail);
    m_parts.push_back(m_finTop);
}

void CodModel::render(f64 scale) {
    EntityModel::render(scale);
}

void CodModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                          f64 ageInTicks, f64 netHeadYaw,
                          f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 CodModel.setRotationAngles
    // float f = 1.0F; if (!entityIn.isInWater()) { f = 1.5F; }
    // this.tail.rotateAngleY = -f * 0.45F * MathHelper.sin(0.6F * ageInTicks);
    f32 f = m_isInWater ? 1.0f : 1.5f;
    m_tail->setRotateAngleY(-f * 0.45f * static_cast<f32>(std::sin(0.6 * ageInTicks)));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== SalmonModel ====================
// 参考 MC 1.16.5 SalmonModel

SalmonModel::SalmonModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    setupParts();
}

void SalmonModel::setupParts() {
    // 参考 MC 1.16.5 SalmonModel 构造函数
    // bodyFront: 纹理 (0, 0), (-1.5, -2.5, 0) 到 (1.5, 2.5, 8), 旋转点 (0, 20, 0)
    m_bodyFront = std::make_shared<ModelRenderer>("bodyFront");
    m_bodyFront->setTextureOffset(0, 0);
    m_bodyFront->addBox(-1.5f, -2.5f, 0.0f, 3.0f, 5.0f, 8.0f);
    m_bodyFront->setRotationPoint(0.0f, 20.0f, 0.0f);

    // bodyRear: 纹理 (0, 13), (-1.5, -2.5, 0) 到 (1.5, 2.5, 8), 旋转点 (0, 20, 8)
    m_bodyRear = std::make_shared<ModelRenderer>("bodyRear");
    m_bodyRear->setTextureOffset(0, 13);
    m_bodyRear->addBox(-1.5f, -2.5f, 0.0f, 3.0f, 5.0f, 8.0f);
    m_bodyRear->setRotationPoint(0.0f, 20.0f, 8.0f);

    // head: 纹理 (22, 0), (-1, -2, -3) 到 (1, 2, 0), 旋转点 (0, 20, 0)
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(22, 0);
    m_head->addBox(-1.0f, -2.0f, -3.0f, 2.0f, 4.0f, 3.0f);
    m_head->setRotationPoint(0.0f, 20.0f, 0.0f);

    // tail: 纹理 (20, 10), (0, -2.5, 0) 到 (0, 2.5, 6), 子部件位于 bodyRear (0, 0, 8)
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(20, 10);
    m_tail->addBox(0.0f, -2.5f, 0.0f, 0.0f, 5.0f, 6.0f);
    m_tail->setRotationPoint(0.0f, 0.0f, 8.0f);
    m_bodyRear->addChild(m_tail);

    // dorsalFin: 纹理 (2, 1), (0, 0, 0) 到 (0, 2, 3), 子部件位于 bodyFront (0, -4.5, 5)
    m_dorsalFin = std::make_shared<ModelRenderer>("dorsalFin");
    m_dorsalFin->setTextureOffset(2, 1);
    m_dorsalFin->addBox(0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 3.0f);
    m_dorsalFin->setRotationPoint(0.0f, -4.5f, 5.0f);
    m_bodyFront->addChild(m_dorsalFin);

    // ventralFin: 纹理 (0, 2), (0, 0, 0) 到 (0, 2, 4), 子部件位于 bodyRear (0, -4.5, -1)
    m_ventralFin = std::make_shared<ModelRenderer>("ventralFin");
    m_ventralFin->setTextureOffset(0, 2);
    m_ventralFin->addBox(0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 4.0f);
    m_ventralFin->setRotationPoint(0.0f, -4.5f, -1.0f);
    m_bodyRear->addChild(m_ventralFin);

    // finRight: 纹理 (-4, 0), (-2, 0, 0) 到 (0, 0, 2), 旋转点 (-1.5, 21.5, 0)
    m_finRight = std::make_shared<ModelRenderer>("finRight");
    m_finRight->setTextureOffset(-4, 0);
    m_finRight->addBox(-2.0f, 0.0f, 0.0f, 2.0f, 0.0f, 2.0f);
    m_finRight->setRotationPoint(-1.5f, 21.5f, 0.0f);
    m_finRight->setRotateAngleZ(static_cast<f32>(-PI / 4.0));

    // finLeft: 纹理 (0, 0), (0, 0, 0) 到 (2, 0, 2), 旋转点 (1.5, 21.5, 0)
    m_finLeft = std::make_shared<ModelRenderer>("finLeft");
    m_finLeft->setTextureOffset(0, 0);
    m_finLeft->addBox(0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 2.0f);
    m_finLeft->setRotationPoint(1.5f, 21.5f, 0.0f);
    m_finLeft->setRotateAngleZ(static_cast<f32>(PI / 4.0));

    // 添加到部件列表（子部件会跟随父部件渲染，不需要单独添加）
    m_parts.push_back(m_bodyFront);
    m_parts.push_back(m_bodyRear);
    m_parts.push_back(m_head);
    m_parts.push_back(m_finRight);
    m_parts.push_back(m_finLeft);
}

void SalmonModel::render(f64 scale) {
    EntityModel::render(scale);
}

void SalmonModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                             f64 ageInTicks, f64 netHeadYaw,
                             f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 SalmonModel.setRotationAngles
    // float f = 1.0F; float f1 = 1.0F;
    // if (!entityIn.isInWater()) { f = 1.3F; f1 = 1.7F; }
    // this.bodyRear.rotateAngleY = -f * 0.25F * MathHelper.sin(f1 * 0.6F * ageInTicks);
    f32 f = m_isInWater ? 1.0f : 1.3f;
    f32 f1 = m_isInWater ? 1.0f : 1.7f;
    m_bodyRear->setRotateAngleY(-f * 0.25f * static_cast<f32>(std::sin(f1 * 0.6 * ageInTicks)));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== DolphinModel ====================
// 参考 MC 1.16.5 DolphinModel

DolphinModel::DolphinModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void DolphinModel::setupParts() {
    // 简化版本 - 基本结构保留
    // 参考 MC 1.16.5 DolphinModel
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-1.0f, -2.0f, -5.0f, 2.0f, 4.0f, 10.0f);
    m_body->setRotationPoint(0.0f, 18.0f, 0.0f);
    m_parts.push_back(m_body);

    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(24, 0);
    m_tail->addBox(-1.0f, -1.0f, 0.0f, 2.0f, 2.0f, 4.0f);
    m_tail->setRotationPoint(0.0f, 18.0f, 5.0f);
    m_parts.push_back(m_tail);

    m_finRight = std::make_shared<ModelRenderer>("finRight");
    m_finRight->setTextureOffset(36, 0);
    m_finRight->addBox(-2.0f, 0.0f, 0.0f, 2.0f, 1.0f, 2.0f);
    m_finRight->setRotationPoint(-1.0f, 16.0f, -2.0f);
    m_parts.push_back(m_finRight);

    m_finLeft = std::make_shared<ModelRenderer>("finLeft");
    m_finLeft->setTextureOffset(36, 0);
    m_finLeft->addBox(0.0f, 0.0f, 0.0f, 2.0f, 1.0f, 2.0f);
    m_finLeft->setRotationPoint(1.0f, 16.0f, -2.0f);
    m_parts.push_back(m_finLeft);

    m_finBack = std::make_shared<ModelRenderer>("finBack");
    m_finBack->setTextureOffset(44, 0);
    m_finBack->addBox(0.0f, -2.0f, 0.0f, 0.0f, 2.0f, 2.0f);
    m_finBack->setRotationPoint(0.0f, 16.0f, 0.0f);
    m_parts.push_back(m_finBack);
}

void DolphinModel::render(f64 scale) {
    EntityModel::render(scale);
}

void DolphinModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                              f64 ageInTicks, f64 netHeadYaw,
                              f64 headPitch, f64 scale) {
    m_tail->setRotateAngleY(static_cast<f32>(std::sin(limbSwing * 0.5) * limbSwingAmount * 0.5));
    m_body->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    (void)ageInTicks;
    (void)netHeadYaw;
    (void)scale;
}

// ==================== TurtleModel ====================
// 参考 MC 1.16.5 TurtleModel

TurtleModel::TurtleModel()
    : EntityModel()
{
    setTextureSize(128, 64);
    setupParts();
}

void TurtleModel::setupParts() {
    // 简化版本 - 基本结构保留
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-5.0f, -4.0f, -6.0f, 10.0f, 4.0f, 12.0f);
    m_body->setRotationPoint(0.0f, 20.0f, 0.0f);
    m_parts.push_back(m_body);

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(44, 0);
    m_head->addBox(-2.0f, -2.0f, -2.0f, 4.0f, 3.0f, 3.0f);
    m_head->setRotationPoint(0.0f, 19.0f, -6.0f);
    m_parts.push_back(m_head);

    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureOffset(0, 16);
    m_legFrontRight->addBox(-1.0f, 0.0f, -1.0f, 3.0f, 2.0f, 3.0f);
    m_legFrontRight->setRotationPoint(-4.0f, 20.0f, -4.0f);
    m_parts.push_back(m_legFrontRight);

    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureOffset(0, 16);
    m_legFrontLeft->addBox(-2.0f, 0.0f, -1.0f, 3.0f, 2.0f, 3.0f);
    m_legFrontLeft->setRotationPoint(4.0f, 20.0f, -4.0f);
    m_parts.push_back(m_legFrontLeft);

    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureOffset(0, 16);
    m_legBackRight->addBox(-1.0f, 0.0f, -2.0f, 3.0f, 2.0f, 3.0f);
    m_legBackRight->setRotationPoint(-4.0f, 20.0f, 5.0f);
    m_parts.push_back(m_legBackRight);

    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureOffset(0, 16);
    m_legBackLeft->addBox(-2.0f, 0.0f, -2.0f, 3.0f, 2.0f, 3.0f);
    m_legBackLeft->setRotationPoint(4.0f, 20.0f, 5.0f);
    m_parts.push_back(m_legBackLeft);
}

void TurtleModel::render(f64 scale) {
    EntityModel::render(scale);
}

void TurtleModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                             f64 ageInTicks, f64 netHeadYaw,
                             f64 headPitch, f64 scale) {
    f32 swimAngle = static_cast<f32>(std::cos(limbSwing * 0.4) * limbSwingAmount * 0.5);
    m_legFrontRight->setRotateAngleX(swimAngle);
    m_legFrontLeft->setRotateAngleX(-swimAngle);
    m_legBackRight->setRotateAngleX(-swimAngle);
    m_legBackLeft->setRotateAngleX(swimAngle);

    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    (void)ageInTicks;
    (void)scale;
}

} // namespace mc::client::renderer::entity::model::aquatic
