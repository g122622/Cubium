#include "BipedModel.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model {

namespace {
    constexpr f64 PI = 3.14159265359;
}

BipedModel::BipedModel() {
    setTextureSize(64, 32);
    m_modelScale = 0.0f;
    m_yOffset = 0.0f;

    // 创建部件
    m_bipedHead = std::make_shared<ModelRenderer>("bipedHead");
    m_bipedHeadwear = std::make_shared<ModelRenderer>("bipedHeadwear");
    m_bipedBody = std::make_shared<ModelRenderer>("bipedBody");
    m_bipedRightArm = std::make_shared<ModelRenderer>("bipedRightArm");
    m_bipedLeftArm = std::make_shared<ModelRenderer>("bipedLeftArm");
    m_bipedRightLeg = std::make_shared<ModelRenderer>("bipedRightLeg");
    m_bipedLeftLeg = std::make_shared<ModelRenderer>("bipedLeftLeg");

    setupParts();

    // 添加到部件列表
    m_parts.push_back(m_bipedHead);
    m_parts.push_back(m_bipedHeadwear);
    m_parts.push_back(m_bipedBody);
    m_parts.push_back(m_bipedRightArm);
    m_parts.push_back(m_bipedLeftArm);
    m_parts.push_back(m_bipedRightLeg);
    m_parts.push_back(m_bipedLeftLeg);
}

BipedModel::BipedModel(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight) {
    setTextureSize(textureWidth, textureHeight);
    m_modelScale = scale;
    m_yOffset = yOffset;

    // 创建部件
    m_bipedHead = std::make_shared<ModelRenderer>("bipedHead");
    m_bipedHeadwear = std::make_shared<ModelRenderer>("bipedHeadwear");
    m_bipedBody = std::make_shared<ModelRenderer>("bipedBody");
    m_bipedRightArm = std::make_shared<ModelRenderer>("bipedRightArm");
    m_bipedLeftArm = std::make_shared<ModelRenderer>("bipedLeftArm");
    m_bipedRightLeg = std::make_shared<ModelRenderer>("bipedRightLeg");
    m_bipedLeftLeg = std::make_shared<ModelRenderer>("bipedLeftLeg");

    setupParts();

    // 添加到部件列表
    m_parts.push_back(m_bipedHead);
    m_parts.push_back(m_bipedHeadwear);
    m_parts.push_back(m_bipedBody);
    m_parts.push_back(m_bipedRightArm);
    m_parts.push_back(m_bipedLeftArm);
    m_parts.push_back(m_bipedRightLeg);
    m_parts.push_back(m_bipedLeftLeg);
}

void BipedModel::setupParts() {
    // 参考 MC 1.16.5 BipedModel 构造函数
    // Java: super(true, 16.0F, 0.0F, 2.0F, 2.0F, 24.0F) - AgeableModel 参数

    // 头部 - Java: tex (0, 0)
    m_bipedHead->setTextureOffset(0, 0);
    m_bipedHead->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, m_modelScale);
    m_bipedHead->setRotationPoint(0.0f, 0.0f + m_yOffset, 0.0f);

    // 帽子层 - Java: tex (32, 0), scale + 0.5F
    m_bipedHeadwear->setTextureOffset(32, 0);
    m_bipedHeadwear->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f, m_modelScale + 0.5f);
    m_bipedHeadwear->setRotationPoint(0.0f, 0.0f + m_yOffset, 0.0f);

    // 身体 - Java: tex (16, 16)
    m_bipedBody->setTextureOffset(16, 16);
    m_bipedBody->addBox(-4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f, m_modelScale);
    m_bipedBody->setRotationPoint(0.0f, 0.0f + m_yOffset, 0.0f);

    // 右臂 - Java: tex (40, 16)
    m_bipedRightArm->setTextureOffset(40, 16);
    m_bipedRightArm->addBox(-3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, m_modelScale);
    m_bipedRightArm->setRotationPoint(-5.0f, 2.0f + m_yOffset, 0.0f);

    // 左臂 - Java: tex (40, 16), mirror=true
    m_bipedLeftArm->setTextureOffset(40, 16);
    m_bipedLeftArm->setMirror(true);
    m_bipedLeftArm->addBox(-1.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, m_modelScale);
    m_bipedLeftArm->setRotationPoint(5.0f, 2.0f + m_yOffset, 0.0f);

    // 右腿 - Java: tex (0, 16)
    // 注意: X 偏移是 -1.9F，不是 -2.0F
    m_bipedRightLeg->setTextureOffset(0, 16);
    m_bipedRightLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, m_modelScale);
    m_bipedRightLeg->setRotationPoint(-1.9f, 12.0f + m_yOffset, 0.0f);

    // 左腿 - Java: tex (0, 16), mirror=true
    // 注意: X 偏移是 1.9F，不是 2.0F
    m_bipedLeftLeg->setTextureOffset(0, 16);
    m_bipedLeftLeg->setMirror(true);
    m_bipedLeftLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, m_modelScale);
    m_bipedLeftLeg->setRotationPoint(1.9f, 12.0f + m_yOffset, 0.0f);
}

void BipedModel::render(f64 scale) {
    EntityModel::render(scale);
}

void BipedModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                            f64 ageInTicks, f64 netHeadYaw,
                            f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 BipedModel.setRotationAngles

    // 头部旋转
    f32 headYawRad = static_cast<f32>(netHeadYaw * PI / 180.0);
    f32 headPitchRad = static_cast<f32>(headPitch * PI / 180.0);

    m_bipedHead->setRotateAngleY(headYawRad);
    m_bipedHead->setRotateAngleX(headPitchRad);

    // 帽子跟随头部
    m_bipedHeadwear->setRotateAngleY(headYawRad);
    m_bipedHeadwear->setRotateAngleX(headPitchRad);

    // 身体旋转
    m_bipedBody->setRotateAngleY(0.0f);

    // 重置手臂旋转点
    m_bipedRightArm->setRotationPointZ(0.0f);
    m_bipedRightArm->setRotationPointX(-5.0f);
    m_bipedLeftArm->setRotationPointZ(0.0f);
    m_bipedLeftArm->setRotationPointX(5.0f);

    // 速度因子
    f32 f = 1.0f;

    // 步态动画
    // Java: MathHelper.cos(limbSwing * 0.6662F + (float)Math.PI) * 2.0F * limbSwingAmount * 0.5F / f
    f32 limbSwingFloat = static_cast<f32>(limbSwing);
    f32 limbSwingAmountFloat = static_cast<f32>(limbSwingAmount);

    m_bipedRightArm->setRotateAngleX(static_cast<f32>(std::cos(limbSwingFloat * 0.6662f + PI) * 2.0 * limbSwingAmountFloat * 0.5 / f));
    m_bipedLeftArm->setRotateAngleX(static_cast<f32>(std::cos(limbSwingFloat * 0.6662f) * 2.0 * limbSwingAmountFloat * 0.5 / f));

    m_bipedRightArm->setRotateAngleZ(0.0f);
    m_bipedLeftArm->setRotateAngleZ(0.0f);

    // 腿部动画 - 注意右腿是 cos(limbSwing)，左腿是 cos(limbSwing + PI)
    m_bipedRightLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwingFloat * 0.6662f) * 1.4 * limbSwingAmountFloat / f));
    m_bipedLeftLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwingFloat * 0.6662f + PI) * 1.4 * limbSwingAmountFloat / f));

    m_bipedRightLeg->setRotateAngleY(0.0f);
    m_bipedLeftLeg->setRotateAngleY(0.0f);
    m_bipedRightLeg->setRotateAngleZ(0.0f);
    m_bipedLeftLeg->setRotateAngleZ(0.0f);

    // 坐姿
    if (m_isSitting) {
        m_bipedRightArm->setRotateAngleX(m_bipedRightArm->rotateAngleX() + static_cast<f32>(-PI / 5.0));
        m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() + static_cast<f32>(-PI / 5.0));
        m_bipedRightLeg->setRotateAngleX(-1.4137167f);
        m_bipedRightLeg->setRotateAngleY(static_cast<f32>(PI / 10.0));
        m_bipedRightLeg->setRotateAngleZ(0.07853982f);
        m_bipedLeftLeg->setRotateAngleX(-1.4137167f);
        m_bipedLeftLeg->setRotateAngleY(static_cast<f32>(-PI / 10.0));
        m_bipedLeftLeg->setRotateAngleZ(-0.07853982f);
    }

    // 手臂姿态
    m_bipedRightArm->setRotateAngleY(0.0f);
    m_bipedLeftArm->setRotateAngleY(0.0f);

    // 先处理右手姿态，再处理左手姿态
    handleRightArmPose();
    handleLeftArmPose();

    // 挥动动画
    handleSwingAnimation(ageInTicks);

    // 蹲伏
    if (m_isSneaking) {
        m_bipedBody->setRotateAngleX(0.5f);
        m_bipedRightArm->setRotateAngleX(m_bipedRightArm->rotateAngleX() + 0.4f);
        m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() + 0.4f);
        m_bipedRightLeg->setRotationPointZ(4.0f);
        m_bipedLeftLeg->setRotationPointZ(4.0f);
        m_bipedRightLeg->setRotationPointY(12.2f);
        m_bipedLeftLeg->setRotationPointY(12.2f);
        m_bipedHead->setRotationPointY(4.2f);
        m_bipedBody->setRotationPointY(3.2f);
        m_bipedLeftArm->setRotationPointY(5.2f);
        m_bipedRightArm->setRotationPointY(5.2f);
    } else {
        m_bipedBody->setRotateAngleX(0.0f);
        m_bipedRightLeg->setRotationPointZ(0.1f);
        m_bipedLeftLeg->setRotationPointZ(0.1f);
        m_bipedRightLeg->setRotationPointY(12.0f);
        m_bipedLeftLeg->setRotationPointY(12.0f);
        m_bipedHead->setRotationPointY(0.0f + m_yOffset);
        m_bipedBody->setRotationPointY(0.0f + m_yOffset);
        m_bipedLeftArm->setRotationPointY(2.0f + m_yOffset);
        m_bipedRightArm->setRotationPointY(2.0f + m_yOffset);
    }

    (void)scale;
}

void BipedModel::handleRightArmPose() {
    // 参考 MC 1.16.5 BipedModel.func_241654_b_
    switch (m_rightArmPose) {
        case ArmPose::Empty:
            m_bipedRightArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::Block:
            // Java: rotateAngleX * 0.5F - 0.9424779F
            m_bipedRightArm->setRotateAngleX(m_bipedRightArm->rotateAngleX() * 0.5f - 0.9424779f);
            m_bipedRightArm->setRotateAngleY(static_cast<f32>(-PI / 6.0));
            break;
        case ArmPose::Item:
            // Java: rotateAngleX * 0.5F - PI / 10F
            m_bipedRightArm->setRotateAngleX(m_bipedRightArm->rotateAngleX() * 0.5f - static_cast<f32>(PI / 10.0));
            m_bipedRightArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::ThrowSpear:
            // Java: rotateAngleX * 0.5F - PI
            m_bipedRightArm->setRotateAngleX(m_bipedRightArm->rotateAngleX() * 0.5f - static_cast<f32>(PI));
            m_bipedRightArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::BowAndArrow:
            // Java: 头部关联
            m_bipedRightArm->setRotateAngleY(-0.1f + m_bipedHead->rotateAngleY());
            m_bipedLeftArm->setRotateAngleY(0.1f + m_bipedHead->rotateAngleY() + 0.4f);
            m_bipedRightArm->setRotateAngleX(static_cast<f32>(-PI / 2.0) + m_bipedHead->rotateAngleX());
            m_bipedLeftArm->setRotateAngleX(static_cast<f32>(-PI / 2.0) + m_bipedHead->rotateAngleX());
            break;
        case ArmPose::CrossbowCharge:
            // TODO: 需要实现 ModelHelper.func_239102_a_
            break;
        case ArmPose::CrossbowHold:
            // TODO: 需要实现 ModelHelper.func_239104_a_
            break;
    }
}

void BipedModel::handleLeftArmPose() {
    // 参考 MC 1.16.5 BipedModel.func_241655_c_
    switch (m_leftArmPose) {
        case ArmPose::Empty:
            m_bipedLeftArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::Block:
            // Java: rotateAngleX * 0.5F - 0.9424779F
            m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() * 0.5f - 0.9424779f);
            m_bipedLeftArm->setRotateAngleY(static_cast<f32>(PI / 6.0));
            break;
        case ArmPose::Item:
            // Java: rotateAngleX * 0.5F - PI / 10F
            m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() * 0.5f - static_cast<f32>(PI / 10.0));
            m_bipedLeftArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::ThrowSpear:
            // Java: rotateAngleX * 0.5F - PI
            m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() * 0.5f - static_cast<f32>(PI));
            m_bipedLeftArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::BowAndArrow:
            // Java: 头部关联（已在右手处理中设置）
            break;
        case ArmPose::CrossbowCharge:
            // TODO: 需要实现 ModelHelper.func_239102_a_
            break;
        case ArmPose::CrossbowHold:
            // TODO: 需要实现 ModelHelper.func_239104_a_
            break;
    }
}

void BipedModel::handleSwingAnimation(f64 ageInTicks) {
    // 参考 MC 1.16.5 BipedModel.func_230486_a_
    if (m_swingProgress <= 0.0f) {
        return;
    }

    // 身体扭动
    f32 swingProgress = m_swingProgress;
    m_bipedBody->setRotateAngleY(static_cast<f32>(std::sin(std::sqrt(swingProgress) * PI * 2.0) * 0.2));

    // 手臂位置调整
    m_bipedRightArm->setRotationPointZ(static_cast<f32>(std::sin(m_bipedBody->rotateAngleY()) * 5.0));
    m_bipedRightArm->setRotationPointX(static_cast<f32>(-std::cos(m_bipedBody->rotateAngleY()) * 5.0));
    m_bipedLeftArm->setRotationPointZ(static_cast<f32>(-std::sin(m_bipedBody->rotateAngleY()) * 5.0));
    m_bipedLeftArm->setRotationPointX(static_cast<f32>(std::cos(m_bipedBody->rotateAngleY()) * 5.0));

    m_bipedRightArm->setRotateAngleY(m_bipedRightArm->rotateAngleY() + m_bipedBody->rotateAngleY());
    m_bipedLeftArm->setRotateAngleY(m_bipedLeftArm->rotateAngleY() + m_bipedBody->rotateAngleY());
    m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() + m_bipedBody->rotateAngleY());

    // 挥动动画（假设右手为主手）
    f32 f = 1.0f - swingProgress;
    f = f * f;
    f = f * f;
    f = 1.0f - f;
    f32 f1 = static_cast<f32>(std::sin(f * PI));
    f32 f2 = static_cast<f32>(std::sin(swingProgress * PI) * -(m_bipedHead->rotateAngleX() - 0.7f) * 0.75f);

    m_bipedRightArm->setRotateAngleX(m_bipedRightArm->rotateAngleX() - (f1 * 1.2f + f2));
    m_bipedRightArm->setRotateAngleY(m_bipedRightArm->rotateAngleY() + m_bipedBody->rotateAngleY() * 2.0f);
    m_bipedRightArm->setRotateAngleZ(m_bipedRightArm->rotateAngleZ() + static_cast<f32>(std::sin(swingProgress * PI) * -0.4f));

    (void)ageInTicks;
}

f32 BipedModel::rotLerpRad(f32 angle, f32 maxAngle, f32 target) {
    f32 f = std::fmod(target - maxAngle, static_cast<f32>(PI * 2.0));
    if (f < -static_cast<f32>(PI)) {
        f += static_cast<f32>(PI * 2.0);
    }
    if (f >= static_cast<f32>(PI)) {
        f -= static_cast<f32>(PI * 2.0);
    }
    return maxAngle + angle * f;
}

} // namespace mc::client::renderer::entity::model
