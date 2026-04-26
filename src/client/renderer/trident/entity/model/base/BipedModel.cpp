#include "BipedModel.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model {

BipedModel::BipedModel()
    : AgeableModel(true, 16.0f, 0.0f, 2.0f, 2.0f, 24.0f)  // 参考 Java: super(true, 16.0F, 0.0F, 2.0F, 2.0F, 24.0F)
{
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

BipedModel::BipedModel(f32 scale, f32 yOffset, i32 textureWidth, i32 textureHeight)
    : AgeableModel(true, 16.0f, 0.0f, 2.0f, 2.0f, 24.0f)  // 参考 Java: super(true, 16.0F, 0.0F, 2.0F, 2.0F, 24.0F)
{
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

std::vector<std::shared_ptr<ModelRenderer>> BipedModel::getHeadParts() const {
    return { m_bipedHead };
}

std::vector<std::shared_ptr<ModelRenderer>> BipedModel::getBodyParts() const {
    return { m_bipedBody, m_bipedRightArm, m_bipedLeftArm, m_bipedRightLeg, m_bipedLeftLeg, m_bipedHeadwear };
}

void BipedModel::render(f64 scale) {
    AgeableModel::render(scale);
}

void BipedModel::setLivingAnimations(f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 /*partialTick*/) {
    // 子类可重写此方法设置游泳动画等状态
    // Java: this.swimAnimation = entityIn.getSwimAnimation(partialTick);
}

void BipedModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                            f64 ageInTicks, f64 netHeadYaw,
                            f64 headPitch, f64 /*scale*/) {
    // 参考 MC 1.16.5 BipedModel.setRotationAngles

    bool isElytraFlying = m_elytraFlyingTicks > 4;
    bool isActuallySwimming = m_isActuallySwimming;

    // 头部旋转
    f32 headYawRad = static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0);
    f32 headPitchRad = static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0);

    // 鞘翅飞行或游泳时的头部角度
    if (isElytraFlying) {
        m_bipedHead->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0));
    } else if (m_swimAnimation > 0.0f) {
        if (isActuallySwimming) {
            m_bipedHead->setRotateAngleX(rotLerpRad(m_swimAnimation, m_bipedHead->rotateAngleX(), static_cast<f32>(-mc::math::PI_DOUBLE / 4.0)));
        } else {
            m_bipedHead->setRotateAngleX(rotLerpRad(m_swimAnimation, m_bipedHead->rotateAngleX(), headPitchRad));
        }
    } else {
        m_bipedHead->setRotateAngleX(headPitchRad);
    }
    m_bipedHead->setRotateAngleY(headYawRad);

    // 身体旋转
    m_bipedBody->setRotateAngleY(0.0f);

    // 重置手臂旋转点
    m_bipedRightArm->setRotationPointZ(0.0f);
    m_bipedRightArm->setRotationPointX(-5.0f);
    m_bipedLeftArm->setRotationPointZ(0.0f);
    m_bipedLeftArm->setRotationPointX(5.0f);

    // 速度因子
    f32 f = 1.0f;
    if (isElytraFlying) {
        // Java: f = (float)entityIn.getMotion().lengthSquared();
        // f = f / 0.2F; f = f * f * f;
        // 简化处理，保持 f = 1.0f
    }
    if (f < 1.0f) {
        f = 1.0f;
    }

    // 步态动画
    f32 limbSwingFloat = static_cast<f32>(limbSwing);
    f32 limbSwingAmountFloat = static_cast<f32>(limbSwingAmount);

    // 手臂摆动 - Java: MathHelper.cos(limbSwing * 0.6662F + mc::math::PI_DOUBLE) * 2.0F * limbSwingAmount * 0.5F / f
    m_bipedRightArm->setRotateAngleX(static_cast<f32>(std::cos(limbSwingFloat * 0.6662f + mc::math::PI_DOUBLE) * 2.0 * limbSwingAmountFloat * 0.5 / f));
    m_bipedLeftArm->setRotateAngleX(static_cast<f32>(std::cos(limbSwingFloat * 0.6662f) * 2.0 * limbSwingAmountFloat * 0.5 / f));

    m_bipedRightArm->setRotateAngleZ(0.0f);
    m_bipedLeftArm->setRotateAngleZ(0.0f);

    // 腿部动画 - Java: cos(limbSwing * 0.6662F) * 1.4F * limbSwingAmount / f
    m_bipedRightLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwingFloat * 0.6662f) * 1.4 * limbSwingAmountFloat / f));
    m_bipedLeftLeg->setRotateAngleX(static_cast<f32>(std::cos(limbSwingFloat * 0.6662f + mc::math::PI_DOUBLE) * 1.4 * limbSwingAmountFloat / f));

    m_bipedRightLeg->setRotateAngleY(0.0f);
    m_bipedLeftLeg->setRotateAngleY(0.0f);
    m_bipedRightLeg->setRotateAngleZ(0.0f);
    m_bipedLeftLeg->setRotateAngleZ(0.0f);

    // 坐姿
    if (m_isSitting) {
        m_bipedRightArm->setRotateAngleX(m_bipedRightArm->rotateAngleX() + static_cast<f32>(-mc::math::PI_DOUBLE / 5.0));
        m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() + static_cast<f32>(-mc::math::PI_DOUBLE / 5.0));
        m_bipedRightLeg->setRotateAngleX(-1.4137167f);
        m_bipedRightLeg->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE / 10.0));
        m_bipedRightLeg->setRotateAngleZ(0.07853982f);
        m_bipedLeftLeg->setRotateAngleX(-1.4137167f);
        m_bipedLeftLeg->setRotateAngleY(static_cast<f32>(-mc::math::PI_DOUBLE / 10.0));
        m_bipedLeftLeg->setRotateAngleZ(-0.07853982f);
    }

    // 手臂姿态处理 - 参考 Java: 根据主手和姿态类型决定处理顺序
    m_bipedRightArm->setRotateAngleY(0.0f);
    m_bipedLeftArm->setRotateAngleY(0.0f);

    // Java: boolean flag2 = entityIn.getPrimaryHand() == HandSide.RIGHT;
    // boolean flag3 = flag2 ? this.leftArmPose.func_241657_a_() : this.rightArmPose.func_241657_a_();
    // func_241657_a_() 返回是否需要双臂协调（弓箭、弩等）
    bool isRightHanded = (m_mainHand == HandSide::Right);
    bool needsCrossArmCoord = isRightHanded
        ? (m_leftArmPose == ArmPose::BowAndArrow || m_leftArmPose == ArmPose::CrossbowCharge || m_leftArmPose == ArmPose::CrossbowHold)
        : (m_rightArmPose == ArmPose::BowAndArrow || m_rightArmPose == ArmPose::CrossbowCharge || m_rightArmPose == ArmPose::CrossbowHold);

    if (isRightHanded != needsCrossArmCoord) {
        // 先左手，后右手
        handleLeftArmPose();
        handleRightArmPose();
    } else {
        // 先右手，后左手
        handleRightArmPose();
        handleLeftArmPose();
    }

    // 挥动动画
    handleSwingAnimation(ageInTicks);

    // 游泳动画
    if (m_swimAnimation > 0.0f) {
        handleSwimAnimation(limbSwing);
    }

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

    // 帽子跟随头部
    m_bipedHeadwear->copyModelAngles(*m_bipedHead);
}

void BipedModel::handleSwimAnimation(f64 limbSwing) {
    // 参考 MC 1.16.5 BipedModel.setRotationAngles 中的游泳动画部分
    f32 f1 = static_cast<f32>(std::fmod(limbSwing, 26.0));
    HandSide mainHand = getMainHand();
    f32 f2 = (mainHand == HandSide::Right && m_swingProgress > 0.0f) ? 0.0f : m_swimAnimation;
    f32 f3 = (mainHand == HandSide::Left && m_swingProgress > 0.0f) ? 0.0f : m_swimAnimation;

    if (f1 < 14.0f) {
        m_bipedLeftArm->setRotateAngleX(rotLerpRad(f3, m_bipedLeftArm->rotateAngleX(), 0.0f));
        m_bipedRightArm->setRotateAngleX(static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleX()));
        m_bipedLeftArm->setRotateAngleY(rotLerpRad(f3, m_bipedLeftArm->rotateAngleY(), static_cast<f32>(mc::math::PI_DOUBLE)));
        m_bipedRightArm->setRotateAngleY(static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleY() + f2 * mc::math::PI_DOUBLE));
        m_bipedLeftArm->setRotateAngleZ(rotLerpRad(f3, m_bipedLeftArm->rotateAngleZ(), static_cast<f32>(mc::math::PI_DOUBLE + 1.8707964 * getArmAngleSq(f1) / getArmAngleSq(14.0f))));
        m_bipedRightArm->setRotateAngleZ(static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleZ() + f2 * (mc::math::PI_DOUBLE - 1.8707964 * getArmAngleSq(f1) / getArmAngleSq(14.0f))));
    } else if (f1 >= 14.0f && f1 < 22.0f) {
        f32 f6 = (f1 - 14.0f) / 8.0f;
        m_bipedLeftArm->setRotateAngleX(rotLerpRad(f3, m_bipedLeftArm->rotateAngleX(), static_cast<f32>(mc::math::PI_DOUBLE / 2.0 * f6)));
        m_bipedRightArm->setRotateAngleX(static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleX() + f2 * mc::math::PI_DOUBLE / 2.0 * f6));
        m_bipedLeftArm->setRotateAngleY(rotLerpRad(f3, m_bipedLeftArm->rotateAngleY(), static_cast<f32>(mc::math::PI_DOUBLE)));
        m_bipedRightArm->setRotateAngleY(static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleY() + f2 * mc::math::PI_DOUBLE));
        m_bipedLeftArm->setRotateAngleZ(rotLerpRad(f3, m_bipedLeftArm->rotateAngleZ(), 5.012389f - 1.8707964f * f6));
        m_bipedRightArm->setRotateAngleZ(static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleZ() + f2 * (1.2707963f + 1.8707964f * f6)));
    } else if (f1 >= 22.0f && f1 < 26.0f) {
        f32 f4 = (f1 - 22.0f) / 4.0f;
        m_bipedLeftArm->setRotateAngleX(rotLerpRad(f3, m_bipedLeftArm->rotateAngleX(), static_cast<f32>(mc::math::PI_DOUBLE / 2.0 - mc::math::PI_DOUBLE / 2.0 * f4)));
        m_bipedRightArm->setRotateAngleX(static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleX() + f2 * (mc::math::PI_DOUBLE / 2.0 - mc::math::PI_DOUBLE / 2.0 * f4)));
        m_bipedLeftArm->setRotateAngleY(rotLerpRad(f3, m_bipedLeftArm->rotateAngleY(), static_cast<f32>(mc::math::PI_DOUBLE)));
        m_bipedRightArm->setRotateAngleY(static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleY() + f2 * mc::math::PI_DOUBLE));
        m_bipedLeftArm->setRotateAngleZ(rotLerpRad(f3, m_bipedLeftArm->rotateAngleZ(), static_cast<f32>(mc::math::PI_DOUBLE)));
        m_bipedRightArm->setRotateAngleZ(static_cast<f32>((1.0f - f2) * m_bipedRightArm->rotateAngleZ() + f2 * mc::math::PI_DOUBLE));
    }

    // 腿部游泳动画
    f32 swimLegAngle1 = 0.3f * static_cast<f32>(std::cos(limbSwing * 0.33333334 + mc::math::PI_DOUBLE));
    f32 swimLegAngle2 = 0.3f * static_cast<f32>(std::cos(limbSwing * 0.33333334));
    m_bipedLeftLeg->setRotateAngleX(static_cast<f32>((1.0f - m_swimAnimation) * m_bipedLeftLeg->rotateAngleX() + m_swimAnimation * swimLegAngle1));
    m_bipedRightLeg->setRotateAngleX(static_cast<f32>((1.0f - m_swimAnimation) * m_bipedRightLeg->rotateAngleX() + m_swimAnimation * swimLegAngle2));
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
            m_bipedRightArm->setRotateAngleY(static_cast<f32>(-mc::math::PI_DOUBLE / 6.0));
            break;
        case ArmPose::Item:
            // Java: rotateAngleX * 0.5F - mc::math::PI_DOUBLE / 10F
            m_bipedRightArm->setRotateAngleX(m_bipedRightArm->rotateAngleX() * 0.5f - static_cast<f32>(mc::math::PI_DOUBLE / 10.0));
            m_bipedRightArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::ThrowSpear:
            // Java: rotateAngleX * 0.5F - mc::math::PI_DOUBLE
            m_bipedRightArm->setRotateAngleX(m_bipedRightArm->rotateAngleX() * 0.5f - static_cast<f32>(mc::math::PI_DOUBLE));
            m_bipedRightArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::BowAndArrow:
            // Java: 头部关联
            m_bipedRightArm->setRotateAngleY(-0.1f + m_bipedHead->rotateAngleY());
            m_bipedLeftArm->setRotateAngleY(0.1f + m_bipedHead->rotateAngleY() + 0.4f);
            m_bipedRightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0) + m_bipedHead->rotateAngleX());
            m_bipedLeftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0) + m_bipedHead->rotateAngleX());
            break;
        case ArmPose::CrossbowCharge:
            // Java: ModelHelper.func_239102_a_(this.bipedRightArm, this.bipedLeftArm, entityIn, true)
            // 简化实现
            m_bipedRightArm->setRotateAngleY(m_bipedHead->rotateAngleY());
            m_bipedRightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
            break;
        case ArmPose::CrossbowHold:
            // Java: ModelHelper.func_239104_a_(this.bipedRightArm, this.bipedLeftArm, this.bipedHead, true)
            // 简化实现
            m_bipedRightArm->setRotateAngleY(-0.1f + m_bipedHead->rotateAngleY());
            m_bipedRightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
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
            m_bipedLeftArm->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE / 6.0));
            break;
        case ArmPose::Item:
            // Java: rotateAngleX * 0.5F - mc::math::PI_DOUBLE / 10F
            m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() * 0.5f - static_cast<f32>(mc::math::PI_DOUBLE / 10.0));
            m_bipedLeftArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::ThrowSpear:
            // Java: rotateAngleX * 0.5F - mc::math::PI_DOUBLE
            m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() * 0.5f - static_cast<f32>(mc::math::PI_DOUBLE));
            m_bipedLeftArm->setRotateAngleY(0.0f);
            break;
        case ArmPose::BowAndArrow:
            // Java: 头部关联（已在右手处理中设置）
            m_bipedRightArm->setRotateAngleY(-0.1f + m_bipedHead->rotateAngleY() - 0.4f);
            m_bipedLeftArm->setRotateAngleY(0.1f + m_bipedHead->rotateAngleY());
            m_bipedRightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0) + m_bipedHead->rotateAngleX());
            m_bipedLeftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0) + m_bipedHead->rotateAngleX());
            break;
        case ArmPose::CrossbowCharge:
            // Java: ModelHelper.func_239102_a_(this.bipedRightArm, this.bipedLeftArm, entityIn, false)
            // 简化实现
            m_bipedLeftArm->setRotateAngleY(m_bipedHead->rotateAngleY());
            m_bipedLeftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
            break;
        case ArmPose::CrossbowHold:
            // Java: ModelHelper.func_239104_a_(this.bipedRightArm, this.bipedLeftArm, this.bipedHead, false)
            // 简化实现
            m_bipedLeftArm->setRotateAngleY(0.1f + m_bipedHead->rotateAngleY());
            m_bipedLeftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
            break;
    }
}

void BipedModel::handleSwingAnimation(f64 /*ageInTicks*/) {
    // 参考 MC 1.16.5 BipedModel.func_230486_a_
    if (m_swingProgress <= 0.0f) {
        return;
    }

    // 获取挥动的手臂
    HandSide mainHand = getMainHand();
    auto swingArm = (mainHand == HandSide::Left) ? m_bipedLeftArm : m_bipedRightArm;

    // 身体扭动
    f32 swingProgress = m_swingProgress;
    f32 bodyRotateY = static_cast<f32>(std::sin(std::sqrt(swingProgress) * mc::math::PI_DOUBLE * 2.0) * 0.2);
    if (mainHand == HandSide::Left) {
        bodyRotateY = -bodyRotateY;
    }
    m_bipedBody->setRotateAngleY(bodyRotateY);

    // 手臂位置调整
    m_bipedRightArm->setRotationPointZ(static_cast<f32>(std::sin(m_bipedBody->rotateAngleY()) * 5.0));
    m_bipedRightArm->setRotationPointX(static_cast<f32>(-std::cos(m_bipedBody->rotateAngleY()) * 5.0));
    m_bipedLeftArm->setRotationPointZ(static_cast<f32>(-std::sin(m_bipedBody->rotateAngleY()) * 5.0));
    m_bipedLeftArm->setRotationPointX(static_cast<f32>(std::cos(m_bipedBody->rotateAngleY()) * 5.0));

    m_bipedRightArm->setRotateAngleY(m_bipedRightArm->rotateAngleY() + m_bipedBody->rotateAngleY());
    m_bipedLeftArm->setRotateAngleY(m_bipedLeftArm->rotateAngleY() + m_bipedBody->rotateAngleY());
    m_bipedLeftArm->setRotateAngleX(m_bipedLeftArm->rotateAngleX() + m_bipedBody->rotateAngleY());

    // 挥动动画
    f32 f = 1.0f - swingProgress;
    f = f * f;
    f = f * f;
    f = 1.0f - f;
    f32 f1 = static_cast<f32>(std::sin(f * mc::math::PI_DOUBLE));
    f32 f2 = static_cast<f32>(std::sin(swingProgress * mc::math::PI_DOUBLE) * -(m_bipedHead->rotateAngleX() - 0.7f) * 0.75f);

    swingArm->setRotateAngleX(swingArm->rotateAngleX() - (f1 * 1.2f + f2));
    swingArm->setRotateAngleY(swingArm->rotateAngleY() + m_bipedBody->rotateAngleY() * 2.0f);
    swingArm->setRotateAngleZ(swingArm->rotateAngleZ() + static_cast<f32>(std::sin(swingProgress * mc::math::PI_DOUBLE) * -0.4f));
}

void BipedModel::setVisible(bool visible) {
    m_bipedHead->setVisible(visible);
    m_bipedHeadwear->setVisible(visible);
    m_bipedBody->setVisible(visible);
    m_bipedRightArm->setVisible(visible);
    m_bipedLeftArm->setVisible(visible);
    m_bipedRightLeg->setVisible(visible);
    m_bipedLeftLeg->setVisible(visible);
}

void BipedModel::copyModelAttributesTo(BipedModel& target) const {
    // 参考 MC 1.16.5 BipedModel.setModelAttributes()
    // super.copyModelAttributesTo(modelIn) - 复制 EntityModel 的属性
    target.setSwingProgress(m_swingProgress);
    target.setSitting(m_isSitting);
    target.setChild(m_isChild);

    // 复制 BipedModel 特有属性
    target.m_leftArmPose = m_leftArmPose;
    target.m_rightArmPose = m_rightArmPose;
    target.m_isSneaking = m_isSneaking;
    target.m_swimAnimation = m_swimAnimation;
    target.m_mainHand = m_mainHand;

    // 复制部件角度 - Java: modelIn.bipedHead.copyModelAngles(this.bipedHead) 等
    target.m_bipedHead->copyModelAngles(*m_bipedHead);
    target.m_bipedHeadwear->copyModelAngles(*m_bipedHeadwear);
    target.m_bipedBody->copyModelAngles(*m_bipedBody);
    target.m_bipedRightArm->copyModelAngles(*m_bipedRightArm);
    target.m_bipedLeftArm->copyModelAngles(*m_bipedLeftArm);
    target.m_bipedRightLeg->copyModelAngles(*m_bipedRightLeg);
    target.m_bipedLeftLeg->copyModelAngles(*m_bipedLeftLeg);
}

std::shared_ptr<ModelRenderer> BipedModel::getArmForSide(HandSide side) {
    return (side == HandSide::Left) ? m_bipedLeftArm : m_bipedRightArm;
}

HandSide BipedModel::getMainHand() const {
    // 如果正在挥动的手是主手，返回主手；否则返回副手
    if (m_swingingHand == m_mainHand) {
        return m_mainHand;
    }
    return (m_mainHand == HandSide::Left) ? HandSide::Right : HandSide::Left;
}

f32 BipedModel::rotLerpRad(f32 angle, f32 maxAngle, f32 target) {
    f32 f = std::fmod(target - maxAngle, static_cast<f32>(mc::math::PI_DOUBLE * 2.0));
    if (f < -static_cast<f32>(mc::math::PI_DOUBLE)) {
        f += static_cast<f32>(mc::math::PI_DOUBLE * 2.0);
    }
    if (f >= static_cast<f32>(mc::math::PI_DOUBLE)) {
        f -= static_cast<f32>(mc::math::PI_DOUBLE * 2.0);
    }
    return maxAngle + angle * f;
}

f32 BipedModel::getArmAngleSq(f32 limbSwing) {
    return -65.0f * limbSwing + limbSwing * limbSwing;
}

} // namespace mc::client::renderer::entity::model
