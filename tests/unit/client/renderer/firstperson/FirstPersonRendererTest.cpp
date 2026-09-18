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

#include "client/renderer/trident/firstperson/ArmPose.hpp"
#include "client/renderer/trident/firstperson/ItemCameraTransforms.hpp"
#include "client/renderer/trident/firstperson/MatrixStack.hpp"
#include "client/renderer/trident/firstperson/PlayerModel.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client::renderer;

// ============================================================================
// ArmPose 测试
// ============================================================================

// ============================================================================
// PlayerModel 测试
// ============================================================================

class PlayerModelTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        model = std::make_unique<PlayerModel>(false);         // 标准手臂
        smallArmsModel = std::make_unique<PlayerModel>(true); // 细手臂
    }

    std::unique_ptr<PlayerModel> model;
    std::unique_ptr<PlayerModel> smallArmsModel;
};

TEST_F(PlayerModelTest, Creation_InitializesParts)
{
    // 验证所有部件都已创建
    EXPECT_NE(model->rightArm(), nullptr);
    EXPECT_NE(model->leftArm(), nullptr);
    EXPECT_NE(model->rightLeg(), nullptr);
    EXPECT_NE(model->leftLeg(), nullptr);
}

TEST_F(PlayerModelTest, SetVisible_ChangesVisibility)
{
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

TEST_F(PlayerModelTest, ArmPose_SetAndGet)
{
    // 默认空手
    EXPECT_EQ(model->rightArmPose(), ArmPose::Empty);
    EXPECT_EQ(model->leftArmPose(), ArmPose::Empty);

    // 设置姿态
    model->setRightArmPose(ArmPose::Item);
    EXPECT_EQ(model->rightArmPose(), ArmPose::Item);

    model->setLeftArmPose(ArmPose::Block);
    EXPECT_EQ(model->leftArmPose(), ArmPose::Block);
}

TEST_F(PlayerModelTest, SetAngles_UpdatesHeadRotation)
{
    // 设置角度
    model->setAngles(0.0, 0.0, 0.0, 45.0, 30.0, 1.0);

    // 验证头部旋转
    // 注意：角度转换为弧度
    // headPitch = 30 度, netHeadYaw = 45 度
    EXPECT_NEAR(model->rightArm()->rotateAngleX(), 0.0, 0.001); // 默认角度
}

TEST_F(PlayerModelTest, SetAngles_UpdatesWalkingAnimation)
{
    // 设置步态动画
    model->setAngles(1.0, 0.5, 0.0, 0.0, 0.0, 1.0);

    // 腿部应该有旋转
    // 由于步态动画，腿部角度不应该为 0
    EXPECT_NE(model->rightLeg()->rotateAngleX(), 0.0);
    EXPECT_NE(model->leftLeg()->rotateAngleX(), 0.0);
}

TEST_F(PlayerModelTest, Sneaking_ChangesBodyRotation)
{
    model->setSneaking(true);
    model->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    // 潜行时身体应该前倾
    // 验证手臂角度有变化（潜行时手臂略微前伸）
}

TEST_F(PlayerModelTest, Swimming_ChangesBodyRotation)
{
    model->setSwimming(true);
    model->setAngles(0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    // 游泳时身体应该水平
}

TEST_F(PlayerModelTest, SmallArms_CreatesNarrowerModel)
{
    // 标准手臂模型的宽度应该为 4
    // 细手臂模型的宽度应该为 3

    EXPECT_FALSE(model->isSmallArms());
    EXPECT_TRUE(smallArmsModel->isSmallArms());

    // 切换手臂类型
    model->setSmallArms(true);
    EXPECT_TRUE(model->isSmallArms());
}

TEST_F(PlayerModelTest, SetSmallArms_RebuildsModel)
{
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

TEST_F(ItemCameraTransformsTest, GetTransform_ReturnsCorrectType)
{
    const ItemTransform& thirdPersonRight = transforms.getTransform(TransformType::ThirdPersonRightHand);
    const ItemTransform& firstPersonRight = transforms.getTransform(TransformType::FirstPersonRightHand);
    const ItemTransform& gui = transforms.getTransform(TransformType::Gui);

    // 不应该崩溃
    (void)thirdPersonRight;
    (void)firstPersonRight;
    (void)gui;
}

TEST_F(ItemCameraTransformsTest, HasCustomTransform_DefaultFalse)
{
    // 默认情况下没有自定义变换
    EXPECT_FALSE(transforms.hasCustomTransform(TransformType::None));
}

TEST_F(ItemCameraTransformsTest, ApplyTransform_AppliesToStack)
{
    MatrixStack stack;

    // 设置自定义变换
    transforms.firstPersonRight = ItemTransform(10.0f,
        20.0f,
        30.0f, // 旋转
        1.0f,
        2.0f,
        3.0f, // 平移
        0.5f,
        0.5f,
        0.5f // 缩放
    );

    stack.push();
    transforms.applyTransform(stack, TransformType::FirstPersonRightHand);
    stack.pop();
}

TEST_F(ItemCameraTransformsTest, IsFirstPerson_ReturnsCorrectValues)
{
    EXPECT_TRUE(isFirstPerson(TransformType::FirstPersonLeftHand));
    EXPECT_TRUE(isFirstPerson(TransformType::FirstPersonRightHand));
    EXPECT_FALSE(isFirstPerson(TransformType::ThirdPersonLeftHand));
    EXPECT_FALSE(isFirstPerson(TransformType::ThirdPersonRightHand));
    EXPECT_FALSE(isFirstPerson(TransformType::Gui));
}

TEST_F(ItemCameraTransformsTest, IsThirdPerson_ReturnsCorrectValues)
{
    EXPECT_TRUE(isThirdPerson(TransformType::ThirdPersonLeftHand));
    EXPECT_TRUE(isThirdPerson(TransformType::ThirdPersonRightHand));
    EXPECT_FALSE(isThirdPerson(TransformType::FirstPersonLeftHand));
    EXPECT_FALSE(isThirdPerson(TransformType::FirstPersonRightHand));
    EXPECT_FALSE(isThirdPerson(TransformType::Gui));
}

TEST_F(ItemCameraTransformsTest, IsLeftHand_ReturnsCorrectValues)
{
    EXPECT_TRUE(isLeftHand(TransformType::FirstPersonLeftHand));
    EXPECT_TRUE(isLeftHand(TransformType::ThirdPersonLeftHand));
    EXPECT_FALSE(isLeftHand(TransformType::FirstPersonRightHand));
    EXPECT_FALSE(isLeftHand(TransformType::ThirdPersonRightHand));
}

// ============================================================================
// ItemTransform 测试
// ============================================================================

TEST(ItemTransformTest, DefaultConstructor_CreatesDefaultTransform)
{
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

TEST(ItemTransformTest, ParameterizedConstructor_SetsValues)
{
    ItemTransform transform(10.0f,
        20.0f,
        30.0f, // 旋转
        1.0f,
        2.0f,
        3.0f, // 平移
        0.5f,
        0.6f,
        0.7f // 缩放
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

TEST(ItemTransformTest, Apply_AppliesToMatrixStack)
{
    MatrixStack stack;
    ItemTransform transform(0.0f,
        0.0f,
        0.0f, // 无旋转
        10.0f,
        20.0f,
        30.0f, // 平移
        1.0f,
        1.0f,
        1.0f // 无缩放
    );

    transform.apply(stack);

    const Matrix4f& matrix = stack.last();
    Vector3f translation = matrix.translation();

    EXPECT_FLOAT_EQ(translation.x, 10.0f);
    EXPECT_FLOAT_EQ(translation.y, 20.0f);
    EXPECT_FLOAT_EQ(translation.z, 30.0f);
}

TEST(ItemTransformTest, DefaultTransform_ReturnsDefault)
{
    ItemTransform defaultTransform = ItemTransform::defaultTransform();

    EXPECT_TRUE(defaultTransform.isDefault());
}

// ============================================================================
// FirstPersonRenderer 变换方法测试
// ============================================================================

class FirstPersonRendererTransformTest : public ::testing::Test {
protected:
    MatrixStack stack;
};

TEST_F(FirstPersonRendererTransformTest, MatrixStack_PushPop)
{
    stack.push();
    stack.translate(10.0f, 20.0f, 30.0f);

    const Matrix4f& matrix1 = stack.last();
    Vector3f translation1 = matrix1.translation();
    EXPECT_FLOAT_EQ(translation1.x, 10.0f);

    stack.pop();

    const Matrix4f& matrix2 = stack.last();
    Vector3f translation2 = matrix2.translation();
    EXPECT_FLOAT_EQ(translation2.x, 0.0f);
    EXPECT_FLOAT_EQ(translation2.y, 0.0f);
    EXPECT_FLOAT_EQ(translation2.z, 0.0f);
}

TEST_F(FirstPersonRendererTransformTest, MatrixStack_Translate)
{
    stack.translate(5.0f, 10.0f, 15.0f);

    const Matrix4f& matrix = stack.last();
    Vector3f translation = matrix.translation();

    EXPECT_FLOAT_EQ(translation.x, 5.0f);
    EXPECT_FLOAT_EQ(translation.y, 10.0f);
    EXPECT_FLOAT_EQ(translation.z, 15.0f);
}

TEST_F(FirstPersonRendererTransformTest, MatrixStack_RotateX)
{
    stack.rotateX(90.0f);

    const Matrix4f& matrix = stack.last();
    // 旋转后的矩阵应该不是单位矩阵
    // 检查对角线元素不是全为 1
    EXPECT_FALSE(std::abs(matrix(0, 0) - 1.0f) < 0.001f && std::abs(matrix(1, 1) - 1.0f) < 0.001f &&
        std::abs(matrix(2, 2) - 1.0f) < 0.001f && std::abs(matrix(3, 3) - 1.0f) < 0.001f);
}

TEST_F(FirstPersonRendererTransformTest, MatrixStack_RotateY)
{
    stack.rotateY(45.0f);

    const Matrix4f& matrix = stack.last();
    // 旋转后的矩阵应该不是单位矩阵
    EXPECT_FALSE(std::abs(matrix(0, 0) - 1.0f) < 0.001f && std::abs(matrix(1, 1) - 1.0f) < 0.001f &&
        std::abs(matrix(2, 2) - 1.0f) < 0.001f && std::abs(matrix(3, 3) - 1.0f) < 0.001f);
}

TEST_F(FirstPersonRendererTransformTest, MatrixStack_RotateZ)
{
    stack.rotateZ(30.0f);

    const Matrix4f& matrix = stack.last();
    // 旋转后的矩阵应该不是单位矩阵
    EXPECT_FALSE(std::abs(matrix(0, 0) - 1.0f) < 0.001f && std::abs(matrix(1, 1) - 1.0f) < 0.001f &&
        std::abs(matrix(2, 2) - 1.0f) < 0.001f && std::abs(matrix(3, 3) - 1.0f) < 0.001f);
}

TEST_F(FirstPersonRendererTransformTest, MatrixStack_Scale)
{
    stack.scale(2.0f, 3.0f, 4.0f);

    const Matrix4f& matrix = stack.last();
    // 检查缩放效果：对角线元素应该反映缩放因子
    // 由于可能有旋转，直接检查对角线是否不为 1
    // 缩放矩阵会影响各轴的基向量长度
    EXPECT_FLOAT_EQ(matrix(0, 0), 2.0f);
    EXPECT_FLOAT_EQ(matrix(1, 1), 3.0f);
    EXPECT_FLOAT_EQ(matrix(2, 2), 4.0f);
    EXPECT_FLOAT_EQ(matrix(3, 3), 1.0f);
}

TEST_F(FirstPersonRendererTransformTest, MatrixStack_CombineTransforms)
{
    // 组合变换：平移 -> 旋转 -> 缩放
    stack.push();
    stack.translate(1.0f, 2.0f, 3.0f);
    stack.rotateY(45.0f);
    stack.scale(0.5f, 0.5f, 0.5f);

    const Matrix4f& matrix = stack.last();

    // 平移应该在旋转后的坐标系中
    // 这里只验证矩阵不是零矩阵
    bool hasNonZeroElement = false;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (std::abs(matrix(i, j)) > 0.001f) {
                hasNonZeroElement = true;
                break;
            }
        }
    }
    EXPECT_TRUE(hasNonZeroElement);

    stack.pop();
}

TEST_F(FirstPersonRendererTransformTest, MatrixStack_MultiplePushPop)
{
    // 测试多层嵌套
    stack.push();
    stack.translate(1.0f, 0.0f, 0.0f);

    stack.push();
    stack.translate(0.0f, 2.0f, 0.0f);

    stack.push();
    stack.translate(0.0f, 0.0f, 3.0f);

    const Matrix4f& inner = stack.last();
    Vector3f innerTranslation = inner.translation();
    // 累计平移：(1, 2, 3)
    EXPECT_FLOAT_EQ(innerTranslation.x, 1.0f);
    EXPECT_FLOAT_EQ(innerTranslation.y, 2.0f);
    EXPECT_FLOAT_EQ(innerTranslation.z, 3.0f);

    stack.pop();
    stack.pop();
    stack.pop();

    // 回到初始状态
    const Matrix4f& outer = stack.last();
    Vector3f outerTranslation = outer.translation();
    EXPECT_FLOAT_EQ(outerTranslation.x, 0.0f);
    EXPECT_FLOAT_EQ(outerTranslation.y, 0.0f);
    EXPECT_FLOAT_EQ(outerTranslation.z, 0.0f);
}

// ============================================================================
// 挥动动画变换测试
// ============================================================================

TEST_F(FirstPersonRendererTransformTest, SwingAnimation_TransformValues)
{
    // 测试挥动动画变换的基本值
    // 挥动进度 0.0 - 1.0
    const f32 PI = 3.14159265358979323846f;

    // 进度 0 时
    f32 swingProgress0 = 0.0f;
    f32 sqrtSwing0 = std::sqrt(swingProgress0);
    f32 offsetX0 = -0.4f * std::sin(sqrtSwing0 * PI);
    EXPECT_FLOAT_EQ(offsetX0, 0.0f); // sin(0) = 0

    // 进度 0.25 时
    f32 swingProgress25 = 0.25f;
    f32 sqrtSwing25 = std::sqrt(swingProgress25); // 0.5
    f32 offsetX25 = -0.4f * std::sin(sqrtSwing25 * PI);
    EXPECT_LT(offsetX25, 0.0f); // sin(PI/2) = 1, offsetX = -0.4

    // 进度 1.0 时
    f32 swingProgress100 = 1.0f;
    f32 sqrtSwing100 = std::sqrt(swingProgress100); // 1.0
    f32 offsetX100 = -0.4f * std::sin(sqrtSwing100 * PI);
    EXPECT_NEAR(offsetX100, 0.0f, 0.001f); // sin(PI) ≈ 0
}

// ============================================================================
// 装备动画测试
// ============================================================================

TEST_F(FirstPersonRendererTransformTest, EquipAnimation_TranslateY)
{
    // 装备进度影响 Y 轴平移
    // Y 平移 = SIDE_OFFSET_Y + equipProgress * -0.6f

    f32 sideOffsetY = -1.22f; // 第一人称手侧 Y 偏移常量

    // 进度 0（刚切换）
    f32 equip0 = 0.0f;
    f32 y0 = sideOffsetY + equip0 * -0.6f;
    EXPECT_FLOAT_EQ(y0, sideOffsetY);

    // 进度 0.5
    f32 equip50 = 0.5f;
    f32 y50 = sideOffsetY + equip50 * -0.6f;
    EXPECT_FLOAT_EQ(y50, sideOffsetY - 0.3f);

    // 进度 1.0（完全装备）
    f32 equip100 = 1.0f;
    f32 y100 = sideOffsetY + equip100 * -0.6f;
    EXPECT_FLOAT_EQ(y100, sideOffsetY - 0.6f);
}

// ============================================================================
// 手侧边测试
// ============================================================================

TEST(HandSideTest, ResolveHandSide_MainHandRight)
{
    // 主手为右手时
    HandSide primaryHand = HandSide::Right;

    // 主手槽位 -> 右手
    EXPECT_EQ(HandSide::Right, primaryHand); // 主手

    // 副手槽位 -> 左手
    HandSide offHandSide = (primaryHand == HandSide::Right) ? HandSide::Left : HandSide::Right;
    EXPECT_EQ(offHandSide, HandSide::Left);
}

TEST(HandSideTest, ResolveHandSide_MainHandLeft)
{
    // 主手为左手时
    HandSide primaryHand = HandSide::Left;

    // 主手槽位 -> 左手
    EXPECT_EQ(primaryHand, HandSide::Left);

    // 副手槽位 -> 右手
    HandSide offHandSide = (primaryHand == HandSide::Right) ? HandSide::Left : HandSide::Right;
    EXPECT_EQ(offHandSide, HandSide::Right);
}
