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

#include "ProjectileModels.hpp"
#include "client/renderer/trident/entity/model/core/EntityModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <memory>

namespace mc::client::renderer::entity::model::projectile {

// ==================== TridentModel ====================

TridentModel::TridentModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    _setupParts();
}

void TridentModel::_setupParts()
{
    // 纹理尺寸 32x32

    // 主杆: 纹理 (0, 6), 尺寸 1x25x1
    // addBox(-0.5F, 2.0F, -0.5F, 1.0F, 25.0F, 1.0F, 0.0F)
    m_shaft = std::make_shared<ModelRenderer>("shaft");
    m_shaft->setTextureOffset(0, 6);
    m_shaft->addBox(-0.5f, 2.0f, -0.5f, 1.0f, 25.0f, 1.0f);
    m_shaft->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_shaft);

    // 横杆: 纹理 (4, 0), 尺寸 3x2x1
    // addBox(-1.5F, 0.0F, -0.5F, 3.0F, 2.0F, 1.0F)
    m_crossbar = std::make_shared<ModelRenderer>("crossbar");
    m_crossbar->setTextureOffset(4, 0);
    m_crossbar->addBox(-1.5f, 0.0f, -0.5f, 3.0f, 2.0f, 1.0f);
    m_shaft->addChild(m_crossbar);

    // 左叉尖: 纹理 (4, 3), 尺寸 1x4x1
    // addBox(-2.5F, -3.0F, -0.5F, 1.0F, 4.0F, 1.0F)
    m_leftProng = std::make_shared<ModelRenderer>("leftProng");
    m_leftProng->setTextureOffset(4, 3);
    m_leftProng->addBox(-2.5f, -3.0f, -0.5f, 1.0f, 4.0f, 1.0f);
    m_shaft->addChild(m_leftProng);

    // 中叉尖: 纹理 (0, 0), 尺寸 1x4x1
    // addBox(-0.5F, -4.0F, -0.5F, 1.0F, 4.0F, 1.0F, 0.0F)
    m_middleProng = std::make_shared<ModelRenderer>("middleProng");
    m_middleProng->setTextureOffset(0, 0);
    m_middleProng->addBox(-0.5f, -4.0f, -0.5f, 1.0f, 4.0f, 1.0f);
    m_shaft->addChild(m_middleProng);

    // 右叉尖: 纹理 (4, 3), 镜像, 尺寸 1x4x1
    // addBox(1.5F, -3.0F, -0.5F, 1.0F, 4.0F, 1.0F)
    // mirror = true
    m_rightProng = std::make_shared<ModelRenderer>("rightProng");
    m_rightProng->setMirror(true);
    m_rightProng->setTextureOffset(4, 3);
    m_rightProng->addBox(1.5f, -3.0f, -0.5f, 1.0f, 4.0f, 1.0f);
    m_shaft->addChild(m_rightProng);
}

void TridentModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void TridentModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 三叉戟没有动画角度设置
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== ShulkerBulletModel ====================

ShulkerBulletModel::ShulkerBulletModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    _setupParts();
}

void ShulkerBulletModel::_setupParts()
{
    m_bullet = std::make_shared<ModelRenderer>("bullet");
    m_bullet->setTextureOffset(0, 0);
    m_bullet->addBox(-4.0f, -4.0f, -1.0f, 8.0f, 8.0f, 2.0f);
    m_bullet->setTextureOffset(0, 10);
    m_bullet->addBox(-1.0f, -4.0f, -4.0f, 2.0f, 8.0f, 8.0f);
    m_bullet->setTextureOffset(20, 0);
    m_bullet->addBox(-4.0f, -1.0f, -4.0f, 8.0f, 2.0f, 8.0f);
    m_bullet->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_bullet);
}

void ShulkerBulletModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void ShulkerBulletModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    m_bullet->setRotateAngleY(mc::math::toRadians(static_cast<f32>(netHeadYaw)));
    m_bullet->setRotateAngleX(mc::math::toRadians(static_cast<f32>(headPitch)));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)ageInTicks;
    (void)scale;
}

// ==================== LlamaSpitModel ====================

LlamaSpitModel::LlamaSpitModel()
    : EntityModel()
{
    setTextureSize(8, 8);
    _setupParts(0.0f);
}

LlamaSpitModel::LlamaSpitModel(f32 scale)
    : EntityModel()
{
    setTextureSize(8, 8);
    _setupParts(scale);
}

void LlamaSpitModel::_setupParts(f32 scale)
{
    // 创建一个由 7 个 box 组成的十字形结构
    m_main = std::make_shared<ModelRenderer>("main");
    m_main->setTextureOffset(0, 0);
    // 西面 (-X 方向)
    m_main->addBox(-4.0f, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, scale);
    // 下面 (-Y 方向)
    m_main->addBox(0.0f, -4.0f, 0.0f, 2.0f, 2.0f, 2.0f, scale);
    // 北面 (-Z 方向)
    m_main->addBox(0.0f, 0.0f, -4.0f, 2.0f, 2.0f, 2.0f, scale);
    // 中心
    m_main->addBox(0.0f, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, scale);
    // 东面 (+X 方向)
    m_main->addBox(2.0f, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, scale);
    // 上面 (+Y 方向)
    m_main->addBox(0.0f, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, scale);
    // 南面 (+Z 方向)
    m_main->addBox(0.0f, 0.0f, 2.0f, 2.0f, 2.0f, 2.0f, scale);
    m_main->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_main);
}

void LlamaSpitModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void LlamaSpitModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 没有动画
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== EnderCrystalModel ====================

EnderCrystalModel::EnderCrystalModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    _setupParts(0.0f);
}

EnderCrystalModel::EnderCrystalModel(f32 scale)
    : EntityModel()
{
    setTextureSize(64, 32);
    _setupParts(scale);
}

void EnderCrystalModel::_setupParts(f32 scale)
{
    // 核心立方体
    m_cube = std::make_shared<ModelRenderer>("cube");
    m_cube->setTextureOffset(32, 0);
    m_cube->addBox(-4.0f, -4.0f, -4.0f, 8.0f, 8.0f, 8.0f, scale);
    m_cube->setRotationPoint(0.0f, 8.0f, 0.0f);
    m_parts.push_back(m_cube);

    // 玻璃外壳
    m_glass = std::make_shared<ModelRenderer>("glass");
    m_glass->setTextureOffset(0, 0);
    m_glass->addBox(-4.0f, -4.0f, -4.0f, 8.0f, 8.0f, 8.0f, scale);
    m_glass->setRotationPoint(0.0f, 8.0f, 0.0f);
    m_parts.push_back(m_glass);

    // 基座: Java addBox(-6.0F, 0.0F, -6.0F, 12.0F, 4.0F, 12.0F)
    m_base = std::make_shared<ModelRenderer>("base");
    m_base->setTextureOffset(0, 16);
    m_base->addBox(-6.0f, 0.0f, -6.0f, 12.0f, 4.0f, 12.0f, scale);
    m_base->setRotationPoint(0.0f, 8.0f, 0.0f);
    m_parts.push_back(m_base);
}

void EnderCrystalModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void EnderCrystalModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 末影水晶旋转动画
    m_cube->setRotateAngleY(static_cast<f32>(ageInTicks * 0.1));
    m_glass->setRotateAngleY(static_cast<f32>(ageInTicks * 0.05));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== SpectralArrowModel ====================

SpectralArrowModel::SpectralArrowModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    _setupParts();
}

void SpectralArrowModel::_setupParts()
{
    m_arrow = std::make_shared<ModelRenderer>("arrow");
    m_arrow->setTextureOffset(0, 0);
    m_arrow->addBox(0.0f, -0.5f, -0.5f, 16.0f, 1.0f, 1.0f);
    m_arrow->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_arrow);
}

void SpectralArrowModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void SpectralArrowModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    (void)limbSwing;
    (void)limbSwingAmount;
    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== WitherSkullModel ====================

WitherSkullModel::WitherSkullModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    _setupParts();
}

void WitherSkullModel::_setupParts()
{
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -8.0f, -4.0f, 8.0f, 8.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_head);
}

void WitherSkullModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void WitherSkullModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    m_head->setRotateAngleY(mc::math::toRadians(static_cast<f32>(netHeadYaw)));
    m_head->setRotateAngleX(mc::math::toRadians(static_cast<f32>(headPitch)));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)ageInTicks;
    (void)scale;
}

// ==================== DragonFireballModel ====================

DragonFireballModel::DragonFireballModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    _setupParts();
}

void DragonFireballModel::_setupParts()
{
    m_core = std::make_shared<ModelRenderer>("core");
    m_core->setTextureOffset(0, 0);
    m_core->addBox(-4.0f, -4.0f, -4.0f, 8.0f, 8.0f, 8.0f);
    m_core->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_core);

    m_outer = std::make_shared<ModelRenderer>("outer");
    m_outer->setTextureOffset(0, 16);
    m_outer->addBox(-6.0f, -6.0f, -6.0f, 12.0f, 12.0f, 12.0f);
    m_outer->setRotationPoint(0.0f, 0.0f, 0.0f);
    m_parts.push_back(m_outer);
}

void DragonFireballModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void DragonFireballModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    m_core->setRotateAngleY(static_cast<f32>(ageInTicks * 0.1));
    m_outer->setRotateAngleY(static_cast<f32>(-ageInTicks * 0.05));
    m_outer->setRotateAngleX(static_cast<f32>(ageInTicks * 0.03));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== EvokerFangsModel ====================

EvokerFangsModel::EvokerFangsModel()
    : EntityModel()
{
    setTextureSize(64, 32);
    _setupParts();
}

void EvokerFangsModel::_setupParts()
{
    // 底座: 10x12x10，旋转点 (-5, 22, -5)
    m_base = std::make_shared<ModelRenderer>("base");
    m_base->setTextureOffset(0, 0);
    m_base->addBox(0.0f, 0.0f, 0.0f, 10.0f, 12.0f, 10.0f);
    m_base->setRotationPoint(-5.0f, 22.0f, -5.0f);
    m_parts.push_back(m_base);

    // 上颚: 4x14x8，旋转点 (1.5, 22, -4)
    m_upperJaw = std::make_shared<ModelRenderer>("upperJaw");
    m_upperJaw->setTextureOffset(40, 0);
    m_upperJaw->addBox(0.0f, 0.0f, 0.0f, 4.0f, 14.0f, 8.0f);
    m_upperJaw->setRotationPoint(1.5f, 22.0f, -4.0f);
    m_parts.push_back(m_upperJaw);

    // 下颚: 4x14x8，旋转点 (-1.5, 22, 4)
    m_lowerJaw = std::make_shared<ModelRenderer>("lowerJaw");
    m_lowerJaw->setTextureOffset(40, 0);
    m_lowerJaw->addBox(0.0f, 0.0f, 0.0f, 4.0f, 14.0f, 8.0f);
    m_lowerJaw->setRotationPoint(-1.5f, 22.0f, 4.0f);
    m_parts.push_back(m_lowerJaw);
}

void EvokerFangsModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void EvokerFangsModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // f = limbSwing * 2.0，限制在 [0, 1]
    // f = 1.0 - f * f * f
    f32 f = static_cast<f32>(limbSwing * 2.0);
    if (f > 1.0f) f = 1.0f;
    f = 1.0f - f * f * f;

    // 上颚 Z 轴旋转
    m_upperJaw->setRotateAngleZ(static_cast<f32>(mc::math::PI_DOUBLE - f * 0.35 * mc::math::PI_DOUBLE));
    // 下颚 Z 轴旋转，Y 轴旋转
    m_lowerJaw->setRotateAngleZ(static_cast<f32>(mc::math::PI_DOUBLE + f * 0.35 * mc::math::PI_DOUBLE));
    m_lowerJaw->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE));

    // Y 位置动画
    f32 f1 = static_cast<f32>((limbSwing + std::sin(limbSwing * 2.7)) * 0.6 * 12.0);
    m_upperJaw->setRotationPointY(24.0f - f1);
    m_lowerJaw->setRotationPointY(24.0f - f1);
    m_base->setRotationPointY(24.0f - f1);

    (void)ageInTicks;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

} // namespace mc::client::renderer::entity::model::projectile
