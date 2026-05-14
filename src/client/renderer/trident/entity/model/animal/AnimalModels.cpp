#include "AnimalModels.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::animal {

// ==================== PigModel ====================

PigModel::PigModel()
    : QuadrupedModel(6, 0.0f, false, 4.0f, 4.0f, 2.0f, 2.0f, 24.0f)
{
    // 参考 MC 1.16.5 PigModel：在基础四足模型上追加猪鼻子
    // Java: this.headModel.setTextureOffset(16, 16).addBox(-2.0F, 0.0F, -9.0F, 4.0F, 3.0F, 1.0F, scale);
    m_head->setTextureOffset(16, 16).addBox(-2.0f, 0.0f, -9.0f, 4.0f, 3.0f, 1.0f);

    m_textureWidth = 64;
    m_textureHeight = 32;
}

void PigModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 调用基类动画
    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    (void)scale;
    (void)ageInTicks;
}

// ==================== CowModel ====================

CowModel::CowModel()
    : QuadrupedModel(12, 0.0f, false, 10.0f, 4.0f, 2.0f, 2.0f, 24.0f)
{
    // 参考 MC 1.16.5 CowModel：重建头、身（腿在 QuadrupedModel 中已创建，需调整位置）
    // Java: this.headModel = new ModelRenderer(this, 0, 0);
    //       this.headModel.addBox(-4.0F, -4.0F, -6.0F, 8.0F, 8.0F, 6.0F, 0.0F);
    //       this.headModel.setRotationPoint(0.0F, 4.0F, -8.0F);
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->addBox(-4.0f, -4.0f, -6.0f, 8.0f, 8.0f, 6.0f);
    m_head->setRotationPoint(0.0f, 4.0f, -8.0f);
    // 牛角
    // Java: this.headModel.setTextureOffset(22, 0).addBox(-5.0F, -5.0F, -4.0F, 1.0F, 3.0F, 1.0F, 0.0F);
    //       this.headModel.setTextureOffset(22, 0).addBox(4.0F, -5.0F, -4.0F, 1.0F, 3.0F, 1.0F, 0.0F);
    m_head->setTextureOffset(22, 0).addBox(-5.0f, -5.0f, -4.0f, 1.0f, 3.0f, 1.0f);
    m_head->setTextureOffset(22, 0).addBox(4.0f, -5.0f, -4.0f, 1.0f, 3.0f, 1.0f);

    // Java: this.body = new ModelRenderer(this, 18, 4);
    //       this.body.addBox(-6.0F, -10.0F, -7.0F, 12.0F, 18.0F, 10.0F, 0.0F);
    //       this.body.setRotationPoint(0.0F, 5.0F, 2.0F);
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(18, 4).addBox(-6.0f, -10.0f, -7.0f, 12.0f, 18.0f, 10.0f);
    m_body->setRotationPoint(0.0f, 5.0f, 2.0f);
    // Java: this.body.setTextureOffset(52, 0).addBox(-2.0F, 2.0F, -8.0F, 4.0F, 6.0F, 1.0F);
    m_body->setTextureOffset(52, 0).addBox(-2.0f, 2.0f, -8.0f, 4.0f, 6.0f, 1.0f);

    // 腿部：QuadrupedModel 已创建，但需要调整位置
    // Java CowModel 构造函数末尾调整:
    //   --this.legBackRight.rotationPointX;   // -3 → -4
    //   ++this.legBackLeft.rotationPointX;    //  3 →  4
    //   --this.legFrontRight.rotationPointX;  // -3 → -4
    //   ++this.legFrontLeft.rotationPointX;   //  3 →  4
    //   --this.legFrontRight.rotationPointZ;  // -5 → -6
    //   --this.legFrontLeft.rotationPointZ;   // -5 → -6
    // 牛腿高度 12，所以 Y = 24 - 12 = 12
    m_legBackRight->setRotationPoint(-4.0f, 12.0f, 7.0f);
    m_legBackLeft->setRotationPoint(4.0f, 12.0f, 7.0f);
    m_legFrontRight->setRotationPoint(-4.0f, 12.0f, -6.0f);
    m_legFrontLeft->setRotationPoint(4.0f, 12.0f, -6.0f);

    // 更新部件列表
    m_parts.clear();
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_legFrontRight);
    m_parts.push_back(m_legFrontLeft);
    m_parts.push_back(m_legBackRight);
    m_parts.push_back(m_legBackLeft);

    m_textureWidth = 64;
    m_textureHeight = 32;
}

void CowModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    (void)scale;
    (void)ageInTicks;
}

// ==================== SheepModel ====================

SheepModel::SheepModel()
    : QuadrupedModel(12, 0.0f, false, 8.0f, 4.0f, 2.0f, 2.0f, 24.0f)
{
    // 参考 MC 1.16.5 SheepModel：重建头、身
    // Java: this.headModel = new ModelRenderer(this, 0, 0);
    //       this.headModel.addBox(-3.0F, -4.0F, -6.0F, 6.0F, 6.0F, 8.0F, 0.0F);
    //       this.headModel.setRotationPoint(0.0F, 6.0F, -8.0F);
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-3.0f, -4.0f, -6.0f, 6.0f, 6.0f, 8.0f);
    m_head->setRotationPoint(0.0f, 6.0f, -8.0f);

    // Java: this.body = new ModelRenderer(this, 28, 8);
    //       this.body.addBox(-4.0F, -10.0F, -7.0F, 8.0F, 16.0F, 6.0F, 0.0F);
    //       this.body.setRotationPoint(0.0F, 5.0F, 2.0F);
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(28, 8).addBox(-4.0f, -10.0f, -7.0f, 8.0f, 16.0f, 6.0f);
    m_body->setRotationPoint(0.0f, 5.0f, 2.0f);

    // 腿部使用 QuadrupedModel 默认位置（羊不需要调整腿部位置）
    // 羊腿高度 12，所以 Y = 24 - 12 = 12

    // 更新部件列表
    m_parts.clear();
    m_parts.push_back(m_head);
    m_parts.push_back(m_body);
    m_parts.push_back(m_legFrontRight);
    m_parts.push_back(m_legFrontLeft);
    m_parts.push_back(m_legBackRight);
    m_parts.push_back(m_legBackLeft);

    m_textureWidth = 64;
    m_textureHeight = 32;
}

void SheepModel::setAngles(f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    QuadrupedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 参考 MC 1.16.5 SheepModel.setRotationAngles
    // Java: this.headModel.rotateAngleX = this.headRotationAngleX;
    // 用实体的头部旋转角度覆盖默认的 headPitch 旋转
    m_head->setRotateAngleX(static_cast<f32>(m_headRotationAngleX));

    (void)scale;
    (void)ageInTicks;
}

void SheepModel::setLivingAnimations(f64 /*limbSwing*/, f64 /*limbSwingAmount*/, f64 /*partialTick*/)
{
    // 参考 MC 1.16.5 SheepModel.setLivingAnimations
    // Java:
    //   this.headModel.rotationPointY = 6.0F + entityIn.getHeadRotationPointY(partialTick) * 9.0F;
    //   this.headRotationAngleX = entityIn.getHeadRotationAngleX(partialTick);
    //
    // 头部 Y 位置由 setHeadRotation() 设置的 m_headRotationPointY 控制
    // 这里应用偏移：默认 6.0 + rotationPointY * 9.0
    m_head->setRotationPointY(6.0f + m_headRotationPointY * 9.0f);
}

// ==================== ChickenModel ====================

ChickenModel::ChickenModel()
    : AgeableModel() // 使用 AgeableModel 默认构造函数
{
    // 参考 MC 1.16.5 ChickenModel
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-2.0f, -6.0f, -2.0f, 4.0f, 6.0f, 3.0f);
    m_head->setRotationPoint(0.0f, 15.0f, -4.0f);

    // 喙（bill）
    m_beak = std::make_shared<ModelRenderer>("beak");
    m_beak->setTextureOffset(14, 0);
    m_beak->addBox(-2.0f, -4.0f, -4.0f, 4.0f, 2.0f, 2.0f);
    m_beak->setRotationPoint(0.0f, 15.0f, -4.0f);

    // 肉垂（chin）
    m_wattle = std::make_shared<ModelRenderer>("wattle");
    m_wattle->setTextureOffset(14, 4);
    m_wattle->addBox(-1.0f, -2.0f, -3.0f, 2.0f, 2.0f, 2.0f);
    m_wattle->setRotationPoint(0.0f, 15.0f, -4.0f);

    m_comb.reset();

    // 身体
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 9);
    m_body->addBox(-3.0f, -4.0f, -3.0f, 6.0f, 8.0f, 6.0f);
    m_body->setRotationPoint(0.0f, 16.0f, 0.0f);

    // 右翼
    m_rightWing = std::make_shared<ModelRenderer>("rightWing");
    m_rightWing->setTextureOffset(24, 13);
    m_rightWing->addBox(0.0f, 0.0f, -3.0f, 1.0f, 4.0f, 6.0f);
    m_rightWing->setRotationPoint(-4.0f, 13.0f, 0.0f);

    // 左翼
    m_leftWing = std::make_shared<ModelRenderer>("leftWing");
    m_leftWing->setTextureOffset(24, 13);
    m_leftWing->addBox(-1.0f, 0.0f, -3.0f, 1.0f, 4.0f, 6.0f);
    m_leftWing->setRotationPoint(4.0f, 13.0f, 0.0f);

    // 右腿
    m_rightLeg = std::make_shared<ModelRenderer>("rightLeg");
    m_rightLeg->setTextureOffset(26, 0);
    m_rightLeg->addBox(-1.0f, 0.0f, -3.0f, 3.0f, 5.0f, 3.0f);
    m_rightLeg->setRotationPoint(-2.0f, 19.0f, 1.0f);

    // 左腿
    m_leftLeg = std::make_shared<ModelRenderer>("leftLeg");
    m_leftLeg->setTextureOffset(26, 0);
    m_leftLeg->addBox(-1.0f, 0.0f, -3.0f, 3.0f, 5.0f, 3.0f);
    m_leftLeg->setRotationPoint(1.0f, 19.0f, 1.0f);

    // 添加到部件列表 - 注意：AgeableModel 会通过 getHeadParts/getBodyParts 处理
    m_parts.push_back(m_head);
    m_parts.push_back(m_beak);
    m_parts.push_back(m_wattle);
    m_parts.push_back(m_body);
    m_parts.push_back(m_rightWing);
    m_parts.push_back(m_leftWing);
    m_parts.push_back(m_rightLeg);
    m_parts.push_back(m_leftLeg);

    m_textureWidth = 64;
    m_textureHeight = 32;
}

std::vector<std::shared_ptr<ModelRenderer>> ChickenModel::getHeadParts() const
{
    std::vector<std::shared_ptr<ModelRenderer>> parts;
    parts.push_back(m_head);
    parts.push_back(m_beak);
    parts.push_back(m_wattle);
    if (m_comb) {
        parts.push_back(m_comb);
    }
    return parts;
}

std::vector<std::shared_ptr<ModelRenderer>> ChickenModel::getBodyParts() const
{
    std::vector<std::shared_ptr<ModelRenderer>> parts;
    parts.push_back(m_body);
    parts.push_back(m_rightWing);
    parts.push_back(m_leftWing);
    parts.push_back(m_rightLeg);
    parts.push_back(m_leftLeg);
    return parts;
}

void ChickenModel::render(f64 scale)
{
    EntityModel::render(scale);
}

void ChickenModel::setAngles(
    f64 limbSwing, f64 limbSwingAmount, f64 ageInTicks, f64 netHeadYaw, f64 headPitch, f64 scale)
{
    // 头部旋转
    m_head->setRotateAngleX(static_cast<f32>(math::toRadians(static_cast<f32>(headPitch))));
    m_head->setRotateAngleY(static_cast<f32>(math::toRadians(static_cast<f32>(netHeadYaw))));

    // 喙、肉垂、鸡冠跟随头部
    m_beak->setRotateAngleX(m_head->rotateAngleX());
    m_beak->setRotateAngleY(m_head->rotateAngleY());
    m_wattle->setRotateAngleX(m_head->rotateAngleX());
    m_wattle->setRotateAngleY(m_head->rotateAngleY());
    if (m_comb) {
        m_comb->setRotateAngleX(m_head->rotateAngleX());
        m_comb->setRotateAngleY(m_head->rotateAngleY());
    }

    // 身体基础姿态（水平）
    m_body->setRotateAngleX(math::PI * 0.5f);

    // 步态动画
    const f64 walkAngle = limbSwing * 0.6662f;
    const f64 walkAmount = limbSwingAmount * 1.4f;

    // 腿部动画
    m_rightLeg->setRotateAngleX(static_cast<f32>(std::cos(walkAngle) * walkAmount));
    m_leftLeg->setRotateAngleX(static_cast<f32>(std::cos(walkAngle + math::PI) * walkAmount));

    // 翅膀动画（与原版一致，按年龄tick摆动）
    m_rightWing->setRotateAngleZ(static_cast<f32>(ageInTicks));
    m_leftWing->setRotateAngleZ(static_cast<f32>(-ageInTicks));

    (void)scale;
}

} // namespace mc::client::renderer::entity::model::animal
