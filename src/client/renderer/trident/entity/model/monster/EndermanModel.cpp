#include "EndermanModel.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

namespace {
    constexpr f64 PI = 3.14159265359;
    constexpr f64 DEG_TO_RAD = PI / 180.0;
    // 末影人 Y 偏移：-14（比普通生物高）
    constexpr f32 ENDERMAN_Y_OFFSET = -14.0f;
    // 手臂/腿角度限制：±0.4 弧度
    constexpr f32 ARM_LEG_ANGLE_LIMIT = 0.4f;
}

EndermanModel::EndermanModel()
    : BipedModel()
{
    // 参考 MC 1.16.5 EndermanModel
    // 纹理尺寸：64x32
    setTextureSize(64, 32);
    setupParts();
}

void EndermanModel::setupParts() {
    // 参考 MC 1.16.5 EndermanModel 构造函数
    // super(0.0F, -14.0F, 64, 32)
    // 末影人有独特的身体比例：手臂和腿非常长

    // 头部外层（头套）：8x8x8，纹理位置 (0, 16)
    // MC: this.bipedHeadwear = new ModelRenderer(this, 0, 16);
    //     this.bipedHeadwear.addBox(-4.0F, -8.0F, -4.0F, 8.0F, 8.0F, 8.0F, scale - 0.5F);
    //     this.bipedHeadwear.setRotationPoint(0.0F, -14.0F, 0.0F);
    if (m_head) {
        m_head->setTextureSize(64, 32);
        m_head->setTextureOffset(0, 0);
        // 头部内层尺寸保持标准
        m_head->setRotationPoint(0.0f, ENDERMAN_Y_OFFSET, 0.0f);
    }

    // 身体：8x12x4，纹理位置 (32, 16)
    // MC: this.bipedBody = new ModelRenderer(this, 32, 16);
    //     this.bipedBody.addBox(-4.0F, 0.0F, -2.0F, 8.0F, 12.0F, 4.0F, scale);
    //     this.bipedBody.setRotationPoint(0.0F, -14.0F, 0.0F);
    if (m_body) {
        m_body->setTextureSize(64, 32);
        m_body->setTextureOffset(32, 16);
        m_body->addBox(-4.0f, 0.0f, -2.0f, 8.0f, 12.0f, 4.0f, 0.0f);
        m_body->setRotationPoint(0.0f, ENDERMAN_Y_OFFSET, 0.0f);
    }

    // 右臂：2x30x2，纹理位置 (56, 0)
    // MC: this.bipedRightArm = new ModelRenderer(this, 56, 0);
    //     this.bipedRightArm.addBox(-1.0F, -2.0F, -1.0F, 2.0F, 30.0F, 2.0F, scale);
    //     this.bipedRightArm.setRotationPoint(-3.0F, -12.0F, 0.0F);
    if (m_rightArm) {
        m_rightArm->setTextureSize(64, 32);
        m_rightArm->setTextureOffset(56, 0);
        m_rightArm->addBox(-1.0f, -2.0f, -1.0f, 2.0f, 30.0f, 2.0f, 0.0f);
        m_rightArm->setRotationPoint(-3.0f, -12.0f, 0.0f);
    }

    // 左臂：2x30x2，纹理位置 (56, 0)，镜像
    // MC: this.bipedLeftArm = new ModelRenderer(this, 56, 0);
    //     this.bipedLeftArm.mirror = true;
    //     this.bipedLeftArm.addBox(-1.0F, -2.0F, -1.0F, 2.0F, 30.0F, 2.0F, scale);
    //     this.bipedLeftArm.setRotationPoint(5.0F, -12.0F, 0.0F);
    if (m_leftArm) {
        m_leftArm->setTextureSize(64, 32);
        m_leftArm->setTextureOffset(56, 0);
        m_leftArm->setMirror(true);
        m_leftArm->addBox(-1.0f, -2.0f, -1.0f, 2.0f, 30.0f, 2.0f, 0.0f);
        m_leftArm->setRotationPoint(5.0f, -12.0f, 0.0f);
    }

    // 右腿：2x30x2，纹理位置 (56, 0)
    // MC: this.bipedRightLeg = new ModelRenderer(this, 56, 0);
    //     this.bipedRightLeg.addBox(-1.0F, 0.0F, -1.0F, 2.0F, 30.0F, 2.0F, scale);
    //     this.bipedRightLeg.setRotationPoint(-2.0F, -2.0F, 0.0F);
    if (m_rightLeg) {
        m_rightLeg->setTextureSize(64, 32);
        m_rightLeg->setTextureOffset(56, 0);
        m_rightLeg->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 30.0f, 2.0f, 0.0f);
        m_rightLeg->setRotationPoint(-2.0f, -2.0f, 0.0f);
    }

    // 左腿：2x30x2，纹理位置 (56, 0)，镜像
    // MC: this.bipedLeftLeg = new ModelRenderer(this, 56, 0);
    //     this.bipedLeftLeg.mirror = true;
    //     this.bipedLeftLeg.addBox(-1.0F, 0.0F, -1.0F, 2.0F, 30.0F, 2.0F, scale);
    //     this.bipedLeftLeg.setRotationPoint(2.0F, -2.0F, 0.0F);
    if (m_leftLeg) {
        m_leftLeg->setTextureSize(64, 32);
        m_leftLeg->setTextureOffset(56, 0);
        m_leftLeg->setMirror(true);
        m_leftLeg->addBox(-1.0f, 0.0f, -1.0f, 2.0f, 30.0f, 2.0f, 0.0f);
        m_leftLeg->setRotationPoint(2.0f, -2.0f, 0.0f);
    }
}

void EndermanModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                               f64 ageInTicks, f64 netHeadYaw,
                               f64 headPitch, f64 scale) {
    // 调用基类设置基础动画
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 参考 MC 1.16.5 EndermanModel.setRotationAngles

    // 头部始终显示
    if (m_head) {
        m_head->setVisible(true);
    }

    // 身体旋转
    if (m_body) {
        m_body->setRotateAngleX(0.0f);
        m_body->setRotationPointY(ENDERMAN_Y_OFFSET);
        m_body->setRotationPointZ(0.0f);
    }

    // 手臂和腿的角度限制：±0.4 弧度
    // MC: this.bipedRightArm.rotateAngleX = (float)((double)this.bipedRightArm.rotateAngleX * 0.5D);
    //     if (this.bipedRightArm.rotateAngleX > 0.4F) this.bipedRightArm.rotateAngleX = 0.4F;
    //     if (this.bipedRightArm.rotateAngleX < -0.4F) this.bipedRightArm.rotateAngleX = -0.4F;
    if (m_rightArm) {
        f32 armX = m_rightArm->rotateAngleX() * 0.5f;
        armX = std::clamp(armX, -ARM_LEG_ANGLE_LIMIT, ARM_LEG_ANGLE_LIMIT);
        m_rightArm->setRotateAngleX(armX);
    }

    if (m_leftArm) {
        f32 armX = m_leftArm->rotateAngleX() * 0.5f;
        armX = std::clamp(armX, -ARM_LEG_ANGLE_LIMIT, ARM_LEG_ANGLE_LIMIT);
        m_leftArm->setRotateAngleX(armX);
    }

    if (m_rightLeg) {
        f32 legX = m_rightLeg->rotateAngleX() * 0.5f;
        legX = std::clamp(legX, -ARM_LEG_ANGLE_LIMIT, ARM_LEG_ANGLE_LIMIT);
        m_rightLeg->setRotateAngleX(legX);
    }

    if (m_leftLeg) {
        f32 legX = m_leftLeg->rotateAngleX() * 0.5f;
        legX = std::clamp(legX, -ARM_LEG_ANGLE_LIMIT, ARM_LEG_ANGLE_LIMIT);
        m_leftLeg->setRotateAngleX(legX);
    }

    // 携带方块状态
    // MC: if (this.isCarrying) {
    //         this.bipedRightArm.rotateAngleX = -0.5F;
    //         this.bipedLeftArm.rotateAngleX = -0.5F;
    //         this.bipedRightArm.rotateAngleZ = 0.05F;
    //         this.bipedLeftArm.rotateAngleZ = -0.05F;
    //     }
    if (m_carrying) {
        if (m_rightArm) {
            m_rightArm->setRotateAngleX(-0.5f);
            m_rightArm->setRotateAngleZ(0.05f);
        }
        if (m_leftArm) {
            m_leftArm->setRotateAngleX(-0.5f);
            m_leftArm->setRotateAngleZ(-0.05f);
        }
    }

    // 位置重置
    // MC: this.bipedRightArm.rotationPointZ = 0.0F;
    //     this.bipedLeftLeg.rotationPointZ = 0.0F;
    //     this.bipedRightLeg.rotationPointY = -5.0F;
    //     this.bipedLeftLeg.rotationPointY = -5.0F;
    if (m_rightArm) {
        m_rightArm->setRotationPointZ(0.0f);
    }
    if (m_leftArm) {
        m_leftArm->setRotationPointZ(0.0f);
    }
    if (m_rightLeg) {
        m_rightLeg->setRotationPointZ(0.0f);
        m_rightLeg->setRotationPointY(-5.0f);
    }
    if (m_leftLeg) {
        m_leftLeg->setRotationPointZ(0.0f);
        m_leftLeg->setRotationPointY(-5.0f);
    }

    // 头部位置
    // MC: this.bipedHead.rotationPointZ = -0.0F;
    //     this.bipedHead.rotationPointY = -13.0F;
    if (m_head) {
        m_head->setRotationPointZ(0.0f);
        m_head->setRotationPointY(-13.0f);
    }

    // 攻击/尖叫状态：头部下移
    // MC: if (this.isAttacking) {
    //         this.bipedHead.rotationPointY -= 5.0F;
    //     }
    if (m_attacking && m_head) {
        m_head->setRotationPointY(m_head->rotationPointY() - 5.0f);
    }

    // 同步头部外层位置和角度
    // MC: this.bipedHeadwear.rotationPointX = this.bipedHead.rotationPointX;
    //     this.bipedHeadwear.rotationPointY = this.bipedHead.rotationPointY;
    //     this.bipedHeadwear.rotationPointZ = this.bipedHead.rotationPointZ;
    //     this.bipedHeadwear.rotateAngleX = this.bipedHead.rotateAngleX;
    //     this.bipedHeadwear.rotateAngleY = this.bipedHead.rotateAngleY;
    //     this.bipedHeadwear.rotateAngleZ = this.bipedHead.rotateAngleZ;
    if (m_head && m_headwear) {
        m_headwear->setRotationPoint(m_head->rotationPointX(), m_head->rotationPointY(), m_head->rotationPointZ());
        m_headwear->setRotateAngleX(m_head->rotateAngleX());
        m_headwear->setRotateAngleY(m_head->rotateAngleY());
        m_headwear->setRotateAngleZ(m_head->rotateAngleZ());
    }

    // 最终手臂位置
    // MC: this.bipedRightArm.setRotationPoint(-5.0F, -12.0F, 0.0F);
    //     this.bipedLeftArm.setRotationPoint(5.0F, -12.0F, 0.0F);
    if (m_rightArm) {
        m_rightArm->setRotationPoint(-5.0f, -12.0f, 0.0f);
    }
    if (m_leftArm) {
        m_leftArm->setRotationPoint(5.0f, -12.0f, 0.0f);
    }

    (void)ageInTicks;  // 末影人不使用 ageInTicks
    (void)scale;       // 已在 render() 中使用
}

} // namespace mc::client::renderer::entity::model::monster
