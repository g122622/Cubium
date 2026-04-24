#include "SquidModel.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::animal {

SquidModel::SquidModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void SquidModel::setupParts() {
    // 参考 MC 1.16.5 SquidModel

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-6.0f, -8.0f, -6.0f, 12.0f, 16.0f, 12.0f);
    m_body->setRotationPoint(0.0f, 8.0f, 0.0f);
    m_parts.push_back(m_body);

    // 8 条触手
    for (i32 i = 0; i < 8; ++i) {
        m_tentacles[i] = std::make_shared<ModelRenderer>("tentacle" + std::to_string(i));
        m_tentacles[i]->setTextureOffset(48, 0);
        m_tentacles[i]->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f);

        // 触手围绕身体底部排列
        f64 angle = i * PI / 4.0;
        f32 x = static_cast<f32>(std::sin(angle) * 5.0);
        f32 z = static_cast<f32>(std::cos(angle) * 5.0);
        m_tentacles[i]->setRotationPoint(x, 15.0f, z);
        m_parts.push_back(m_tentacles[i]);
    }
}

void SquidModel::render(f64 scale) {
    EntityModel::render(scale);
}

void SquidModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                            f64 ageInTicks, f64 netHeadYaw,
                            f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 SquidModel.setRotationAngles

    // 身体旋转
    m_body->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));
    m_body->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));

    // 触手摆动动画
    for (i32 i = 0; i < 8; ++i) {
        f32 phase = static_cast<f32>(i * PI / 4.0);
        f32 tentacleAngle = static_cast<f32>(std::sin(ageInTicks * 0.3 + phase) * 0.5);
        m_tentacles[i]->setRotateAngleX(tentacleAngle);
        m_tentacles[i]->setRotateAngleZ(static_cast<f32>(std::cos(ageInTicks * 0.2 + phase) * 0.3));
    }

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)scale;
}

} // namespace mc::client::renderer::entity::model::animal
