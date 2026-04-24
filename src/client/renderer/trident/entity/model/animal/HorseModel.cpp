#include "HorseModel.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::animal {

namespace {
    constexpr f64 PI = 3.14159265359;
}

HorseModel::HorseModel(f32 scale)
    : AgeableModel()
{
    setTextureSize(64, 64);

    // 参考 MC 1.16.5 HorseModel
    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 32);
    m_body->addBox(-5.0f, -8.0f, -17.0f, 10.0f, 10.0f, 22.0f, 0.05f + static_cast<f64>(scale));
    m_body->setRotationPoint(0.0f, 11.0f, 5.0f);

    // 头部
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 35);
    m_head->addBox(-2.05f, -6.0f, -2.0f, 4.0f, 12.0f, 7.0f, static_cast<f64>(scale));
    m_head->setRotateAngleX(static_cast<f32>(PI / 6.0));  // 30度

    // 头部子部件 - 马鬃上部
    auto headTop = std::make_shared<ModelRenderer>("headTop");
    headTop->setTextureOffset(0, 13);
    headTop->addBox(-3.0f, -11.0f, -2.0f, 6.0f, 5.0f, 7.0f, static_cast<f64>(scale));
    m_head->addChild(headTop);

    // 马鬃
    auto mane = std::make_shared<ModelRenderer>("mane");
    mane->setTextureOffset(56, 36);
    mane->addBox(-1.0f, -11.0f, 5.01f, 2.0f, 16.0f, 2.0f, static_cast<f64>(scale));
    m_head->addChild(mane);

    // 口鼻部
    auto muzzle = std::make_shared<ModelRenderer>("muzzle");
    muzzle->setTextureOffset(0, 25);
    muzzle->addBox(-2.0f, -11.0f, -7.0f, 4.0f, 5.0f, 5.0f, static_cast<f64>(scale));
    m_head->addChild(muzzle);

    // 后腿（站立）
    m_backLeftLeg = std::make_shared<ModelRenderer>("backLeftLeg");
    m_backLeftLeg->setTextureOffset(48, 21);
    m_backLeftLeg->addBox(-3.0f, -1.01f, -1.0f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale));
    m_backLeftLeg->setRotationPoint(4.0f, 14.0f, 7.0f);

    m_backRightLeg = std::make_shared<ModelRenderer>("backRightLeg");
    m_backRightLeg->setTextureOffset(48, 21);
    m_backRightLeg->addBox(-1.0f, -1.01f, -1.0f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale));
    m_backRightLeg->setRotationPoint(-4.0f, 14.0f, 7.0f);

    // 前腿（站立）
    m_frontLeftLeg = std::make_shared<ModelRenderer>("frontLeftLeg");
    m_frontLeftLeg->setTextureOffset(48, 21);
    m_frontLeftLeg->addBox(-3.0f, -1.01f, -1.9f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale));
    m_frontLeftLeg->setRotationPoint(4.0f, 6.0f, -12.0f);

    m_frontRightLeg = std::make_shared<ModelRenderer>("frontRightLeg");
    m_frontRightLeg->setTextureOffset(48, 21);
    m_frontRightLeg->addBox(-1.0f, -1.01f, -1.9f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale));
    m_frontRightLeg->setRotationPoint(-4.0f, 6.0f, -12.0f);

    // 后腿（奔跑 - 用于幼体）
    m_backLeftLegBaby = std::make_shared<ModelRenderer>("backLeftLegBaby");
    m_backLeftLegBaby->setTextureOffset(48, 21);
    m_backLeftLegBaby->addBox(-3.0f, -1.01f, -1.0f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale + 5.5f));
    m_backLeftLegBaby->setRotationPoint(4.0f, 14.0f, 7.0f);

    m_backRightLegBaby = std::make_shared<ModelRenderer>("backRightLegBaby");
    m_backRightLegBaby->setTextureOffset(48, 21);
    m_backRightLegBaby->addBox(-1.0f, -1.01f, -1.0f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale + 5.5f));
    m_backRightLegBaby->setRotationPoint(-4.0f, 14.0f, 7.0f);

    // 前腿（奔跑 - 用于幼体）
    m_frontLeftLegBaby = std::make_shared<ModelRenderer>("frontLeftLegBaby");
    m_frontLeftLegBaby->setTextureOffset(48, 21);
    m_frontLeftLegBaby->addBox(-3.0f, -1.01f, -1.9f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale + 5.5f));
    m_frontLeftLegBaby->setRotationPoint(4.0f, 6.0f, -12.0f);

    m_frontRightLegBaby = std::make_shared<ModelRenderer>("frontRightLegBaby");
    m_frontRightLegBaby->setTextureOffset(48, 21);
    m_frontRightLegBaby->addBox(-1.0f, -1.01f, -1.9f, 4.0f, 11.0f, 4.0f, static_cast<f64>(scale + 5.5f));
    m_frontRightLegBaby->setRotationPoint(-4.0f, 6.0f, -12.0f);

    // 尾巴
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(42, 36);
    m_tail->addBox(-1.5f, 0.0f, 0.0f, 3.0f, 14.0f, 4.0f, static_cast<f64>(scale));
    m_tail->setRotationPoint(0.0f, -5.0f, 2.0f);
    m_tail->setRotateAngleX(static_cast<f32>(PI / 6.0));
    m_body->addChild(m_tail);

    // 添加所有部件到列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_backLeftLeg);
    m_parts.push_back(m_backRightLeg);
    m_parts.push_back(m_frontLeftLeg);
    m_parts.push_back(m_frontRightLeg);
}

void HorseModel::render(f64 scale) {
    for (auto& part : m_parts) {
        if (part) {
            part->render(scale);
        }
    }
}

void HorseModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                            f64 /*ageInTicks*/, f64 netHeadYaw,
                            f64 headPitch, f64 /*scale*/) {
    // 头部旋转
    m_head->setRotateAngleX(static_cast<f32>(PI / 6.0 + headPitch * PI / 180.0));
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));

    // 身体
    m_body->setRotationPointY(11.0f);

    // 腿部动画
    f32 legSwing = static_cast<f32>(std::cos(limbSwing * 0.6662) * limbSwingAmount);
    f32 legSwingAlt = static_cast<f32>(std::cos(limbSwing * 0.6662 + PI) * limbSwingAmount);

    m_backLeftLeg->setRotateAngleX(legSwing);
    m_backRightLeg->setRotateAngleX(legSwingAlt);
    m_frontLeftLeg->setRotateAngleX(legSwingAlt);
    m_frontRightLeg->setRotateAngleX(legSwing);

    // 幼体腿部（奔跑）
    m_backLeftLegBaby->setRotateAngleX(legSwing);
    m_backRightLegBaby->setRotateAngleX(legSwingAlt);
    m_frontLeftLegBaby->setRotateAngleX(legSwingAlt);
    m_frontRightLegBaby->setRotateAngleX(legSwing);

    // 鞍部件可见性
    for (auto& part : m_saddleParts) {
        if (part) {
            part->setVisible(m_saddled);
        }
    }

    // 骑乘部件可见性
    for (auto& part : m_ridingParts) {
        if (part) {
            part->setVisible(m_ridden && m_saddled);
        }
    }
}

} // namespace mc::client::renderer::entity::model::animal
