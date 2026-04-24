#include "MoreMonsterModels.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

namespace {
    constexpr f64 PI = 3.14159265359;
}

// ==================== IllagerModel ====================

IllagerModel::IllagerModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    setupParts(0.0f, 0.0f, 64, 64);
}

IllagerModel::IllagerModel(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight)
    : EntityModel()
{
    setTextureSize(textureWidth, textureHeight);
    setupParts(scale, yOffset, textureWidth, textureHeight);
}

void IllagerModel::setupParts(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight) {
    // 参考 MC 1.16.5 IllagerModel
    (void)textureWidth;
    (void)textureHeight;

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setRotationPoint(0.0f, yOffset, 0.0f);
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -10.0f, -4.0f, 8.0f, 10.0f, 8.0f, scale);
    m_parts.push_back(m_head);

    m_hat = std::make_shared<ModelRenderer>("hat");
    m_hat->setTextureOffset(32, 0);
    m_hat->addBox(-4.0f, -10.0f, -4.0f, 8.0f, 12.0f, 8.0f, scale + 0.45f);
    m_hat->setVisible(false);
    m_parts.push_back(m_hat);

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setRotationPoint(0.0f, yOffset, 0.0f);
    m_body->setTextureOffset(16, 20);
    m_body->addBox(-4.0f, 0.0f, -3.0f, 8.0f, 12.0f, 6.0f, scale);
    m_body->setTextureOffset(0, 38);
    m_body->addBox(-4.0f, 0.0f, -3.0f, 8.0f, 18.0f, 6.0f, scale + 0.5f);
    m_parts.push_back(m_body);

    m_arms = std::make_shared<ModelRenderer>("arms");
    m_arms->setRotationPoint(0.0f, yOffset + 2.0f, 0.0f);
    m_arms->setTextureOffset(44, 22);
    m_arms->addBox(-8.0f, -2.0f, -2.0f, 4.0f, 8.0f, 4.0f, scale);
    // 镜像手臂
    m_arms->setTextureOffset(44, 22);
    m_arms->addBox(4.0f, -2.0f, -2.0f, 4.0f, 8.0f, 4.0f, scale);
    m_arms->setTextureOffset(40, 38);
    m_arms->addBox(-4.0f, 2.0f, -2.0f, 8.0f, 4.0f, 4.0f, scale);
    m_parts.push_back(m_arms);

    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_rightLeg->setTextureOffset(0, 22);
    m_rightLeg->setRotationPoint(-2.0f, 12.0f + yOffset, 0.0f);
    m_rightLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, scale);
    m_parts.push_back(m_rightLeg);

    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");
    m_leftLeg->setTextureOffset(0, 22);
    m_leftLeg->setMirror(true);
    m_leftLeg->setRotationPoint(2.0f, 12.0f + yOffset, 0.0f);
    m_leftLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, scale);
    m_parts.push_back(m_leftLeg);

    m_rightArm = std::make_shared<ModelRenderer>("rightArm");
    m_rightArm->setTextureOffset(40, 46);
    m_rightArm->setRotationPoint(-5.0f, 2.0f + yOffset, 0.0f);
    m_rightArm->addBox(-3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, scale);
    m_parts.push_back(m_rightArm);

    m_leftArm = std::make_shared<ModelRenderer>("leftArm");
    m_leftArm->setTextureOffset(40, 46);
    m_leftArm->setMirror(true);
    m_leftArm->setRotationPoint(5.0f, 2.0f + yOffset, 0.0f);
    m_leftArm->addBox(-1.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, scale);
    m_parts.push_back(m_leftArm);
}

void IllagerModel::render(f64 scale) {
    EntityModel::render(scale);
}

void IllagerModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                              f64 ageInTicks, f64 netHeadYaw,
                              f64 headPitch, f64 scale) {
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    m_arms->setRotationPointY(3.0f);
    m_arms->setRotationPointZ(-1.0f);
    m_arms->setRotateAngleX(-0.75f);

    f32 legSwing = static_cast<f32>(std::cos(limbSwing * 0.6662) * limbSwingAmount);
    m_rightArm->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + PI) * 2.0 * limbSwingAmount * 0.5));
    m_leftArm->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662) * 2.0 * limbSwingAmount * 0.5));
    m_rightLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662) * 1.4 * limbSwingAmount * 0.5));
    m_leftLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + PI) * 1.4 * limbSwingAmount * 0.5));

    (void)ageInTicks;
    (void)scale;
}

// ==================== VexModel ====================

VexModel::VexModel()
    : BipedModel(0.0f, 0.0f, 64, 64)
{
    // 左腿隐藏
    m_leftLeg->setVisible(false);
    // 头部装饰隐藏
    m_headwear->setVisible(false);

    // 右腿（特殊的腿）
    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_rightLeg->setTextureOffset(32, 0);
    m_rightLeg->addBox(-1.0f, -1.0f, -2.0f, 6.0f, 10.0f, 4.0f, 0.0f);
    m_rightLeg->setRotationPoint(-1.9f, 12.0f, 0.0f);
    m_parts.push_back(m_rightLeg);

    // 翅膀
    m_rightWing = std::make_shared<ModelRenderer>("rightWing");
    m_rightWing->setTextureOffset(0, 32);
    m_rightWing->addBox(-20.0f, 0.0f, 0.0f, 20.0f, 12.0f, 1.0f);
    m_parts.push_back(m_rightWing);

    m_leftWing = std::make_shared<ModelRenderer>("leftWing");
    m_leftWing->setTextureOffset(0, 32);
    m_leftWing->setMirror(true);
    m_leftWing->addBox(0.0f, 0.0f, 0.0f, 20.0f, 12.0f, 1.0f);
    m_parts.push_back(m_leftWing);
}

void VexModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                          f64 ageInTicks, f64 netHeadYaw,
                          f64 headPitch, f64 scale) {
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 恼鬼有漂浮的手臂
    m_rightArm->setRotateAngleX(static_cast<f32>(PI * 1.5));
    m_leftArm->setRotateAngleX(static_cast<f32>(PI * 1.5));

    m_rightLeg->setRotateAngleX(static_cast<f32>(m_rightLeg->rotateAngleX() + PI / 5.0));

    m_rightWing->setRotationPointZ(2.0f);
    m_leftWing->setRotationPointZ(2.0f);
    m_rightWing->setRotationPointY(1.0f);
    m_leftWing->setRotationPointY(1.0f);
    m_rightWing->setRotateAngleY(static_cast<f32>(0.47123894 + std::cos(ageInTicks * 0.8) * PI * 0.05));
    m_leftWing->setRotateAngleY(-m_rightWing->rotateAngleY());
    m_leftWing->setRotateAngleZ(-0.47123894f);
    m_leftWing->setRotateAngleX(0.47123894f);
    m_rightWing->setRotateAngleX(0.47123894f);
    m_rightWing->setRotateAngleZ(0.47123894f);
}

// ==================== IronGolemModel ====================

IronGolemModel::IronGolemModel()
    : EntityModel()
{
    setTextureSize(128, 128);
    setupParts();
}

void IronGolemModel::setupParts() {
    // 参考 MC 1.16.5 IronGolemModel

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setRotationPoint(0.0f, -7.0f, -2.0f);
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -12.0f, -5.5f, 8.0f, 10.0f, 8.0f);
    m_head->setTextureOffset(24, 0);
    m_head->addBox(-1.0f, -5.0f, -7.5f, 2.0f, 4.0f, 2.0f);
    m_parts.push_back(m_head);

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setRotationPoint(0.0f, -7.0f, 0.0f);
    m_body->setTextureOffset(0, 40);
    m_body->addBox(-9.0f, -2.0f, -6.0f, 18.0f, 12.0f, 11.0f);
    m_body->setTextureOffset(0, 70);
    m_body->addBox(-4.5f, 10.0f, -3.0f, 9.0f, 5.0f, 6.0f, 0.5f);
    m_parts.push_back(m_body);

    m_rightArm = std::make_shared<ModelRenderer>("rightArm");
    m_rightArm->setRotationPoint(0.0f, -7.0f, 0.0f);
    m_rightArm->setTextureOffset(60, 21);
    m_rightArm->addBox(-13.0f, -2.5f, -3.0f, 4.0f, 30.0f, 6.0f);
    m_parts.push_back(m_rightArm);

    m_leftArm = std::make_shared<ModelRenderer>("leftArm");
    m_leftArm->setRotationPoint(0.0f, -7.0f, 0.0f);
    m_leftArm->setTextureOffset(60, 58);
    m_leftArm->addBox(9.0f, -2.5f, -3.0f, 4.0f, 30.0f, 6.0f);
    m_parts.push_back(m_leftArm);

    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");
    m_leftLeg->setTextureOffset(37, 0);
    m_leftLeg->setRotationPoint(-4.0f, 11.0f, 0.0f);
    m_leftLeg->addBox(-3.5f, -3.0f, -3.0f, 6.0f, 16.0f, 5.0f);
    m_parts.push_back(m_leftLeg);

    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_rightLeg->setTextureOffset(60, 0);
    m_rightLeg->setMirror(true);
    m_rightLeg->setRotationPoint(5.0f, 11.0f, 0.0f);
    m_rightLeg->addBox(-3.5f, -3.0f, -3.0f, 6.0f, 16.0f, 5.0f);
    m_parts.push_back(m_rightLeg);
}

void IronGolemModel::render(f64 scale) {
    EntityModel::render(scale);
}

void IronGolemModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                f64 ageInTicks, f64 netHeadYaw,
                                f64 headPitch, f64 scale) {
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    // 使用三角波函数而不是正弦
    f32 legSwing = static_cast<f32>(1.5 * std::abs(2.0 * std::fmod(limbSwing, 13.0) / 13.0 - 1.0) * limbSwingAmount);
    m_leftLeg->setRotateAngleX(-legSwing);
    m_rightLeg->setRotateAngleX(legSwing);
    m_leftLeg->setRotateAngleY(0.0f);
    m_rightLeg->setRotateAngleY(0.0f);

    // 手臂动画
    m_rightArm->setRotateAngleX(static_cast<f32>((-0.2 + 1.5 * std::abs(2.0 * std::fmod(limbSwing, 13.0) / 13.0 - 1.0)) * limbSwingAmount));
    m_leftArm->setRotateAngleX(static_cast<f32>((-0.2 - 1.5 * std::abs(2.0 * std::fmod(limbSwing, 13.0) / 13.0 - 1.0)) * limbSwingAmount));

    (void)ageInTicks;
}

// ==================== SnowGolemModel ====================

SnowGolemModel::SnowGolemModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    setupParts();
}

void SnowGolemModel::setupParts() {
    // 参考 MC 1.16.5 SnowManModel

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, -0.5f);
    m_head->setRotationPoint(0.0f, 4.0f, 0.0f);
    m_parts.push_back(m_head);

    m_rightHand = std::make_shared<ModelRenderer>("rightHand");
    m_rightHand->setTextureOffset(32, 0);
    m_rightHand->addBox(-1.0f, 0.0f, -1.0f, 12.0f, 2.0f, 2.0f, -0.5f);
    m_rightHand->setRotationPoint(0.0f, 6.0f, 0.0f);
    m_parts.push_back(m_rightHand);

    m_leftHand = std::make_shared<ModelRenderer>("leftHand");
    m_leftHand->setTextureOffset(32, 0);
    m_leftHand->addBox(-1.0f, 0.0f, -1.0f, 12.0f, 2.0f, 2.0f, -0.5f);
    m_leftHand->setRotationPoint(0.0f, 6.0f, 0.0f);
    m_parts.push_back(m_leftHand);

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 16);
    m_body->addBox(-5.0f, -10.0f, -5.0f, 10.0f, 10.0f, 10.0f, -0.5f);
    m_body->setRotationPoint(0.0f, 13.0f, 0.0f);
    m_parts.push_back(m_body);

    m_bottomBody = std::make_shared<ModelRenderer>("bottomBody");
    m_bottomBody->setTextureOffset(0, 36);
    m_bottomBody->addBox(-6.0f, -12.0f, -6.0f, 12.0f, 12.0f, 12.0f, -0.5f);
    m_bottomBody->setRotationPoint(0.0f, 24.0f, 0.0f);
    m_parts.push_back(m_bottomBody);
}

void SnowGolemModel::render(f64 scale) {
    EntityModel::render(scale);
}

void SnowGolemModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                f64 ageInTicks, f64 netHeadYaw,
                                f64 headPitch, f64 scale) {
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    m_body->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0 * 0.25));

    f32 sinY = static_cast<f32>(std::sin(m_body->rotateAngleY()));
    f32 cosY = static_cast<f32>(std::cos(m_body->rotateAngleY()));

    m_rightHand->setRotateAngleZ(1.0f);
    m_leftHand->setRotateAngleZ(-1.0f);
    m_rightHand->setRotateAngleY(m_body->rotateAngleY());
    m_leftHand->setRotateAngleY(static_cast<f32>(PI + m_body->rotateAngleY()));
    m_rightHand->setRotationPointX(cosY * 5.0f);
    m_rightHand->setRotationPointZ(-sinY * 5.0f);
    m_leftHand->setRotationPointX(-cosY * 5.0f);
    m_leftHand->setRotationPointZ(sinY * 5.0f);

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)ageInTicks;
    (void)scale;
}

// ==================== BeeModel ====================

BeeModel::BeeModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    setupParts();
}

void BeeModel::setupParts() {
    // 参考 MC 1.16.5 BeeModel

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setRotationPoint(0.0f, 19.0f, 0.0f);
    m_parts.push_back(m_body);

    m_torso = std::make_shared<ModelRenderer>("torso");
    m_torso->setTextureOffset(0, 0);
    m_torso->addBox(-3.5f, -4.0f, -5.0f, 7.0f, 7.0f, 10.0f);
    m_parts.push_back(m_torso);

    m_stinger = std::make_shared<ModelRenderer>("stinger");
    m_stinger->setTextureOffset(26, 7);
    m_stinger->addBox(0.0f, -1.0f, 5.0f, 0.0f, 1.0f, 2.0f);
    m_parts.push_back(m_stinger);

    m_leftAntenna = std::make_shared<ModelRenderer>("leftAntenna");
    m_leftAntenna->setTextureOffset(2, 0);
    m_leftAntenna->setRotationPoint(0.0f, -2.0f, -5.0f);
    m_leftAntenna->addBox(1.5f, -2.0f, -3.0f, 1.0f, 2.0f, 3.0f);
    m_parts.push_back(m_leftAntenna);

    m_rightAntenna = std::make_shared<ModelRenderer>("rightAntenna");
    m_rightAntenna->setTextureOffset(2, 3);
    m_rightAntenna->setRotationPoint(0.0f, -2.0f, -5.0f);
    m_rightAntenna->addBox(-2.5f, -2.0f, -3.0f, 1.0f, 2.0f, 3.0f);
    m_parts.push_back(m_rightAntenna);

    m_rightWing = std::make_shared<ModelRenderer>("rightWing");
    m_rightWing->setTextureOffset(0, 18);
    m_rightWing->setRotationPoint(-1.5f, -4.0f, -3.0f);
    m_rightWing->setRotateAngleY(-0.2618f);
    m_rightWing->addBox(-9.0f, 0.0f, 0.0f, 9.0f, 0.0f, 6.0f, 0.001f);
    m_parts.push_back(m_rightWing);

    m_leftWing = std::make_shared<ModelRenderer>("leftWing");
    m_leftWing->setTextureOffset(0, 18);
    m_leftWing->setMirror(true);
    m_leftWing->setRotationPoint(1.5f, -4.0f, -3.0f);
    m_leftWing->setRotateAngleY(0.2618f);
    m_leftWing->addBox(0.0f, 0.0f, 0.0f, 9.0f, 0.0f, 6.0f, 0.001f);
    m_parts.push_back(m_leftWing);

    m_frontLegs = std::make_shared<ModelRenderer>("frontLegs");
    m_frontLegs->setRotationPoint(1.5f, 3.0f, -2.0f);
    m_frontLegs->setTextureOffset(26, 1);
    m_frontLegs->addBox(-5.0f, 0.0f, 0.0f, 7.0f, 2.0f, 0.0f);
    m_parts.push_back(m_frontLegs);

    m_middleLegs = std::make_shared<ModelRenderer>("middleLegs");
    m_middleLegs->setRotationPoint(1.5f, 3.0f, 0.0f);
    m_middleLegs->setTextureOffset(26, 3);
    m_middleLegs->addBox(-5.0f, 0.0f, 0.0f, 7.0f, 2.0f, 0.0f);
    m_parts.push_back(m_middleLegs);

    m_backLegs = std::make_shared<ModelRenderer>("backLegs");
    m_backLegs->setRotationPoint(1.5f, 3.0f, 2.0f);
    m_backLegs->setTextureOffset(26, 5);
    m_backLegs->addBox(-5.0f, 0.0f, 0.0f, 7.0f, 2.0f, 0.0f);
    m_parts.push_back(m_backLegs);
}

void BeeModel::render(f64 scale) {
    EntityModel::render(scale);
}

void BeeModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                          f64 ageInTicks, f64 netHeadYaw,
                          f64 headPitch, f64 scale) {
    // 翅膀扇动
    f32 wingAngle = static_cast<f32>(std::cos(ageInTicks * 2.1) * PI * 0.15);
    m_rightWing->setRotateAngleZ(wingAngle);
    m_leftWing->setRotateAngleZ(-wingAngle);

    m_frontLegs->setRotateAngleX(static_cast<f32>(PI / 4.0));
    m_middleLegs->setRotateAngleX(static_cast<f32>(PI / 4.0));
    m_backLegs->setRotateAngleX(static_cast<f32>(PI / 4.0));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
}

// ==================== FoxModel ====================

FoxModel::FoxModel()
    : EntityModel()
{
    setTextureSize(48, 32);
    setupParts();
}

void FoxModel::setupParts() {
    // 参考 MC 1.16.5 FoxModel

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(1, 5);
    m_head->addBox(-3.0f, -2.0f, -5.0f, 8.0f, 6.0f, 6.0f);
    m_head->setRotationPoint(-1.0f, 16.5f, -3.0f);
    m_parts.push_back(m_head);

    m_rightEar = std::make_shared<ModelRenderer>("rightEar");
    m_rightEar->setTextureOffset(8, 1);
    m_rightEar->addBox(-3.0f, -4.0f, -4.0f, 2.0f, 2.0f, 1.0f);
    m_parts.push_back(m_rightEar);

    m_leftEar = std::make_shared<ModelRenderer>("leftEar");
    m_leftEar->setTextureOffset(15, 1);
    m_leftEar->addBox(3.0f, -4.0f, -4.0f, 2.0f, 2.0f, 1.0f);
    m_parts.push_back(m_leftEar);

    m_snout = std::make_shared<ModelRenderer>("snout");
    m_snout->setTextureOffset(6, 18);
    m_snout->addBox(-1.0f, 2.01f, -8.0f, 4.0f, 2.0f, 3.0f);
    m_parts.push_back(m_snout);

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(24, 15);
    m_body->addBox(-3.0f, 3.999f, -3.5f, 6.0f, 11.0f, 6.0f);
    m_body->setRotationPoint(0.0f, 16.0f, -6.0f);
    m_parts.push_back(m_body);

    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureOffset(13, 24);
    m_legBackRight->addBox(2.0f, 0.5f, -1.0f, 2.0f, 6.0f, 2.0f, 0.001f);
    m_legBackRight->setRotationPoint(-5.0f, 17.5f, 7.0f);
    m_parts.push_back(m_legBackRight);

    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureOffset(4, 24);
    m_legBackLeft->addBox(2.0f, 0.5f, -1.0f, 2.0f, 6.0f, 2.0f, 0.001f);
    m_legBackLeft->setRotationPoint(-1.0f, 17.5f, 7.0f);
    m_parts.push_back(m_legBackLeft);

    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureOffset(13, 24);
    m_legFrontRight->addBox(2.0f, 0.5f, -1.0f, 2.0f, 6.0f, 2.0f, 0.001f);
    m_legFrontRight->setRotationPoint(-5.0f, 17.5f, 0.0f);
    m_parts.push_back(m_legFrontRight);

    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureOffset(4, 24);
    m_legFrontLeft->addBox(2.0f, 0.5f, -1.0f, 2.0f, 6.0f, 2.0f, 0.001f);
    m_legFrontLeft->setRotationPoint(-1.0f, 17.5f, 0.0f);
    m_parts.push_back(m_legFrontLeft);

    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(30, 0);
    m_tail->addBox(2.0f, 0.0f, -1.0f, 4.0f, 9.0f, 5.0f);
    m_tail->setRotationPoint(-4.0f, 15.0f, -1.0f);
    m_parts.push_back(m_tail);
}

void FoxModel::render(f64 scale) {
    EntityModel::render(scale);
}

void FoxModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                          f64 ageInTicks, f64 netHeadYaw,
                          f64 headPitch, f64 scale) {
    m_body->setRotateAngleX(static_cast<f32>(PI / 2.0));
    m_tail->setRotateAngleX(-0.05235988f);

    m_legBackRight->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662) * 1.4 * limbSwingAmount));
    m_legBackLeft->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + PI) * 1.4 * limbSwingAmount));
    m_legFrontRight->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + PI) * 1.4 * limbSwingAmount));
    m_legFrontLeft->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662) * 1.4 * limbSwingAmount));

    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));

    (void)ageInTicks;
}

// ==================== PandaModel ====================

PandaModel::PandaModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    setupParts(0, 0.0f);
}

PandaModel::PandaModel(i32 textureOffset, f32 scale)
    : EntityModel()
{
    setTextureSize(64, 64);
    setupParts(textureOffset, scale);
}

void PandaModel::setupParts(i32 textureOffset, f32 scale) {
    // 参考 MC 1.16.5 PandaModel (extends QuadrupedModel)
    (void)scale;
    (void)textureOffset;

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 6);
    m_head->addBox(-6.5f, -5.0f, -4.0f, 13.0f, 10.0f, 9.0f);
    m_head->setRotationPoint(0.0f, 11.5f, -17.0f);
    m_head->setTextureOffset(45, 16);
    m_head->addBox(-3.5f, 0.0f, -6.0f, 7.0f, 5.0f, 2.0f);
    m_head->setTextureOffset(52, 25);
    m_head->addBox(-8.5f, -8.0f, -1.0f, 5.0f, 4.0f, 1.0f);
    m_head->setTextureOffset(52, 25);
    m_head->addBox(3.5f, -8.0f, -1.0f, 5.0f, 4.0f, 1.0f);
    m_parts.push_back(m_head);

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 25);
    m_body->addBox(-9.5f, -13.0f, -6.5f, 19.0f, 26.0f, 13.0f);
    m_body->setRotationPoint(0.0f, 10.0f, 0.0f);
    m_parts.push_back(m_body);

    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureOffset(40, 0);
    m_legBackRight->addBox(-3.0f, 0.0f, -3.0f, 6.0f, 9.0f, 6.0f);
    m_legBackRight->setRotationPoint(-5.5f, 15.0f, 9.0f);
    m_parts.push_back(m_legBackRight);

    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureOffset(40, 0);
    m_legBackLeft->addBox(-3.0f, 0.0f, -3.0f, 6.0f, 9.0f, 6.0f);
    m_legBackLeft->setRotationPoint(5.5f, 15.0f, 9.0f);
    m_parts.push_back(m_legBackLeft);

    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureOffset(40, 0);
    m_legFrontRight->addBox(-3.0f, 0.0f, -3.0f, 6.0f, 9.0f, 6.0f);
    m_legFrontRight->setRotationPoint(-5.5f, 15.0f, -9.0f);
    m_parts.push_back(m_legFrontRight);

    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureOffset(40, 0);
    m_legFrontLeft->addBox(-3.0f, 0.0f, -3.0f, 6.0f, 9.0f, 6.0f);
    m_legFrontLeft->setRotationPoint(5.5f, 15.0f, -9.0f);
    m_parts.push_back(m_legFrontLeft);
}

void PandaModel::render(f64 scale) {
    EntityModel::render(scale);
}

void PandaModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                            f64 ageInTicks, f64 netHeadYaw,
                            f64 headPitch, f64 scale) {
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));

    f32 legSwing = static_cast<f32>(std::cos(limbSwing * 0.6662) * limbSwingAmount);
    m_legFrontRight->setRotateAngleX(-legSwing);
    m_legFrontLeft->setRotateAngleX(legSwing);
    m_legBackRight->setRotateAngleX(legSwing);
    m_legBackLeft->setRotateAngleX(-legSwing);

    (void)ageInTicks;
}

// ==================== ParrotModel ====================

ParrotModel::ParrotModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    setupParts();
}

void ParrotModel::setupParts() {
    // 参考 MC 1.16.5 ParrotModel

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(2, 8);
    m_body->addBox(-1.5f, 0.0f, -1.5f, 3.0f, 6.0f, 3.0f);
    m_body->setRotationPoint(0.0f, 16.5f, -3.0f);
    m_parts.push_back(m_body);

    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(22, 1);
    m_tail->addBox(-1.5f, -1.0f, -1.0f, 3.0f, 4.0f, 1.0f);
    m_tail->setRotationPoint(0.0f, 21.07f, 1.16f);
    m_parts.push_back(m_tail);

    m_wingLeft = std::make_shared<ModelRenderer>("wingLeft");
    m_wingLeft->setTextureOffset(19, 8);
    m_wingLeft->addBox(-0.5f, 0.0f, -1.5f, 1.0f, 5.0f, 3.0f);
    m_wingLeft->setRotationPoint(1.5f, 16.94f, -2.76f);
    m_parts.push_back(m_wingLeft);

    m_wingRight = std::make_shared<ModelRenderer>("wingRight");
    m_wingRight->setTextureOffset(19, 8);
    m_wingRight->addBox(-0.5f, 0.0f, -1.5f, 1.0f, 5.0f, 3.0f);
    m_wingRight->setRotationPoint(-1.5f, 16.94f, -2.76f);
    m_parts.push_back(m_wingRight);

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(2, 2);
    m_head->addBox(-1.0f, -1.5f, -1.0f, 2.0f, 3.0f, 2.0f);
    m_head->setRotationPoint(0.0f, 15.69f, -2.76f);
    m_parts.push_back(m_head);

    m_head2 = std::make_shared<ModelRenderer>("head2");
    m_head2->setTextureOffset(10, 0);
    m_head2->addBox(-1.0f, -0.5f, -2.0f, 2.0f, 1.0f, 4.0f);
    m_head2->setRotationPoint(0.0f, -2.0f, -1.0f);
    m_parts.push_back(m_head2);

    m_beak1 = std::make_shared<ModelRenderer>("beak1");
    m_beak1->setTextureOffset(11, 7);
    m_beak1->addBox(-0.5f, -1.0f, -0.5f, 1.0f, 2.0f, 1.0f);
    m_beak1->setRotationPoint(0.0f, -0.5f, -1.5f);
    m_parts.push_back(m_beak1);

    m_beak2 = std::make_shared<ModelRenderer>("beak2");
    m_beak2->setTextureOffset(16, 7);
    m_beak2->addBox(-0.5f, 0.0f, -0.5f, 1.0f, 2.0f, 1.0f);
    m_beak2->setRotationPoint(0.0f, -1.75f, -2.45f);
    m_parts.push_back(m_beak2);

    m_feather = std::make_shared<ModelRenderer>("feather");
    m_feather->setTextureOffset(2, 18);
    m_feather->addBox(0.0f, -4.0f, -2.0f, 0.0f, 5.0f, 4.0f);
    m_feather->setRotationPoint(0.0f, -2.15f, 0.15f);
    m_parts.push_back(m_feather);

    m_legLeft = std::make_shared<ModelRenderer>("legLeft");
    m_legLeft->setTextureOffset(14, 18);
    m_legLeft->addBox(-0.5f, 0.0f, -0.5f, 1.0f, 2.0f, 1.0f);
    m_legLeft->setRotationPoint(1.0f, 22.0f, -1.05f);
    m_parts.push_back(m_legLeft);

    m_legRight = std::make_shared<ModelRenderer>("legRight");
    m_legRight->setTextureOffset(14, 18);
    m_legRight->addBox(-0.5f, 0.0f, -0.5f, 1.0f, 2.0f, 1.0f);
    m_legRight->setRotationPoint(-1.0f, 22.0f, -1.05f);
    m_parts.push_back(m_legRight);
}

void ParrotModel::render(f64 scale) {
    EntityModel::render(scale);
}

void ParrotModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                             f64 ageInTicks, f64 netHeadYaw,
                             f64 headPitch, f64 scale) {
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));

    m_body->setRotateAngleX(0.4937f);
    m_wingLeft->setRotateAngleX(-0.6981f);
    m_wingLeft->setRotateAngleY(static_cast<f32>(-PI));
    m_wingRight->setRotateAngleX(-0.6981f);
    m_wingRight->setRotateAngleY(static_cast<f32>(-PI));

    m_legLeft->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662) * 1.4 * limbSwingAmount));
    m_legRight->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + PI) * 1.4 * limbSwingAmount));

    m_feather->setRotateAngleX(-0.2214f);

    (void)ageInTicks;
}

// ==================== PhantomModel ====================

PhantomModel::PhantomModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    setupParts();
}

void PhantomModel::setupParts() {
    // 参考 MC 1.16.5 PhantomModel

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 8);
    m_body->addBox(-3.0f, -2.0f, -8.0f, 5.0f, 3.0f, 9.0f);
    m_parts.push_back(m_body);

    m_tail1 = std::make_shared<ModelRenderer>("tail1");
    m_tail1->setTextureOffset(3, 20);
    m_tail1->addBox(-2.0f, 0.0f, 0.0f, 3.0f, 2.0f, 6.0f);
    m_tail1->setRotationPoint(0.0f, -2.0f, 1.0f);
    m_parts.push_back(m_tail1);

    m_tail2 = std::make_shared<ModelRenderer>("tail2");
    m_tail2->setTextureOffset(4, 29);
    m_tail2->addBox(-1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 6.0f);
    m_tail2->setRotationPoint(0.0f, 0.5f, 6.0f);
    m_parts.push_back(m_tail2);

    m_leftWingBody = std::make_shared<ModelRenderer>("leftWingBody");
    m_leftWingBody->setTextureOffset(23, 12);
    m_leftWingBody->addBox(0.0f, 0.0f, 0.0f, 6.0f, 2.0f, 9.0f);
    m_leftWingBody->setRotationPoint(2.0f, -2.0f, -8.0f);
    m_leftWingBody->setRotateAngleZ(0.1f);
    m_parts.push_back(m_leftWingBody);

    m_leftWing = std::make_shared<ModelRenderer>("leftWing");
    m_leftWing->setTextureOffset(16, 24);
    m_leftWing->addBox(0.0f, 0.0f, 0.0f, 13.0f, 1.0f, 9.0f);
    m_leftWing->setRotationPoint(6.0f, 0.0f, 0.0f);
    m_leftWing->setRotateAngleZ(0.1f);
    m_parts.push_back(m_leftWing);

    m_rightWingBody = std::make_shared<ModelRenderer>("rightWingBody");
    m_rightWingBody->setTextureOffset(23, 12);
    m_rightWingBody->setMirror(true);
    m_rightWingBody->addBox(-6.0f, 0.0f, 0.0f, 6.0f, 2.0f, 9.0f);
    m_rightWingBody->setRotationPoint(-3.0f, -2.0f, -8.0f);
    m_rightWingBody->setRotateAngleZ(-0.1f);
    m_parts.push_back(m_rightWingBody);

    m_rightWing = std::make_shared<ModelRenderer>("rightWing");
    m_rightWing->setTextureOffset(16, 24);
    m_rightWing->setMirror(true);
    m_rightWing->addBox(-13.0f, 0.0f, 0.0f, 13.0f, 1.0f, 9.0f);
    m_rightWing->setRotationPoint(-6.0f, 0.0f, 0.0f);
    m_rightWing->setRotateAngleZ(-0.1f);
    m_parts.push_back(m_rightWing);

    // 头部
    auto head = std::make_shared<ModelRenderer>("head");
    head->setTextureOffset(0, 0);
    head->addBox(-4.0f, -2.0f, -5.0f, 7.0f, 3.0f, 5.0f);
    head->setRotationPoint(0.0f, 1.0f, -7.0f);
    head->setRotateAngleX(0.2f);
    m_parts.push_back(head);

    m_body->setRotateAngleX(-0.1f);
}

void PhantomModel::render(f64 scale) {
    EntityModel::render(scale);
}

void PhantomModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                              f64 ageInTicks, f64 netHeadYaw,
                              f64 headPitch, f64 scale) {
    f32 f = static_cast<f32>(ageInTicks * 0.13);
    f32 wingAngle = static_cast<f32>(std::cos(f) * 16.0 * PI / 180.0);

    m_leftWingBody->setRotateAngleZ(wingAngle);
    m_leftWing->setRotateAngleZ(wingAngle);
    m_rightWingBody->setRotateAngleZ(-wingAngle);
    m_rightWing->setRotateAngleZ(-wingAngle);

    m_tail1->setRotateAngleX(static_cast<f32>(-(5.0 + std::cos(f * 2.0) * 5.0) * PI / 180.0));
    m_tail2->setRotateAngleX(static_cast<f32>(-(5.0 + std::cos(f * 2.0) * 5.0) * PI / 180.0));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
}

// ==================== RavagerModel ====================

RavagerModel::RavagerModel()
    : EntityModel()
{
    setTextureSize(128, 128);
    setupParts();
}

void RavagerModel::setupParts() {
    // 参考 MC 1.16.5 RavagerModel

    m_neck = std::make_shared<ModelRenderer>("neck");
    m_neck->setRotationPoint(0.0f, -7.0f, -1.5f);
    m_neck->setTextureOffset(68, 73);
    m_neck->addBox(-5.0f, -1.0f, -18.0f, 10.0f, 10.0f, 18.0f);
    m_parts.push_back(m_neck);

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setRotationPoint(0.0f, 16.0f, -17.0f);
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-8.0f, -20.0f, -14.0f, 16.0f, 20.0f, 16.0f);
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-2.0f, -6.0f, -18.0f, 4.0f, 8.0f, 4.0f);
    m_parts.push_back(m_head);

    // 角
    auto horn1 = std::make_shared<ModelRenderer>("horn1");
    horn1->setRotationPoint(-10.0f, -14.0f, -8.0f);
    horn1->setTextureOffset(74, 55);
    horn1->addBox(0.0f, -14.0f, -2.0f, 2.0f, 14.0f, 4.0f);
    horn1->setRotateAngleX(1.0995574f);
    m_parts.push_back(horn1);

    auto horn2 = std::make_shared<ModelRenderer>("horn2");
    horn2->setMirror(true);
    horn2->setRotationPoint(8.0f, -14.0f, -8.0f);
    horn2->setTextureOffset(74, 55);
    horn2->addBox(0.0f, -14.0f, -2.0f, 2.0f, 14.0f, 4.0f);
    horn2->setRotateAngleX(1.0995574f);
    m_parts.push_back(horn2);

    m_jaw = std::make_shared<ModelRenderer>("jaw");
    m_jaw->setRotationPoint(0.0f, -2.0f, 2.0f);
    m_jaw->setTextureOffset(0, 36);
    m_jaw->addBox(-8.0f, 0.0f, -16.0f, 16.0f, 3.0f, 16.0f);
    m_parts.push_back(m_jaw);

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 55);
    m_body->addBox(-7.0f, -10.0f, -7.0f, 14.0f, 16.0f, 20.0f);
    m_body->setTextureOffset(0, 91);
    m_body->addBox(-6.0f, 6.0f, -7.0f, 12.0f, 13.0f, 18.0f, 0.5f);
    m_body->setRotationPoint(0.0f, 1.0f, 2.0f);
    m_body->setRotateAngleX(static_cast<f32>(PI / 2.0));
    m_parts.push_back(m_body);

    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureOffset(96, 0);
    m_legBackRight->addBox(-4.0f, 0.0f, -4.0f, 8.0f, 37.0f, 8.0f);
    m_legBackRight->setRotationPoint(-8.0f, -13.0f, 18.0f);
    m_parts.push_back(m_legBackRight);

    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureOffset(96, 0);
    m_legBackLeft->setMirror(true);
    m_legBackLeft->addBox(-4.0f, 0.0f, -4.0f, 8.0f, 37.0f, 8.0f);
    m_legBackLeft->setRotationPoint(8.0f, -13.0f, 18.0f);
    m_parts.push_back(m_legBackLeft);

    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureOffset(64, 0);
    m_legFrontRight->addBox(-4.0f, 0.0f, -4.0f, 8.0f, 37.0f, 8.0f);
    m_legFrontRight->setRotationPoint(-8.0f, -13.0f, -5.0f);
    m_parts.push_back(m_legFrontRight);

    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureOffset(64, 0);
    m_legFrontLeft->setMirror(true);
    m_legFrontLeft->addBox(-4.0f, 0.0f, -4.0f, 8.0f, 37.0f, 8.0f);
    m_legFrontLeft->setRotationPoint(8.0f, -13.0f, -5.0f);
    m_parts.push_back(m_legFrontLeft);
}

void RavagerModel::render(f64 scale) {
    EntityModel::render(scale);
}

void RavagerModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                              f64 ageInTicks, f64 netHeadYaw,
                              f64 headPitch, f64 scale) {
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));

    f32 f = 0.4f * static_cast<f32>(limbSwingAmount);
    m_legBackRight->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662) * f));
    m_legBackLeft->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + PI) * f));
    m_legFrontRight->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + PI) * f));
    m_legFrontLeft->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662) * f));

    (void)ageInTicks;
}

} // namespace mc::client::renderer::entity::model::monster
