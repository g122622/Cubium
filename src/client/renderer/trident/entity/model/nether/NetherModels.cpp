#include "NetherModels.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::nether {

// ==================== GhastModel ====================

GhastModel::GhastModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

void GhastModel::setupParts()
{
    // 参考 MC 1.16.5 GhastModel
    // 身体尺寸: 16x16x16，中心在原点
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-8.0f, -8.0f, -8.0f, 16.0f, 16.0f, 16.0f);
    m_body->setRotationPoint(0.0f, 17.6f, 0.0f); // Java: rotationPointY = 17.6F
    m_parts.push_back(m_body);

    // 9 条触手
    // Java 触手位置计算:
    // float f = (((float)(i % 3) - (float)(i / 3 % 2) * 0.5F + 0.25F) / 2.0F * 2.0F - 1.0F) * 5.0F;
    // float f1 = ((float)(i / 3) / 2.0F * 2.0F - 1.0F) * 5.0F;
    // 触手长度: random.nextInt(7) + 8  -> 8 到 14

    // 使用固定种子生成触手长度（与 Java 原版一致）
    mc::math::Random rng(1660);

    for (i32 i = 0; i < 9; ++i) {
        m_tentacles[i] = std::make_shared<ModelRenderer>("tentacle" + std::to_string(i));
        m_tentacles[i]->setTextureOffset(0, 0);

        // 随机长度 8-14（Java: random.nextInt(7) + 8）
        i32 length = rng.nextInt(8, 14);

        // Java 公式计算 X 和 Z 位置
        f32 f = (((static_cast<f32>(i % 3) - static_cast<f32>(i / 3 % 2) * 0.5f + 0.25f) / 2.0f * 2.0f - 1.0f) * 5.0f);
        f32 f1 = ((static_cast<f32>(i / 3) / 2.0f * 2.0f - 1.0f) * 5.0f);

        m_tentacles[i]->addBox(-1.0f, 0.0f, -1.0f, 2.0f, static_cast<f32>(length), 2.0f);
        m_tentacles[i]->setRotationPoint(f, 24.6f, f1); // Java: rotationPointY = 24.6F
        m_parts.push_back(m_tentacles[i]);
    }
}

void GhastModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void GhastModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 身体跟随头部旋转
    m_body->setRotateAngleY(static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0));
    m_body->setRotateAngleX(static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0));

    // Java 触手动画: rotateAngleX = 0.2F * MathHelper.sin(ageInTicks * 0.3F + (float)i) + 0.4F
    for (i32 i = 0; i < 9; ++i) {
        f32 angle = static_cast<f32>(0.2 * std::sin(ageInTicks * 0.3 + static_cast<f64>(i)) + 0.4);
        m_tentacles[i]->setRotateAngleX(angle);
    }

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)scale;
}

// ==================== MagmaCubeModel ====================

MagmaCubeModel::MagmaCubeModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    setupParts();
}

MagmaCubeModel::MagmaCubeModel(i32 size)
    : EntityModel()
    , m_size(size)
{
    setTextureSize(64, 32);
    setupParts();
}

void MagmaCubeModel::setupParts()
{
    // 参考 MC 1.16.5 MagmaCubeModel
    // 8 个薄片状的 segments，每个是 8x1x8
    // 纹理偏移根据索引变化：
    // i=0,1: (0, 16)
    // i=2: (24, 10)
    // i=3: (24, 19)
    // i=4-7: (0, 16)
    for (i32 i = 0; i < 8; ++i) {
        m_segments[i] = std::make_shared<ModelRenderer>("segment" + std::to_string(i));

        // 计算纹理偏移
        i32 texU = 0;
        i32 texV = 16;
        if (i == 2) {
            texU = 24;
            texV = 10;
        } else if (i == 3) {
            texU = 24;
            texV = 19;
        }

        m_segments[i]->setTextureOffset(texU, texV);
        // Java: addBox(-4.0F, (float)(16 + i), -4.0F, 8.0F, 1.0F, 8.0F)
        // 注意：Y 坐标是 16 + i，表示每个 segment 垂直堆叠
        m_segments[i]->addBox(-4.0f, static_cast<f32>(16 + i), -4.0f, 8.0f, 1.0f, 8.0f);
        // 旋转点在原点，Y 方向偏移通过 setLivingAnimations 动态设置
        m_segments[i]->setRotationPoint(0.0f, 0.0f, 0.0f);
        m_parts.push_back(m_segments[i]);
    }

    // 核心: 4x4x4，位于 Y=18
    // Java: addBox(-2.0F, 18.0F, -2.0F, 4.0F, 4.0F, 4.0F)
    m_core = std::make_shared<ModelRenderer>("core");
    m_core->setTextureOffset(0, 16);
    m_core->addBox(-2.0f, 18.0f, -2.0f, 4.0f, 4.0f, 4.0f);
    m_core->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_core);
}

void MagmaCubeModel::setSquishFactor(f32 squishFactor, f32 prevSquishFactor)
{
    m_squishFactor = squishFactor;
    m_prevSquishFactor = prevSquishFactor;
}

void MagmaCubeModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void MagmaCubeModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 应用挤压动画
    // Java: setLivingAnimations 中
    // float f = MathHelper.lerp(partialTick, entityIn.prevSquishFactor, entityIn.squishFactor);
    // this.segments[i].rotationPointY = (float)(-(4 - i)) * f * 1.7F;
    f32 f = m_squishFactor;
    if (f < 0.0f) f = 0.0f;

    for (i32 i = 0; i < 8; ++i) {
        f32 offsetY = static_cast<f32>(-(4 - i)) * f * 1.7f;
        m_segments[i]->setRotationPointY(offsetY);
    }

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== PiglinModel ====================

PiglinModel::PiglinModel()
    : PiglinModel(0.0f, 64, 64)
{}

PiglinModel::PiglinModel(f32 scale, i32 textureWidth, i32 textureHeight)
    : BipedModel(scale, 0.0f, textureWidth, textureHeight)
{
    // 参考 MC 1.16.5 PiglinModel
    // PiglinModel 继承自 PlayerModel，使用标准手臂（false = 非纤细）
    // 重新设置猪灵特有的头部（包含鼻子、眼睛）
    // 注意：BipedModel 构造函数已经创建了基础部件，需要重新配置头部

    // 清除基类创建的头部部件，重新创建猪灵头部
    m_bipedHead = std::make_shared<ModelRenderer>("head");
    // 头部主体: textureOffset(0, 0), addBox(-5, -8, -4, 10, 8, 8)
    m_bipedHead->setTextureSize(textureWidth, textureHeight);
    m_bipedHead->setTextureOffset(0, 0);
    m_bipedHead->addBox(-5.0f, -8.0f, -4.0f, 10.0f, 8.0f, 8.0f, static_cast<f64>(scale));
    // 鼻子: textureOffset(31, 1), addBox(-2, -4, -5, 4, 4, 1)
    m_bipedHead->setTextureOffset(31, 1);
    m_bipedHead->addBox(-2.0f, -4.0f, -5.0f, 4.0f, 4.0f, 1.0f, static_cast<f64>(scale));
    // 右眼: textureOffset(2, 4), addBox(2, -2, -5, 1, 2, 1)
    m_bipedHead->setTextureOffset(2, 4);
    m_bipedHead->addBox(2.0f, -2.0f, -5.0f, 1.0f, 2.0f, 1.0f, static_cast<f64>(scale));
    // 左眼: textureOffset(2, 0), addBox(-3, -2, -5, 1, 2, 1)
    m_bipedHead->setTextureOffset(2, 0);
    m_bipedHead->addBox(-3.0f, -2.0f, -5.0f, 1.0f, 2.0f, 1.0f, static_cast<f64>(scale));
    m_bipedHead->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 右耳: textureOffset(51, 6), addBox(0, 0, -2, 1, 5, 4), rotationPoint(4.5, -6, 0)
    m_rightEar = std::make_shared<ModelRenderer>("rightEar");
    m_rightEar->setTextureSize(textureWidth, textureHeight);
    m_rightEar->setTextureOffset(51, 6);
    m_rightEar->addBox(0.0f, 0.0f, -2.0f, 1.0f, 5.0f, 4.0f, static_cast<f64>(scale));
    m_rightEar->setRotationPoint(4.5f, -6.0f, 0.0f);
    m_bipedHead->addChild(m_rightEar);

    // 左耳: textureOffset(39, 6), addBox(-1, 0, -2, 1, 5, 4), rotationPoint(-4.5, -6, 0)
    m_leftEar = std::make_shared<ModelRenderer>("leftEar");
    m_leftEar->setTextureSize(textureWidth, textureHeight);
    m_leftEar->setTextureOffset(39, 6);
    m_leftEar->addBox(-1.0f, 0.0f, -2.0f, 1.0f, 5.0f, 4.0f, static_cast<f64>(scale));
    m_leftEar->setRotationPoint(-4.5f, -6.0f, 0.0f);
    m_bipedHead->addChild(m_leftEar);

    // 头部盔甲层: textureOffset(0, 0), addBox(-5, -8, -4, 10, 8, 8, scale + 0.5)
    m_bipedHeadwearPiglin = std::make_shared<ModelRenderer>("headwear");
    m_bipedHeadwearPiglin->setTextureSize(textureWidth, textureHeight);
    m_bipedHeadwearPiglin->setTextureOffset(0, 0);
    m_bipedHeadwearPiglin->addBox(-5.0f, -8.0f, -4.0f, 10.0f, 8.0f, 8.0f, static_cast<f64>(scale) + 0.5);
    m_bipedHeadwearPiglin->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 重新设置身体（Java: bipedBody = new ModelRenderer(this, 16, 16)）
    // 注意：BipedModel 已创建身体，但猪灵需要重新设置纹理偏移
    m_bipedBody->setTextureSize(textureWidth, textureHeight);
    m_bipedBody->setTextureOffset(16, 16);
    // 身体已在 BipedModel 中创建，尺寸 8x12x4 正确

    // 重新设置手臂尺寸 - 猪灵使用标准手臂（宽度4），不是纤细手臂（宽度3）
    // Java 原版 PiglinModel 调用 super(p_i232336_1_, false) - false = 非纤细手臂
    // 右臂: textureOffset(40, 16), addBox(-3, -2, -2, 4, 12, 4) 标准手臂
    m_bipedRightArm = std::make_shared<ModelRenderer>("rightArm");
    m_bipedRightArm->setTextureSize(textureWidth, textureHeight);
    m_bipedRightArm->setTextureOffset(40, 16);
    m_bipedRightArm->addBox(-3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, static_cast<f64>(scale));
    m_bipedRightArm->setRotationPoint(-5.0f, 2.0f, 0.0f);

    // 左臂: textureOffset(32, 48), mirror, addBox(-1, -2, -2, 4, 12, 4) 标准手臂
    m_bipedLeftArm = std::make_shared<ModelRenderer>("leftArm");
    m_bipedLeftArm->setTextureSize(textureWidth, textureHeight);
    m_bipedLeftArm->setTextureOffset(32, 48);
    m_bipedLeftArm->setMirror(true);
    m_bipedLeftArm->addBox(-1.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, static_cast<f64>(scale));
    m_bipedLeftArm->setRotationPoint(5.0f, 2.0f, 0.0f);

    // 外观层部件
    // 右臂外层: textureOffset(40, 32), addBox(-3, -2, -2, 4, 12, 4, scale + 0.25)
    m_bipedRightArmwear = std::make_shared<ModelRenderer>("rightArmwear");
    m_bipedRightArmwear->setTextureSize(textureWidth, textureHeight);
    m_bipedRightArmwear->setTextureOffset(40, 32);
    m_bipedRightArmwear->addBox(-3.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, static_cast<f64>(scale) + 0.25);
    m_bipedRightArmwear->setRotationPoint(-5.0f, 2.0f, 10.0f);

    // 左臂外层: textureOffset(48, 48), addBox(-1, -2, -2, 4, 12, 4, scale + 0.25)
    m_bipedLeftArmwear = std::make_shared<ModelRenderer>("leftArmwear");
    m_bipedLeftArmwear->setTextureSize(textureWidth, textureHeight);
    m_bipedLeftArmwear->setTextureOffset(48, 48);
    m_bipedLeftArmwear->addBox(-1.0f, -2.0f, -2.0f, 4.0f, 12.0f, 4.0f, static_cast<f64>(scale) + 0.25);
    m_bipedLeftArmwear->setRotationPoint(5.0f, 2.0f, 0.0f);

    // 右腿外层: textureOffset(0, 32), addBox(-2, 0, -2, 4, 12, 4, scale + 0.25)
    m_bipedRightLegwear = std::make_shared<ModelRenderer>("rightLegwear");
    m_bipedRightLegwear->setTextureSize(textureWidth, textureHeight);
    m_bipedRightLegwear->setTextureOffset(0, 32);
    m_bipedRightLegwear->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, static_cast<f64>(scale) + 0.25);
    m_bipedRightLegwear->setRotationPoint(-1.9f, 12.0f, 0.0f);

    // 左腿外层: textureOffset(0, 48), addBox(-2, 0, -2, 4, 12, 4, scale + 0.25)
    m_bipedLeftLegwear = std::make_shared<ModelRenderer>("leftLegwear");
    m_bipedLeftLegwear->setTextureSize(textureWidth, textureHeight);
    m_bipedLeftLegwear->setTextureOffset(0, 48);
    m_bipedLeftLegwear->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 12.0f, 4.0f, static_cast<f64>(scale) + 0.25);
    m_bipedLeftLegwear->setRotationPoint(1.9f, 12.0f, 0.0f);

    // 身体外层: textureOffset(16, 32), addBox(-4, 0, -2, 8, 12, 4, scale + 0.25)
    m_bipedBodyWear = std::make_shared<ModelRenderer>("bodyWear");
    m_bipedBodyWear->setTextureSize(textureWidth, textureHeight);
    m_bipedBodyWear->setTextureOffset(16, 32);
    m_bipedBodyWear->addBox(-4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f, static_cast<f64>(scale) + 0.25);
    m_bipedBodyWear->setRotationPoint(0.0f, 0.0f, 0.0f);

    // 更新部件列表
    m_parts.clear();
    m_parts.push_back(m_bipedHead);
    m_parts.push_back(m_bipedHeadwearPiglin);
    m_parts.push_back(m_bipedBody);
    m_parts.push_back(m_bipedBodyWear);
    m_parts.push_back(m_bipedRightArm);
    m_parts.push_back(m_bipedLeftArm);
    m_parts.push_back(m_bipedRightArmwear);
    m_parts.push_back(m_bipedLeftArmwear);
    m_parts.push_back(m_bipedRightLeg);
    m_parts.push_back(m_bipedLeftLeg);
    m_parts.push_back(m_bipedRightLegwear);
    m_parts.push_back(m_bipedLeftLegwear);
}

void PiglinModel::copyAnglesToWear()
{
    // 参考 MC 1.16.5 PiglinModel.setRotationAngles 末尾的 copyModelAngles 调用
    if (m_bipedLeftLeg && m_bipedLeftLegwear) {
        m_bipedLeftLegwear->copyModelAngles(*m_bipedLeftLeg);
    }
    if (m_bipedRightLeg && m_bipedRightLegwear) {
        m_bipedRightLegwear->copyModelAngles(*m_bipedRightLeg);
    }
    if (m_bipedLeftArm && m_bipedLeftArmwear) {
        m_bipedLeftArmwear->copyModelAngles(*m_bipedLeftArm);
    }
    if (m_bipedRightArm && m_bipedRightArmwear) {
        m_bipedRightArmwear->copyModelAngles(*m_bipedRightArm);
    }
    if (m_bipedBody && m_bipedBodyWear) {
        m_bipedBodyWear->copyModelAngles(*m_bipedBody);
    }
    if (m_bipedHead && m_bipedHeadwearPiglin) {
        m_bipedHeadwearPiglin->copyModelAngles(*m_bipedHead);
    }
}

void PiglinModel::handleRightArmPose()
{
    // 猪灵特有手臂姿态处理
    // 如果是特定动作，跳过基类处理
    if (m_action == static_cast<i32>(Action::ATTACKING_WITH_MELEE_WEAPON) && m_swingProgress == 0.0f) {
        if (!m_leftHanded) {
            m_bipedRightArm->setRotateAngleX(-1.8f);
        }
        return;
    }
    // 否则调用基类处理
    BipedModel::handleRightArmPose();
}

void PiglinModel::handleLeftArmPose()
{
    // 猪灵特有手臂姿态处理
    if (m_action == static_cast<i32>(Action::ATTACKING_WITH_MELEE_WEAPON) && m_swingProgress == 0.0f) {
        if (m_leftHanded) {
            m_bipedLeftArm->setRotateAngleX(-1.8f);
        }
        return;
    }
    // 否则调用基类处理
    BipedModel::handleLeftArmPose();
}

void PiglinModel::render(f64 scale)
{
    BipedModel::render(scale);
}

void PiglinModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 参考 MC 1.16.5 PiglinModel.setRotationAngles
    // 注意：Java 原版先复制身体、头部、手臂角度到外层，然后调用 super

    // 复制角度到外层部件
    copyAnglesToWear();

    // 调用基类设置基础动画
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 耳朵动画 - Java: mc::math::PI_DOUBLE/6 = 0.52359877559...
    f32 f1 = static_cast<f32>(ageInTicks * 0.1 + limbSwing * 0.5);
    f32 f2 = 0.08f + static_cast<f32>(limbSwingAmount * 0.4);
    m_rightEar->setRotateAngleZ(static_cast<f32>(-mc::math::PI_DOUBLE / 6.0 - std::cos(f1 * 1.2) * f2));
    m_leftEar->setRotateAngleZ(static_cast<f32>(mc::math::PI_DOUBLE / 6.0 + std::cos(f1) * f2));

    // 根据动作状态设置动画
    if (m_action == static_cast<i32>(Action::DANCING)) {
        // 跳舞动画
        f32 f3 = static_cast<f32>(ageInTicks / 60.0);
        m_leftEar->setRotateAngleZ(
            static_cast<f32>(mc::math::PI_DOUBLE / 6.0 + mc::math::PI_DOUBLE / 180.0 * std::sin(f3 * 30.0) * 10.0));
        m_rightEar->setRotateAngleZ(
            static_cast<f32>(-mc::math::PI_DOUBLE / 6.0 - mc::math::PI_DOUBLE / 180.0 * std::cos(f3 * 30.0) * 10.0));
        m_bipedHead->setRotationPointX(static_cast<f32>(std::sin(f3 * 10.0)));
        m_bipedHead->setRotationPointY(static_cast<f32>(std::sin(f3 * 40.0) + 0.4));
        m_bipedRightArm->setRotateAngleZ(
            static_cast<f32>(mc::math::PI_DOUBLE / 180.0 * (70.0 + std::cos(f3 * 40.0) * 10.0)));
        m_bipedLeftArm->setRotateAngleZ(-m_bipedRightArm->rotateAngleZ());
        m_bipedRightArm->setRotationPointY(static_cast<f32>(std::sin(f3 * 40.0) * 0.5 + 1.5));
        m_bipedLeftArm->setRotationPointY(m_bipedRightArm->rotationPointY());
        m_bipedBody->setRotationPointY(static_cast<f32>(std::sin(f3 * 40.0) * 0.35));
    } else if (m_action == static_cast<i32>(Action::ATTACKING_WITH_MELEE_WEAPON) && m_swingProgress == 0.0f) {
        // 近战攻击姿态
        if (m_leftHanded) {
            m_bipedLeftArm->setRotateAngleX(-1.8f);
        } else {
            m_bipedRightArm->setRotateAngleX(-1.8f);
        }
    } else if (m_action == static_cast<i32>(Action::CROSSBOW_HOLD)) {
        // 弩持有姿态 - 参考 ModelHelper.func_239104_a_
        if (!m_leftHanded) {
            // 右手持弩
            m_bipedRightArm->setRotateAngleY(-0.3f);
            m_bipedRightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
            m_bipedLeftArm->setRotateAngleY(0.6f);
            m_bipedLeftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
        } else {
            // 左手持弩
            m_bipedLeftArm->setRotateAngleY(0.3f);
            m_bipedLeftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
            m_bipedRightArm->setRotateAngleY(-0.6f);
            m_bipedRightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
        }
    } else if (m_action == static_cast<i32>(Action::CROSSBOW_CHARGE)) {
        // 弩装填姿态 - 参考 ModelHelper.func_239102_a_
        if (!m_leftHanded) {
            m_bipedRightArm->setRotateAngleY(-0.8f);
            m_bipedRightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
            m_bipedLeftArm->setRotateAngleY(0.8f);
            m_bipedLeftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
        } else {
            m_bipedLeftArm->setRotateAngleY(0.8f);
            m_bipedLeftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
            m_bipedRightArm->setRotateAngleY(-0.8f);
            m_bipedRightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
        }
    } else if (m_action == static_cast<i32>(Action::ADMIRING_ITEM)) {
        // 欣赏物品
        m_bipedHead->setRotateAngleX(0.5f);
        m_bipedHead->setRotateAngleY(0.0f);
        if (m_leftHanded) {
            m_bipedRightArm->setRotateAngleY(-0.5f);
            m_bipedRightArm->setRotateAngleX(-0.9f);
        } else {
            m_bipedLeftArm->setRotateAngleY(0.5f);
            m_bipedLeftArm->setRotateAngleX(-0.9f);
        }
    }

    (void)scale;
}

// ==================== BoarModel ====================

BoarModel::BoarModel()
    : ::mc::client::renderer::entity::model::AgeableModel(true, 8.0f, 6.0f, 1.9f, 2.0f, 24.0f)
{
    setTextureSize(128, 64);
    setupParts();
}

void BoarModel::setupParts()
{
    // 参考 MC 1.16.5 BoarModel
    // 构造函数参数: AgeableModel(true, 8.0F, 6.0F, 1.9F, 2.0F, 24.0F)

    // 身体: textureOffset(1, 1), addBox(-8, -7, -13, 16, 14, 26), rotationPoint(0, 7, 0)
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(1, 1);
    m_body->addBox(-8.0f, -7.0f, -13.0f, 16.0f, 14.0f, 26.0f);
    m_body->setRotationPoint(0.0f, 7.0f, 0.0f);

    // 鬃毛: textureOffset(90, 33), addBox(0, 0, -9, 0, 10, 19, 0.001), rotationPoint(0, -14, -5)
    // 作为身体的子部件
    m_mane = std::make_shared<ModelRenderer>("mane");
    m_mane->setTextureOffset(90, 33);
    m_mane->addBox(0.0f, 0.0f, -9.0f, 0.0f, 10.0f, 19.0f, 0.001f);
    m_mane->setRotationPoint(0.0f, -14.0f, -5.0f);
    m_body->addChild(m_mane);

    // 头部: textureOffset(61, 1), addBox(-7, -3, -19, 14, 6, 19), rotationPoint(0, 2, -12)
    // rotateAngleX = 0.87266463F (50度)
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(61, 1);
    m_head->addBox(-7.0f, -3.0f, -19.0f, 14.0f, 6.0f, 19.0f);
    m_head->setRotationPoint(0.0f, 2.0f, -12.0f);
    m_head->setRotateAngleX(0.87266463f);

    // 左獠牙: textureOffset(1, 1), addBox(-6, -1, -2, 6, 1, 4), rotationPoint(-6, -2, -3)
    // rotateAngleZ = -0.6981317F (-40度)
    m_leftTusk = std::make_shared<ModelRenderer>("leftTusk");
    m_leftTusk->setTextureOffset(1, 1);
    m_leftTusk->addBox(-6.0f, -1.0f, -2.0f, 6.0f, 1.0f, 4.0f);
    m_leftTusk->setRotationPoint(-6.0f, -2.0f, -3.0f);
    m_leftTusk->setRotateAngleZ(-0.6981317f);
    m_head->addChild(m_leftTusk);

    // 右獠牙: textureOffset(1, 6), addBox(0, -1, -2, 6, 1, 4), rotationPoint(6, -2, -3)
    // rotateAngleZ = 0.6981317F (40度)
    m_rightTusk = std::make_shared<ModelRenderer>("rightTusk");
    m_rightTusk->setTextureOffset(1, 6);
    m_rightTusk->addBox(0.0f, -1.0f, -2.0f, 6.0f, 1.0f, 4.0f);
    m_rightTusk->setRotationPoint(6.0f, -2.0f, -3.0f);
    m_rightTusk->setRotateAngleZ(0.6981317f);
    m_head->addChild(m_rightTusk);

    // 左耳（作为头部子部件）: textureOffset(10, 13), addBox(-1, -11, -1, 2, 11, 2)
    // rotationPoint(-7, 2, -12)
    m_leftEar = std::make_shared<ModelRenderer>("leftEar");
    m_leftEar->setTextureOffset(10, 13);
    m_leftEar->addBox(-1.0f, -11.0f, -1.0f, 2.0f, 11.0f, 2.0f);
    m_leftEar->setRotationPoint(-7.0f, 2.0f, -12.0f);
    m_head->addChild(m_leftEar);

    // 右耳: textureOffset(1, 13), addBox(-1, -11, -1, 2, 11, 2)
    // rotationPoint(7, 2, -12)
    m_rightEar = std::make_shared<ModelRenderer>("rightEar");
    m_rightEar->setTextureOffset(1, 13);
    m_rightEar->addBox(-1.0f, -11.0f, -1.0f, 2.0f, 11.0f, 2.0f);
    m_rightEar->setRotationPoint(7.0f, 2.0f, -12.0f);
    m_head->addChild(m_rightEar);

    // 右前腿: textureOffset(66, 42), addBox(-3, 0, -3, 6, 14, 6), rotationPoint(-4, 10, -8.5)
    m_rightFrontLeg = std::make_shared<ModelRenderer>("rightFrontLeg");
    m_rightFrontLeg->setTextureOffset(66, 42);
    m_rightFrontLeg->addBox(-3.0f, 0.0f, -3.0f, 6.0f, 14.0f, 6.0f);
    m_rightFrontLeg->setRotationPoint(-4.0f, 10.0f, -8.5f);

    // 左前腿: textureOffset(41, 42), addBox(-3, 0, -3, 6, 14, 6), rotationPoint(4, 10, -8.5)
    m_leftFrontLeg = std::make_shared<ModelRenderer>("leftFrontLeg");
    m_leftFrontLeg->setTextureOffset(41, 42);
    m_leftFrontLeg->addBox(-3.0f, 0.0f, -3.0f, 6.0f, 14.0f, 6.0f);
    m_leftFrontLeg->setRotationPoint(4.0f, 10.0f, -8.5f);

    // 右后腿: textureOffset(21, 45), addBox(-2.5, 0, -2.5, 5, 11, 5), rotationPoint(-5, 13, 10)
    m_rightBackLeg = std::make_shared<ModelRenderer>("rightBackLeg");
    m_rightBackLeg->setTextureOffset(21, 45);
    m_rightBackLeg->addBox(-2.5f, 0.0f, -2.5f, 5.0f, 11.0f, 5.0f);
    m_rightBackLeg->setRotationPoint(-5.0f, 13.0f, 10.0f);

    // 左后腿: textureOffset(0, 45), addBox(-2.5, 0, -2.5, 5, 11, 5), rotationPoint(5, 13, 10)
    m_leftBackLeg = std::make_shared<ModelRenderer>("leftBackLeg");
    m_leftBackLeg->setTextureOffset(0, 45);
    m_leftBackLeg->addBox(-2.5f, 0.0f, -2.5f, 5.0f, 11.0f, 5.0f);
    m_leftBackLeg->setRotationPoint(5.0f, 13.0f, 10.0f);

    // 添加到部件列表
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_rightFrontLeg);
    m_parts.push_back(m_leftFrontLeg);
    m_parts.push_back(m_rightBackLeg);
    m_parts.push_back(m_leftBackLeg);
}

std::vector<std::shared_ptr<ModelRenderer>> BoarModel::getHeadParts() const
{
    return {m_head};
}

std::vector<std::shared_ptr<ModelRenderer>> BoarModel::getBodyParts() const
{
    return {m_body, m_rightFrontLeg, m_leftFrontLeg, m_rightBackLeg, m_leftBackLeg};
}

void BoarModel::render(f64 scale)
{
    ::mc::client::renderer::entity::model::AgeableModel::render(scale);
}

void BoarModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 参考 MC 1.16.5 BoarModel.setRotationAngles
    // 獠牙动画: rotateAngleZ += limbSwingAmount * sin(limbSwing)
    f32 tuskSwing = static_cast<f32>(limbSwingAmount * std::sin(limbSwing));
    m_leftTusk->setRotateAngleZ(-0.6981317f - tuskSwing);
    m_rightTusk->setRotateAngleZ(0.6981317f + tuskSwing);

    // 头部旋转
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0));
    // 头部 X 旋转: lerp(f, 0.87266463F, -0.34906584F) 基于 func_230290_eL_()
    // 简化版本：保持基础角度
    m_head->setRotateAngleX(0.87266463f);

    // 腿部动画
    f32 legSwing = static_cast<f32>(std::cos(limbSwing) * 1.2 * limbSwingAmount);
    m_rightFrontLeg->setRotateAngleX(legSwing);
    m_leftFrontLeg->setRotateAngleX(
        static_cast<f32>(std::cos(limbSwing + mc::math::PI_DOUBLE) * 1.2 * limbSwingAmount));
    m_rightBackLeg->setRotateAngleX(m_leftFrontLeg->rotateAngleX());
    m_leftBackLeg->setRotateAngleX(m_rightFrontLeg->rotateAngleX());

    (void)ageInTicks;
    (void)scale;
}

// ==================== StriderModel ====================

StriderModel::StriderModel()
    : EntityModel()
{
    setTextureSize(64, 128); // Java: textureHeight = 128
    setupParts();
}

void StriderModel::setupParts()
{
    // 参考 MC 1.16.5 StriderModel
    // 身体: textureOffset(0, 0), rotationPoint(0, 1, 0), addBox(-8, -6, -8, 16, 14, 16)
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-8.0f, -6.0f, -8.0f, 16.0f, 14.0f, 16.0f);
    m_body->setRotationPoint(0.0f, 1.0f, 0.0f);
    m_parts.push_back(m_body);

    // 左腿: textureOffset(0, 32), rotationPoint(-4, 8, 0), addBox(-2, 0, -2, 4, 16, 4)
    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");
    m_leftLeg->setTextureOffset(0, 32);
    m_leftLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 16.0f, 4.0f);
    m_leftLeg->setRotationPoint(-4.0f, 8.0f, 0.0f);
    m_parts.push_back(m_leftLeg);

    // 右腿: textureOffset(0, 55), rotationPoint(4, 8, 0), addBox(-2, 0, -2, 4, 16, 4)
    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_rightLeg->setTextureOffset(0, 55);
    m_rightLeg->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 16.0f, 4.0f);
    m_rightLeg->setRotationPoint(4.0f, 8.0f, 0.0f);
    m_parts.push_back(m_rightLeg);

    // 6 个毛发/皮瓣部件（作为身体子部件）
    // Java 原版中左侧三个皮瓣都设置了 mirror=true
    // 左下皮瓣: textureOffset(16, 65), mirror=true, addBox(-12, 0, 0, 12, 0, 16)
    m_flapLeftBottom = std::make_shared<ModelRenderer>("flapLeftBottom");
    m_flapLeftBottom->setMirror(true); // Java: mirror=true
    m_flapLeftBottom->setTextureOffset(16, 65);
    m_flapLeftBottom->addBox(-12.0f, 0.0f, 0.0f, 12.0f, 0.0f, 16.0f);
    m_flapLeftBottom->setRotationPoint(-8.0f, 4.0f, -8.0f);
    m_flapLeftBottom->setRotateAngleZ(-1.2217305f);
    m_body->addChild(m_flapLeftBottom);

    // 左中皮瓣: textureOffset(16, 49), mirror=true, addBox(-12, 0, 0, 12, 0, 16)
    m_flapLeftMiddle = std::make_shared<ModelRenderer>("flapLeftMiddle");
    m_flapLeftMiddle->setMirror(true); // Java: mirror=true
    m_flapLeftMiddle->setTextureOffset(16, 49);
    m_flapLeftMiddle->addBox(-12.0f, 0.0f, 0.0f, 12.0f, 0.0f, 16.0f);
    m_flapLeftMiddle->setRotationPoint(-8.0f, -1.0f, -8.0f);
    m_flapLeftMiddle->setRotateAngleZ(-1.134464f);
    m_body->addChild(m_flapLeftMiddle);

    // 左上皮瓣: textureOffset(16, 33), mirror=true, addBox(-12, 0, 0, 12, 0, 16)
    m_flapLeftTop = std::make_shared<ModelRenderer>("flapLeftTop");
    m_flapLeftTop->setMirror(true); // Java: mirror=true
    m_flapLeftTop->setTextureOffset(16, 33);
    m_flapLeftTop->addBox(-12.0f, 0.0f, 0.0f, 12.0f, 0.0f, 16.0f);
    m_flapLeftTop->setRotationPoint(-8.0f, -5.0f, -8.0f);
    m_flapLeftTop->setRotateAngleZ(-0.87266463f);
    m_body->addChild(m_flapLeftTop);

    // 右上皮瓣: textureOffset(16, 33), addBox(0, 0, 0, 12, 0, 16), rotationPoint(8, -6, -8)
    m_flapRightTop = std::make_shared<ModelRenderer>("flapRightTop");
    m_flapRightTop->setTextureOffset(16, 33);
    m_flapRightTop->addBox(0.0f, 0.0f, 0.0f, 12.0f, 0.0f, 16.0f);
    m_flapRightTop->setRotationPoint(8.0f, -6.0f, -8.0f);
    m_flapRightTop->setRotateAngleZ(0.87266463f);
    m_body->addChild(m_flapRightTop);

    // 右中皮瓣: textureOffset(16, 49), addBox(0, 0, 0, 12, 0, 16), rotationPoint(8, -2, -8)
    m_flapRightMiddle = std::make_shared<ModelRenderer>("flapRightMiddle");
    m_flapRightMiddle->setTextureOffset(16, 49);
    m_flapRightMiddle->addBox(0.0f, 0.0f, 0.0f, 12.0f, 0.0f, 16.0f);
    m_flapRightMiddle->setRotationPoint(8.0f, -2.0f, -8.0f);
    m_flapRightMiddle->setRotateAngleZ(1.134464f);
    m_body->addChild(m_flapRightMiddle);

    // 右下皮瓣: textureOffset(16, 65), addBox(0, 0, 0, 12, 0, 16), rotationPoint(8, 3, -8)
    m_flapRightBottom = std::make_shared<ModelRenderer>("flapRightBottom");
    m_flapRightBottom->setTextureOffset(16, 65);
    m_flapRightBottom->addBox(0.0f, 0.0f, 0.0f, 12.0f, 0.0f, 16.0f);
    m_flapRightBottom->setRotationPoint(8.0f, 3.0f, -8.0f);
    m_flapRightBottom->setRotateAngleZ(1.2217305f);
    m_body->addChild(m_flapRightBottom);
}

void StriderModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void StriderModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 参考 MC 1.16.5 StriderModel.setRotationAngles
    // 限制 limbSwingAmount 最大为 0.25
    f32 swingAmount = static_cast<f32>(std::min(limbSwingAmount, 0.25));

    // 身体旋转 - Java 原版：有乘客时不旋转身体
    // if (entityIn.getPassengers().size() <= 0) {
    //     this.field_239120_f_.rotateAngleX = headPitch * mc::math::PI_DOUBLE/180;
    //     this.field_239120_f_.rotateAngleY = netHeadYaw * mc::math::PI_DOUBLE/180;
    // } else {
    //     this.field_239120_f_.rotateAngleX = 0.0F;
    //     this.field_239120_f_.rotateAngleY = 0.0F;
    // }
    if (!m_hasPassengers) {
        m_body->setRotateAngleX(static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0));
        m_body->setRotateAngleY(static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0));
    } else {
        m_body->setRotateAngleX(0.0f);
        m_body->setRotateAngleY(0.0f);
    }

    // 身体 Z 轴摇摆: rotateAngleZ = 0.1F * sin(limbSwing * 1.5F) * 4.0F * limbSwingAmount
    m_body->setRotateAngleZ(0.1f * static_cast<f32>(std::sin(limbSwing * 1.5) * 4.0 * swingAmount));

    // 身体 Y 位置动画: rotationPointY = 2.0F - 2.0F * cos(limbSwing * 1.5F) * 2.0F * limbSwingAmount
    m_body->setRotationPointY(2.0f - 2.0f * static_cast<f32>(std::cos(limbSwing * 1.5) * 2.0 * swingAmount));

    // 腿部动画
    m_rightLeg->setRotateAngleX(static_cast<f32>(std::sin(limbSwing * 1.5 * 0.5) * 2.0 * swingAmount));
    m_leftLeg->setRotateAngleX(
        static_cast<f32>(std::sin(limbSwing * 1.5 * 0.5 + mc::math::PI_DOUBLE) * 2.0 * swingAmount));

    // 腿部 Z 轴旋转
    m_rightLeg->setRotateAngleZ(0.17453292f * static_cast<f32>(std::cos(limbSwing * 1.5 * 0.5) * swingAmount));
    m_leftLeg->setRotateAngleZ(
        0.17453292f * static_cast<f32>(std::cos(limbSwing * 1.5 * 0.5 + mc::math::PI_DOUBLE) * swingAmount));

    // 腿部 Y 位置动画
    m_rightLeg->setRotationPointY(
        8.0f + 2.0f * static_cast<f32>(std::sin(limbSwing * 1.5 * 0.5 + mc::math::PI_DOUBLE) * 2.0 * swingAmount));
    m_leftLeg->setRotationPointY(8.0f + 2.0f * static_cast<f32>(std::sin(limbSwing * 1.5 * 0.5) * 2.0 * swingAmount));

    // 皮瓣基础角度
    m_flapLeftBottom->setRotateAngleZ(-1.2217305f);
    m_flapLeftMiddle->setRotateAngleZ(-1.134464f);
    m_flapLeftTop->setRotateAngleZ(-0.87266463f);
    m_flapRightTop->setRotateAngleZ(0.87266463f);
    m_flapRightMiddle->setRotateAngleZ(1.134464f);
    m_flapRightBottom->setRotateAngleZ(1.2217305f);

    // 皮瓣动画叠加
    f32 f1 = static_cast<f32>(std::cos(limbSwing * 1.5 + mc::math::PI_DOUBLE) * swingAmount);
    m_flapLeftBottom->setRotateAngleZ(-1.2217305f + f1 * 1.3f);
    m_flapLeftMiddle->setRotateAngleZ(-1.134464f + f1 * 1.2f);
    m_flapLeftTop->setRotateAngleZ(-0.87266463f + f1 * 0.6f);
    m_flapRightTop->setRotateAngleZ(0.87266463f + f1 * 0.6f);
    m_flapRightMiddle->setRotateAngleZ(1.134464f + f1 * 1.2f);
    m_flapRightBottom->setRotateAngleZ(1.2217305f + f1 * 1.3f);

    // 年龄 tick 动画叠加
    m_flapLeftBottom->setRotateAngleZ(
        m_flapLeftBottom->rotateAngleZ() + 0.05f * static_cast<f32>(std::sin(ageInTicks * 1.0 * -0.4)));
    m_flapLeftMiddle->setRotateAngleZ(
        m_flapLeftMiddle->rotateAngleZ() + 0.1f * static_cast<f32>(std::sin(ageInTicks * 1.0 * 0.2)));
    m_flapLeftTop->setRotateAngleZ(
        m_flapLeftTop->rotateAngleZ() + 0.1f * static_cast<f32>(std::sin(ageInTicks * 1.0 * 0.4)));
    m_flapRightTop->setRotateAngleZ(
        m_flapRightTop->rotateAngleZ() + 0.1f * static_cast<f32>(std::sin(ageInTicks * 1.0 * 0.4)));
    m_flapRightMiddle->setRotateAngleZ(
        m_flapRightMiddle->rotateAngleZ() + 0.1f * static_cast<f32>(std::sin(ageInTicks * 1.0 * 0.2)));
    m_flapRightBottom->setRotateAngleZ(
        m_flapRightBottom->rotateAngleZ() + 0.05f * static_cast<f32>(std::sin(ageInTicks * 1.0 * -0.4)));

    (void)scale;
}

} // namespace mc::client::renderer::entity::model::nether
