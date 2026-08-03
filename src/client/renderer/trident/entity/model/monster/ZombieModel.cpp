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
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "common/core/Types.hpp"
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

    // 对应 MC 1.21.11 AbstractZombieModel.setupAnim() 调用
    // AnimationUtils.animateZombieArms(leftArm, rightArm, isAggressive, renderState)。
    //
    // MC 原版逻辑：
    //   flag = (swingAnimationType != STAB)  // 持剑刺击时跳过僵尸手臂动画
    //   if (flag) {
    //       f1 = -PI / (aggressive ? 1.5 : 2.25)   // 手臂前伸基础角度
    //       f2 = sin(attackTime * PI)
    //       f3 = sin((1 - (1-attackTime)^2) * PI)
    //       rightArm.zRot = 0; rightArm.yRot = -(0.1 - f2*0.6); rightArm.xRot = f1 + f2*1.2 - f3*0.4
    //       leftArm.zRot  = 0; leftArm.yRot  = (0.1 - f2*0.6);  leftArm.xRot  = f1 + f2*1.2 - f3*0.4
    //   }
    //   bobArms(rightArm, leftArm, ageInTicks)  // 无条件执行
    //
    // TODO: 目前项目尚未实现 SwingAnimationType（持剑刺击 vs 挥砍区分），
    //       此处暂以 flag=true（非刺击）处理，与持空手/持非剑武器时的表现一致。
    //       待武器系统引入 SwingAnimationType 后，需在此处读取并按 MC 原版守卫。

    f32 swingProgress = m_swingProgress; // 对应 MC renderState.attackTime

    f32 f1 = static_cast<f32>(-mc::math::PI_DOUBLE / (m_isAggressive ? 1.5 : 2.25));
    f32 f2 = static_cast<f32>(std::sin(swingProgress * mc::math::PI_DOUBLE));
    f32 f3 = static_cast<f32>(std::sin((1.0 - (1.0 - swingProgress) * (1.0 - swingProgress)) * mc::math::PI_DOUBLE));

    if (m_leftArm && m_rightArm) {
        // rightArm（对应 MC p_102104_）
        m_rightArm->setRotateAngleZ(0.0f);
        m_rightArm->setRotateAngleY(-(0.1f - f2 * 0.6f));
        m_rightArm->setRotateAngleX(f1);
        m_rightArm->setRotateAngleX(m_rightArm->rotateAngleX() + (f2 * 1.2f - f3 * 0.4f));

        // leftArm（对应 MC p_102103_）
        m_leftArm->setRotateAngleZ(0.0f);
        m_leftArm->setRotateAngleY(0.1f - f2 * 0.6f);
        m_leftArm->setRotateAngleX(f1);
        m_leftArm->setRotateAngleX(m_leftArm->rotateAngleX() + (f2 * 1.2f - f3 * 0.4f));

        // bobArms：无条件执行（对应 MC animateZombieArms 末尾的 bobArms 调用）
        // bobModelPart(rightArm, age, 1.0)：zRot += cos(age*0.09)*0.05 + 0.05；xRot += sin(age*0.067)*0.05
        // bobModelPart(leftArm,  age, -1.0)：zRot -= cos(age*0.09)*0.05 + 0.05；xRot -= sin(age*0.067)*0.05
        m_rightArm->setRotateAngleZ(
            m_rightArm->rotateAngleZ() + static_cast<f32>(std::cos(ageInTicks * 0.09) * 0.05 + 0.05));
        m_leftArm->setRotateAngleZ(
            m_leftArm->rotateAngleZ() - static_cast<f32>(std::cos(ageInTicks * 0.09) * 0.05 + 0.05));
        m_rightArm->setRotateAngleX(m_rightArm->rotateAngleX() + static_cast<f32>(std::sin(ageInTicks * 0.067) * 0.05));
        m_leftArm->setRotateAngleX(m_leftArm->rotateAngleX() - static_cast<f32>(std::sin(ageInTicks * 0.067) * 0.05));
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
