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

#include "client/renderer/trident/firstperson/MatrixStack.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc::client::renderer;
using namespace mc;

class MatrixStackTest : public ::testing::Test {
protected:
    void SetUp() override { stack = std::make_unique<MatrixStack>(); }

    std::unique_ptr<MatrixStack> stack;
};

// ============================================================================
// 基础操作测试
// ============================================================================

TEST_F(MatrixStackTest, InitialState_IsIdentity)
{
    const Matrix4f& matrix = stack->last();

    // 检查是否为单位矩阵
    EXPECT_FLOAT_EQ(matrix(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(matrix(1, 1), 1.0f);
    EXPECT_FLOAT_EQ(matrix(2, 2), 1.0f);
    EXPECT_FLOAT_EQ(matrix(3, 3), 1.0f);

    // 检查非对角线元素为0
    EXPECT_FLOAT_EQ(matrix(0, 1), 0.0f);
    EXPECT_FLOAT_EQ(matrix(0, 2), 0.0f);
    EXPECT_FLOAT_EQ(matrix(0, 3), 0.0f);
    EXPECT_FLOAT_EQ(matrix(1, 0), 0.0f);
    EXPECT_FLOAT_EQ(matrix(1, 2), 0.0f);
    EXPECT_FLOAT_EQ(matrix(1, 3), 0.0f);
}

TEST_F(MatrixStackTest, PushPop_MaintainsState)
{
    // Push 后应该是同一矩阵
    stack->push();
    EXPECT_EQ(stack->depth(), 2u);

    // 修改栈顶矩阵
    stack->translate(1.0f, 2.0f, 3.0f);

    // Pop 后应该恢复原矩阵
    stack->pop();
    EXPECT_EQ(stack->depth(), 1u);

    const Matrix4f& matrix = stack->last();
    EXPECT_FLOAT_EQ(matrix(0, 3), 0.0f);
    EXPECT_FLOAT_EQ(matrix(1, 3), 0.0f);
    EXPECT_FLOAT_EQ(matrix(2, 3), 0.0f);
}

TEST_F(MatrixStackTest, DepthIncreases_WithPush)
{
    EXPECT_EQ(stack->depth(), 1u);

    stack->push();
    EXPECT_EQ(stack->depth(), 2u);

    stack->push();
    EXPECT_EQ(stack->depth(), 3u);

    stack->pop();
    EXPECT_EQ(stack->depth(), 2u);
}

TEST_F(MatrixStackTest, Clear_ResetsStack)
{
    stack->push();
    stack->push();
    stack->push();
    EXPECT_EQ(stack->depth(), 4u);

    stack->clear();

    EXPECT_EQ(stack->depth(), 1u);
    const Matrix4f& matrix = stack->last();
    EXPECT_FLOAT_EQ(matrix(0, 0), 1.0f);
}

// ============================================================================
// 变换测试
// ============================================================================

TEST_F(MatrixStackTest, Translate_AffectsTranslation)
{
    stack->translate(10.0f, 20.0f, 30.0f);

    const Matrix4f& matrix = stack->last();
    Vector3f translation = matrix.translation();

    EXPECT_FLOAT_EQ(translation.x, 10.0f);
    EXPECT_FLOAT_EQ(translation.y, 20.0f);
    EXPECT_FLOAT_EQ(translation.z, 30.0f);
}

TEST_F(MatrixStackTest, Translate_Accumulates)
{
    stack->translate(1.0f, 0.0f, 0.0f);
    stack->translate(0.0f, 2.0f, 0.0f);
    stack->translate(0.0f, 0.0f, 3.0f);

    const Matrix4f& matrix = stack->last();
    Vector3f translation = matrix.translation();

    EXPECT_FLOAT_EQ(translation.x, 1.0f);
    EXPECT_FLOAT_EQ(translation.y, 2.0f);
    EXPECT_FLOAT_EQ(translation.z, 3.0f);
}

TEST_F(MatrixStackTest, Scale_AffectsScale)
{
    stack->scale(2.0f, 3.0f, 4.0f);

    const Matrix4f& matrix = stack->last();

    // 检查对角线上的缩放值
    EXPECT_FLOAT_EQ(matrix(0, 0), 2.0f);
    EXPECT_FLOAT_EQ(matrix(1, 1), 3.0f);
    EXPECT_FLOAT_EQ(matrix(2, 2), 4.0f);
}

TEST_F(MatrixStackTest, UniformScale)
{
    stack->scale(2.0f);

    const Matrix4f& matrix = stack->last();

    EXPECT_FLOAT_EQ(matrix(0, 0), 2.0f);
    EXPECT_FLOAT_EQ(matrix(1, 1), 2.0f);
    EXPECT_FLOAT_EQ(matrix(2, 2), 2.0f);
}

// ============================================================================
// 旋转测试
// ============================================================================

TEST_F(MatrixStackTest, RotateX_90Degrees)
{
    stack->rotateX(90.0f);

    const Matrix4f& matrix = stack->last();

    // 绕 X 轴旋转 90 度后：
    // Y 轴应该变成 Z 轴
    // Z 轴应该变成 -Y 轴
    EXPECT_NEAR(matrix(1, 1), 0.0f, 0.0001f);
    EXPECT_NEAR(matrix(1, 2), -1.0f, 0.0001f);
    EXPECT_NEAR(matrix(2, 1), 1.0f, 0.0001f);
    EXPECT_NEAR(matrix(2, 2), 0.0f, 0.0001f);
}

TEST_F(MatrixStackTest, RotateY_90Degrees)
{
    stack->rotateY(90.0f);

    const Matrix4f& matrix = stack->last();

    // 绕 Y 轴旋转 90 度后：
    // X 轴应该变成 -Z 轴
    // Z 轴应该变成 X 轴
    EXPECT_NEAR(matrix(0, 0), 0.0f, 0.0001f);
    EXPECT_NEAR(matrix(0, 2), 1.0f, 0.0001f);
    EXPECT_NEAR(matrix(2, 0), -1.0f, 0.0001f);
    EXPECT_NEAR(matrix(2, 2), 0.0f, 0.0001f);
}

TEST_F(MatrixStackTest, RotateZ_90Degrees)
{
    stack->rotateZ(90.0f);

    const Matrix4f& matrix = stack->last();

    // 绕 Z 轴旋转 90 度后：
    // X 轴应该变成 Y 轴
    // Y 轴应该变成 -X 轴
    EXPECT_NEAR(matrix(0, 0), 0.0f, 0.0001f);
    EXPECT_NEAR(matrix(0, 1), -1.0f, 0.0001f);
    EXPECT_NEAR(matrix(1, 0), 1.0f, 0.0001f);
    EXPECT_NEAR(matrix(1, 1), 0.0f, 0.0001f);
}

TEST_F(MatrixStackTest, Rotate_FullRotation)
{
    // 绕 X 轴旋转 360 度应该回到原点
    stack->rotateX(360.0f);

    const Matrix4f& matrix = stack->last();

    EXPECT_NEAR(matrix(0, 0), 1.0f, 0.0001f);
    EXPECT_NEAR(matrix(1, 1), 1.0f, 0.0001f);
    EXPECT_NEAR(matrix(2, 2), 1.0f, 0.0001f);
}

// ============================================================================
// 组合变换测试
// ============================================================================

TEST_F(MatrixStackTest, Combined_TranslateRotate)
{
    // 先平移后旋转
    stack->translate(1.0f, 0.0f, 0.0f);
    stack->rotateZ(90.0f);

    const Matrix4f& matrix = stack->last();
    Vector3f translation = matrix.translation();

    // PoseStack 右乘语义：current = current * R。
    // 先 translate 再 rotate 时，平移分量保持不变（等价于先旋转模型再平移）。
    EXPECT_NEAR(translation.x, 1.0f, 0.0001f);
    EXPECT_NEAR(translation.y, 0.0f, 0.0001f);
    EXPECT_NEAR(translation.z, 0.0f, 0.0001f);
}

TEST_F(MatrixStackTest, Combined_ScaleTranslate)
{
    stack->scale(2.0f, 2.0f, 2.0f);
    stack->translate(1.0f, 1.0f, 1.0f);

    const Matrix4f& matrix = stack->last();
    Vector3f translation = matrix.translation();

    // 右乘语义下，先缩放再平移，平移会受当前基向量缩放影响。
    EXPECT_FLOAT_EQ(translation.x, 2.0f);
    EXPECT_FLOAT_EQ(translation.y, 2.0f);
    EXPECT_FLOAT_EQ(translation.z, 2.0f);
}

// ============================================================================
// 嵌套变换测试
// ============================================================================

TEST_F(MatrixStackTest, NestedTransforms)
{
    // 外层变换
    stack->translate(10.0f, 0.0f, 0.0f);

    stack->push();
    {
        // 内层变换
        stack->translate(5.0f, 5.0f, 0.0f);
        stack->rotateZ(45.0f);

        const Matrix4f& inner = stack->last();
        // 内层应该包含两层变换
        // 这里只验证平移部分
        Vector3f innerTranslation = inner.translation();
        // 注意：平移是右乘，所以旋转会影响平移方向
    }
    stack->pop();

    // 外层应该只有外层变换
    const Matrix4f& outer = stack->last();
    Vector3f outerTranslation = outer.translation();
    EXPECT_FLOAT_EQ(outerTranslation.x, 10.0f);
}

// ============================================================================
// Matrix4f 测试
// ============================================================================

TEST(Matrix4fTest, Identity_IsCorrect)
{
    Matrix4f identity = Matrix4f::identity();

    EXPECT_FLOAT_EQ(identity(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(identity(1, 1), 1.0f);
    EXPECT_FLOAT_EQ(identity(2, 2), 1.0f);
    EXPECT_FLOAT_EQ(identity(3, 3), 1.0f);

    // 非对角线为 0
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            if (row != col) {
                EXPECT_FLOAT_EQ(identity(row, col), 0.0f);
            }
        }
    }
}

TEST(Matrix4fTest, Multiplication_Identity)
{
    Matrix4f identity = Matrix4f::identity();
    Matrix4f result = identity * identity;

    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            if (row == col) {
                EXPECT_FLOAT_EQ(result(row, col), 1.0f);
            } else {
                EXPECT_FLOAT_EQ(result(row, col), 0.0f);
            }
        }
    }
}

TEST(Matrix4fTest, SetTranslation_Works)
{
    Matrix4f matrix = Matrix4f::identity();
    matrix.setTranslation(5.0f, 10.0f, 15.0f);

    Vector3f translation = matrix.translation();
    EXPECT_FLOAT_EQ(translation.x, 5.0f);
    EXPECT_FLOAT_EQ(translation.y, 10.0f);
    EXPECT_FLOAT_EQ(translation.z, 15.0f);
}
