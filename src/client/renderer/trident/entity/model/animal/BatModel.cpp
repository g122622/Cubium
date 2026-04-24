#include "BatModel.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::animal {

namespace {
    constexpr f64 PI = 3.14159265359;
}

BatModel::BatModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void BatModel::setupParts() {
    // 参考 MC 1.16.5 BatModel

    // 头部
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-3.0f, -3.0f, -3.0f, 6.0f, 6.0f, 6.0f);
    m_head->setRotationPoint(0.0f, 14.0f, -6.0f);
    m_parts.push_back(m_head);

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(24, 0);
    m_body->addBox(-3.0f, -4.0f, -3.0f, 6.0f, 8.0f, 6.0f);
    m_body->setRotationPoint(0.0f, 14.0f, 0.0f);
    m_parts.push_back(m_body);

    // 右翼
    m_rightWing = std::make_shared<ModelRenderer>("rightWing");
    m_rightWing->setTextureOffset(0, 12);
    m_rightWing->addBox(-6.0f, 0.0f, 0.0f, 6.0f, 6.0f, 1.0f);
    m_rightWing->setRotationPoint(-3.0f, 14.0f, 0.0f);
    m_parts.push_back(m_rightWing);

    // 左翼
    m_leftWing = std::make_shared<ModelRenderer>("leftWing");
    m_leftWing->setTextureOffset(0, 12);
    m_leftWing->addBox(0.0f, 0.0f, 0.0f, 6.0f, 6.0f, 1.0f);
    m_leftWing->setRotationPoint(3.0f, 14.0f, 0.0f);
    m_parts.push_back(m_leftWing);

    // 右外翼
    m_outerRightWing = std::make_shared<ModelRenderer>("outerRightWing");
    m_outerRightWing->setTextureOffset(14, 12);
    m_outerRightWing->addBox(-8.0f, -2.0f, 0.0f, 8.0f, 2.0f, 1.0f);
    m_outerRightWing->setRotationPoint(-6.0f, 4.0f, 0.0f);
    m_rightWing->addChild(m_outerRightWing);

    // 左外翼
    m_outerLeftWing = std::make_shared<ModelRenderer>("outerLeftWing");
    m_outerLeftWing->setTextureOffset(14, 12);
    m_outerLeftWing->addBox(0.0f, -2.0f, 0.0f, 8.0f, 2.0f, 1.0f);
    m_outerLeftWing->setRotationPoint(6.0f, 4.0f, 0.0f);
    m_leftWing->addChild(m_outerLeftWing);
}

void BatModel::render(f64 scale) {
    EntityModel::render(scale);
}

void BatModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                          f64 ageInTicks, f64 netHeadYaw,
                          f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 BatModel.setRotationAngles

    // 头部旋转
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    // 翅膀扑动动画
    // MC: MathHelper.cos(ageInTicks * 0.1F) * 0.8F
    f32 wingAngle = static_cast<f32>(std::cos(ageInTicks * 0.1) * 0.8);

    // 身体轻微上下摆动
    m_body->setRotationPointY(14.0f + static_cast<f32>(std::sin(ageInTicks * 0.1) * 0.5));

    // 右翼旋转
    m_rightWing->setRotateAngleZ(static_cast<f32>(PI / 4.0) + wingAngle);
    m_leftWing->setRotateAngleZ(static_cast<f32>(-PI / 4.0) - wingAngle);

    // 外翼跟随
    m_outerRightWing->setRotateAngleZ(-0.2f);
    m_outerLeftWing->setRotateAngleZ(0.2f);

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)scale;
}

} // namespace mc::client::renderer::entity::model::animal
