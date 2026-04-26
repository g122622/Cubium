#include "OcelotModel.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::animal {

namespace {
    constexpr f64 PI = 3.14159265359;
}

OcelotModel::OcelotModel(f32 scale)
    : AgeableModel()
{
    setTextureSize(64, 32);

    // 参考 MC 1.16.5 OcelotModel
    // 头部
    m_head = std::make_shared<ModelRenderer>("head");
    // 主头部盒子 (textureOffset 0, 0)
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-2.5f, -2.0f, -3.0f, 5.0f, 4.0f, 5.0f, static_cast<f64>(scale));
    // 鼻子 (textureOffset 0, 24)
    m_head->setTextureOffset(0, 24);
    m_head->addBox(-1.5f, 0.0f, -4.0f, 3.0f, 2.0f, 2.0f, static_cast<f64>(scale));
    // 左耳 (textureOffset 0, 10)
    m_head->setTextureOffset(0, 10);
    m_head->addBox(-2.0f, -3.0f, 0.0f, 1.0f, 1.0f, 2.0f, static_cast<f64>(scale));
    // 右耳 (textureOffset 6, 10)
    m_head->setTextureOffset(6, 10);
    m_head->addBox(1.0f, -3.0f, 0.0f, 1.0f, 1.0f, 2.0f, static_cast<f64>(scale));
    m_head->setRotationPoint(0.0f, 15.0f, -9.0f);

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(20, 0);
    m_body->addBox(-2.0f, 3.0f, -8.0f, 4.0f, 16.0f, 6.0f, static_cast<f64>(scale));
    m_body->setRotationPoint(0.0f, 12.0f, -10.0f);

    // 尾巴1
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(0, 15);
    m_tail->addBox(-0.5f, 0.0f, 0.0f, 1.0f, 8.0f, 1.0f, static_cast<f64>(scale));
    m_tail->setRotateAngleX(0.9f);
    m_tail->setRotationPoint(0.0f, 15.0f, 8.0f);

    // 尾巴2
    m_tail2 = std::make_shared<ModelRenderer>("tail2");
    m_tail2->setTextureOffset(4, 15);
    m_tail2->addBox(-0.5f, 0.0f, 0.0f, 1.0f, 8.0f, 1.0f, static_cast<f64>(scale));
    m_tail2->setRotationPoint(0.0f, 20.0f, 14.0f);

    // 后左腿
    m_backLeftLeg = std::make_shared<ModelRenderer>("backLeftLeg");
    m_backLeftLeg->setTextureOffset(8, 13);
    m_backLeftLeg->addBox(-1.0f, 0.0f, 1.0f, 2.0f, 6.0f, 2.0f, static_cast<f64>(scale));
    m_backLeftLeg->setRotationPoint(1.1f, 18.0f, 5.0f);

    // 后右腿
    m_backRightLeg = std::make_shared<ModelRenderer>("backRightLeg");
    m_backRightLeg->setTextureOffset(8, 13);
    m_backRightLeg->addBox(-1.0f, 0.0f, 1.0f, 2.0f, 6.0f, 2.0f, static_cast<f64>(scale));
    m_backRightLeg->setRotationPoint(-1.1f, 18.0f, 5.0f);

    // 前左腿
    m_frontLeftLeg = std::make_shared<ModelRenderer>("frontLeftLeg");
    m_frontLeftLeg->setTextureOffset(40, 0);
    m_frontLeftLeg->addBox(-1.0f, 0.0f, 0.0f, 2.0f, 10.0f, 2.0f, static_cast<f64>(scale));
    m_frontLeftLeg->setRotationPoint(1.2f, 14.1f, -5.0f);

    // 前右腿
    m_frontRightLeg = std::make_shared<ModelRenderer>("frontRightLeg");
    m_frontRightLeg->setTextureOffset(40, 0);
    m_frontRightLeg->addBox(-1.0f, 0.0f, 0.0f, 2.0f, 10.0f, 2.0f, static_cast<f64>(scale));
    m_frontRightLeg->setRotationPoint(-1.2f, 14.1f, -5.0f);

    // 添加所有部件到列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_backLeftLeg);
    m_parts.push_back(m_backRightLeg);
    m_parts.push_back(m_frontLeftLeg);
    m_parts.push_back(m_frontRightLeg);
    m_parts.push_back(m_tail);
    m_parts.push_back(m_tail2);
}

void OcelotModel::render(f64 scale) {
    for (auto& part : m_parts) {
        if (part) {
            part->render(scale);
        }
    }
}

void OcelotModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                            f64 ageInTicks, f64 netHeadYaw,
                            f64 headPitch, f64 /*scale*/) {
    // 头部旋转
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));

    if (m_state != 3) {
        // 身体水平
        m_body->setRotateAngleX(static_cast<f32>(PI / 2.0));

        if (m_state == 2) {
            // 奔跑动画
            m_backLeftLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662) * limbSwingAmount));
            m_backRightLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + 0.3) * limbSwingAmount));
            m_frontLeftLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + PI + 0.3) * limbSwingAmount));
            m_frontRightLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + PI) * limbSwingAmount));
            m_tail2->setRotateAngleX(static_cast<f32>(1.7278761 + (PI / 10.0) * std::cos(limbSwing) * limbSwingAmount));
        } else {
            // 行走动画
            m_backLeftLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662) * limbSwingAmount));
            m_backRightLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + PI) * limbSwingAmount));
            m_frontLeftLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + PI) * limbSwingAmount));
            m_frontRightLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662) * limbSwingAmount));

            // 参考 MC 1.16.5 OcelotModel.setRotationAngles
            // state==1 时尾巴角度使用 PI/4，其他情况（state==0 或默认）使用 0.47123894
            if (m_state == 1) {
                m_tail2->setRotateAngleX(static_cast<f32>(1.7278761 + (PI / 4.0) * std::cos(limbSwing) * limbSwingAmount));
            } else {
                m_tail2->setRotateAngleX(static_cast<f32>(1.7278761 + 0.47123894 * std::cos(limbSwing) * limbSwingAmount));
            }
        }
    }
}

} // namespace mc::client::renderer::entity::model::animal
