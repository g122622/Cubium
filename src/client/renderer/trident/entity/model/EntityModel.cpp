#include "EntityModel.hpp"
#include "../../../../../common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer {

// ==================== EntityModel ====================

void EntityModel::render(f64 scale) {
    for (auto& part : m_parts) {
        if (part) {
            part->render(scale);
        }
    }
}

void EntityModel::setAngles(f64 /*limbSwing*/, f64 /*limbSwingAmount*/,
                            f64 /*ageInTicks*/, f64 /*netHeadYaw*/,
                            f64 /*headPitch*/, f64 /*scale*/) {
    // 基类不实现动画
}

void EntityModel::copyAnglesTo(const EntityModel& /*target*/) const {
    // TODO: 实现角度复制
}

void EntityModel::generateMesh(std::vector<ModelVertex>& vertices,
                                std::vector<u32>& indices,
                                f64 scale) const {
    for (const auto& part : m_parts) {
        if (part) {
            part->generateMesh(vertices, indices, scale);
        }
    }
}

// ==================== QuadrupedModel ====================

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

// ==================== BipedModel ====================

BipedModel::BipedModel() {
    // 创建部件
    m_head = std::make_shared<ModelRenderer>("head");
    m_headwear = std::make_shared<ModelRenderer>("headwear");
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

} // namespace mc::client::renderer
