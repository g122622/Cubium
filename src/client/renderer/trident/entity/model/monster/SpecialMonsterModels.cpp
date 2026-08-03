/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SpecialMonsterModels.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <memory>
#include <string>

namespace mc::client::renderer::entity::model::monster {

namespace {
// 守卫者尖刺位置常量
constexpr f32 SPINE_ROT_X[] = {1.75f, 0.25f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.5f, 1.25f, 0.75f, 0.0f, 0.0f};
constexpr f32 SPINE_ROT_Y[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.25f, 1.75f, 1.25f, 0.75f, 0.0f, 0.0f, 0.0f, 0.0f};
constexpr f32 SPINE_ROT_Z[] = {0.0f, 0.0f, 0.25f, 1.75f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.75f, 1.25f};
constexpr f32 SPINE_POS_X[] = {0.0f, 0.0f, 8.0f, -8.0f, -8.0f, 8.0f, 8.0f, -8.0f, 0.0f, 0.0f, 8.0f, -8.0f};
constexpr f32 SPINE_POS_Y[] = {-8.0f, -8.0f, -8.0f, -8.0f, 0.0f, 0.0f, 0.0f, 0.0f, 8.0f, 8.0f, 8.0f, 8.0f};
constexpr f32 SPINE_POS_Z[] = {8.0f, -8.0f, 0.0f, 0.0f, -8.0f, -8.0f, 8.0f, 8.0f, 8.0f, -8.0f, 0.0f, 0.0f};

// 蠹虫身体尺寸常量
constexpr i32 SILVERFISH_BOX_LENGTH[][3] = {
    {3, 2, 2}, {4, 3, 2}, {6, 4, 3}, {3, 3, 3}, {2, 2, 3}, {2, 1, 2}, {1, 1, 2}};
constexpr i32 SILVERFISH_TEX_POS[][2] = {{0, 0}, {0, 4}, {0, 9}, {0, 16}, {0, 22}, {11, 0}, {13, 4}};

// 末影螨身体尺寸常量
constexpr i32 ENDERMITE_BODY_SIZES[][3] = {{4, 3, 2}, {6, 4, 5}, {3, 3, 1}, {1, 2, 1}};
constexpr i32 ENDERMITE_TEX_POS[][2] = {{0, 0}, {0, 5}, {0, 14}, {0, 18}};
} // namespace

// ==================== WitherModel ====================

WitherModel::WitherModel()
    : EntityModel()
{
    setTextureSize(64, 64);
    _setupParts();
}

WitherModel::WitherModel(f32 scale)
    : EntityModel()
{
    setTextureSize(64, 64);
    // TODO: scale 参数用于盔甲层渲染，暂未实现
    (void)scale;
    _setupParts();
}

void WitherModel::_setupParts()
{
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

void WitherModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void WitherModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    f32 f = static_cast<f32>(std::cos(ageInTicks * 0.1));

    m_upperBodyParts[1]->setRotateAngleX(static_cast<f32>((0.065 + 0.05 * f) * mc::math::PI_DOUBLE));
    m_upperBodyParts[2]->setRotationPoint(-2.0f,
        static_cast<f32>(6.9 + std::cos(m_upperBodyParts[1]->rotateAngleX()) * 10.0),
        static_cast<f32>(-0.5 + std::sin(m_upperBodyParts[1]->rotateAngleX()) * 10.0));
    m_upperBodyParts[2]->setRotateAngleX(static_cast<f32>((0.265 + 0.1 * f) * mc::math::PI_DOUBLE));

    // 主头：由标准 setAngles 参数（netHeadYaw/headPitch）驱动
    m_heads[0]->setRotateAngleY(static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0));
    m_heads[0]->setRotateAngleX(static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0));

    // 侧头：由 setSideHeadRotations 提供的独立朝向驱动。
    // 对应 MC 1.21.11 WitherBossModel.setupHeadRotation(state, head, index)：
    //   head.yRot = (yHeadRots[index] - bodyRot) * PI / 180
    //   head.xRot = xHeadRots[index] * PI / 180
    // setSideHeadRotations 接收的 yaw 已是 (yHeadRots[index] - bodyRot) 的结果，
    // pitch 直接为 xHeadRots[index]。
    //
    // 当 setSideHeadRotations 未被调用时（例如 CPU 渲染路径的旧代码或测试），
    // 回退到复制主头旋转，保持视觉一致。
    if (m_hasSideHeadRotations) {
        m_heads[1]->setRotateAngleY(m_sideHeadYaw[0] * static_cast<f32>(mc::math::PI_DOUBLE / 180.0));
        m_heads[1]->setRotateAngleX(m_sideHeadPitch[0] * static_cast<f32>(mc::math::PI_DOUBLE / 180.0));
        m_heads[2]->setRotateAngleY(m_sideHeadYaw[1] * static_cast<f32>(mc::math::PI_DOUBLE / 180.0));
        m_heads[2]->setRotateAngleX(m_sideHeadPitch[1] * static_cast<f32>(mc::math::PI_DOUBLE / 180.0));
    } else {
        m_heads[1]->setRotateAngleY(static_cast<f32>(m_heads[0]->rotateAngleY()));
        m_heads[1]->setRotateAngleX(static_cast<f32>(m_heads[0]->rotateAngleX()));
        m_heads[2]->setRotateAngleY(static_cast<f32>(m_heads[0]->rotateAngleY()));
        m_heads[2]->setRotateAngleX(static_cast<f32>(m_heads[0]->rotateAngleX()));
    }

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
    _setupParts();
}

SlimeModel::SlimeModel(i32 size)
    : EntityModel()
    , m_size(size)
{
    setTextureSize(64, 32);
    _setupParts();
}

void SlimeModel::_setupParts()
{
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

void SlimeModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void SlimeModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
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
    _setupParts();
}

void GuardianModel::_setupParts()
{
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

    _updateSpines(0.0f, 0.0f);
}

void GuardianModel::_updateSpines(f64 ageInTicks, f64 spikeAnimation)
{
    for (i32 i = 0; i < 12; ++i) {
        m_spines[i]->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE * SPINE_ROT_X[i]));
        m_spines[i]->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE * SPINE_ROT_Y[i]));
        m_spines[i]->setRotateAngleZ(static_cast<f32>(mc::math::PI_DOUBLE * SPINE_ROT_Z[i]));
        f32 scale = static_cast<f32>(1.0 + std::cos(ageInTicks * 1.5 + i) * 0.01 - spikeAnimation);
        m_spines[i]->setRotationPoint(SPINE_POS_X[i] * scale, 16.0f + SPINE_POS_Y[i] * scale, SPINE_POS_Z[i] * scale);
    }
}

void GuardianModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void GuardianModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    m_body->setRotateAngleY(static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0));
    m_body->setRotateAngleX(static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0));

    // 使用成员变量中的动画值
    f32 spikeAnim = 1.0f - m_spikeAnimation;
    f32 spineScale = spikeAnim * 0.55f;
    _updateSpines(ageInTicks, spineScale);

    m_eye->setRotationPointZ(-8.25f);

    // 眼睛追踪目标实体
    if (m_targetEyeY > 0.0f) {
        m_eye->setRotationPointY(0.0f);
    } else {
        m_eye->setRotationPointY(1.0f);
    }
    m_eye->setRotationPointX(m_targetEyeOffset);

    // 尾巴动画
    f32 tailAnim = m_tailAnimation;
    m_tail[0]->setRotateAngleY(static_cast<f32>(std::sin(tailAnim) * mc::math::PI_DOUBLE * 0.05));
    m_tail[1]->setRotateAngleY(static_cast<f32>(std::sin(tailAnim) * mc::math::PI_DOUBLE * 0.1));
    m_tail[1]->setRotationPoint(-1.5f, 0.5f, 14.0f);
    m_tail[2]->setRotateAngleY(static_cast<f32>(std::sin(tailAnim) * mc::math::PI_DOUBLE * 0.15));
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
    _setupParts();
}

void ShulkerModel::_setupParts()
{
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

void ShulkerModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void ShulkerModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    f32 peekAmount = m_peekAmount; // 从成员变量获取
    f32 peekAngle = static_cast<f32>((0.5 + peekAmount) * mc::math::PI_DOUBLE);
    f32 f2 = -1.0f + static_cast<f32>(std::sin(peekAngle));
    f32 f3 = 0.0f;

    if (peekAngle > static_cast<f32>(mc::math::PI_DOUBLE)) {
        f3 = static_cast<f32>(std::sin(ageInTicks * 0.1) * 0.7);
    }

    m_lid->setRotationPoint(0.0f, static_cast<f32>(16.0 + std::sin(peekAngle) * 8.0 + f3), 0.0f);

    if (peekAmount > 0.3f) {
        m_lid->setRotateAngleY(f2 * f2 * f2 * f2 * static_cast<f32>(mc::math::PI_DOUBLE * 0.125));
    } else {
        m_lid->setRotateAngleY(0.0f);
    }

    m_head->setRotateAngleX(static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0));
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)scale;
}

// ==================== SilverfishModel ====================

SilverfishModel::SilverfishModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    _setupParts();
}

void SilverfishModel::_setupParts()
{
    f32 zPos = -3.5f;

    for (i32 i = 0; i < 7; ++i) {
        m_bodyParts[i] = std::make_shared<ModelRenderer>("body" + std::to_string(i));
        m_bodyParts[i]->setTextureOffset(SILVERFISH_TEX_POS[i][0], SILVERFISH_TEX_POS[i][1]);
        m_bodyParts[i]->addBox(static_cast<f32>(SILVERFISH_BOX_LENGTH[i][0]) * -0.5f,
            0.0f,
            static_cast<f32>(SILVERFISH_BOX_LENGTH[i][2]) * -0.5f,
            static_cast<f32>(SILVERFISH_BOX_LENGTH[i][0]),
            static_cast<f32>(SILVERFISH_BOX_LENGTH[i][1]),
            static_cast<f32>(SILVERFISH_BOX_LENGTH[i][2]));
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
    m_wings[0]->addBox(-5.0f,
        0.0f,
        static_cast<f32>(SILVERFISH_BOX_LENGTH[2][2]) * -0.5f,
        10.0f,
        8.0f,
        static_cast<f32>(SILVERFISH_BOX_LENGTH[2][2]));
    m_wings[0]->setRotationPoint(0.0f, 16.0f, m_zPlacement[2]);
    m_parts.push_back(m_wings[0]);

    m_wings[1] = std::make_shared<ModelRenderer>("wing1");
    m_wings[1]->setTextureOffset(20, 11);
    m_wings[1]->addBox(-3.0f,
        0.0f,
        static_cast<f32>(SILVERFISH_BOX_LENGTH[4][2]) * -0.5f,
        6.0f,
        4.0f,
        static_cast<f32>(SILVERFISH_BOX_LENGTH[4][2]));
    m_wings[1]->setRotationPoint(0.0f, 20.0f, m_zPlacement[4]);
    m_parts.push_back(m_wings[1]);

    m_wings[2] = std::make_shared<ModelRenderer>("wing2");
    m_wings[2]->setTextureOffset(20, 18);
    m_wings[2]->addBox(-3.0f,
        0.0f,
        static_cast<f32>(SILVERFISH_BOX_LENGTH[4][2]) * -0.5f,
        6.0f,
        5.0f,
        static_cast<f32>(SILVERFISH_BOX_LENGTH[1][2]));
    m_wings[2]->setRotationPoint(0.0f, 19.0f, m_zPlacement[1]);
    m_parts.push_back(m_wings[2]);
}

void SilverfishModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void SilverfishModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    for (i32 i = 0; i < 7; ++i) {
        m_bodyParts[i]->setRotateAngleY(static_cast<f32>(std::cos(ageInTicks * 0.9 + i * 0.15 * mc::math::PI_DOUBLE) *
            mc::math::PI_DOUBLE * 0.05 * (1 + std::abs(i - 2))));
        m_bodyParts[i]->setRotationPointX(static_cast<f32>(
            std::sin(ageInTicks * 0.9 + i * 0.15 * mc::math::PI_DOUBLE) * mc::math::PI_DOUBLE * 0.2 * std::abs(i - 2)));
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
    _setupParts();
}

void EndermiteModel::_setupParts()
{
    f32 zPos = -3.5f;

    for (i32 i = 0; i < 4; ++i) {
        m_bodyParts[i] = std::make_shared<ModelRenderer>("body" + std::to_string(i));
        m_bodyParts[i]->setTextureOffset(ENDERMITE_TEX_POS[i][0], ENDERMITE_TEX_POS[i][1]);
        m_bodyParts[i]->addBox(static_cast<f32>(ENDERMITE_BODY_SIZES[i][0]) * -0.5f,
            0.0f,
            static_cast<f32>(ENDERMITE_BODY_SIZES[i][2]) * -0.5f,
            static_cast<f32>(ENDERMITE_BODY_SIZES[i][0]),
            static_cast<f32>(ENDERMITE_BODY_SIZES[i][1]),
            static_cast<f32>(ENDERMITE_BODY_SIZES[i][2]));
        m_bodyParts[i]->setRotationPoint(0.0f, static_cast<f32>(24 - ENDERMITE_BODY_SIZES[i][1]), zPos);
        m_parts.push_back(m_bodyParts[i]);

        if (i < 3) {
            zPos += static_cast<f32>((ENDERMITE_BODY_SIZES[i][2] + ENDERMITE_BODY_SIZES[i + 1][2]) * 0.5);
        }
    }
}

void EndermiteModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void EndermiteModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    for (i32 i = 0; i < 4; ++i) {
        m_bodyParts[i]->setRotateAngleY(static_cast<f32>(std::cos(ageInTicks * 0.9 + i * 0.15 * mc::math::PI_DOUBLE) *
            mc::math::PI_DOUBLE * 0.01 * (1 + std::abs(i - 2))));
        m_bodyParts[i]->setRotationPointX(static_cast<f32>(
            std::sin(ageInTicks * 0.9 + i * 0.15 * mc::math::PI_DOUBLE) * mc::math::PI_DOUBLE * 0.1 * std::abs(i - 2)));
    }

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

} // namespace mc::client::renderer::entity::model::monster
