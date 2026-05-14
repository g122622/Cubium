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

/**
 * @file BipedModelTranslateHandTest.cpp
 * @brief BipedModel::translateHand 单元测试
 *
 * 测试覆盖：
 * - translateHand 方法返回正确的变换矩阵
 * - 右手和左手返回独立的矩阵
 * - 手臂旋转角度影响矩阵
 */

#include <gtest/gtest.h>
#include <array>
#include <cmath>

#include "client/renderer/trident/entity/model/base/BipedModel.hpp"

namespace mc::client::renderer {
namespace {

using namespace mc::client::renderer::entity::model;

class BipedModelTranslateHandTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_model = std::make_unique<BipedModel>();
    }

    std::unique_ptr<BipedModel> m_model;
};

TEST_F(BipedModelTranslateHandTest, RightHandReturnsValidMatrix)
{
    std::array<f64, 16> matrix = {0};
    m_model->translateHand(HandSide::Right, matrix);

    // 验证矩阵格式正确（4x4 行主序）
    // 单位矩阵对角线元素
    EXPECT_DOUBLE_EQ(matrix[0], 1.0);
    EXPECT_DOUBLE_EQ(matrix[5], 1.0);
    EXPECT_DOUBLE_EQ(matrix[10], 1.0);
    EXPECT_DOUBLE_EQ(matrix[15], 1.0);
}

TEST_F(BipedModelTranslateHandTest, LeftHandReturnsValidMatrix)
{
    std::array<f64, 16> matrix = {0};
    m_model->translateHand(HandSide::Left, matrix);

    EXPECT_DOUBLE_EQ(matrix[0], 1.0);
    EXPECT_DOUBLE_EQ(matrix[5], 1.0);
    EXPECT_DOUBLE_EQ(matrix[10], 1.0);
    EXPECT_DOUBLE_EQ(matrix[15], 1.0);
}

TEST_F(BipedModelTranslateHandTest, RightArmRotationPointIsCorrect)
{
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);
    rightArm->setRotationPoint(-5.0, 2.0, 0.0);

    std::array<f64, 16> matrix = {0};
    m_model->translateHand(HandSide::Right, matrix);

    // 验证平移部分
    EXPECT_DOUBLE_EQ(matrix[3], -5.0);
    EXPECT_DOUBLE_EQ(matrix[7], 2.0);
    EXPECT_DOUBLE_EQ(matrix[11], 0.0);
}

TEST_F(BipedModelTranslateHandTest, LeftArmRotationPointIsCorrect)
{
    auto leftArm = m_model->getLeftArm();
    ASSERT_NE(leftArm, nullptr);
    leftArm->setRotationPoint(5.0, 2.0, 0.0);

    std::array<f64, 16> matrix = {0};
    m_model->translateHand(HandSide::Left, matrix);

    EXPECT_DOUBLE_EQ(matrix[3], 5.0);
    EXPECT_DOUBLE_EQ(matrix[7], 2.0);
    EXPECT_DOUBLE_EQ(matrix[11], 0.0);
}

TEST_F(BipedModelTranslateHandTest, RotationAngleAffectsMatrix)
{
    auto rightArm = m_model->getRightArm();
    ASSERT_NE(rightArm, nullptr);

    rightArm->setRotationPoint(-5.0, 2.0, 0.0);
    rightArm->setRotateAngleX(0.0);

    std::array<f64, 16> matrix1 = {0};
    m_model->translateHand(HandSide::Right, matrix1);

    rightArm->setRotateAngleX(0.5);

    std::array<f64, 16> matrix2 = {0};
    m_model->translateHand(HandSide::Right, matrix2);

    bool matricesDifferent = false;
    for (int i = 0; i < 16; ++i) {
        if (std::abs(matrix1[i] - matrix2[i]) > 1e-10) {
            matricesDifferent = true;
            break;
        }
    }
    EXPECT_TRUE(matricesDifferent) << "Arm rotation should affect the final matrix";
}

TEST_F(BipedModelTranslateHandTest, BothHandsReturnDifferentMatrices)
{
    m_model->getRightArm()->setRotationPoint(-5.0, 2.0, 0.0);
    m_model->getLeftArm()->setRotationPoint(5.0, 2.0, 0.0);

    std::array<f64, 16> rightMatrix = {0};
    std::array<f64, 16> leftMatrix = {0};

    m_model->translateHand(HandSide::Right, rightMatrix);
    m_model->translateHand(HandSide::Left, leftMatrix);

    EXPECT_NE(rightMatrix[3], leftMatrix[3]);
    EXPECT_DOUBLE_EQ(rightMatrix[3], -leftMatrix[3]);
}

TEST_F(BipedModelTranslateHandTest, MatrixElementsAreFinite)
{
    std::array<f64, 16> matrix = {0};
    m_model->translateHand(HandSide::Right, matrix);

    for (int i = 0; i < 16; ++i) {
        EXPECT_TRUE(std::isfinite(matrix[i])) << "Matrix element " << i << " is not finite";
    }
}

} // anonymous namespace
} // namespace mc::client::renderer
