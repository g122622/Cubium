#include "BipedModel.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model {

BipedModel::BipedModel() {
    // 创建部件
    m_head = std::make_shared<ModelRenderer>("head");
    m_headwear = std::make_shared<ModelRenderer>("headwear");
    m_headOverlay = m_headwear; // 别名
    m_body = std::make_shared<ModelRenderer>("body");
    m_rightArm = std::make_shared<ModelRenderer>("rightArm");
    m_leftArm = std::make_shared<ModelRenderer>("leftArm");
    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");

    setupParts();

    // 添加到部件列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_headwear);
    m_parts.push_back(m_body);
    m_parts.push_back(m_rightArm);
    m_parts.push_back(m_leftArm);
    m_parts.push_back(m_rightLeg);
    m_parts.push_back(m_leftLeg);
}

BipedModel::BipedModel(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight) {
    setTextureSize(textureWidth, textureHeight);

    // 创建部件
    m_head = std::make_shared<ModelRenderer>("head");
    m_headwear = std::make_shared<ModelRenderer>("headwear");
    m_headOverlay = m_headwear; // 别名
    m_body = std::make_shared<ModelRenderer>("body");
    m_rightArm = std::make_shared<ModelRenderer>("rightArm");
    m_leftArm = std::make_shared<ModelRenderer>("leftArm");
    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");

    (void)scale;
    (void)yOffset;
    setupParts();

    // 添加到部件列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_headwear);
    m_parts.push_back(m_body);
    m_parts.push_back(m_rightArm);
    m_parts.push_back(m_leftArm);
    m_parts.push_back(m_rightLeg);
    m_parts.push_back(m_leftLeg);
}

void BipedModel::setupParts() {
    // 默认双足模型尺寸（玩家）

    // 头部
    m_head->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 帽子层（略大于头部）
    m_headwear->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, 0.5f);
    m_headwear->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 身体
    m_body->addBox(-4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f);
    m_body->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 右臂
    m_rightArm->addBox(-3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_rightArm->setRotationPoint(-5.0f, 2.0f, 0.0f);

    // 左臂
    m_leftArm->addBox(-1.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_leftArm->setRotationPoint(5.0f, 2.0f, 0.0f);

    // 右腿
    m_rightLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_rightLeg->setRotationPoint(-2.0f, 12.0f, 0.0f);

    // 左腿
    m_leftLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f);
    m_leftLeg->setRotationPoint(2.0f, 12.0f, 0.0f);
}

void BipedModel::render(f64 scale) {
    EntityModel::render(scale);
}

void BipedModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                            f64 /*ageInTicks*/, f64 netHeadYaw,
                            f64 headPitch, f64 /*scale*/) {
    // 头部旋转
    m_head->setRotateAngleX(math::toRadians(headPitch));
    m_head->setRotateAngleY(math::toRadians(netHeadYaw));

    // 帽子跟随头部
    m_headwear->setRotateAngleX(math::toRadians(headPitch));
    m_headwear->setRotateAngleY(math::toRadians(netHeadYaw));

    // 步态动画
    f64 walkAngle = limbSwing * math::PI;
    f64 walkAmount = limbSwingAmount;

    // 手臂摆动
    m_rightArm->setRotateAngleX(std::cos(walkAngle) * 2.0f * walkAmount);
    m_leftArm->setRotateAngleX(-std::cos(walkAngle) * 2.0f * walkAmount);

    // 腿部摆动
    m_rightLeg->setRotateAngleX(std::cos(walkAngle) * 1.4f * walkAmount);
    m_leftLeg->setRotateAngleX(-std::cos(walkAngle) * 1.4f * walkAmount);
}

} // namespace mc::client::renderer::entity::model
