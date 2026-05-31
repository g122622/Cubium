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

#include "ZombieModel.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

ZombieModel::ZombieModel(bool slim)
    : BipedModel()
    , m_slim(slim)
{
    // 普通僵尸使用 64x64 纹理，尸壳/溺尸使用 64x32
    setTextureSize(64, 64);
    setupParts();
}

void ZombieModel::setupParts()
{
    // 僵尸的部件尺寸与玩家相同
    // 由 BipedModel 基类设置
}

void ZombieModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 调用基类设置基础动画
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 攻击动画处理

    // 使用基类的 swingProgress 字段
    f32 swingProgress = m_swingProgress;

    // 计算 f 和 f1 用于攻击动画
    f32 f = static_cast<f32>(std::sin(swingProgress * mc::math::PI_DOUBLE));
    f32 f1 = static_cast<f32>(std::sin((1.0 - (1.0 - swingProgress) * (1.0 - swingProgress)) * mc::math::PI_DOUBLE));

    if (m_leftArm && m_rightArm) {
        // 重置 Z 轴旋转
        m_leftArm->setRotateAngleZ(0.0f);
        m_rightArm->setRotateAngleZ(0.0f);

        // Y 轴旋转
        m_leftArm->setRotateAngleY(-(0.1f - f * 0.6f));
        m_rightArm->setRotateAngleY(0.1f - f * 0.6f);

        // X 轴旋转基值，手臂前伸的基础角度
        m_leftArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));
        m_rightArm->setRotateAngleX(static_cast<f32>(-mc::math::PI_DOUBLE / 2.0));

        // 添加攻击动画 (f * 1.2F - f1 * 0.4F)
        m_leftArm->setRotateAngleX(m_leftArm->rotateAngleX() - (f * 1.2f - f1 * 0.4f));
        m_rightArm->setRotateAngleX(m_rightArm->rotateAngleX() - (f * 1.2f - f1 * 0.4f));

        // 手臂抖动效果
        m_leftArm->setRotateAngleZ(
            m_leftArm->rotateAngleZ() + static_cast<f32>(std::cos(ageInTicks * 0.09) * 0.05 + 0.05));
        m_rightArm->setRotateAngleZ(
            m_rightArm->rotateAngleZ() - static_cast<f32>(std::cos(ageInTicks * 0.09) * 0.05 + 0.05));
        m_leftArm->setRotateAngleX(m_leftArm->rotateAngleX() + static_cast<f32>(std::sin(ageInTicks * 0.067) * 0.05));
        m_rightArm->setRotateAngleX(m_rightArm->rotateAngleX() - static_cast<f32>(std::sin(ageInTicks * 0.067) * 0.05));
    }
}

void ZombieModel::setTextureDimensions(bool useSlimTexture)
{
    // 普通僵尸使用 64x64，尸壳/溺尸使用 64x32
    if (useSlimTexture) {
        setTextureSize(64, 32);
    } else {
        setTextureSize(64, 64);
    }
}

} // namespace mc::client::renderer::entity::model::monster
