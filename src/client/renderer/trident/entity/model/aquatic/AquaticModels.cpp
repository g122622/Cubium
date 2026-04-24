#include "AquaticModels.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::aquatic {

namespace {
    constexpr f64 PI = 3.14159265359;
}

// ==================== CodModel ====================

CodModel::CodModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    setupParts();
}

void CodModel::setupParts() {
    // 参考 MC 1.16.5 CodModel
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-1.0f, -2.0f, -3.0f, 2.0f, 4.0f, 6.0f);
    m_body->setRotationPoint(0.0f, 20.0f, 0.0f);
    m_parts.push_back(m_body);

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(16, 0);
    m_head->addBox(-1.0f, -1.0f, -2.0f, 2.0f, 2.0f, 2.0f);
    m_head->setRotationPoint(0.0f, 20.0f, -3.0f);
    m_parts.push_back(m_head);

    m_nose = std::make_shared<ModelRenderer>("nose");
    m_nose->setTextureOffset(24, 0);
    m_nose->addBox(0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);
    m_nose->setRotationPoint(0.0f, 20.0f, -5.0f);
    m_parts.push_back(m_nose);

    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(0, 10);
    m_tail->addBox(0.0f, -1.0f, 0.0f, 0.0f, 2.0f, 3.0f);
    m_tail->setRotationPoint(0.0f, 20.0f, 3.0f);
    m_parts.push_back(m_tail);
}

void CodModel::render(f64 scale) {
    EntityModel::render(scale);
}

void CodModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                          f64 ageInTicks, f64 netHeadYaw,
                          f64 headPitch, f64 scale) {
    m_tail->setRotateAngleY(static_cast<f32>(std::sin(limbSwing * 0.3) * limbSwingAmount * 0.6));
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== SalmonModel ====================

SalmonModel::SalmonModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    setupParts();
}

void SalmonModel::setupParts() {
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-0.5f, -2.0f, -4.0f, 1.0f, 3.0f, 8.0f);
    m_body->setRotationPoint(0.0f, 20.0f, 0.0f);
    m_parts.push_back(m_body);

    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(18, 0);
    m_tail->addBox(0.0f, -1.5f, 0.0f, 0.0f, 3.0f, 3.0f);
    m_tail->setRotationPoint(0.0f, 20.0f, 4.0f);
    m_parts.push_back(m_tail);
}

void SalmonModel::render(f64 scale) {
    EntityModel::render(scale);
}

void SalmonModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                             f64 ageInTicks, f64 netHeadYaw,
                             f64 headPitch, f64 scale) {
    m_tail->setRotateAngleY(static_cast<f32>(std::sin(limbSwing * 0.4) * limbSwingAmount * 0.5));
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== DolphinModel ====================

DolphinModel::DolphinModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void DolphinModel::setupParts() {
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

TurtleModel::TurtleModel()
    : EntityModel()
{
    setTextureSize(128, 64);
    setupParts();
}

void TurtleModel::setupParts() {
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
