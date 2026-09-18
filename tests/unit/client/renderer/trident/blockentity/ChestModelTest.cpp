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

#include "client/renderer/trident/blockentity/model/ChestModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/util/math/MathConstants.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc::client::renderer::blockentity::model;
using namespace mc;

// 导入 ModelVertex 类型
using ModelVertex = mc::client::renderer::entity::model::ModelVertex;

class ChestModelTest : public ::testing::Test {
protected:
    void SetUp() override { model_ = std::make_unique<ChestModel>(); }

    std::unique_ptr<ChestModel> model_;
};

// ========== 基础创建测试 ==========

TEST_F(ChestModelTest, Create_HasCorrectDefaultType)
{
    EXPECT_EQ(model_->getChestType(), ChestModel::ChestType::Single);
}

TEST_F(ChestModelTest, Create_HasAllParts)
{
    EXPECT_NE(model_->getBottom(), nullptr);
    EXPECT_NE(model_->getLid(), nullptr);
    EXPECT_NE(model_->getLatch(), nullptr);
}

// ========== 箱子类型切换测试 ==========

TEST_F(ChestModelTest, SetChestType_Single)
{
    model_->setChestType(ChestModel::ChestType::Single);
    EXPECT_EQ(model_->getChestType(), ChestModel::ChestType::Single);
}

TEST_F(ChestModelTest, SetChestType_Left)
{
    model_->setChestType(ChestModel::ChestType::Left);
    EXPECT_EQ(model_->getChestType(), ChestModel::ChestType::Left);
}

TEST_F(ChestModelTest, SetChestType_Right)
{
    model_->setChestType(ChestModel::ChestType::Right);
    EXPECT_EQ(model_->getChestType(), ChestModel::ChestType::Right);
}

// ========== 盖子角度测试 ==========

TEST_F(ChestModelTest, SetLidAngle_Zero)
{
    model_->setLidAngle(0.0f);
    // 验证部件旋转被设置（角度应为0）
    EXPECT_FLOAT_EQ(model_->getLid()->rotateAngleX(), 0.0f);
}

TEST_F(ChestModelTest, SetLidAngle_FullOpen)
{
    model_->setLidAngle(1.0f);
    // 缓动后角度 = 1.0 - (1.0 - 1.0)^3 = 1.0
    // 弧度 = 1.0 * PI/2 = PI/2
    EXPECT_FLOAT_EQ(model_->getLid()->rotateAngleX(), -math::PI / 2.0f);
}

TEST_F(ChestModelTest, SetLidAngle_HalfOpen)
{
    model_->setLidAngle(0.5f);
    // 缓动后角度 = 1.0 - (1.0 - 0.5)^3 = 1.0 - 0.125 = 0.875
    // 弧度 = 0.875 * PI/2
    f32 expected = -ChestModel::applyEasing(0.5f) * (math::PI / 2.0f);
    EXPECT_FLOAT_EQ(model_->getLid()->rotateAngleX(), expected);
}

TEST_F(ChestModelTest, SetLidAngle_LatchFollowsLid)
{
    model_->setLidAngle(0.75f);
    // 锁扣应与盖子旋转角度相同
    EXPECT_FLOAT_EQ(model_->getLid()->rotateAngleX(), model_->getLatch()->rotateAngleX());
}

// ========== 缓动函数测试 ==========

TEST_F(ChestModelTest, ApplyEasing_Zero)
{
    EXPECT_FLOAT_EQ(ChestModel::applyEasing(0.0f), 0.0f);
}

TEST_F(ChestModelTest, ApplyEasing_One)
{
    EXPECT_FLOAT_EQ(ChestModel::applyEasing(1.0f), 1.0f);
}

TEST_F(ChestModelTest, ApplyEasing_Half)
{
    // 0.5 -> 1.0 - 0.5 = 0.5 -> 0.5^3 = 0.125 -> 1.0 - 0.125 = 0.875
    EXPECT_FLOAT_EQ(ChestModel::applyEasing(0.5f), 0.875f);
}

TEST_F(ChestModelTest, ApplyEasing_Quarter)
{
    // 0.25 -> 1.0 - 0.25 = 0.75 -> 0.75^3 = 0.421875 -> 1.0 - 0.421875 = 0.578125
    EXPECT_FLOAT_EQ(ChestModel::applyEasing(0.25f), 0.578125f);
}

TEST_F(ChestModelTest, ApplyEasing_ThreeQuarters)
{
    // 0.75 -> 1.0 - 0.75 = 0.25 -> 0.25^3 = 0.015625 -> 1.0 - 0.015625 = 0.984375
    EXPECT_FLOAT_EQ(ChestModel::applyEasing(0.75f), 0.984375f);
}

// ========== 网格生成测试 ==========

TEST_F(ChestModelTest, GenerateMesh_ProducesVertices)
{
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    model_->generateMesh(vertices, indices);

    // 箱子应该产生顶点和索引
    EXPECT_GT(vertices.size(), 0u);
    EXPECT_GT(indices.size(), 0u);
}

TEST_F(ChestModelTest, GenerateMesh_IndexCountDivisibleByThree)
{
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    model_->generateMesh(vertices, indices);

    // 索引应该是三角形的倍数
    EXPECT_EQ(indices.size() % 3, 0u);
}

// ========== 可见性测试 ==========

TEST_F(ChestModelTest, SetAllVisible_True)
{
    model_->setAllVisible(true);
    EXPECT_TRUE(model_->getBottom()->isVisible());
    EXPECT_TRUE(model_->getLid()->isVisible());
    EXPECT_TRUE(model_->getLatch()->isVisible());
}

TEST_F(ChestModelTest, SetAllVisible_False)
{
    model_->setAllVisible(false);
    EXPECT_FALSE(model_->getBottom()->isVisible());
    EXPECT_FALSE(model_->getLid()->isVisible());
    EXPECT_FALSE(model_->getLatch()->isVisible());
}
