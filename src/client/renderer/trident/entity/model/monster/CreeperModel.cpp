#include "CreeperModel.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

CreeperModel::CreeperModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts(0.0f);
}

CreeperModel::CreeperModel(f32 scale)
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts(scale);
}

void CreeperModel::setupParts(f32 scale) {
    // 参考 MC 1.16.5 CreeperModel
    // 纹理尺寸：64x32

    // 头部：8x8x8，纹理位置 (0, 0)
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureSize(64, 32);
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, scale);
    m_head->setRotationPoint(0.0f, 6.0f, 0.0f);
    m_parts.push_back(m_head);

    // 盔甲头部层（用于闪电苦力怕）：纹理位置 (32, 0)
    // 参考 MC 1.16.5: creeperArmor.addBox(-4.0F, -8.0F, -4.0F, 8.0F, 8.0F, 8.0F, scale + 0.5F);
    m_armorHead = std::make_shared<ModelRenderer>("creeperArmor");
    m_armorHead->setTextureSize(64, 32);
    m_armorHead->setTextureOffset(32, 0);
    m_armorHead->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, scale + 0.5f);
    m_armorHead->setRotationPoint(0.0f, 6.0f, 0.0f);
    // 盔甲层不加入 m_parts，单独渲染

    // 身体：8x12x4，纹理位置 (16, 16)
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureSize(64, 32);
    m_body->setTextureOffset(16, 16);
    m_body->addBox(-4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f, scale);
    m_body->setRotationPoint(0.0f, 6.0f, 0.0f);
    m_parts.push_back(m_body);

    // 腿部：4x6x4，纹理位置 (0, 16)
    // 右前腿 (leg1)
    m_legFrontRight = std::make_shared<ModelRenderer>("leg1");
    m_legFrontRight->setTextureSize(64, 32);
    m_legFrontRight->setTextureOffset(0, 16);
    m_legFrontRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f, scale);
    m_legFrontRight->setRotationPoint(-2.0f, 18.0f, 4.0f);
    m_parts.push_back(m_legFrontRight);

    // 左前腿 (leg2)
    m_legFrontLeft = std::make_shared<ModelRenderer>("leg2");
    m_legFrontLeft->setTextureSize(64, 32);
    m_legFrontLeft->setTextureOffset(0, 16);
    m_legFrontLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f, scale);
    m_legFrontLeft->setRotationPoint(2.0f, 18.0f, 4.0f);
    m_parts.push_back(m_legFrontLeft);

    // 右后腿 (leg3)
    m_legBackRight = std::make_shared<ModelRenderer>("leg3");
    m_legBackRight->setTextureSize(64, 32);
    m_legBackRight->setTextureOffset(0, 16);
    m_legBackRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f, scale);
    m_legBackRight->setRotationPoint(-2.0f, 18.0f, -4.0f);
    m_parts.push_back(m_legBackRight);

    // 左后腿 (leg4)
    m_legBackLeft = std::make_shared<ModelRenderer>("leg4");
    m_legBackLeft->setTextureSize(64, 32);
    m_legBackLeft->setTextureOffset(0, 16);
    m_legBackLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 6.0f, 4.0f, scale);
    m_legBackLeft->setRotationPoint(2.0f, 18.0f, -4.0f);
    m_parts.push_back(m_legBackLeft);
}

void CreeperModel::render(f64 scale) {
    EntityModel::render(scale);
}

void CreeperModel::renderArmor(f64 scale) {
    if (m_armorHead) {
        m_armorHead->render(scale);
    }
}

void CreeperModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                              f64 ageInTicks, f64 netHeadYaw,
                              f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 CreeperModel.setRotationAngles

    // 头部旋转
    m_head->setRotateAngleY(netHeadYaw * mc::math::PI_DOUBLE / 180.0);
    m_head->setRotateAngleX(headPitch * mc::math::PI_DOUBLE / 180.0);

    // 同步盔甲层旋转
    if (m_armorHead) {
        m_armorHead->setRotateAngleY(m_head->rotateAngleY());
        m_armorHead->setRotateAngleX(m_head->rotateAngleX());
    }

    // 腿部动画
    // MC 1.16.5:
    // leg1.rotateAngleX = MathHelper.cos(limbSwing * 0.6662F) * 1.4F * limbSwingAmount;
    // leg2.rotateAngleX = MathHelper.cos(limbSwing * 0.6662F + mc::math::PI_DOUBLE) * 1.4F * limbSwingAmount;
    // leg3.rotateAngleX = MathHelper.cos(limbSwing * 0.6662F + mc::math::PI_DOUBLE) * 1.4F * limbSwingAmount;
    // leg4.rotateAngleX = MathHelper.cos(limbSwing * 0.6662F) * 1.4F * limbSwingAmount;
    f32 legSwing1 = static_cast<f32>(std::cos(limbSwing * 0.6662) * 1.4 * limbSwingAmount);
    f32 legSwing2 = static_cast<f32>(std::cos(limbSwing * 0.6662 + mc::math::PI_DOUBLE) * 1.4 * limbSwingAmount);

    m_legFrontRight->setRotateAngleX(legSwing1);  // leg1
    m_legFrontLeft->setRotateAngleX(legSwing2);   // leg2
    m_legBackRight->setRotateAngleX(legSwing2);   // leg3
    m_legBackLeft->setRotateAngleX(legSwing1);    // leg4

    (void)ageInTicks;  // 苦力怕没有使用 ageInTicks 的动画
    (void)scale;       // 已在 render() 中使用
}

} // namespace mc::client::renderer::entity::model::monster
