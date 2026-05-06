#include "SquidModel.hpp"
#include "common/util/math/MathConstants.hpp"
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
    m_body->setRotationPoint(0.0f, 8.0f, 0.0f);  // Java: rotationPointY += 8.0F
    m_parts.push_back(m_body);

    // 8 条触手
    // Java:
    // double d0 = (double)j * Math.PI * 2.0D / (double)this.legs.length;
    // float f = (float)Math.cos(d0) * 5.0F;  // X 位置
    // float f1 = (float)Math.sin(d0) * 5.0F; // Z 位置
    // this.legs[j].addBox(-1.0F, 0.0F, -1.0F, 2.0F, 18.0F, 2.0F);  // 长度18
    // d0 = (double)j * Math.PI * -2.0D / (double)this.legs.length + (Math.PI / 2D);
    // this.legs[j].rotateAngleY = (float)d0;
    for (i32 i = 0; i < 8; ++i) {
        m_tentacles[i] = std::make_shared<ModelRenderer>("tentacle" + std::to_string(i));
        m_tentacles[i]->setTextureOffset(48, 0);

        // 计算位置 - 注意 cos 用于 X，sin 用于 Z
        f64 d0 = static_cast<f64>(i) * mc::math::PI * 2.0 / 8.0;
        f32 x = static_cast<f32>(std::cos(d0) * 5.0);  // Java: cos(d0) * 5.0
        f32 z = static_cast<f32>(std::sin(d0) * 5.0);  // Java: sin(d0) * 5.0

        // 触手长度为 18，不是 8
        m_tentacles[i]->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 18.0f, 2.0f);
        m_tentacles[i]->setRotationPoint(x, 15.0f, z);

        // 设置 Y 旋转 - Java 用 -2.0D 并且加上 PI/2
        f64 rotateY = static_cast<f64>(i) * mc::math::PI * -2.0 / 8.0 + (mc::math::PI / 2.0);
        m_tentacles[i]->setRotateAngleY(static_cast<f32>(rotateY));

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
    // Java: modelrenderer.rotateAngleX = ageInTicks;
    // 触手动画直接使用 ageInTicks 作为 X 旋转角度

    for (i32 i = 0; i < 8; ++i) {
        m_tentacles[i]->setRotateAngleX(static_cast<f32>(ageInTicks));
    }

    // 身体不旋转
    (void)netHeadYaw;
    (void)headPitch;
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)scale;
}

} // namespace mc::client::renderer::entity::model::animal
