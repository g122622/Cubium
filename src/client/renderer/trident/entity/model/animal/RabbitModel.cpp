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

#include "RabbitModel.hpp"
#include "client/renderer/trident/entity/model/core/AgeableModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include <memory>
#include <vector>

namespace mc::client::renderer::entity::model::animal {

RabbitModel::RabbitModel()
    : AgeableModel(
          // 幼体头部缩放: 0.56666666 = 17/30
          // 身体使用 matrixStack.scale(0.4F, 0.4F, 0.4F)
          // childHeadScale = 17F / 30F ≈ 0.56666666
          // AgeableModel 的 1.5f / childHeadScale = 1.5 / 0.56666666 ≈ 2.647
          true,          // isChildHeadScaled
          5.0f,          // childHeadOffsetY
          2.0f,          // childHeadOffsetZ
          17.0f / 30.0f, // childHeadScale = 0.56666666
          2.5f,          // childBodyScale = 1/0.4 = 2.5
          24.0f          // childBodyOffsetY
      )
{
    setTextureSize(64, 32);
    _setupParts();
}

void RabbitModel::_setupParts()
{
    // 所有部件都有初始旋转角度

    // 左脚
    m_leftFoot = std::make_shared<ModelRenderer>("rabbitLeftFoot");
    m_leftFoot->setTextureOffset(26, 24);
    m_leftFoot->setMirror(true);
    m_leftFoot->addBox(-1.0f, 5.5f, -3.7f, 2.0f, 1.0f, 7.0f);
    m_leftFoot->setRotationPoint(3.0f, 17.5f, 3.7f);
    m_leftFoot->setRotateAngleX(0.0f);
    m_parts.push_back(m_leftFoot);

    // 右脚
    m_rightFoot = std::make_shared<ModelRenderer>("rabbitRightFoot");
    m_rightFoot->setTextureOffset(8, 24);
    m_rightFoot->setMirror(true);
    m_rightFoot->addBox(-1.0f, 5.5f, -3.7f, 2.0f, 1.0f, 7.0f);
    m_rightFoot->setRotationPoint(-3.0f, 17.5f, 3.7f);
    m_rightFoot->setRotateAngleX(0.0f);
    m_parts.push_back(m_rightFoot);

    // 左大腿
    m_leftThigh = std::make_shared<ModelRenderer>("rabbitLeftThigh");
    m_leftThigh->setTextureOffset(30, 15);
    m_leftThigh->setMirror(true);
    m_leftThigh->addBox(-1.0f, 0.0f, 0.0f, 2.0f, 4.0f, 5.0f);
    m_leftThigh->setRotationPoint(3.0f, 17.5f, 3.7f);
    m_leftThigh->setRotateAngleX(-0.34906584f);
    m_parts.push_back(m_leftThigh);

    // 右大腿
    m_rightThigh = std::make_shared<ModelRenderer>("rabbitRightThigh");
    m_rightThigh->setTextureOffset(16, 15);
    m_rightThigh->setMirror(true);
    m_rightThigh->addBox(-1.0f, 0.0f, 0.0f, 2.0f, 4.0f, 5.0f);
    m_rightThigh->setRotationPoint(-3.0f, 17.5f, 3.7f);
    m_rightThigh->setRotateAngleX(-0.34906584f);
    m_parts.push_back(m_rightThigh);

    // 身体
    m_body = std::make_shared<ModelRenderer>("rabbitBody");
    m_body->setTextureOffset(0, 0);
    m_body->setMirror(true);
    m_body->addBox(-3.0f, -2.0f, -10.0f, 6.0f, 5.0f, 10.0f);
    m_body->setRotationPoint(0.0f, 19.0f, 8.0f);
    m_body->setRotateAngleX(-0.34906584f);
    m_parts.push_back(m_body);

    // 左前腿
    m_leftArm = std::make_shared<ModelRenderer>("rabbitLeftArm");
    m_leftArm->setTextureOffset(8, 15);
    m_leftArm->setMirror(true);
    m_leftArm->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 7.0f, 2.0f);
    m_leftArm->setRotationPoint(3.0f, 17.0f, -1.0f);
    m_leftArm->setRotateAngleX(-0.17453292f);
    m_parts.push_back(m_leftArm);

    // 右前腿
    m_rightArm = std::make_shared<ModelRenderer>("rabbitRightArm");
    m_rightArm->setTextureOffset(0, 15);
    m_rightArm->setMirror(true);
    m_rightArm->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 7.0f, 2.0f);
    m_rightArm->setRotationPoint(-3.0f, 17.0f, -1.0f);
    m_rightArm->setRotateAngleX(-0.17453292f);
    m_parts.push_back(m_rightArm);

    // 头部
    m_head = std::make_shared<ModelRenderer>("rabbitHead");
    m_head->setTextureOffset(32, 0);
    m_head->setMirror(true);
    m_head->addBox(-2.5f, -4.0f, -5.0f, 5.0f, 4.0f, 5.0f);
    m_head->setRotationPoint(0.0f, 16.0f, -1.0f);
    m_parts.push_back(m_head);

    // 右耳
    m_rightEar = std::make_shared<ModelRenderer>("rabbitRightEar");
    m_rightEar->setTextureOffset(52, 0);
    m_rightEar->setMirror(true);
    m_rightEar->addBox(-2.5f, -9.0f, -1.0f, 2.0f, 5.0f, 1.0f);
    m_rightEar->setRotationPoint(0.0f, 16.0f, -1.0f);
    m_rightEar->setRotateAngleY(-0.2617994f);
    m_parts.push_back(m_rightEar);

    // 左耳
    m_leftEar = std::make_shared<ModelRenderer>("rabbitLeftEar");
    m_leftEar->setTextureOffset(58, 0);
    m_leftEar->setMirror(true);
    m_leftEar->addBox(0.5f, -9.0f, -1.0f, 2.0f, 5.0f, 1.0f);
    m_leftEar->setRotationPoint(0.0f, 16.0f, -1.0f);
    m_leftEar->setRotateAngleY(0.2617994f);
    m_parts.push_back(m_leftEar);

    // 尾巴
    m_tail = std::make_shared<ModelRenderer>("rabbitTail");
    m_tail->setTextureOffset(52, 6);
    m_tail->setMirror(true);
    m_tail->addBox(-1.5f, -1.5f, 0.0f, 3.0f, 3.0f, 2.0f);
    m_tail->setRotationPoint(0.0f, 20.0f, 7.0f);
    m_tail->setRotateAngleX(-0.3490659f);
    m_parts.push_back(m_tail);

    // 鼻子
    m_nose = std::make_shared<ModelRenderer>("rabbitNose");
    m_nose->setTextureOffset(32, 9);
    m_nose->setMirror(true);
    m_nose->addBox(-0.5f, -2.5f, -5.5f, 1.0f, 1.0f, 1.0f);
    m_nose->setRotationPoint(0.0f, 16.0f, -1.0f);
    m_parts.push_back(m_nose);
}

void RabbitModel::render(f64 scale)
{
    // 成年兔子: scale(0.6) + translate(0, 1/16, 0)
    // 幼体兔子由 AgeableModel::render() 处理头部和身体分开缩放

    if (m_isChild) {
        AgeableModel::render(scale);
    } else {
        // 成年: 整体缩放 0.6，并向上移动 1/16
        for (auto& part : m_parts) {
            f64 origY = part->rotationPointY();
            part->setRotationPointY(origY + 1.0f);
            part->render(scale * 0.6);
            part->setRotationPointY(origY);
        }
    }
}

std::vector<std::shared_ptr<ModelRenderer>> RabbitModel::getHeadParts() const
{
    std::vector<std::shared_ptr<ModelRenderer>> parts;
    parts.push_back(m_head);
    parts.push_back(m_rightEar);
    parts.push_back(m_leftEar);
    parts.push_back(m_nose);
    return parts;
}

std::vector<std::shared_ptr<ModelRenderer>> RabbitModel::getBodyParts() const
{
    std::vector<std::shared_ptr<ModelRenderer>> parts;
    parts.push_back(m_body);
    parts.push_back(m_leftFoot);
    parts.push_back(m_rightFoot);
    parts.push_back(m_leftThigh);
    parts.push_back(m_rightThigh);
    parts.push_back(m_leftArm);
    parts.push_back(m_rightArm);
    parts.push_back(m_tail);
    return parts;
}

void RabbitModel::setAngles(
    f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 /*ageInTicks*/, f64 netHeadYaw, f64 headPitch, f64 /*scale*/)
{
    // 跳跃动画在 setLivingAnimations 中计算

    // 头部、鼻子、耳朵跟随头部俯仰
    f32 headPitchRad = mc::math::toRadians(static_cast<f32>(headPitch));
    f32 headYawRad = mc::math::toRadians(static_cast<f32>(netHeadYaw));

    m_nose->setRotateAngleX(headPitchRad);
    m_head->setRotateAngleX(headPitchRad);
    m_rightEar->setRotateAngleX(headPitchRad);
    m_leftEar->setRotateAngleX(headPitchRad);

    m_nose->setRotateAngleY(headYawRad);
    m_head->setRotateAngleY(headYawRad);
    // 耳朵有额外的 Y 偏移
    m_rightEar->setRotateAngleY(headYawRad - 0.2617994f);
    m_leftEar->setRotateAngleY(headYawRad + 0.2617994f);

    // 跳跃动画角度计算
    f32 thighAngle = mc::math::toRadians(m_jumpRotation * 50.0f - 21.0f);
    f32 footAngle = mc::math::toRadians(m_jumpRotation * 50.0f);
    f32 armAngle = mc::math::toRadians(m_jumpRotation * -40.0f - 11.0f);

    // 基础旋转角度
    constexpr f32 baseThighAngle = -0.34906584f;
    constexpr f32 baseArmAngle = -0.17453292f;

    m_leftThigh->setRotateAngleX(baseThighAngle + thighAngle);
    m_rightThigh->setRotateAngleX(baseThighAngle + thighAngle);
    m_leftFoot->setRotateAngleX(footAngle);
    m_rightFoot->setRotateAngleX(footAngle);
    m_leftArm->setRotateAngleX(baseArmAngle + armAngle);
    m_rightArm->setRotateAngleX(baseArmAngle + armAngle);
}

void RabbitModel::setLivingAnimations(f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 /*partialTick*/)
{
    // 跳跃动画的 jumpRotation 由 EntityRendererManager::_createModelForEntity 通过
    // setJumpRotation(sin(jumpCompletion * PI)) 在外部设置，本方法无需处理。
    // 完整数据流：
    //   服务端 RabbitEntity::startJumping() 广播 RabbitJump(1) 状态码
    //   → 客户端 ClientApplicationNetwork.onEntityStatus 调用 ClientEntity::setRabbitJumpStart()
    //   → ClientEntity::tick() 中 tickRabbitJump() 推进 m_rabbitJumpTicks
    //   → EntityRendererManager 读取 rabbitJumpCompletion(partialTick) 计算 jumpRotation
    //   → setJumpRotation(sin(jumpCompletion * PI))
    //   → setAngles 中根据 m_jumpRotation 计算 thigh/foot/arm 旋转角度
}

void RabbitModel::setJumpRotation(f32 jumpRotation)
{
    m_jumpRotation = jumpRotation;
}

} // namespace mc::client::renderer::entity::model::animal
