#include "AquaticModels.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::aquatic {

// ==================== CodModel ====================
// 参考 MC 1.16.5 CodModel

CodModel::CodModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    setupParts();
}

void CodModel::setupParts() {
    // 参考 MC 1.16.5 CodModel 构造函数
    // body: (-1, -2, 0) 到 (1, 2, 7), 纹理 (0, 0), 旋转点 (0, 22, 0)
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-1.0f, -2.0f, 0.0f, 2.0f, 4.0f, 7.0f);
    m_body->setRotationPoint(0.0f, 22.0f, 0.0f);

    // head: 纹理 (11, 0), (-1, -2, -3) 到 (1, 2, 0), 旋转点 (0, 22, 0)
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(11, 0);
    m_head->addBox(-1.0f, -2.0f, -3.0f, 2.0f, 4.0f, 3.0f);
    m_head->setRotationPoint(0.0f, 22.0f, 0.0f);

    // headFront: 纹理 (0, 0), (-1, -2, -1) 到 (1, 1, 0), 旋转点 (0, 22, -3)
    m_headFront = std::make_shared<ModelRenderer>("headFront");
    m_headFront->setTextureOffset(0, 0);
    m_headFront->addBox(-1.0f, -2.0f, -1.0f, 2.0f, 3.0f, 1.0f);
    m_headFront->setRotationPoint(0.0f, 22.0f, -3.0f);

    // finRight: 纹理 (22, 1), (-2, 0, -1) 到 (0, 0, 1), 旋转点 (-1, 23, 0)
    m_finRight = std::make_shared<ModelRenderer>("finRight");
    m_finRight->setTextureOffset(22, 1);
    m_finRight->addBox(-2.0f, 0.0f, -1.0f, 2.0f, 0.0f, 2.0f);
    m_finRight->setRotationPoint(-1.0f, 23.0f, 0.0f);
    m_finRight->setRotateAngleZ(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0));

    // finLeft: 纹理 (22, 4), (0, 0, -1) 到 (2, 0, 1), 旋转点 (1, 23, 0)
    m_finLeft = std::make_shared<ModelRenderer>("finLeft");
    m_finLeft->setTextureOffset(22, 4);
    m_finLeft->addBox(0.0f, 0.0f, -1.0f, 2.0f, 0.0f, 2.0f);
    m_finLeft->setRotationPoint(1.0f, 23.0f, 0.0f);
    m_finLeft->setRotateAngleZ(static_cast<f32>(mc::math::PI_DOUBLE / 4.0));

    // tail: 纹理 (22, 3), (0, -2, 0) 到 (0, 2, 4), 旋转点 (0, 22, 7)
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(22, 3);
    m_tail->addBox(0.0f, -2.0f, 0.0f, 0.0f, 4.0f, 4.0f);
    m_tail->setRotationPoint(0.0f, 22.0f, 7.0f);

    // finTop: 纹理 (20, -6), (0, -1, -1) 到 (0, 0, 5), 旋转点 (0, 20, 0)
    m_finTop = std::make_shared<ModelRenderer>("finTop");
    m_finTop->setTextureOffset(20, -6);
    m_finTop->addBox(0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 6.0f);
    m_finTop->setRotationPoint(0.0f, 20.0f, 0.0f);

    // 添加到部件列表
    m_parts.push_back(m_body);
    m_parts.push_back(m_head);
    m_parts.push_back(m_headFront);
    m_parts.push_back(m_finRight);
    m_parts.push_back(m_finLeft);
    m_parts.push_back(m_tail);
    m_parts.push_back(m_finTop);
}

void CodModel::render(f64 scale) {
    EntityModel::render(scale);
}

void CodModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                          f64 ageInTicks, f64 netHeadYaw,
                          f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 CodModel.setRotationAngles
    // float f = 1.0F; if (!entityIn.isInWater()) { f = 1.5F; }
    // this.tail.rotateAngleY = -f * 0.45F * MathHelper.sin(0.6F * ageInTicks);
    f32 f = m_isInWater ? 1.0f : 1.5f;
    m_tail->setRotateAngleY(-f * 0.45f * static_cast<f32>(std::sin(0.6 * ageInTicks)));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== SalmonModel ====================
// 参考 MC 1.16.5 SalmonModel

SalmonModel::SalmonModel()
    : EntityModel()
{
    setTextureSize(32, 32);
    setupParts();
}

void SalmonModel::setupParts() {
    // 参考 MC 1.16.5 SalmonModel 构造函数
    // bodyFront: 纹理 (0, 0), (-1.5, -2.5, 0) 到 (1.5, 2.5, 8), 旋转点 (0, 20, 0)
    m_bodyFront = std::make_shared<ModelRenderer>("bodyFront");
    m_bodyFront->setTextureOffset(0, 0);
    m_bodyFront->addBox(-1.5f, -2.5f, 0.0f, 3.0f, 5.0f, 8.0f);
    m_bodyFront->setRotationPoint(0.0f, 20.0f, 0.0f);

    // bodyRear: 纹理 (0, 13), (-1.5, -2.5, 0) 到 (1.5, 2.5, 8), 旋转点 (0, 20, 8)
    m_bodyRear = std::make_shared<ModelRenderer>("bodyRear");
    m_bodyRear->setTextureOffset(0, 13);
    m_bodyRear->addBox(-1.5f, -2.5f, 0.0f, 3.0f, 5.0f, 8.0f);
    m_bodyRear->setRotationPoint(0.0f, 20.0f, 8.0f);

    // head: 纹理 (22, 0), (-1, -2, -3) 到 (1, 2, 0), 旋转点 (0, 20, 0)
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(22, 0);
    m_head->addBox(-1.0f, -2.0f, -3.0f, 2.0f, 4.0f, 3.0f);
    m_head->setRotationPoint(0.0f, 20.0f, 0.0f);

    // tail: 纹理 (20, 10), (0, -2.5, 0) 到 (0, 2.5, 6), 子部件位于 bodyRear (0, 0, 8)
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(20, 10);
    m_tail->addBox(0.0f, -2.5f, 0.0f, 0.0f, 5.0f, 6.0f);
    m_tail->setRotationPoint(0.0f, 0.0f, 8.0f);
    m_bodyRear->addChild(m_tail);

    // dorsalFin: 纹理 (2, 1), (0, 0, 0) 到 (0, 2, 3), 子部件位于 bodyFront (0, -4.5, 5)
    m_dorsalFin = std::make_shared<ModelRenderer>("dorsalFin");
    m_dorsalFin->setTextureOffset(2, 1);
    m_dorsalFin->addBox(0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 3.0f);
    m_dorsalFin->setRotationPoint(0.0f, -4.5f, 5.0f);
    m_bodyFront->addChild(m_dorsalFin);

    // ventralFin: 纹理 (0, 2), (0, 0, 0) 到 (0, 2, 4), 子部件位于 bodyRear (0, -4.5, -1)
    m_ventralFin = std::make_shared<ModelRenderer>("ventralFin");
    m_ventralFin->setTextureOffset(0, 2);
    m_ventralFin->addBox(0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 4.0f);
    m_ventralFin->setRotationPoint(0.0f, -4.5f, -1.0f);
    m_bodyRear->addChild(m_ventralFin);

    // finRight: 纹理 (-4, 0), (-2, 0, 0) 到 (0, 0, 2), 旋转点 (-1.5, 21.5, 0)
    m_finRight = std::make_shared<ModelRenderer>("finRight");
    m_finRight->setTextureOffset(-4, 0);
    m_finRight->addBox(-2.0f, 0.0f, 0.0f, 2.0f, 0.0f, 2.0f);
    m_finRight->setRotationPoint(-1.5f, 21.5f, 0.0f);
    m_finRight->setRotateAngleZ(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0));

    // finLeft: 纹理 (0, 0), (0, 0, 0) 到 (2, 0, 2), 旋转点 (1.5, 21.5, 0)
    m_finLeft = std::make_shared<ModelRenderer>("finLeft");
    m_finLeft->setTextureOffset(0, 0);
    m_finLeft->addBox(0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 2.0f);
    m_finLeft->setRotationPoint(1.5f, 21.5f, 0.0f);
    m_finLeft->setRotateAngleZ(static_cast<f32>(mc::math::PI_DOUBLE / 4.0));

    // 添加到部件列表（子部件会跟随父部件渲染，不需要单独添加）
    m_parts.push_back(m_bodyFront);
    m_parts.push_back(m_bodyRear);
    m_parts.push_back(m_head);
    m_parts.push_back(m_finRight);
    m_parts.push_back(m_finLeft);
}

void SalmonModel::render(f64 scale) {
    EntityModel::render(scale);
}

void SalmonModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                             f64 ageInTicks, f64 netHeadYaw,
                             f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 SalmonModel.setRotationAngles
    // float f = 1.0F; float f1 = 1.0F;
    // if (!entityIn.isInWater()) { f = 1.3F; f1 = 1.7F; }
    // this.bodyRear.rotateAngleY = -f * 0.25F * MathHelper.sin(f1 * 0.6F * ageInTicks);
    f32 f = m_isInWater ? 1.0f : 1.3f;
    f32 f1 = m_isInWater ? 1.0f : 1.7f;
    m_bodyRear->setRotateAngleY(-f * 0.25f * static_cast<f32>(std::sin(f1 * 0.6 * ageInTicks)));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== DolphinModel ====================
// 参考 MC 1.16.5 DolphinModel

DolphinModel::DolphinModel()
    : EntityModel()
{
    setTextureSize(64, 64);  // Java: textureWidth = 64, textureHeight = 64
    setupParts();
}

void DolphinModel::setupParts() {
    // 参考 MC 1.16.5 DolphinModel 构造函数
    // body: textureOffset(22, 0), addBox(-4, -7, 0, 8, 7, 13), rotationPoint(0, 22, -5)
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(22, 0);
    m_body->addBox(-4.0f, -7.0f, 0.0f, 8.0f, 7.0f, 13.0f);
    m_body->setRotationPoint(0.0f, 22.0f, -5.0f);

    // 背鳍: textureOffset(51, 0), addBox(-0.5, 0, 8, 1, 4, 5), rotateAngleX = mc::math::PI_DOUBLE/3
    // 这是原版遗漏的重要部件！
    m_dorsalFin = std::make_shared<ModelRenderer>("dorsalFin");
    m_dorsalFin->setTextureOffset(51, 0);
    m_dorsalFin->addBox(-0.5f, 0.0f, 8.0f, 1.0f, 4.0f, 5.0f);
    m_dorsalFin->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 3.0));
    m_body->addChild(m_dorsalFin);

    // tail: textureOffset(0, 19), addBox(-2, -2.5, 0, 4, 5, 11), rotationPoint(0, -2.5, 11) 作为 body 子部件
    // rotateAngleX = -0.10471976F
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(0, 19);
    m_tail->addBox(-2.0f, -2.5f, 0.0f, 4.0f, 5.0f, 11.0f);
    m_tail->setRotationPoint(0.0f, -2.5f, 11.0f);
    m_tail->setRotateAngleX(-0.10471976f);
    m_body->addChild(m_tail);

    // tailFin: textureOffset(19, 20), addBox(-5, -0.5, 0, 10, 1, 6), rotationPoint(0, 0, 9) 作为 tail 子部件
    m_tailFin = std::make_shared<ModelRenderer>("tailFin");
    m_tailFin->setTextureOffset(19, 20);
    m_tailFin->addBox(-5.0f, -0.5f, 0.0f, 10.0f, 1.0f, 6.0f);
    m_tailFin->setRotationPoint(0.0f, 0.0f, 9.0f);
    m_tail->addChild(m_tailFin);

    // 右鳍: textureOffset(48, 20), mirror=true, addBox(-0.5, -4, 0, 1, 4, 7), rotationPoint(2, -2, 4)
    // rotateAngleX = mc::math::PI_DOUBLE/3, rotateAngleZ = 2.0943952F
    m_finRight = std::make_shared<ModelRenderer>("finRight");
    m_finRight->setTextureOffset(48, 20);
    m_finRight->setMirror(true);
    m_finRight->addBox(-0.5f, -4.0f, 0.0f, 1.0f, 4.0f, 7.0f);
    m_finRight->setRotationPoint(2.0f, -2.0f, 4.0f);
    m_finRight->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 3.0));
    m_finRight->setRotateAngleZ(2.0943952f);
    m_body->addChild(m_finRight);

    // 左鳍: textureOffset(48, 20), addBox(-0.5, -4, 0, 1, 4, 7), rotationPoint(-2, -2, 4)
    // rotateAngleX = mc::math::PI_DOUBLE/3, rotateAngleZ = -2.0943952F
    m_finLeft = std::make_shared<ModelRenderer>("finLeft");
    m_finLeft->setTextureOffset(48, 20);
    m_finLeft->addBox(-0.5f, -4.0f, 0.0f, 1.0f, 4.0f, 7.0f);
    m_finLeft->setRotationPoint(-2.0f, -2.0f, 4.0f);
    m_finLeft->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 3.0));
    m_finLeft->setRotateAngleZ(-2.0943952f);
    m_body->addChild(m_finLeft);

    // head: textureOffset(0, 0), addBox(-4, -3, -3, 8, 7, 6), rotationPoint(0, -4, -3) 作为 body 子部件
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(0, 0);
    m_head->addBox(-4.0f, -3.0f, -3.0f, 8.0f, 7.0f, 6.0f);
    m_head->setRotationPoint(0.0f, -4.0f, -3.0f);
    m_body->addChild(m_head);

    // nose: textureOffset(0, 13), addBox(-1, 2, -7, 2, 2, 4) 作为 head 子部件
    m_nose = std::make_shared<ModelRenderer>("nose");
    m_nose->setTextureOffset(0, 13);
    m_nose->addBox(-1.0f, 2.0f, -7.0f, 2.0f, 2.0f, 4.0f);
    m_head->addChild(m_nose);

    // 只有 body 需要添加到 m_parts（其他都是子部件）
    m_parts.push_back(m_body);
}

void DolphinModel::render(f64 scale) {
    EntityModel::render(scale);
}

void DolphinModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                              f64 ageInTicks, f64 netHeadYaw,
                              f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 DolphinModel.setRotationAngles
    // body.rotateAngleX = headPitch * mc::math::PI_DOUBLE/180
    // body.rotateAngleY = netHeadYaw * mc::math::PI_DOUBLE/180
    m_body->setRotateAngleX(static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0));
    m_body->setRotateAngleY(static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0));

    // 修复：Java 原版使用 Entity.horizontalMag(getMotion()) > 1.0E-7D 判断是否在移动
    // 而不是 isInWater！
    // if (Entity.horizontalMag(entityIn.getMotion()) > 1.0E-7D) {
    //     body.rotateAngleX += -0.05F + -0.05F * cos(ageInTicks * 0.3F);
    //     tail.rotateAngleX = -0.1F * cos(ageInTicks * 0.3F);
    //     tailFin.rotateAngleX = -0.2F * cos(ageInTicks * 0.3F);
    // }
    constexpr f64 MOTION_THRESHOLD = 1.0E-7;
    if (m_motionMagnitude > MOTION_THRESHOLD) {
        f32 wave = static_cast<f32>(std::cos(ageInTicks * 0.3));
        m_body->setRotateAngleX(m_body->rotateAngleX() + (-0.05f - 0.05f * wave));
        m_tail->setRotateAngleX(-0.1f * wave);
        m_tailFin->setRotateAngleX(-0.2f * wave);
    } else {
        // 不移动时恢复初始角度
        m_tail->setRotateAngleX(-0.10471976f);
        m_tailFin->setRotateAngleX(0.0f);
    }

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)scale;
}

// ==================== TurtleModel ====================
// 参考 MC 1.16.5 TurtleModel

TurtleModel::TurtleModel()
    : EntityModel()
{
    setTextureSize(128, 64);
    setupParts(0.0f);
}

TurtleModel::TurtleModel(f32 scale)
    : EntityModel()
{
    setTextureSize(128, 64);
    setupParts(scale);
}

void TurtleModel::setupParts(f32 scale) {
    // 参考 MC 1.16.5 TurtleModel 构造函数
    // 构造函数参数: QuadrupedModel(12, scale, true, 120.0F, 0.0F, 9.0F, 6.0F, 120)
    // 但我们直接在这里设置所有部件

    // 头部: textureOffset(3, 0), addBox(-3, -1, -3, 6, 5, 6), rotationPoint(0, 19, -10)
    m_head = std::make_shared<ModelRenderer>("head");
    m_head->setTextureOffset(3, 0);
    m_head->addBox(-3.0f, -1.0f, -3.0f, 6.0f, 5.0f, 6.0f, scale);
    m_head->setRotationPoint(0.0f, 19.0f, -10.0f);
    m_parts.push_back(m_head);

    // 身体: 两个 addBox 组成
    m_body = std::make_shared<ModelRenderer>("body");
    // textureOffset(7, 37), addBox(-9.5, 3, -10, 19, 20, 6)
    m_body->setTextureOffset(7, 37);
    m_body->addBox(-9.5f, 3.0f, -10.0f, 19.0f, 20.0f, 6.0f, scale);
    // textureOffset(31, 1), addBox(-5.5, 3, -13, 11, 18, 3)
    m_body->setTextureOffset(31, 1);
    m_body->addBox(-5.5f, 3.0f, -13.0f, 11.0f, 18.0f, 3.0f, scale);
    m_body->setRotationPoint(0.0f, 11.0f, -10.0f);
    m_parts.push_back(m_body);

    // 怀孕腹部: textureOffset(70, 33), addBox(-4.5, 3, -14, 9, 18, 1), rotationPoint(0, 11, -10)
    m_pregnant = std::make_shared<ModelRenderer>("pregnant");
    m_pregnant->setTextureOffset(70, 33);
    m_pregnant->addBox(-4.5f, 3.0f, -14.0f, 9.0f, 18.0f, 1.0f, scale);
    m_pregnant->setRotationPoint(0.0f, 11.0f, -10.0f);
    m_pregnant->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 2.0));
    m_pregnant->setVisible(false);  // 默认隐藏
    m_parts.push_back(m_pregnant);

    // 右后腿: textureOffset(1, 23), addBox(-2, 0, 0, 4, 1, 10), rotationPoint(-3.5, 22, 11)
    m_legBackRight = std::make_shared<ModelRenderer>("legBackRight");
    m_legBackRight->setTextureOffset(1, 23);
    m_legBackRight->addBox(-2.0f, 0.0f, 0.0f, 4.0f, 1.0f, 10.0f, scale);
    m_legBackRight->setRotationPoint(-3.5f, 22.0f, 11.0f);
    m_parts.push_back(m_legBackRight);

    // 左后腿: textureOffset(1, 12), addBox(-2, 0, 0, 4, 1, 10), rotationPoint(3.5, 22, 11)
    m_legBackLeft = std::make_shared<ModelRenderer>("legBackLeft");
    m_legBackLeft->setTextureOffset(1, 12);
    m_legBackLeft->addBox(-2.0f, 0.0f, 0.0f, 4.0f, 1.0f, 10.0f, scale);
    m_legBackLeft->setRotationPoint(3.5f, 22.0f, 11.0f);
    m_parts.push_back(m_legBackLeft);

    // 右前腿: textureOffset(27, 30), addBox(-13, 0, -2, 13, 1, 5), rotationPoint(-5, 21, -4)
    m_legFrontRight = std::make_shared<ModelRenderer>("legFrontRight");
    m_legFrontRight->setTextureOffset(27, 30);
    m_legFrontRight->addBox(-13.0f, 0.0f, -2.0f, 13.0f, 1.0f, 5.0f, scale);
    m_legFrontRight->setRotationPoint(-5.0f, 21.0f, -4.0f);
    m_parts.push_back(m_legFrontRight);

    // 左前腿: textureOffset(27, 24), addBox(0, 0, -2, 13, 1, 5), rotationPoint(5, 21, -4)
    m_legFrontLeft = std::make_shared<ModelRenderer>("legFrontLeft");
    m_legFrontLeft->setTextureOffset(27, 24);
    m_legFrontLeft->addBox(0.0f, 0.0f, -2.0f, 13.0f, 1.0f, 5.0f, scale);
    m_legFrontLeft->setRotationPoint(5.0f, 21.0f, -4.0f);
    m_parts.push_back(m_legFrontLeft);
}

void TurtleModel::render(f64 scale) {
    // 如果有蛋且不是幼体，渲染怀孕腹部前先下移
    m_pregnant->setVisible(m_hasEgg && !m_isChild);
    EntityModel::render(scale);
}

void TurtleModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                             f64 ageInTicks, f64 netHeadYaw,
                             f64 headPitch, f64 scale) {
    // 参考 MC 1.16.5 TurtleModel.setRotationAngles
    // 后腿 X 轴旋转
    m_legBackRight->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 * 0.6) * 0.5 * limbSwingAmount));
    m_legBackLeft->setRotateAngleX(static_cast<f32>(std::cos(limbSwing * 0.6662 * 0.6 + mc::math::PI_DOUBLE) * 0.5 * limbSwingAmount));

    // 前腿 Z 轴旋转（与后腿相反）
    m_legFrontRight->setRotateAngleZ(static_cast<f32>(std::cos(limbSwing * 0.6662 * 0.6 + mc::math::PI_DOUBLE) * 0.5 * limbSwingAmount));
    m_legFrontLeft->setRotateAngleZ(static_cast<f32>(std::cos(limbSwing * 0.6662 * 0.6) * 0.5 * limbSwingAmount));

    // 前腿 X 和 Y 轴旋转归零
    m_legFrontRight->setRotateAngleX(0.0f);
    m_legFrontLeft->setRotateAngleX(0.0f);
    m_legFrontRight->setRotateAngleY(0.0f);
    m_legFrontLeft->setRotateAngleY(0.0f);
    m_legBackRight->setRotateAngleY(0.0f);
    m_legBackLeft->setRotateAngleY(0.0f);

    // 怀孕腹部 X 轴旋转
    m_pregnant->setRotateAngleX(static_cast<f32>(mc::math::PI_DOUBLE / 2.0));

    // 如果不在水中且在地面，执行爬行动画
    if (!m_isInWater && m_isOnGround) {
        f32 f = m_isDigging ? 4.0f : 1.0f;
        f32 f1 = m_isDigging ? 2.0f : 1.0f;

        // 前腿 Y 轴旋转
        m_legFrontRight->setRotateAngleY(static_cast<f32>(std::cos(f * limbSwing * 5.0 + mc::math::PI_DOUBLE) * 8.0 * limbSwingAmount * f1));
        m_legFrontLeft->setRotateAngleY(static_cast<f32>(std::cos(f * limbSwing * 5.0) * 8.0 * limbSwingAmount * f1));
        m_legFrontRight->setRotateAngleZ(0.0f);
        m_legFrontLeft->setRotateAngleZ(0.0f);

        // 后腿 Y 轴旋转
        m_legBackRight->setRotateAngleY(static_cast<f32>(std::cos(limbSwing * 5.0 + mc::math::PI_DOUBLE) * 3.0 * limbSwingAmount));
        m_legBackLeft->setRotateAngleY(static_cast<f32>(std::cos(limbSwing * 5.0) * 3.0 * limbSwingAmount));
        m_legBackRight->setRotateAngleX(0.0f);
        m_legBackLeft->setRotateAngleX(0.0f);
    }

    // 头部旋转
    m_head->setRotateAngleY(static_cast<f32>(netHeadYaw * mc::math::PI_DOUBLE / 180.0));
    m_head->setRotateAngleX(static_cast<f32>(headPitch * mc::math::PI_DOUBLE / 180.0));

    (void)ageInTicks;
    (void)scale;
}

// ==================== AbstractTropicalFishModel ====================

void AbstractTropicalFishModel::render(f64 scale) {
    EntityModel::render(scale);
}

// ==================== TropicalFishAModel ====================

TropicalFishAModel::TropicalFishAModel(f32 scale)
    : AbstractTropicalFishModel()
{
    setTextureSize(32, 32);
    setupParts(scale);
    // 添加部件到列表
    m_parts.push_back(m_body);
    m_parts.push_back(m_tail);
    m_parts.push_back(m_finRight);
    m_parts.push_back(m_finLeft);
    m_parts.push_back(m_finTop);
}

void TropicalFishAModel::setupParts(f32 scale) {
    // 参考 MC 1.16.5 TropicalFishAModel
    // body: textureOffset(0, 0), addBox(-1, -1.5, -3, 2, 3, 6, scale), rotationPoint(0, 22, 0)
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 0);
    m_body->addBox(-1.0f, -1.5f, -3.0f, 2.0f, 3.0f, 6.0f, scale);
    m_body->setRotationPoint(0.0f, 22.0f, 0.0f);

    // tail: textureOffset(22, -6), addBox(0, -1.5, 0, 0, 3, 6, scale), rotationPoint(0, 22, 3)
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(22, -6);
    m_tail->addBox(0.0f, -1.5f, 0.0f, 0.0f, 3.0f, 6.0f, scale);
    m_tail->setRotationPoint(0.0f, 22.0f, 3.0f);

    // finRight: textureOffset(2, 16), addBox(-2, -1, 0, 2, 2, 0, scale), rotationPoint(-1, 22.5, 0)
    // rotateAngleY = mc::math::PI_DOUBLE/4
    m_finRight = std::make_shared<ModelRenderer>("finRight");
    m_finRight->setTextureOffset(2, 16);
    m_finRight->addBox(-2.0f, -1.0f, 0.0f, 2.0f, 2.0f, 0.0f, scale);
    m_finRight->setRotationPoint(-1.0f, 22.5f, 0.0f);
    m_finRight->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE / 4.0));

    // finLeft: textureOffset(2, 12), addBox(0, -1, 0, 2, 2, 0, scale), rotationPoint(1, 22.5, 0)
    // rotateAngleY = -mc::math::PI_DOUBLE/4
    m_finLeft = std::make_shared<ModelRenderer>("finLeft");
    m_finLeft->setTextureOffset(2, 12);
    m_finLeft->addBox(0.0f, -1.0f, 0.0f, 2.0f, 2.0f, 0.0f, scale);
    m_finLeft->setRotationPoint(1.0f, 22.5f, 0.0f);
    m_finLeft->setRotateAngleY(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0));

    // finTop: textureOffset(10, -5), addBox(0, -3, 0, 0, 3, 6, scale), rotationPoint(0, 20.5, -3)
    m_finTop = std::make_shared<ModelRenderer>("finTop");
    m_finTop->setTextureOffset(10, -5);
    m_finTop->addBox(0.0f, -3.0f, 0.0f, 0.0f, 3.0f, 6.0f, scale);
    m_finTop->setRotationPoint(0.0f, 20.5f, -3.0f);
}

void TropicalFishAModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                    f64 ageInTicks, f64 netHeadYaw,
                                    f64 headPitch, f64 scale) {
    // float f = 1.0F; if (!entityIn.isInWater()) { f = 1.5F; }
    // this.tail.rotateAngleY = -f * 0.45F * MathHelper.sin(0.6F * ageInTicks);
    f32 f = m_isInWater ? 1.0f : 1.5f;
    m_tail->setRotateAngleY(-f * 0.45f * static_cast<f32>(std::sin(0.6 * ageInTicks)));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

// ==================== TropicalFishBModel ====================

TropicalFishBModel::TropicalFishBModel(f32 scale)
    : AbstractTropicalFishModel()
{
    setTextureSize(32, 32);
    setupParts(scale);
    // 添加部件到列表
    m_parts.push_back(m_body);
    m_parts.push_back(m_tail);
    m_parts.push_back(m_finRight);
    m_parts.push_back(m_finLeft);
    m_parts.push_back(m_finTop);
    m_parts.push_back(m_finBottom);
}

void TropicalFishBModel::setupParts(f32 scale) {
    // 参考 MC 1.16.5 TropicalFishBModel
    // body: textureOffset(0, 20), addBox(-1, -3, -3, 2, 6, 6, scale), rotationPoint(0, 19, 0)
    m_body = std::make_shared<ModelRenderer>("body");
    m_body->setTextureOffset(0, 20);
    m_body->addBox(-1.0f, -3.0f, -3.0f, 2.0f, 6.0f, 6.0f, scale);
    m_body->setRotationPoint(0.0f, 19.0f, 0.0f);

    // tail: textureOffset(21, 16), addBox(0, -3, 0, 0, 6, 5, scale), rotationPoint(0, 19, 3)
    m_tail = std::make_shared<ModelRenderer>("tail");
    m_tail->setTextureOffset(21, 16);
    m_tail->addBox(0.0f, -3.0f, 0.0f, 0.0f, 6.0f, 5.0f, scale);
    m_tail->setRotationPoint(0.0f, 19.0f, 3.0f);

    // finRight: textureOffset(2, 16), addBox(-2, 0, 0, 2, 2, 0, scale), rotationPoint(-1, 20, 0)
    // rotateAngleY = mc::math::PI_DOUBLE/4
    m_finRight = std::make_shared<ModelRenderer>("finRight");
    m_finRight->setTextureOffset(2, 16);
    m_finRight->addBox(-2.0f, 0.0f, 0.0f, 2.0f, 2.0f, 0.0f, scale);
    m_finRight->setRotationPoint(-1.0f, 20.0f, 0.0f);
    m_finRight->setRotateAngleY(static_cast<f32>(mc::math::PI_DOUBLE / 4.0));

    // finLeft: textureOffset(2, 12), addBox(0, 0, 0, 2, 2, 0, scale), rotationPoint(1, 20, 0)
    // rotateAngleY = -mc::math::PI_DOUBLE/4
    m_finLeft = std::make_shared<ModelRenderer>("finLeft");
    m_finLeft->setTextureOffset(2, 12);
    m_finLeft->addBox(0.0f, 0.0f, 0.0f, 2.0f, 2.0f, 0.0f, scale);
    m_finLeft->setRotationPoint(1.0f, 20.0f, 0.0f);
    m_finLeft->setRotateAngleY(static_cast<f32>(-mc::math::PI_DOUBLE / 4.0));

    // finTop: textureOffset(20, 11), addBox(0, -4, 0, 0, 4, 6, scale), rotationPoint(0, 16, -3)
    m_finTop = std::make_shared<ModelRenderer>("finTop");
    m_finTop->setTextureOffset(20, 11);
    m_finTop->addBox(0.0f, -4.0f, 0.0f, 0.0f, 4.0f, 6.0f, scale);
    m_finTop->setRotationPoint(0.0f, 16.0f, -3.0f);

    // finBottom: textureOffset(20, 21), addBox(0, 0, 0, 0, 4, 6, scale), rotationPoint(0, 22, -3)
    m_finBottom = std::make_shared<ModelRenderer>("finBottom");
    m_finBottom->setTextureOffset(20, 21);
    m_finBottom->addBox(0.0f, 0.0f, 0.0f, 0.0f, 4.0f, 6.0f, scale);
    m_finBottom->setRotationPoint(0.0f, 22.0f, -3.0f);
}

void TropicalFishBModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                                    f64 ageInTicks, f64 netHeadYaw,
                                    f64 headPitch, f64 scale) {
    // float f = 1.0F; if (!entityIn.isInWater()) { f = 1.5F; }
    // this.tail.rotateAngleY = -f * 0.45F * MathHelper.sin(0.6F * ageInTicks);
    f32 f = m_isInWater ? 1.0f : 1.5f;
    m_tail->setRotateAngleY(-f * 0.45f * static_cast<f32>(std::sin(0.6 * ageInTicks)));

    (void)limbSwing;
    (void)limbSwingAmount;
    (void)netHeadYaw;
    (void)headPitch;
    (void)scale;
}

} // namespace mc::client::renderer::entity::model::aquatic
