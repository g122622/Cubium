#include "QuadrupedModel.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model {

QuadrupedModel::QuadrupedModel() {
    // 创建部件
    m_head = std::make_shared<ModelRenderer>("head");
    m_body = std::make_shared<ModelRenderer>("body");
    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");

    setupParts();

    // 添加到部件列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_legFrontRight);
    m_parts.push_back(m_legFrontLeft);
    m_parts.push_back(m_legBackRight);
    m_parts.push_back(m_legBackLeft);
}

void QuadrupedModel::setupParts() {
    // 默认四足动物（接近 PigModel 基础参数）
    // 参考 MC 1.16.5 QuadrupedModel(legHeight=6)

    // 头部（基础头壳）
    m_head->addBox(-4.0f, -4.0f, -8.0f, 8.0f, 8.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 12.0f, -6.0f);

    // 身体（在 setAngles 中旋转到水平）
    m_body->addBox(-5.0f, -10.0f, -7.0f, 10.0f, 16.0f, 8.0f);
    m_body->setRotationPoint(0.0f, 11.0f, 2.0f);

    // 前右腿
    m_legFrontRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f);
    m_legFrontRight->setRotationPoint(-3.0f, 18.0f, -5.0f);

    // 前左腿
    m_legFrontLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f);
    m_legFrontLeft->setRotationPoint(3.0f, 18.0f, -5.0f);

    // 后右腿
    m_legBackRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f);
    m_legBackRight->setRotationPoint(-3.0f, 18.0f, 7.0f);

    // 后左腿
    m_legBackLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f);
    m_legBackLeft->setRotationPoint(3.0f, 18.0f, 7.0f);
}

void QuadrupedModel::render(f64 scale) {
    EntityModel::render(scale);
}

void QuadrupedModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                f64 /*ageInTicks*/, f64 netHeadYaw,
                                f64 headPitch, f64 /*scale*/) {
    // 头部旋转
    m_head->setRotateAngleX(math::toRadians(headPitch));
    m_head->setRotateAngleY(math::toRadians(netHeadYaw));

    // 身体默认姿态（与 Java 版一致）
    m_body->setRotateAngleX(math::PI * 0.5f);

    // 步态动画（与 MC 1.16.5 一致）
    const f64 walkAngle = limbSwing * 0.6662f;
    const f64 walkAmount = limbSwingAmount * 1.4f;

    m_legBackRight->setRotateAngleX(std::cos(walkAngle) * walkAmount);
    m_legBackLeft->setRotateAngleX(std::cos(walkAngle + math::PI) * walkAmount);
    m_legFrontRight->setRotateAngleX(std::cos(walkAngle + math::PI) * walkAmount);
    m_legFrontLeft->setRotateAngleX(std::cos(walkAngle) * walkAmount);
}

} // namespace mc::client::renderer::entity::model
