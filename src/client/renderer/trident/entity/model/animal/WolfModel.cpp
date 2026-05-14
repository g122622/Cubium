#include "WolfModel.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::animal {

WolfModel::WolfModel()
    : AgeableModel() // WolfModel 使用 AgeableModel 默认构造函数
{
    setTextureSize(64, 32);

    // 参考 MC 1.16.5 WolfModel 构造函数
    f32 f = 0.0f; // scale
    f32 f1 = 13.5f;

    // 头部 - Java: this.head.setRotationPoint(-1.0F, 13.5F, -7.0F);
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setRotationPoint(-1.0f, f1, -7.0f);

    // 头部子部件 - Java: this.headChild.addBox(-2.0F, -3.0F, -2.0F, 6.0F, 6.0F, 4.0F, 0.0F);
    m_headChild = std::make_shared<ModelRenderer>("headChild");
    m_headChild->setTextureOffset(0, 0);
    m_headChild->addBox(-2.0f, -3.0f, -2.0f, 6.0f, 6.0f, 4.0f, static_cast<f64>(f));
    m_head->addChild(m_headChild);

    // 耳朵和鼻子 - 在 headChild 上添加
    // Java: this.headChild.setTextureOffset(16, 14).addBox(-2.0F, -5.0F, 0.0F, 2.0F, 2.0F, 1.0F, 0.0F);
    auto earLeft = m_headChild->createChild("earLeft");
    earLeft->setTextureOffset(16, 14);
    earLeft->addBox(-2.0f, -5.0f, 0.0f, 2.0f, 2.0f, 1.0f, static_cast<f64>(f));

    // Java: this.headChild.setTextureOffset(16, 14).addBox(2.0F, -5.0F, 0.0F, 2.0F, 2.0F, 1.0F, 0.0F);
    auto earRight = m_headChild->createChild("earRight");
    earRight->setTextureOffset(16, 14);
    earRight->addBox(2.0f, -5.0f, 0.0f, 2.0f, 2.0f, 1.0f, static_cast<f64>(f));

    // 鼻子 - Java: this.headChild.setTextureOffset(0, 10).addBox(-0.5F, 0.0F, -5.0F, 3.0F, 3.0F, 4.0F, 0.0F);
    auto nose = m_headChild->createChild("nose");
    nose->setTextureOffset(0, 10);
    nose->addBox(-0.5f, 0.0f, -5.0f, 3.0f, 3.0f, 4.0f, static_cast<f64>(f));

    // 身体 - Java: this.body.addBox(-3.0F, -2.0F, -3.0F, 6.0F, 9.0F, 6.0F, 0.0F);
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(18, 14);
    m_body->addBox(-3.0f, -2.0f, -3.0f, 6.0f, 9.0f, 6.0f, static_cast<f64>(f));
    m_body->setRotationPoint(0.0f, 14.0f, 2.0f);

    // 鬃毛 - Java: this.mane.addBox(-3.0F, -3.0F, -3.0F, 8.0F, 6.0F, 7.0F, 0.0F);
    m_mane = std::make_shared<ModelRenderer>("mane");
    m_mane->setTextureOffset(21, 0);
    m_mane->addBox(-3.0f, -3.0f, -3.0f, 8.0f, 6.0f, 7.0f, static_cast<f64>(f));
    m_mane->setRotationPoint(-1.0f, 14.0f, 2.0f);

    // 后右腿 - Java: this.legBackRight.addBox(0.0F, 0.0F, -1.0F, 2.0F, 8.0F, 2.0F, 0.0F);
    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureOffset(0, 18);
    m_legBackRight->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, static_cast<f64>(f));
    m_legBackRight->setRotationPoint(-2.5f, 16.0f, 7.0f);

    // 后左腿
    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureOffset(0, 18);
    m_legBackLeft->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, static_cast<f64>(f));
    m_legBackLeft->setRotationPoint(0.5f, 16.0f, 7.0f);

    // 前右腿 - Java: rotationPoint(-2.5F, 16.0F, -4.0F)
    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureOffset(0, 18);
    m_legFrontRight->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, static_cast<f64>(f));
    m_legFrontRight->setRotationPoint(-2.5f, 16.0f, -4.0f);

    // 前左腿
    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureOffset(0, 18);
    m_legFrontLeft->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, static_cast<f64>(f));
    m_legFrontLeft->setRotationPoint(0.5f, 16.0f, -4.0f);

    // 尾巴 - Java: this.tail.setRotationPoint(-1.0F, 12.0F, 8.0F);
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setRotationPoint(-1.0f, 12.0f, 8.0f);

    // 尾巴子部件 - Java: this.tailChild.addBox(0.0F, 0.0F, -1.0F, 2.0F, 8.0F, 2.0F, 0.0F);
    m_tailChild = std::make_shared<ModelRenderer>("tailChild");
    m_tailChild->setTextureOffset(9, 18);
    m_tailChild->addBox(0.0f, 0.0f, -1.0f, 2.0f, 8.0f, 2.0f, static_cast<f64>(f));
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

std::vector<std::shared_ptr<ModelRenderer>> WolfModel::getHeadParts() const
{
    return {m_head};
}

std::vector<std::shared_ptr<ModelRenderer>> WolfModel::getBodyParts() const
{
    return {m_body, m_legBackRight, m_legBackLeft, m_legFrontRight, m_legFrontLeft, m_tail, m_mane};
}

void WolfModel::render(f64 scale)
{
    AgeableModel::render(scale);
}

void WolfModel::setLivingAnimations(f64 limbSwing, f64 limbSwingAmount, f64 /*partialTick*/)
{
    // 参考 MC 1.16.5 WolfModel.setLivingAnimations

    // 愤怒状态（func_233678_J__）：尾巴不摇
    if (m_isAngry) {
        m_tail->setRotateAngleY(0.0f);
    } else {
        m_tail->setRotateAngleY(static_cast<f32>(std::cos(limbSwing * 0.6662) * 1.4 * limbSwingAmount));
    }

    // 坐下状态（func_233684_eK__）
    if (m_isSitting) {
        // Java: 坐下时身体、鬃毛、尾巴和腿的位置
        m_mane->setRotationPoint(-1.0f, 16.0f, -3.0f);
        m_mane->setRotateAngleX(1.2566371f); // 约 72 度
        m_mane->setRotateAngleY(0.0f);
        m_body->setRotationPoint(0.0f, 18.0f, 0.0f);
        m_body->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 4.0)); // 45 度
        m_tail->setRotationPoint(-1.0f, 21.0f, 6.0f);
        m_legBackRight->setRotationPoint(-2.5f, 22.7f, 2.0f);
        m_legBackRight->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE * 1.5)); // 270 度
        m_legBackLeft->setRotationPoint(0.5f, 22.7f, 2.0f);
        m_legBackLeft->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE * 1.5));
        m_legFrontRight->setRotateAngleX(5.811947f); // 约 333 度
        m_legFrontRight->setRotationPoint(-2.49f, 17.0f, -4.0f);
        m_legFrontLeft->setRotateAngleX(5.811947f);
        m_legFrontLeft->setRotationPoint(0.51f, 17.0f, -4.0f);
    } else {
        // 正常站立姿态
        m_body->setRotationPoint(0.0f, 14.0f, 2.0f);
        m_body->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 2.0)); // 90 度
        m_mane->setRotationPoint(-1.0f, 14.0f, -3.0f);
        m_mane->setRotateAngleX(m_body->rotateAngleX());
        m_tail->setRotationPoint(-1.0f, 12.0f, 8.0f);
        m_legBackRight->setRotationPoint(-2.5f, 16.0f, 7.0f);
        m_legBackLeft->setRotationPoint(0.5f, 16.0f, 7.0f);
        m_legFrontRight->setRotationPoint(-2.5f, 16.0f, -4.0f);
        m_legFrontLeft->setRotationPoint(0.5f, 16.0f, -4.0f);

        // 腿部步态动画
        f32 limbSwingFloat = static_cast<f32>(limbSwing);
        f32 limbSwingAmountFloat = static_cast<f32>(limbSwingAmount);
        m_legBackRight->setRotateAngleX(
            static_cast<f32>(std::cos(limbSwingFloat * 0.6662) * 1.4 * limbSwingAmountFloat));
        m_legBackLeft->setRotateAngleX(
            static_cast<f32>(std::cos(limbSwingFloat * 0.6662 + mc::math::PI_DOUBLE) * 1.4 * limbSwingAmountFloat));
        m_legFrontRight->setRotateAngleX(
            static_cast<f32>(std::cos(limbSwingFloat * 0.6662 + mc::math::PI_DOUBLE) * 1.4 * limbSwingAmountFloat));
        m_legFrontLeft->setRotateAngleX(
            static_cast<f32>(std::cos(limbSwingFloat * 0.6662) * 1.4 * limbSwingAmountFloat));
    }

    // 摇晃动画（湿状态抖水）
    // Java: headChild.rotateAngleZ = entityIn.getInterestedAngle(partialTick) + entityIn.getShakeAngle(partialTick,
    // 0.0F);
    m_headChild->setRotateAngleZ(m_interestedAngle + m_shakeAngle);
    m_mane->setRotateAngleZ(m_shakeAngle * -0.08f);
    m_body->setRotateAngleZ(m_shakeAngle * -0.16f);
    m_tailChild->setRotateAngleZ(m_shakeAngle * -0.2f);
}

void WolfModel::setAngles(
    f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 /*scale*/)
{
    // 参考 MC 1.16.5 WolfModel.setRotationAngles
    // 注意：大部分动画在 setLivingAnimations 中处理

    // 头部旋转
    m_head->setRotateAngleX(static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0));
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0));

    // 尾巴角度（ageInTicks 用于尾巴上下摆动）
    m_tail->setRotateAngleX(static_cast<f32>(ageInTicks));
}

void WolfModel::setAnimState(
    bool isSitting, bool isAngry, bool isWet, f32 tailRotation, f32 shakeAngle, f32 interestedAngle)
{
    m_isSitting = isSitting;
    m_isAngry = isAngry;
    m_isWet = isWet;
    m_tailRotation = tailRotation;
    m_shakeAngle = shakeAngle;
    m_interestedAngle = interestedAngle;
}

} // namespace mc::client::renderer::entity::model::animal
