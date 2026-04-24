#include "AnimalModels.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::animal {

// ==================== PigModel ====================

PigModel::PigModel()
    : QuadrupedModel()
{
    // 在基础四足模型上追加猪鼻子
    m_head->setTextureOffset(16, 16).addBox(-2.0f, 0.0f, -9.0f, 4.0f, 3.0f, 1.0f);

    m_textureWidth = 64;
    m_textureHeight = 32;
}

void PigModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                          f64 ageInTicks, f64 netHeadYaw,
                          f64 headPitch, f64 scale) {
    // 调用基类动画
    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    (void)scale;
    (void)ageInTicks;
}

// ==================== CowModel ====================

CowModel::CowModel()
    : QuadrupedModel()
{
    // 参考 MC 1.16.5 CowModel：重建头、身、腿（避免继承基础6格腿）
    m_head = std::make_shared<ModelRenderer>("head");
    m_body = std::make_shared<ModelRenderer>("body");

    m_head->addBox(-4.0f, -4.0f, -6.0f, 8.0f, 8.0f, 6.0f);
    m_head->setRotationPoint(0.0f, 4.0f, -8.0f);
    m_head->setTextureOffset(22, 0).addBox(-5.0f, -5.0f, -4.0f, 1.0f, 3.0f, 1.0f);
    m_head->setTextureOffset(22, 0).addBox(4.0f, -5.0f, -4.0f, 1.0f, 3.0f, 1.0f);

    m_body->setTextureOffset(18, 4).addBox(-6.0f, -10.0f, -7.0f, 12.0f, 18.0f, 10.0f);
    m_body->setRotationPoint(0.0f, 5.0f, 2.0f);
    m_body->setTextureOffset(52, 0).addBox(-2.0f, 2.0f, -8.0f, 4.0f, 6.0f, 1.0f);

    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureOffset(0, 16).addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_legFrontRight->setRotationPoint(-4.0f, 12.0f, -6.0f);

    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureOffset(0, 16).addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_legFrontLeft->setRotationPoint(4.0f, 12.0f, -6.0f);

    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureOffset(0, 16).addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_legBackRight->setRotationPoint(-4.0f, 12.0f, 7.0f);

    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureOffset(0, 16).addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_legBackLeft->setRotationPoint(4.0f, 12.0f, 7.0f);

    m_parts.clear();
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_legFrontRight);
    m_parts.push_back(m_legFrontLeft);
    m_parts.push_back(m_legBackRight);
    m_parts.push_back(m_legBackLeft);

    m_textureWidth = 64;
    m_textureHeight = 32;
}

void CowModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                          f64 ageInTicks, f64 netHeadYaw,
                          f64 headPitch, f64 scale) {
    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    (void)scale;
    (void)ageInTicks;
}

// ==================== SheepModel ====================

SheepModel::SheepModel()
    : QuadrupedModel()
{
    // 参考 MC 1.16.5 SheepModel：重建头、身、腿
    m_head = std::make_shared<ModelRenderer>("head");
    m_body = std::make_shared<ModelRenderer>("body");

    m_head->addBox(-3.0f, -4.0f, -6.0f, 6.0f, 6.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 6.0f, -8.0f);

    m_body->setTextureOffset(28, 8).addBox(-4.0f, -10.0f, -7.0f, 8.0f, 16.0f, 6.0f);
    m_body->setRotationPoint(0.0f, 5.0f, 2.0f);

    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureOffset(0, 16).addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_legFrontRight->setRotationPoint(-3.0f, 12.0f, -5.0f);

    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureOffset(0, 16).addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_legFrontLeft->setRotationPoint(3.0f, 12.0f, -5.0f);

    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureOffset(0, 16).addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_legBackRight->setRotationPoint(-3.0f, 12.0f, 7.0f);

    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureOffset(0, 16).addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_legBackLeft->setRotationPoint(3.0f, 12.0f, 7.0f);

    m_parts.clear();
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_legFrontRight);
    m_parts.push_back(m_legFrontLeft);
    m_parts.push_back(m_legBackRight);
    m_parts.push_back(m_legBackLeft);

    m_textureWidth = 64;
    m_textureHeight = 32;
}

void SheepModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                            f64 ageInTicks, f64 netHeadYaw,
                            f64 headPitch, f64 scale) {
    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    (void)scale;
    (void)ageInTicks;
}

// ==================== ChickenModel ====================

ChickenModel::ChickenModel()
    : EntityModel()
{
    // 参考 MC 1.16.5 ChickenModel
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-2.0f, -6.0f, -2.0f, 4.0f, 6.0f, 3.0f);
    m_head->setRotationPoint(0.0f, 15.0f, -4.0f);

    // 喙（bill）
    m_beak = std::make_shared<ModelRenderer>("beak");
    m_beak->setTextureOffset(14, 0);
    m_beak->addBox(-2.0f, -4.0f, -4.0f, 4.0f, 2.0f, 2.0f);
    m_beak->setRotationPoint(0.0f, 15.0f, -4.0f);

    // 肉垂（chin）
    m_wattle = std::make_shared<ModelRenderer>("wattle");
    m_wattle->setTextureOffset(14, 4);
    m_wattle->addBox(-1.0f, -2.0f, -3.0f, 2.0f, 2.0f, 2.0f);
    m_wattle->setRotationPoint(0.0f, 15.0f, -4.0f);

    m_comb.reset();

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 9);
    m_body->addBox(-3.0f, -4.0f, -3.0f, 6.0f, 8.0f, 6.0f);
    m_body->setRotationPoint(0.0f, 16.0f, 0.0f);

    // 右翼
    m_rightWing = std::make_shared<ModelRenderer>("rightWing");
    m_rightWing->setTextureOffset(24, 13);
    m_rightWing->addBox(0.0f, 0.0f, -3.0f, 1.0f, 4.0f, 6.0f);
    m_rightWing->setRotationPoint(-4.0f, 13.0f, 0.0f);

    // 左翼
    m_leftWing = std::make_shared<ModelRenderer>("leftWing");
    m_leftWing->setTextureOffset(24, 13);
    m_leftWing->addBox(-1.0f, 0.0f, -3.0f, 1.0f, 4.0f, 6.0f);
    m_leftWing->setRotationPoint(4.0f, 13.0f, 0.0f);

    // 右腿
    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_rightLeg->setTextureOffset(26, 0);
    m_rightLeg->addBox(-1.0f, 0.0f, -3.0f, 3.0f, 5.0f, 3.0f);
    m_rightLeg->setRotationPoint(-2.0f, 19.0f, 1.0f);

    // 左腿
    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");
    m_leftLeg->setTextureOffset(26, 0);
    m_leftLeg->addBox(-1.0f, 0.0f, -3.0f, 3.0f, 5.0f, 3.0f);
    m_leftLeg->setRotationPoint(1.0f, 19.0f, 1.0f);

    // 添加到部件列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_beak);
    m_parts.push_back(m_wattle);
    m_parts.push_back(m_body);
    m_parts.push_back(m_rightWing);
    m_parts.push_back(m_leftWing);
    m_parts.push_back(m_rightLeg);
    m_parts.push_back(m_leftLeg);

    m_textureWidth = 64;
    m_textureHeight = 32;
}

void ChickenModel::render(f64 scale) {
    EntityModel::render(scale);
}

void ChickenModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                              f64 ageInTicks, f64 netHeadYaw,
                              f64 headPitch, f64 scale) {
    // 头部旋转
    m_head->setRotateAngleX(math::toRadians(headPitch));
    m_head->setRotateAngleY(math::toRadians(netHeadYaw));

    // 喙、肉垂、鸡冠跟随头部
    m_beak->setRotateAngleX(m_head->rotateAngleX());
    m_beak->setRotateAngleY(m_head->rotateAngleY());
    m_wattle->setRotateAngleX(m_head->rotateAngleX());
    m_wattle->setRotateAngleY(m_head->rotateAngleY());
    if (m_comb) {
        m_comb->setRotateAngleX(m_head->rotateAngleX());
        m_comb->setRotateAngleY(m_head->rotateAngleY());
    }

    // 身体基础姿态（水平）
    m_body->setRotateAngleX(math::PI * 0.5f);

    // 步态动画
    const f64 walkAngle = limbSwing * 0.6662f;
    const f64 walkAmount = limbSwingAmount * 1.4f;

    // 腿部动画
    m_rightLeg->setRotateAngleX(std::cos(walkAngle) * walkAmount);
    m_leftLeg->setRotateAngleX(std::cos(walkAngle + math::PI) * walkAmount);

    // 翅膀动画（与原版一致，按年龄tick摆动）
    m_rightWing->setRotateAngleZ(ageInTicks);
    m_leftWing->setRotateAngleZ(-ageInTicks);

    (void)scale;
}

} // namespace mc::client::renderer::entity::model::animal
