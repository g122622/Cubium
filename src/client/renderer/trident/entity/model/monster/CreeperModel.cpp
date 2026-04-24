#include "CreeperModel.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

namespace {
    constexpr f64 PI = 3.14159265359;
}

CreeperModel::CreeperModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void CreeperModel::setupParts() {
    // 参考 MC 1.16.5 CreeperModel
    // 纹理尺寸：64x32

    // 头部：8x8x8，纹理位置 (0, 0)
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureSize(64, 32);
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, 0.0f);
    m_head->setRotationPoint(0.0f, 6.0f, 0.0f);
    m_parts.push_back(m_head);

    // 身体：8x12x4，纹理位置 (16, 16)
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureSize(64, 32);
    m_body->setTextureOffset(16, 16);
    m_body->addBox(-4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f, 0.0f);
    m_body->setRotationPoint(0.0f, 6.0f, 0.0f);
    m_parts.push_back(m_body);

    // 腿部：4x6x4，纹理位置 (0, 16)
    // 右前腿
    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureSize(64, 32);
    m_legFrontRight->setTextureOffset(0, 16);
    m_legFrontRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f, 0.0f);
    m_legFrontRight->setRotationPoint(-2.0f, 18.0f, 4.0f);
    m_parts.push_back(m_legFrontRight);

    // 左前腿
    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureSize(64, 32);
    m_legFrontLeft->setTextureOffset(0, 16);
    m_legFrontLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f, 0.0f);
    m_legFrontLeft->setRotationPoint(2.0f, 18.0f, 4.0f);
    m_parts.push_back(m_legFrontLeft);

    // 右后腿
    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureSize(64, 32);
    m_legBackRight->setTextureOffset(0, 16);
    m_legBackRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f, 0.0f);
    m_legBackRight->setRotationPoint(-2.0f, 18.0f, -4.0f);
    m_parts.push_back(m_legBackRight);

    // 左后腿
    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureSize(64, 32);
    m_legBackLeft->setTextureOffset(0, 16);
    m_legBackLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f, 0.0f);
    m_legBackLeft->setRotationPoint(2.0f, 18.0f, -4.0f);
    m_parts.push_back(m_legBackLeft);
}

void CreeperModel::render(f64 scale) {
    EntityModel::render(scale);
}

void CreeperModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                              f64 ageInTicks, f64 netHeadYaw,
                              f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 CreeperModel.setRotationAngles

    // 头部旋转
    // 注意：MC 使用弧度，需要转换
    m_head->setRotateAngleY(netHeadYaw * PI / 180.0);
    m_head->setRotateAngleX(headPitch * PI / 180.0);

    // 腿部动画
    // MC 1.16.5: MathHelper.cos(limbSwing * 0.6662F) * 1.4F * limbSwingAmount
    // 对角腿同步移动
    f32 legSwing = static_cast<f32>(std::cos(limbSwing * 0.6662) * 1.4 * limbSwingAmount);

    // 右前腿和左后腿
    m_legFrontRight->setRotateAngleX(legSwing);
    m_legBackLeft->setRotateAngleX(legSwing);

    // 左前腿和右后腿（相位相反）
    m_legFrontLeft->setRotateAngleX(-legSwing);
    m_legBackRight->setRotateAngleX(-legSwing);

    (void)ageInTicks;  // 苦力怕没有使用 ageInTicks 的动画
    (void)scale;       // 已在 render() 中使用
}

} // namespace mc::client::renderer::entity::model::monster
