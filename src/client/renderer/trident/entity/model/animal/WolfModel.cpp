#include "WolfModel.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::animal {

namespace {
    constexpr f64 PI = 3.14159265359;
}

WolfModel::WolfModel()
    : AgeableModel()
{
    setTextureSize(64, 32);

    // 参考 MC 1.16.5 WolfModel 构造函数
    // 纹理尺寸：64x32

    // 头部（旋转点在 -1.0, 13.5, -7.0）
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setRotationPoint(-1.0f, 13.5f, -7.0f);

    // 头部盒子（子部件）
    m_headChild = std::make_shared<ModelRenderer>("headChild");
    m_headChild->setTextureOffset(0, 0);
    m_headChild->addBox(-2.0f, -3.0f, -2.0f, 6.0f, 6.0f, 4.0f, 0.0f);
    m_head->addChild(m_headChild);

    // 鼻子（在头部子部件上添加）
    m_nose = std::make_shared<ModelRenderer>("nose");
    m_nose->setTextureOffset(0, 10);
    m_nose->addBox(-0.5f, 0.0f, -5.0f, 3.0f, 3.0f, 4.0f, 0.0f);
    m_headChild->addChild(m_nose);

    // 耳朵（在头部子部件上添加）
    m_earLeft = std::make_shared<ModelRenderer>("earLeft");
    m_earLeft->setTextureOffset(16, 14);
    m_earLeft->addBox(-2.0f, -5.0f, 0.0f, 2.0f, 2.0f, 1.0f, 0.0f);
    m_headChild->addChild(m_earLeft);

    m_earRight = std::make_shared<ModelRenderer>("earRight");
    m_earRight->setTextureOffset(16, 14);
    m_earRight->addBox(2.0f, -5.0f, 0.0f, 2.0f, 2.0f, 1.0f, 0.0f);
    m_headChild->addChild(m_earRight);

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(18, 14);
    m_body->addBox(-3.0f, -2.0f, -3.0f, 6.0f, 9.0f, 6.0f, 0.0f);
    m_body->setRotationPoint(0.0f, 14.0f, 2.0f);

    // 鬃毛
    m_mane = std::make_shared<ModelRenderer>("mane");
    m_mane->setTextureOffset(21, 0);
    m_mane->addBox(-3.0f, -3.0f, -3.0f, 8.0f, 6.0f, 7.0f, 0.0f);
    m_mane->setRotationPoint(-1.0f, 14.0f, 2.0f);

    // 后腿
    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureOffset(0, 18);
    m_legBackRight->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, 0.0f);
    m_legBackRight->setRotationPoint(-2.5f, 16.0f, 7.0f);

    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureOffset(0, 18);
    m_legBackLeft->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, 0.0f);
    m_legBackLeft->setRotationPoint(0.5f, 16.0f, 7.0f);

    // 前腿
    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureOffset(0, 18);
    m_legFrontRight->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, 0.0f);
    m_legFrontRight->setRotationPoint(-2.5f, 16.0f, -4.0f);

    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureOffset(0, 18);
    m_legFrontLeft->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, 0.0f);
    m_legFrontLeft->setRotationPoint(0.5f, 16.0f, -4.0f);

    // 尾巴（旋转点）
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setRotationPoint(-1.0f, 12.0f, 8.0f);

    // 尾巴盒子（子部件）
    m_tailChild = std::make_shared<ModelRenderer>("tailChild");
    m_tailChild->setTextureOffset(9, 18);
    m_tailChild->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, 0.0f);
    m_tail->addChild(m_tailChild);

    // 添加所有部件到列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_mane);
    m_parts.push_back(m_legBackRight);
    m_parts.push_back(m_legBackLeft);
    m_parts.push_back(m_legFrontRight);
    m_parts.push_back(m_legFrontLeft);
    m_parts.push_back(m_tail);
}

void WolfModel::render(f64 scale) {
    // 应用着色
    // TODO: 在渲染时应用 m_tintR/G/B

    // 渲染所有部件
    for (auto& part : m_parts) {
        if (part) {
            part->render(scale);
        }
    }
}

void WolfModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                          f64 ageInTicks, f64 netHeadYaw,
                          f64 headPitch, f64 /*scale*/) {
    // 头部旋转
    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));

    // 尾巴摆动
    m_tail->setRotateAngleX(static_cast<f32>(ageInTicks));

    // 坐下状态
    if (m_isSitting) {
        // 坐下时的姿态
        m_mane->setRotationPoint(-1.0f, 16.0f, -3.0f);
        m_mane->setRotateAngleX(static_cast<f32>(PI * 0.4));  // 1.2566371F
        m_mane->setRotateAngleY(0.0f);
        m_body->setRotationPoint(0.0f, 18.0f, 0.0f);
        m_body->setRotateAngleX(static_cast<f32>(PI / 4.0));
        m_tail->setRotationPoint(-1.0f, 21.0f, 6.0f);
        m_legBackRight->setRotationPoint(-2.5f, 22.7f, 2.0f);
        m_legBackRight->setRotateAngleX(static_cast<f32>(PI * 1.5));
        m_legBackLeft->setRotationPoint(0.5f, 22.7f, 2.0f);
        m_legBackLeft->setRotateAngleX(static_cast<f32>(PI * 1.5));
        m_legFrontRight->setRotateAngleX(5.811947f);
        m_legFrontRight->setRotationPoint(-2.49f, 17.0f, -4.0f);
        m_legFrontLeft->setRotateAngleX(5.811947f);
        m_legFrontLeft->setRotationPoint(0.51f, 17.0f, -4.0f);
    } else {
        // 正常站立姿态
        m_body->setRotationPoint(0.0f, 14.0f, 2.0f);
        m_body->setRotateAngleX(static_cast<f32>(PI / 2.0));
        m_mane->setRotationPoint(-1.0f, 14.0f, -3.0f);
        m_mane->setRotateAngleX(m_body->rotateAngleX());
        m_tail->setRotationPoint(-1.0f, 12.0f, 8.0f);
        m_legBackRight->setRotationPoint(-2.5f, 16.0f, 7.0f);
        m_legBackLeft->setRotationPoint(0.5f, 16.0f, 7.0f);
        m_legFrontRight->setRotationPoint(-2.5f, 16.0f, -4.0f);
        m_legFrontLeft->setRotationPoint(0.5f, 16.0f, -4.0f);

        // 腿部动画
        f32 legSwing = static_cast<f32>(std::cos(limbSwing * 0.6662) * 1.4 * limbSwingAmount);
        m_legBackRight->setRotateAngleX(legSwing);
        m_legBackLeft->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + PI) * 1.4 * limbSwingAmount));
        m_legFrontRight->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 + PI) * 1.4 * limbSwingAmount));
        m_legFrontLeft->setRotateAngleX(legSwing);
    }

    // 尾巴摇摆（非坐下时）
    if (!m_isSitting && !m_isAngry) {
        m_tail->setRotateAngleY(static_cast<f32>(std::cos(limbSwing * 0.6662) * 1.4 * limbSwingAmount));
    } else if (m_isAngry) {
        m_tail->setRotateAngleY(0.0f);
    }

    // 摇晃动画（湿状态）
    m_headChild->setRotateAngleZ(m_interestedAngle + m_shakeAngle);
    m_mane->setRotateAngleZ(m_shakeAngle * -0.08f);
    m_body->setRotateAngleZ(m_shakeAngle * -0.16f);
    m_tailChild->setRotateAngleZ(m_shakeAngle * -0.2f);
}

void WolfModel::setAnimState(bool isSitting, bool isAngry, bool isWet,
                              f32 tailRotation, f32 shakeAngle, f32 interestedAngle) {
    m_isSitting = isSitting;
    m_isAngry = isAngry;
    m_isWet = isWet;
    m_tailRotation = tailRotation;
    m_shakeAngle = shakeAngle;
    m_interestedAngle = interestedAngle;
}

} // namespace mc::client::renderer::entity::model::animal
