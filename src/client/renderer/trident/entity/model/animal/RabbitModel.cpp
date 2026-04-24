#include "RabbitModel.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::animal {

namespace {
    constexpr f64 PI = 3.14159265359;
}

RabbitModel::RabbitModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void RabbitModel::setupParts() {
    // 参考 MC 1.16.5 RabbitModel

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-3.0f, -6.0f, -5.0f, 6.0f, 6.0f, 8.0f, 0.0f);
    m_body->setRotationPoint(0.0f, 16.0f, 5.0f);
    m_parts.push_back(m_body);

    // 头部
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(16, 0);
    m_head->addBox(-2.5f, -5.0f, -3.0f, 5.0f, 5.0f, 3.0f, 0.0f);
    m_head->setRotationPoint(0.0f, 15.0f, -1.0f);
    m_parts.push_back(m_head);

    // 右耳
    m_rightEar = std::make_shared<ModelRenderer>("rightEar");
    m_rightEar->setTextureOffset(0, 0);
    m_rightEar->addBox(-1.0f, -5.0f, 0.0f, 2.0f, 4.0f, 1.0f, 0.0f);
    m_rightEar->setRotationPoint(-1.5f, 10.0f, -2.0f);
    m_parts.push_back(m_rightEar);

    // 左耳
    m_leftEar = std::make_shared<ModelRenderer>("leftEar");
    m_leftEar->setTextureOffset(0, 0);
    m_leftEar->addBox(-1.0f, -5.0f, 0.0f, 2.0f, 4.0f, 1.0f, 0.0f);
    m_leftEar->setRotationPoint(1.5f, 10.0f, -2.0f);
    m_parts.push_back(m_leftEar);

    // 鼻子
    m_nose = std::make_shared<ModelRenderer>("nose");
    m_nose->setTextureOffset(6, 15);
    m_nose->addBox(-1.0f, -1.0f, -2.0f, 2.0f, 1.0f, 1.0f, 0.0f);
    m_nose->setRotationPoint(0.0f, 13.0f, -3.0f);
    m_parts.push_back(m_nose);

    // 尾巴
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(28, 0);
    m_tail->addBox(-1.0f, -1.0f, 0.0f, 2.0f, 2.0f, 2.0f, 0.0f);
    m_tail->setRotationPoint(0.0f, 15.0f, 12.0f);
    m_parts.push_back(m_tail);

    // 右前腿
    m_rightFrontLeg = std::make_shared<ModelRenderer>("rightFrontLeg");
    m_rightFrontLeg->setTextureOffset(8, 15);
    m_rightFrontLeg->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 4.0f, 2.0f, 0.0f);
    m_rightFrontLeg->setRotationPoint(-2.0f, 20.0f, -2.0f);
    m_parts.push_back(m_rightFrontLeg);

    // 左前腿
    m_leftFrontLeg = std::make_shared<ModelRenderer>("leftFrontLeg");
    m_leftFrontLeg->setTextureOffset(8, 15);
    m_leftFrontLeg->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 4.0f, 2.0f, 0.0f);
    m_leftFrontLeg->setRotationPoint(2.0f, 20.0f, -2.0f);
    m_parts.push_back(m_leftFrontLeg);

    // 右后腿
    m_rightBackLeg = std::make_shared<ModelRenderer>("rightBackLeg");
    m_rightBackLeg->setTextureOffset(0, 15);
    m_rightBackLeg->addBox(-1.5f, 0.0f, -1.5f, 3.0f, 6.0f, 3.0f, 0.0f);
    m_rightBackLeg->setRotationPoint(-2.0f, 18.0f, 8.0f);
    m_parts.push_back(m_rightBackLeg);

    // 左后腿
    m_leftBackLeg = std::make_shared<ModelRenderer>("leftBackLeg");
    m_leftBackLeg->setTextureOffset(0, 15);
    m_leftBackLeg->addBox(-1.5f, 0.0f, -1.5f, 3.0f, 6.0f, 3.0f, 0.0f);
    m_leftBackLeg->setRotationPoint(2.0f, 18.0f, 8.0f);
    m_parts.push_back(m_leftBackLeg);
}

void RabbitModel::render(f64 scale) {
    EntityModel::render(scale);
}

void RabbitModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                             f64 ageInTicks, f64 netHeadYaw,
                             f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 RabbitModel.setRotationAngles

    // 头部旋转
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    // 鼻子跟随头部
    m_nose->setRotateAngleY(m_head->rotateAngleY());
    m_nose->setRotateAngleX(m_head->rotateAngleX());

    // 耳朵动画
    m_rightEar->setRotateAngleZ(static_cast<f32>(std::cos(ageInTicks * 0.1) * 0.1));
    m_leftEar->setRotateAngleZ(static_cast<f32>(-std::cos(ageInTicks * 0.1) * 0.1));

    // 步态动画
    f32 walkAngle = static_cast<f32>(std::cos(limbSwing * 0.6662) * limbSwingAmount);

    // 前腿
    m_rightFrontLeg->setRotateAngleX(walkAngle);
    m_leftFrontLeg->setRotateAngleX(-walkAngle);

    // 后腿（跳跃时有不同动画）
    if (m_jumpProgress > 0.0f) {
        // 跳跃动画
        f32 jumpAngle = m_jumpProgress * PI / 2.0f;
        m_rightBackLeg->setRotateAngleX(jumpAngle);
        m_leftBackLeg->setRotateAngleX(jumpAngle);
    } else {
        m_rightBackLeg->setRotateAngleX(-walkAngle);
        m_leftBackLeg->setRotateAngleX(walkAngle);
    }

    // 尾巴摆动
    m_tail->setRotateAngleY(static_cast<f32>(std::sin(ageInTicks * 0.2) * 0.3));

    (void)scale;
}

} // namespace mc::client::renderer::entity::model::animal
