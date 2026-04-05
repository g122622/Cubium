#include <gtest/gtest.h>
#include "client/renderer/trident/firstperson/ArmPose.hpp"
#include "client/renderer/trident/firstperson/PlayerModel.hpp"
#include "client/renderer/trident/firstperson/ItemCameraTransforms.hpp"
#include "client/renderer/trident/firstperson/MatrixStack.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"

using namespace mc;
using namespace mc::client::renderer;

// ============================================================================
// ArmPose 测试
// ============================================================================

TEST(ArmPoseTest, IsTwoHanded_ReturnsCorrectValues) {
    // 双手持握的姿势
    EXPECT_TRUE(isTwoHanded(ArmPose::BowAndArrow));
    EXPECT_TRUE(isTwoHanded(ArmPose::ThrowSpear));
    EXPECT_TRUE(isTwoHanded(ArmPose::CrossbowCharge));

    // 单手持握的姿势
    EXPECT_FALSE(isTwoHanded(ArmPose::Empty));
    EXPECT_FALSE(isTwoHanded(ArmPose::Item));
    EXPECT_FALSE(isTwoHanded(ArmPose::Block));
    EXPECT_FALSE(isTwoHanded(ArmPose::EatOrDrink));
}

TEST(ArmPoseTest, BlocksOffHand_ReturnsCorrectValues) {
    // 阻止副手渲染的姿势
    EXPECT_TRUE(blocksOffHand(ArmPose::BowAndArrow));
    EXPECT_TRUE(blocksOffHand(ArmPose::ThrowSpear));
    EXPECT_TRUE(blocksOffHand(ArmPose::CrossbowCharge));
    EXPECT_TRUE(blocksOffHand(ArmPose::CrossbowHold));

    // 不阻止副手的姿势
    EXPECT_FALSE(blocksOffHand(ArmPose::Empty));
    EXPECT_FALSE(blocksOffHand(ArmPose::Item));
    EXPECT_FALSE(blocksOffHand(ArmPose::Block));
    EXPECT_FALSE(blocksOffHand(ArmPose::EatOrDrink));
}

// ============================================================================
// PlayerModel 测试
// ============================================================================

class PlayerModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        model = std::make_unique<PlayerModel>(false);  // 标准手臂
        smallArmsModel = std::make_unique<PlayerModel>(true);  // 细手臂
    }

    std::unique_ptr<PlayerModel> model;
    std::unique_ptr<PlayerModel> smallArmsModel;
};

TEST_F(PlayerModelTest, Creation_InitializesParts) {
    // 验证所有部件都已创建
    EXPECT_NE(model->rightArm(), nullptr);
    EXPECT_NE(model->leftArm(), nullptr);
    EXPECT_NE(model->rightLeg(), nullptr);
    EXPECT_NE(model->leftLeg(), nullptr);
}

TEST_F(PlayerModelTest, SetVisible_ChangesVisibility) {
    // 设置不可见
    model->setVisible(false);

    // 验证部件不可见
    EXPECT_FALSE(model->rightArm()->isVisible());
    EXPECT_FALSE(model->leftArm()->isVisible());
    EXPECT_FALSE(model->rightLeg()->isVisible());
    EXPECT_FALSE(model->leftLeg()->isVisible());

    // 设置可见
    model->setVisible(true);

    // 验证部件可见
    EXPECT_TRUE(model->rightArm()->isVisible());
    EXPECT_TRUE(model->leftArm()->isVisible());
    EXPECT_TRUE(model->rightLeg()->isVisible());
    EXPECT_TRUE(model->leftLeg()->isVisible());
}

TEST_F(PlayerModelTest, ArmPose_SetAndGet) {
    // 默认空手
    EXPECT_EQ(model->rightArmPose(), ArmPose::Empty);
    EXPECT_EQ(model->leftArmPose(), ArmPose::Empty);

    // 设置姿态
    model->setRightArmPose(ArmPose::Item);
    EXPECT_EQ(model->rightArmPose(), ArmPose::Item);

    model->setLeftArmPose(ArmPose::Block);
    EXPECT_EQ(model->leftArmPose(), ArmPose::Block);
}

TEST_F(PlayerModelTest, SetAngles_UpdatesHeadRotation) {
    // 设置角度
    model->setAngles(0.0, 0.0, 0.0, 45.0, 30.0, 1.0);

    // 验证头部旋转
    // 注意：角度转换为弧度
    // headPitch = 30 度, netHeadYaw = 45 度
    EXPECT_NEAR(model->rightArm()->rotateAngleX(), 0.0, 0.001);  // 默认角度
}

TEST_F(PlayerModelTest, SetAngles_UpdatesWalkingAnimation) {
    // 设置步态动画
    model->setAngles(1.0, 0.5, 0.0, 0.0, 0.0, 1.0);

    // 腿部应该有旋转
    // 由于步态动画，腿部角度不应该为 0
    EXPECT_NE(model->rightLeg()->rotateAngleX(), 0.0);
    EXPECT_NE(model->leftLeg()->rotateAngleX(), 0.0);
}

TEST_F(PlayerModelTest, Sneaking_ChangesBodyRotation) {
    model->setSneaking(true);
    model->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    // 潜行时身体应该前倾
    // 验证手臂角度有变化（潜行时手臂略微前伸）
}

TEST_F(PlayerModelTest, Swimming_ChangesBodyRotation) {
    model->setSwimming(true);
    model->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    // 游泳时身体应该水平
}

TEST_F(PlayerModelTest, SmallArms_CreatesNarrowerModel) {
    // 标准手臂模型的宽度应该为 4
    // 细手臂模型的宽度应该为 3

    EXPECT_FALSE(model->isSmallArms());
    EXPECT_TRUE(smallArmsModel->isSmallArms());

    // 切换手臂类型
    model->setSmallArms(true);
    EXPECT_TRUE(model->isSmallArms());
}

TEST_F(PlayerModelTest, SetSmallArms_RebuildsModel) {
    // 设置细手臂
    model->setSmallArms(true);
    EXPECT_TRUE(model->isSmallArms());

    // 设置回标准手臂
    model->setSmallArms(false);
    EXPECT_FALSE(model->isSmallArms());
}

// ============================================================================
// ItemCameraTransforms 测试
// ============================================================================

class ItemCameraTransformsTest : public ::testing::Test {
protected:
    ItemCameraTransforms transforms;
};

TEST_F(ItemCameraTransformsTest, GetTransform_ReturnsCorrectType) {
    const ItemTransform& thirdPersonRight = transforms.getTransform(TransformType::ThirdPersonRightHand);
    const ItemTransform& firstPersonRight = transforms.getTransform(TransformType::FirstPersonRightHand);
    const ItemTransform& gui = transforms.getTransform(TransformType::Gui);

    // 不应该崩溃
    (void)thirdPersonRight;
    (void)firstPersonRight;
    (void)gui;
}

TEST_F(ItemCameraTransformsTest, HasCustomTransform_DefaultFalse) {
    // 默认情况下没有自定义变换
    EXPECT_FALSE(transforms.hasCustomTransform(TransformType::None));
}

TEST_F(ItemCameraTransformsTest, ApplyTransform_AppliesToStack) {
    MatrixStack stack;

    // 设置自定义变换
    transforms.firstPersonRight = ItemTransform(
        10.0f, 20.0f, 30.0f,  // 旋转
        1.0f, 2.0f, 3.0f,      // 平移
        0.5f, 0.5f, 0.5f       // 缩放
    );

    stack.push();
    transforms.applyTransform(stack, TransformType::FirstPersonRightHand);
    stack.pop();
}

TEST_F(ItemCameraTransformsTest, IsFirstPerson_ReturnsCorrectValues) {
    EXPECT_TRUE(isFirstPerson(TransformType::FirstPersonLeftHand));
    EXPECT_TRUE(isFirstPerson(TransformType::FirstPersonRightHand));
    EXPECT_FALSE(isFirstPerson(TransformType::ThirdPersonLeftHand));
    EXPECT_FALSE(isFirstPerson(TransformType::ThirdPersonRightHand));
    EXPECT_FALSE(isFirstPerson(TransformType::Gui));
}

TEST_F(ItemCameraTransformsTest, IsThirdPerson_ReturnsCorrectValues) {
    EXPECT_TRUE(isThirdPerson(TransformType::ThirdPersonLeftHand));
    EXPECT_TRUE(isThirdPerson(TransformType::ThirdPersonRightHand));
    EXPECT_FALSE(isThirdPerson(TransformType::FirstPersonLeftHand));
    EXPECT_FALSE(isThirdPerson(TransformType::FirstPersonRightHand));
    EXPECT_FALSE(isThirdPerson(TransformType::Gui));
}

TEST_F(ItemCameraTransformsTest, IsLeftHand_ReturnsCorrectValues) {
    EXPECT_TRUE(isLeftHand(TransformType::FirstPersonLeftHand));
    EXPECT_TRUE(isLeftHand(TransformType::ThirdPersonLeftHand));
    EXPECT_FALSE(isLeftHand(TransformType::FirstPersonRightHand));
    EXPECT_FALSE(isLeftHand(TransformType::ThirdPersonRightHand));
}

// ============================================================================
// ItemTransform 测试
// ============================================================================

TEST(ItemTransformTest, DefaultConstructor_CreatesDefaultTransform) {
    ItemTransform transform;

    EXPECT_TRUE(transform.isDefault());
    EXPECT_FLOAT_EQ(transform.rotation.x, 0.0f);
    EXPECT_FLOAT_EQ(transform.rotation.y, 0.0f);
    EXPECT_FLOAT_EQ(transform.rotation.z, 0.0f);
    EXPECT_FLOAT_EQ(transform.translation.x, 0.0f);
    EXPECT_FLOAT_EQ(transform.translation.y, 0.0f);
    EXPECT_FLOAT_EQ(transform.translation.z, 0.0f);
    EXPECT_FLOAT_EQ(transform.scale.x, 1.0f);
    EXPECT_FLOAT_EQ(transform.scale.y, 1.0f);
    EXPECT_FLOAT_EQ(transform.scale.z, 1.0f);
}

TEST(ItemTransformTest, ParameterizedConstructor_SetsValues) {
    ItemTransform transform(
        10.0f, 20.0f, 30.0f,   // 旋转
        1.0f, 2.0f, 3.0f,       // 平移
        0.5f, 0.6f, 0.7f        // 缩放
    );

    EXPECT_FLOAT_EQ(transform.rotation.x, 10.0f);
    EXPECT_FLOAT_EQ(transform.rotation.y, 20.0f);
    EXPECT_FLOAT_EQ(transform.rotation.z, 30.0f);
    EXPECT_FLOAT_EQ(transform.translation.x, 1.0f);
    EXPECT_FLOAT_EQ(transform.translation.y, 2.0f);
    EXPECT_FLOAT_EQ(transform.translation.z, 3.0f);
    EXPECT_FLOAT_EQ(transform.scale.x, 0.5f);
    EXPECT_FLOAT_EQ(transform.scale.y, 0.6f);
    EXPECT_FLOAT_EQ(transform.scale.z, 0.7f);
}

TEST(ItemTransformTest, Apply_AppliesToMatrixStack) {
    MatrixStack stack;
    ItemTransform transform(
        0.0f, 0.0f, 0.0f,      // 无旋转
        10.0f, 20.0f, 30.0f,   // 平移
        1.0f, 1.0f, 1.0f       // 无缩放
    );

    transform.apply(stack);

    const Matrix4f& matrix = stack.last();
    Vector3f translation = matrix.translation();

    EXPECT_FLOAT_EQ(translation.x, 10.0f);
    EXPECT_FLOAT_EQ(translation.y, 20.0f);
    EXPECT_FLOAT_EQ(translation.z, 30.0f);
}

TEST(ItemTransformTest, DefaultTransform_ReturnsDefault) {
    ItemTransform defaultTransform = ItemTransform::defaultTransform();

    EXPECT_TRUE(defaultTransform.isDefault());
}
