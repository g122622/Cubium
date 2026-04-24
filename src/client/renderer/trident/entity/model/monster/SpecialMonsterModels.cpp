#include "SpecialMonsterModels.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

namespace {
    constexpr f64 PI = 3.14159265359;

    // Guard spine positions from MC 1.16.5
    constexpr f32 SPINE_ROT_X[] = {1.75f, 0.25f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.5f, 1.25f, 0.75f, 0.0f, 0.0f};
    constexpr f32 SPINE_ROT_Y[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.25f, 1.75f, 1.25f, 0.75f, 0.0f, 0.0f, 0.0f, 0.0f};
    constexpr f32 SPINE_ROT_Z[] = {0.0f, 0.0f, 0.25f, 1.75f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.75f, 1.25f};
    constexpr f32 SPINE_POS_X[] = {0.0f, 0.0f, 8.0f, -8.0f, -8.0f, 8.0f, 8.0f, -8.0f, 0.0f, 0.0f, 8.0f, -8.0f};
    constexpr f32 SPINE_POS_Y[] = {-8.0f, -8.0f, -8.0f, -8.0f, 0.0f, 0.0f, 0.0f, 0.0f, 8.0f, 8.0f, 8.0f, 8.0f};
    constexpr f32 SPINE_POS_Z[] = {8.0f, -8.0f, 0.0f, 0.0f, -8.0f, -8.0f, 8.0f, 8.0f, 8.0f, -8.0f, 0.0f, 0.0f};

    // Silverfish body sizes from MC 1.16.5
    constexpr i32 SILVERFISH_BOX_LENGTH[][3] = {
        {3, 2, 2}, {4, 3, 2}, {6, 4, 3}, {3, 3, 3}, {2, 2, 3}, {2, 1, 2}, {1, 1, 2}
    };
    constexpr i32 SILVERFISH_TEX_POS[][2] = {
        {0, 0}, {0, 4}, {0, 9}, {0, 16}, {0, 22}, {11, 0}, {13, 4}
    };

    // Endermite body sizes from MC 1.16.5
    constexpr i32 ENDERMITE_BODY_SIZES[][3] = {
        {4, 3, 2}, {6, 4, 5}, {3, 3, 1}, {1, 2, 1}
    };
    constexpr i32 ENDERMITE_TEX_POS[][2] = {
        {0, 0}, {0, 5}, {0, 14}, {0, 18}
    };
}

// ==================== WitherModel ====================

WitherModel::WitherModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    setupParts();
}

WitherModel::WitherModel(f32 scale)
    : EntityModel()
{
    setTextureSize(64, 64);
    // Note: scale parameter used for armor layer in MC 1.16.5
    (void)scale;
    setupParts();
}

void WitherModel::setupParts() {
    // 参考 MC 1.16.5 WitherModel

    // 三个上半身部件
    m_upperBodyParts[0] = std::make_shared<ModelRenderer>("upperBody0");
    m_upperBodyParts[0]->setTextureOffset(0, 16);
    m_upperBodyParts[0]->addBox(-10.0f, 3.9f, -0.5f, 20.0f, 3.0f, 3.0f);
    m_parts.push_back(m_upperBodyParts[0]);

    m_upperBodyParts[1] = std::make_shared<ModelRenderer>("upperBody1");
    m_upperBodyParts[1]->setRotationPoint(-2.0f, 6.9f, -0.5f);
    m_upperBodyParts[1]->setTextureOffset(0, 22);
    m_upperBodyParts[1]->addBox(0.0f, 0.0f, 0.0f, 3.0f, 10.0f, 3.0f);
    m_upperBodyParts[1]->setTextureOffset(24, 22);
    m_upperBodyParts[1]->addBox(-4.0f, 1.5f, 0.5f, 11.0f, 2.0f, 2.0f);
    m_upperBodyParts[1]->setTextureOffset(24, 22);
    m_upperBodyParts[1]->addBox(-4.0f, 4.0f, 0.5f, 11.0f, 2.0f, 2.0f);
    m_upperBodyParts[1]->setTextureOffset(24, 22);
    m_upperBodyParts[1]->addBox(-4.0f, 6.5f, 0.5f, 11.0f, 2.0f, 2.0f);
    m_parts.push_back(m_upperBodyParts[1]);

    m_upperBodyParts[2] = std::make_shared<ModelRenderer>("upperBody2");
    m_upperBodyParts[2]->setTextureOffset(12, 22);
    m_upperBodyParts[2]->addBox(0.0f, 0.0f, 0.0f, 3.0f, 6.0f, 3.0f);
    m_parts.push_back(m_upperBodyParts[2]);

    // 三个头
    m_heads[0] = std::make_shared<ModelRenderer>("head0");
    m_heads[0]->setTextureOffset(0, 0);
    m_heads[0]->addBox(-4.0f, -4.0f, -4.0f, 8.0f, 8.0f, 8.0f);
    m_parts.push_back(m_heads[0]);

    m_heads[1] = std::make_shared<ModelRenderer>("head1");
    m_heads[1]->setTextureOffset(32, 0);
    m_heads[1]->addBox(-4.0f, -4.0f, -4.0f, 6.0f, 6.0f, 6.0f);
    m_heads[1]->setRotationPoint(-8.0f, 4.0f, 0.0f);
    m_parts.push_back(m_heads[1]);

    m_heads[2] = std::make_shared<ModelRenderer>("head2");
    m_heads[2]->setTextureOffset(32, 0);
    m_heads[2]->addBox(-4.0f, -4.0f, -4.0f, 6.0f, 6.0f, 6.0f);
    m_heads[2]->setRotationPoint(10.0f, 4.0f, 0.0f);
    m_parts.push_back(m_heads[2]);
}

void WitherModel::render(f64 scale) {
    EntityModel::render(scale);
}

void WitherModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                             f64 ageInTicks, f64 netHeadYaw,
                             f64 headPitch, f64 scale) {
    f32 f = static_cast<f32>(std::cos(ageInTicks * 0.1));

    m_upperBodyParts[1]->setRotateAngleX(static_cast<f32>((0.065 + 0.05 * f) * PI));
    m_upperBodyParts[2]->setRotationPoint(-2.0f, static_cast<f32>(6.9 + std::cos(m_upperBodyParts[1]->rotateAngleX()) * 10.0),
                                           static_cast<f32>(-0.5 + std::sin(m_upperBodyParts[1]->rotateAngleX()) * 10.0));
    m_upperBodyParts[2]->setRotateAngleX(static_cast<f32>((0.265 + 0.1 * f) * PI));

    m_heads[0]->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_heads[0]->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    // Side heads would need entity data for head rotation
    // For now, just copy main head rotation
    m_heads[1]->setRotateAngleY(static_cast<f32>(m_heads[0]->rotateAngleY()));
    m_heads[1]->setRotateAngleX(static_cast<f32>(m_heads[0]->rotateAngleX()));
    m_heads[2]->setRotateAngleY(static_cast<f32>(m_heads[0]->rotateAngleY()));
    m_heads[2]->setRotateAngleX(static_cast<f32>(m_heads[0]->rotateAngleX()));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)scale;
}

// ==================== SlimeModel ====================

SlimeModel::SlimeModel()
    : EntityModel()
    , m_size(0)
{
    setTextureSize(64, 32);
    setupParts();
}

SlimeModel::SlimeModel(i32 size)
    : EntityModel()
    , m_size(size)
{
    setTextureSize(64, 32);
    setupParts();
}

void SlimeModel::setupParts() {
    // 参考 MC 1.16.5 SlimeModel
    // size > 0 表示小史莱姆，size == 0 表示大史莱姆

    m_body = std::make_shared<ModelRenderer>("body");
    m_rightEye = std::make_shared<ModelRenderer>("rightEye");
    m_leftEye = std::make_shared<ModelRenderer>("leftEye");
    m_mouth = std::make_shared<ModelRenderer>("mouth");

    if (m_size > 0) {
        // 小史莱姆
        m_body->setTextureOffset(0, m_size);
        m_body->addBox(-3.0f, 17.0f, -3.0f, 6.0f, 6.0f, 6.0f);

        m_rightEye->setTextureOffset(32, 0);
        m_rightEye->addBox(-3.25f, 18.0f, -3.5f, 2.0f, 2.0f, 2.0f);

        m_leftEye->setTextureOffset(32, 4);
        m_leftEye->addBox(1.25f, 18.0f, -3.5f, 2.0f, 2.0f, 2.0f);

        m_mouth->setTextureOffset(32, 8);
        m_mouth->addBox(0.0f, 21.0f, -3.5f, 1.0f, 1.0f, 1.0f);
    } else {
        // 大史莱姆
        m_body->setTextureOffset(0, 0);
        m_body->addBox(-4.0f, 16.0f, -4.0f, 8.0f, 8.0f, 8.0f);
    }

    m_parts.push_back(m_body);
    m_parts.push_back(m_rightEye);
    m_parts.push_back(m_leftEye);
    m_parts.push_back(m_mouth);
}

void SlimeModel::render(f64 scale) {
    EntityModel::render(scale);
}

void SlimeModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                            f64 ageInTicks, f64 netHeadYaw,
                            f64 headPitch, f64 scale) {
    // 史莱姆没有动画
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== GuardianModel ====================

GuardianModel::GuardianModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    setupParts();
}

void GuardianModel::setupParts() {
    // 参考 MC 1.16.5 GuardianModel

    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-6.0f, 10.0f, -8.0f, 12.0f, 12.0f, 16.0f);
    m_body->setTextureOffset(0, 28);
    m_body->addBox(-8.0f, 10.0f, -6.0f, 2.0f, 12.0f, 12.0f);
    m_body->setTextureOffset(0, 28);
    m_body->addBox(6.0f, 10.0f, -6.0f, 2.0f, 12.0f, 12.0f, true);
    m_body->setTextureOffset(16, 40);
    m_body->addBox(-6.0f, 8.0f, -6.0f, 12.0f, 2.0f, 12.0f);
    m_body->setTextureOffset(16, 40);
    m_body->addBox(-6.0f, 22.0f, -6.0f, 12.0f, 2.0f, 12.0f);
    m_parts.push_back(m_body);

    // 12个刺
    for (i32 i = 0; i < 12; ++i) {
        m_spines[i] = std::make_shared<ModelRenderer>("spine" + std::to_string(i));
        m_spines[i]->setTextureOffset(0, 0);
        m_spines[i]->addBox(-1.0f, -4.5f, -1.0f, 2.0f, 9.0f, 2.0f);
        // 刺作为身体的子部件
        m_parts.push_back(m_spines[i]);
    }

    // 眼睛
    m_eye = std::make_shared<ModelRenderer>("eye");
    m_eye->setTextureOffset(8, 0);
    m_eye->addBox(-1.0f, 15.0f, 0.0f, 2.0f, 2.0f, 1.0f);
    m_parts.push_back(m_eye);

    // 3节尾巴
    m_tail[0] = std::make_shared<ModelRenderer>("tail0");
    m_tail[0]->setTextureOffset(40, 0);
    m_tail[0]->addBox(-2.0f, 14.0f, 7.0f, 4.0f, 4.0f, 8.0f);
    m_parts.push_back(m_tail[0]);

    m_tail[1] = std::make_shared<ModelRenderer>("tail1");
    m_tail[1]->setTextureOffset(0, 54);
    m_tail[1]->addBox(0.0f, 14.0f, 0.0f, 3.0f, 3.0f, 7.0f);
    m_parts.push_back(m_tail[1]);

    m_tail[2] = std::make_shared<ModelRenderer>("tail2");
    m_tail[2]->setTextureOffset(41, 32);
    m_tail[2]->addBox(0.0f, 14.0f, 0.0f, 2.0f, 2.0f, 6.0f);
    m_tail[2]->setTextureOffset(25, 19);
    m_tail[2]->addBox(1.0f, 10.5f, 3.0f, 1.0f, 9.0f, 9.0f);
    m_parts.push_back(m_tail[2]);

    updateSpines(0.0f, 0.0f);
}

void GuardianModel::updateSpines(f64 ageInTicks, f64 spikeAnimation) {
    for (i32 i = 0; i < 12; ++i) {
        m_spines[i]->setRotateAngleX(static_cast<f32>(PI * SPINE_ROT_X[i]));
        m_spines[i]->setRotateAngleY(static_cast<f32>(PI * SPINE_ROT_Y[i]));
        m_spines[i]->setRotateAngleZ(static_cast<f32>(PI * SPINE_ROT_Z[i]));
        f32 scale = static_cast<f32>(1.0 + std::cos(ageInTicks * 1.5 + i) * 0.01 - spikeAnimation);
        m_spines[i]->setRotationPoint(SPINE_POS_X[i] * scale, 16.0f + SPINE_POS_Y[i] * scale, SPINE_POS_Z[i] * scale);
    }
}

void GuardianModel::render(f64 scale) {
    EntityModel::render(scale);
}

void GuardianModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                               f64 ageInTicks, f64 netHeadYaw,
                               f64 headPitch, f64 scale) {
    m_body->setRotateAngleY(static_cast<f32>(netHeadYaw * PI / 180.0));
    m_body->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));

    updateSpines(ageInTicks, 0.0f);

    m_eye->setRotationPointZ(-8.25f);

    // 尾巴摆动
    f32 tailAnim = static_cast<f32>(std::sin(ageInTicks * 0.5) * PI * 0.05);
    m_tail[0]->setRotateAngleY(tailAnim);
    m_tail[1]->setRotateAngleY(static_cast<f32>(std::sin(ageInTicks * 0.5) * PI * 0.1));
    m_tail[1]->setRotationPoint(-1.5f, 0.5f, 14.0f);
    m_tail[2]->setRotateAngleY(static_cast<f32>(std::sin(ageInTicks * 0.5) * PI * 0.15));
    m_tail[2]->setRotationPoint(0.5f, 0.5f, 6.0f);

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)scale;
}

// ==================== ElderGuardianModel ====================

ElderGuardianModel::ElderGuardianModel()
    : GuardianModel()
{
    // 远古守卫者使用相同的模型结构，但不同的纹理
}

// ==================== ShulkerModel ====================

ShulkerModel::ShulkerModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    setupParts();
}

void ShulkerModel::setupParts() {
    // 参考 MC 1.16.5 ShulkerModel

    m_base = std::make_shared<ModelRenderer>("base");
    m_base->setTextureOffset(0, 28);
    m_base->addBox(-8.0f, -8.0f, -8.0f, 16.0f, 8.0f, 16.0f);
    m_base->setRotationPoint(0.0f, 24.0f, 0.0f);
    m_parts.push_back(m_base);

    m_lid = std::make_shared<ModelRenderer>("lid");
    m_lid->setTextureOffset(0, 0);
    m_lid->addBox(-8.0f, -16.0f, -8.0f, 16.0f, 12.0f, 16.0f);
    m_lid->setRotationPoint(0.0f, 24.0f, 0.0f);
    m_parts.push_back(m_lid);

    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 52);
    m_head->addBox(-3.0f, 0.0f, -3.0f, 6.0f, 6.0f, 6.0f);
    m_head->setRotationPoint(0.0f, 12.0f, 0.0f);
    m_parts.push_back(m_head);
}

void ShulkerModel::render(f64 scale) {
    EntityModel::render(scale);
}

void ShulkerModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                              f64 ageInTicks, f64 netHeadYaw,
                              f64 headPitch, f64 scale) {
    // 计算 peek 动画 (0.0 到 1.0)
    f32 peekAmount = 0.0f; // 实际应从实体获取
    f32 peekAngle = static_cast<f32>((0.5 + peekAmount) * PI);
    f32 f2 = -1.0f + static_cast<f32>(std::sin(peekAngle));
    f32 f3 = 0.0f;

    if (peekAngle > static_cast<f32>(PI)) {
        f3 = static_cast<f32>(std::sin(ageInTicks * 0.1) * 0.7);
    }

    m_lid->setRotationPoint(0.0f, static_cast<f32>(16.0 + std::sin(peekAngle) * 8.0 + f3), 0.0f);

    if (peekAmount > 0.3f) {
        m_lid->setRotateAngleY(f2 * f2 * f2 * f2 * static_cast<f32>(PI * 0.125));
    } else {
        m_lid->setRotateAngleY(0.0f);
    }

    m_head->setRotateAngleX(static_cast<f32>(headPitch * PI / 180.0));
    // head Y rotation would use entity's rotationYawHead

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)scale;
}

// ==================== SilverfishModel ====================

SilverfishModel::SilverfishModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    setupParts();
}

void SilverfishModel::setupParts() {
    // 参考 MC 1.16.5 SilverfishModel
    f32 zPos = -3.5f;

    for (i32 i = 0; i < 7; ++i) {
        m_bodyParts[i] = std::make_shared<ModelRenderer>("body" + std::to_string(i));
        m_bodyParts[i]->setTextureOffset(SILVERFISH_TEX_POS[i][0], SILVERFISH_TEX_POS[i][1]);
        m_bodyParts[i]->addBox(
            static_cast<f32>(SILVERFISH_BOX_LENGTH[i][0]) * -0.5f, 0.0f,
            static_cast<f32>(SILVERFISH_BOX_LENGTH[i][2]) * -0.5f,
            static_cast<f32>(SILVERFISH_BOX_LENGTH[i][0]),
            static_cast<f32>(SILVERFISH_BOX_LENGTH[i][1]),
            static_cast<f32>(SILVERFISH_BOX_LENGTH[i][2])
        );
        m_bodyParts[i]->setRotationPoint(0.0f, static_cast<f32>(24 - SILVERFISH_BOX_LENGTH[i][1]), zPos);
        m_zPlacement[i] = zPos;
        m_parts.push_back(m_bodyParts[i]);

        if (i < 6) {
            zPos += static_cast<f32>((SILVERFISH_BOX_LENGTH[i][2] + SILVERFISH_BOX_LENGTH[i + 1][2]) * 0.5);
        }
    }

    // 3个翅膀
    m_wings[0] = std::make_shared<ModelRenderer>("wing0");
    m_wings[0]->setTextureOffset(20, 0);
    m_wings[0]->addBox(-5.0f, 0.0f, static_cast<f32>(SILVERFISH_BOX_LENGTH[2][2]) * -0.5f, 10.0f, 8.0f, static_cast<f32>(SILVERFISH_BOX_LENGTH[2][2]));
    m_wings[0]->setRotationPoint(0.0f, 16.0f, m_zPlacement[2]);
    m_parts.push_back(m_wings[0]);

    m_wings[1] = std::make_shared<ModelRenderer>("wing1");
    m_wings[1]->setTextureOffset(20, 11);
    m_wings[1]->addBox(-3.0f, 0.0f, static_cast<f32>(SILVERFISH_BOX_LENGTH[4][2]) * -0.5f, 6.0f, 4.0f, static_cast<f32>(SILVERFISH_BOX_LENGTH[4][2]));
    m_wings[1]->setRotationPoint(0.0f, 20.0f, m_zPlacement[4]);
    m_parts.push_back(m_wings[1]);

    m_wings[2] = std::make_shared<ModelRenderer>("wing2");
    m_wings[2]->setTextureOffset(20, 18);
    m_wings[2]->addBox(-3.0f, 0.0f, static_cast<f32>(SILVERFISH_BOX_LENGTH[4][2]) * -0.5f, 6.0f, 5.0f, static_cast<f32>(SILVERFISH_BOX_LENGTH[1][2]));
    m_wings[2]->setRotationPoint(0.0f, 19.0f, m_zPlacement[1]);
    m_parts.push_back(m_wings[2]);
}

void SilverfishModel::render(f64 scale) {
    EntityModel::render(scale);
}

void SilverfishModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                 f64 ageInTicks, f64 netHeadYaw,
                                 f64 headPitch, f64 scale) {
    for (i32 i = 0; i < 7; ++i) {
        m_bodyParts[i]->setRotateAngleY(static_cast<f32>(std::cos(ageInTicks * 0.9 + i * 0.15 * PI) * PI * 0.05 * (1 + std::abs(i - 2))));
        m_bodyParts[i]->setRotationPointX(static_cast<f32>(std::sin(ageInTicks * 0.9 + i * 0.15 * PI) * PI * 0.2 * std::abs(i - 2)));
    }

    m_wings[0]->setRotateAngleY(static_cast<f32>(m_bodyParts[2]->rotateAngleY()));
    m_wings[1]->setRotateAngleY(static_cast<f32>(m_bodyParts[4]->rotateAngleY()));
    m_wings[1]->setRotationPointX(static_cast<f32>(m_bodyParts[4]->rotationPointX()));
    m_wings[2]->setRotateAngleY(static_cast<f32>(m_bodyParts[1]->rotateAngleY()));
    m_wings[2]->setRotationPointX(static_cast<f32>(m_bodyParts[1]->rotationPointX()));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== EndermiteModel ====================

EndermiteModel::EndermiteModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    setupParts();
}

void EndermiteModel::setupParts() {
    // 参考 MC 1.16.5 EndermiteModel
    f32 zPos = -3.5f;

    for (i32 i = 0; i < 4; ++i) {
        m_bodyParts[i] = std::make_shared<ModelRenderer>("body" + std::to_string(i));
        m_bodyParts[i]->setTextureOffset(ENDERMITE_TEX_POS[i][0], ENDERMITE_TEX_POS[i][1]);
        m_bodyParts[i]->addBox(
            static_cast<f32>(ENDERMITE_BODY_SIZES[i][0]) * -0.5f, 0.0f,
            static_cast<f32>(ENDERMITE_BODY_SIZES[i][2]) * -0.5f,
            static_cast<f32>(ENDERMITE_BODY_SIZES[i][0]),
            static_cast<f32>(ENDERMITE_BODY_SIZES[i][1]),
            static_cast<f32>(ENDERMITE_BODY_SIZES[i][2])
        );
        m_bodyParts[i]->setRotationPoint(0.0f, static_cast<f32>(24 - ENDERMITE_BODY_SIZES[i][1]), zPos);
        m_parts.push_back(m_bodyParts[i]);

        if (i < 3) {
            zPos += static_cast<f32>((ENDERMITE_BODY_SIZES[i][2] + ENDERMITE_BODY_SIZES[i + 1][2]) * 0.5);
        }
    }
}

void EndermiteModel::render(f64 scale) {
    EntityModel::render(scale);
}

void EndermiteModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                f64 ageInTicks, f64 netHeadYaw,
                                f64 headPitch, f64 scale) {
    for (i32 i = 0; i < 4; ++i) {
        m_bodyParts[i]->setRotateAngleY(static_cast<f32>(std::cos(ageInTicks * 0.9 + i * 0.15 * PI) * PI * 0.01 * (1 + std::abs(i - 2))));
        m_bodyParts[i]->setRotationPointX(static_cast<f32>(std::sin(ageInTicks * 0.9 + i * 0.15 * PI) * PI * 0.1 * std::abs(i - 2)));
    }

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

} // namespace mc::client::renderer::entity::model::monster
