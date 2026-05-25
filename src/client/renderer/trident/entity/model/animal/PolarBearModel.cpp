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
 * THE SOFTWARE IS PROVIDED " IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "PolarBearModel.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::animal {

PolarBearModel::PolarBearModel()
    // 参考 MC 1.16.5 PolarBearModel 构造函数：
    // super(12, 0.0F, true, 16.0F, 4.0F, 2.25F, 2.0F, 24);
    : QuadrupedModel(12, 0.0f, true, 16.0f, 4.0f, 2.25f, 2.0f, 24.0f)
{
    // Java: this.textureWidth = 128;
    //       this.textureHeight = 64;
    m_textureWidth = 128;
    m_textureHeight = 64;

    setupParts();

    // 清空部件列表并重新添加（因为基类 setupParts() 已经添加过）
    m_parts.clear();
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_legFrontRight);
    m_parts.push_back(m_legFrontLeft);
    m_parts.push_back(m_legBackRight);
    m_parts.push_back(m_legBackLeft);
}

void PolarBearModel::setupParts()
{
    // 参考 MC 1.16.5 PolarBearModel 构造函数
    // 注意：Java 中鼻子、耳朵、下身体都是直接 addBox 到同一个 ModelRenderer 上，
    // 而不是作为子部件。我们保持这种结构。

    // ==================== 头部 ====================
    // Java:
    // this.headModel = new ModelRenderer(this, 0, 0);
    // this.headModel.addBox(-3.5F, -3.0F, -3.0F, 7.0F, 7.0F, 7.0F, 0.0F);
    // this.headModel.setRotationPoint(0.0F, 10.0F, -16.0F);
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureSize(m_textureWidth, m_textureHeight);
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-3.5f, -3.0f, -3.0f, 7.0f, 7.0f, 7.0f, 0.0f);
    m_head->setRotationPoint(0.0f, 10.0f, -16.0f);

    // 鼻子 - 直接添加到头部（Java 第17行）
    // this.headModel.setTextureOffset(0, 44).addBox(-2.5F, 1.0F, -6.0F, 5.0F, 3.0F, 3.0F, 0.0F);
    m_head->setTextureOffset(0, 44);
    m_head->addBox(-2.5f, 1.0f, -6.0f, 5.0f, 3.0f, 3.0f, 0.0f);

    // 左耳 - 直接添加到头部（Java 第18行）
    // this.headModel.setTextureOffset(26, 0).addBox(-4.5F, -4.0F, -1.0F, 2.0F, 2.0F, 1.0F, 0.0F);
    m_head->setTextureOffset(26, 0);
    m_head->addBox(-4.5f, -4.0f, -1.0f, 2.0f, 2.0f, 1.0f, 0.0f);

    // 右耳（镜像）- 直接添加到头部（Java 第19-21行）
    // ModelRenderer modelrenderer = this.headModel.setTextureOffset(26, 0);
    // modelrenderer.mirror = true;
    // modelrenderer.addBox(2.5F, -4.0F, -1.0F, 2.0F, 2.0F, 1.0F, 0.0F);
    m_head->setTextureOffset(26, 0);
    m_head->setMirror(true);
    m_head->addBox(2.5f, -4.0f, -1.0f, 2.0f, 2.0f, 1.0f, 0.0f);
    m_head->setMirror(false); // 重置镜像状态

    // ==================== 身体 ====================
    // Java:
    // this.body = new ModelRenderer(this);
    // this.body.setTextureOffset(0, 19).addBox(-5.0F, -13.0F, -7.0F, 14.0F, 14.0F, 11.0F, 0.0F);
    // this.body.setTextureOffset(39, 0).addBox(-4.0F, -25.0F, -7.0F, 12.0F, 12.0F, 10.0F, 0.0F);
    // this.body.setRotationPoint(-2.0F, 9.0F, 12.0F);
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureSize(m_textureWidth, m_textureHeight);
    m_body->setTextureOffset(0, 19);
    m_body->addBox(-5.0f, -13.0f, -7.0f, 14.0f, 14.0f, 11.0f, 0.0f);
    m_body->setTextureOffset(39, 0);
    m_body->addBox(-4.0f, -25.0f, -7.0f, 12.0f, 12.0f, 10.0f, 0.0f);
    m_body->setRotationPoint(-2.0f, 9.0f, 12.0f);

    // ==================== 后腿 ====================
    // Java 第26行: int i = 10; (腿高)

    // 右后腿（Java 第27-29行）
    // this.legBackRight = new ModelRenderer(this, 50, 22);
    // this.legBackRight.addBox(-2.0F, 0.0F, -2.0F, 4.0F, 10.0F, 8.0F, 0.0F);
    // this.legBackRight.setRotationPoint(-3.5F, 14.0F, 6.0F);
    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureSize(m_textureWidth, m_textureHeight);
    m_legBackRight->setTextureOffset(50, 22);
    m_legBackRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 10.0f, 8.0f, 0.0f);
    m_legBackRight->setRotationPoint(-3.5f, 14.0f, 6.0f);

    // 左后腿（Java 第30-32行）
    // this.legBackLeft = new ModelRenderer(this, 50, 22);
    // this.legBackLeft.addBox(-2.0F, 0.0F, -2.0F, 4.0F, 10.0F, 8.0F, 0.0F);
    // this.legBackLeft.setRotationPoint(3.5F, 14.0F, 6.0F);
    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureSize(m_textureWidth, m_textureHeight);
    m_legBackLeft->setTextureOffset(50, 22);
    m_legBackLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 10.0f, 8.0f, 0.0f);
    m_legBackLeft->setRotationPoint(3.5f, 14.0f, 6.0f);

    // ==================== 前腿 ====================
    // 右前腿（Java 第33-35行）
    // this.legFrontRight = new ModelRenderer(this, 50, 40);
    // this.legFrontRight.addBox(-2.0F, 0.0F, -2.0F, 4.0F, 10.0F, 6.0F, 0.0F);
    // this.legFrontRight.setRotationPoint(-2.5F, 14.0F, -7.0F);
    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureSize(m_textureWidth, m_textureHeight);
    m_legFrontRight->setTextureOffset(50, 40);
    m_legFrontRight->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 10.0f, 6.0f, 0.0f);
    m_legFrontRight->setRotationPoint(-2.5f, 14.0f, -7.0f);

    // 左前腿（Java 第36-38行）
    // this.legFrontLeft = new ModelRenderer(this, 50, 40);
    // this.legFrontLeft.addBox(-2.0F, 0.0F, -2.0F, 4.0F, 10.0F, 6.0F, 0.0F);
    // this.legFrontLeft.setRotationPoint(2.5F, 14.0F, -7.0F);
    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureSize(m_textureWidth, m_textureHeight);
    m_legFrontLeft->setTextureOffset(50, 40);
    m_legFrontLeft->addBox(-2.0f, 0.0f, -2.0f, 4.0f, 10.0f, 6.0f, 0.0f);
    m_legFrontLeft->setRotationPoint(2.5f, 14.0f, -7.0f);

    // ==================== 位置调整（Java 第39-46行）====================
    // --this.legBackRight.rotationPointX;   // -3.5 - 1 = -4.5
    m_legBackRight->setRotationPointX(m_legBackRight->rotationPointX() - 1.0);

    // ++this.legBackLeft.rotationPointX;    // 3.5 + 1 = 4.5
    m_legBackLeft->setRotationPointX(m_legBackLeft->rotationPointX() + 1.0);

    // this.legBackRight.rotationPointZ += 0.0F; // 无变化
    // this.legBackLeft.rotationPointZ += 0.0F;  // 无变化

    // --this.legFrontRight.rotationPointX;  // -2.5 - 1 = -3.5
    m_legFrontRight->setRotationPointX(m_legFrontRight->rotationPointX() - 1.0);

    // ++this.legFrontLeft.rotationPointX;   // 2.5 + 1 = 3.5
    m_legFrontLeft->setRotationPointX(m_legFrontLeft->rotationPointX() + 1.0);

    // --this.legFrontRight.rotationPointZ;  // -7 - 1 = -8
    m_legFrontRight->setRotationPointZ(m_legFrontRight->rotationPointZ() - 1.0);

    // --this.legFrontLeft.rotationPointZ;   // -7 - 1 = -8
    m_legFrontLeft->setRotationPointZ(m_legFrontLeft->rotationPointZ() - 1.0);
}

std::vector<std::shared_ptr<ModelRenderer>> PolarBearModel::getHeadParts() const
{
    return {m_head};
}

std::vector<std::shared_ptr<ModelRenderer>> PolarBearModel::getBodyParts() const
{
    return {m_body, m_legFrontRight, m_legFrontLeft, m_legBackRight, m_legBackLeft};
}

void PolarBearModel::render(f64 scale)
{
    AgeableModel::render(scale);
}

void PolarBearModel::setStandingProgress(f32 standingProgress)
{
    m_standingProgress = standingProgress;
}

void PolarBearModel::setLivingAnimations(f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 /*partialTick*/)
{
    // 站立动画在 setAngles 中处理
}

void PolarBearModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 先调用基类方法设置基本动画
    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // ==================== 站立动画 ====================
    // 参考 MC 1.16.5 PolarBearModel.setRotationAngles（第52-75行）

    // Java 第54-57行:
    // float f = ageInTicks - (float)entityIn.ticksExisted;
    // float f1 = entityIn.getStandingAnimationScale(f);
    // f1 = f1 * f1;
    // float f2 = 1.0F - f1;
    // 注意：f 的计算在实体的 getStandingAnimationScale 中处理
    // 我们使用外部设置的 m_standingProgress
    const f32 f1 = m_standingProgress * m_standingProgress; // 站立因子（平方）
    const f32 f2 = 1.0f - f1;                               // 反向因子

    // Java 第58-59行: 身体动画
    // this.body.rotateAngleX = ((float)Math.PI / 2F) - f1 * (float)Math.PI * 0.35F;
    // this.body.rotationPointY = 9.0F * f2 + 11.0F * f1;
    m_body->setRotateAngleX(static_cast<f32>(math::PI / 2.0) - f1 * static_cast<f32>(math::PI) * 0.35f);
    m_body->setRotationPointY(9.0f * f2 + 11.0f * f1);

    // Java 第60-62行: 右前腿动画
    // this.legFrontRight.rotationPointY = 14.0F * f2 - 6.0F * f1;
    // this.legFrontRight.rotationPointZ = -8.0F * f2 - 4.0F * f1;
    // this.legFrontRight.rotateAngleX -= f1 * (float)Math.PI * 0.45F;
    m_legFrontRight->setRotationPointY(14.0f * f2 - 6.0f * f1);
    m_legFrontRight->setRotationPointZ(-8.0f * f2 - 4.0f * f1);
    m_legFrontRight->setRotateAngleX(m_legFrontRight->rotateAngleX() - f1 * static_cast<f32>(math::PI) * 0.45f);

    // Java 第63-65行: 左前腿同步
    // this.legFrontLeft.rotationPointY = this.legFrontRight.rotationPointY;
    // this.legFrontLeft.rotationPointZ = this.legFrontRight.rotationPointZ;
    // this.legFrontLeft.rotateAngleX -= f1 * (float)Math.PI * 0.45F;
    m_legFrontLeft->setRotationPointY(m_legFrontRight->rotationPointY());
    m_legFrontLeft->setRotationPointZ(m_legFrontRight->rotationPointZ());
    m_legFrontLeft->setRotateAngleX(m_legFrontLeft->rotateAngleX() - f1 * static_cast<f32>(math::PI) * 0.45f);

    // Java 第66-72行: 头部动画（幼体和成年不同）
    if (m_isChild) {
        // 幼体头部位置
        // this.headModel.rotationPointY = 10.0F * f2 - 9.0F * f1;
        // this.headModel.rotationPointZ = -16.0F * f2 - 7.0F * f1;
        m_head->setRotationPointY(10.0f * f2 - 9.0f * f1);
        m_head->setRotationPointZ(-16.0f * f2 - 7.0f * f1);
    } else {
        // 成年头部位置
        // this.headModel.rotationPointY = 10.0F * f2 - 14.0F * f1;
        // this.headModel.rotationPointZ = -16.0F * f2 - 3.0F * f1;
        m_head->setRotationPointY(10.0f * f2 - 14.0f * f1);
        m_head->setRotationPointZ(-16.0f * f2 - 3.0f * f1);
    }

    // Java 第74行: 头部X旋转增加（抬头）
    // this.headModel.rotateAngleX += f1 * (float)Math.PI * 0.15F;
    m_head->setRotateAngleX(m_head->rotateAngleX() + f1 * static_cast<f32>(math::PI) * 0.15f);

    (void)scale;
    (void)ageInTicks;
}

} // namespace mc::client::renderer::entity::model::animal
