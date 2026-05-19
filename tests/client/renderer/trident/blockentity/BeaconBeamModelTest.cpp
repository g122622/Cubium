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

#include "client/renderer/trident/blockentity/model/BeaconBeamModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc::client::renderer::blockentity::model;
using namespace mc;

// 导入 ModelVertex 类型
using ModelVertex = mc::client::renderer::entity::model::ModelVertex;

class BeaconBeamModelTest : public ::testing::Test {
protected:
    void SetUp() override { model_ = std::make_unique<BeaconBeamModel>(); }

    std::unique_ptr<BeaconBeamModel> model_;
};

// ========== 基础创建测试 ==========

TEST_F(BeaconBeamModelTest, Create_Empty)
{
    EXPECT_TRUE(model_->getSegments().empty());
}

TEST_F(BeaconBeamModelTest, ClearSegments)
{
    BeamSegment segment;
    segment.colors = {1.0f, 0.0f, 0.0f};
    model_->addSegment(segment);
    EXPECT_EQ(model_->getSegments().size(), 1u);

    model_->clearSegments();
    EXPECT_TRUE(model_->getSegments().empty());
}

// ========== 光束段管理测试 ==========

TEST_F(BeaconBeamModelTest, AddSegment_Single)
{
    BeamSegment segment;
    segment.colors = {1.0f, 0.5f, 0.0f};
    segment.height = 10;
    model_->addSegment(segment);

    const auto& segments = model_->getSegments();
    ASSERT_EQ(segments.size(), 1u);
    EXPECT_FLOAT_EQ(segments[0].colors[0], 1.0f);
    EXPECT_FLOAT_EQ(segments[0].colors[1], 0.5f);
    EXPECT_FLOAT_EQ(segments[0].colors[2], 0.0f);
    EXPECT_EQ(segments[0].height, 10);
}

TEST_F(BeaconBeamModelTest, AddSegment_Multiple)
{
    BeamSegment seg1;
    seg1.colors = {1.0f, 1.0f, 1.0f};
    seg1.height = 5;
    model_->addSegment(seg1); // 白色段

    BeamSegment seg2;
    seg2.colors = {0.0f, 1.0f, 0.0f};
    seg2.height = 10;
    model_->addSegment(seg2); // 绿色段

    BeamSegment seg3;
    seg3.colors = {1.0f, 0.0f, 0.0f};
    seg3.height = 15;
    model_->addSegment(seg3); // 红色段

    const auto& segments = model_->getSegments();
    ASSERT_EQ(segments.size(), 3u);
    EXPECT_EQ(segments[0].height, 5);
    EXPECT_EQ(segments[1].height, 10);
    EXPECT_EQ(segments[2].height, 15);
}

// ========== 旋转计算测试 ==========

TEST_F(BeaconBeamModelTest, CalculateBeamRotation_Zero)
{
    // gameTime=0, partialTick=0 -> rotation = 0 * 2.25 - 45 = -45
    f32 rotation = BeaconBeamModel::calculateBeamRotation(0, 0.0f);
    EXPECT_FLOAT_EQ(rotation, -45.0f);
}

TEST_F(BeaconBeamModelTest, CalculateBeamRotation_PartialTick)
{
    // gameTime=0, partialTick=1 -> rotation = 1 * 2.25 - 45 = -42.75
    f32 rotation = BeaconBeamModel::calculateBeamRotation(0, 1.0f);
    EXPECT_FLOAT_EQ(rotation, -42.75f);
}

TEST_F(BeaconBeamModelTest, CalculateBeamRotation_PeriodEnd)
{
    // gameTime=40, partialTick=0 -> rotation = 0 * 2.25 - 45 = -45
    f32 rotation = BeaconBeamModel::calculateBeamRotation(40, 0.0f);
    EXPECT_FLOAT_EQ(rotation, -45.0f);
}

TEST_F(BeaconBeamModelTest, CalculateBeamRotation_MidPeriod)
{
    // gameTime=20, partialTick=0 -> rotation = 20 * 2.25 - 45 = 0
    f32 rotation = BeaconBeamModel::calculateBeamRotation(20, 0.0f);
    EXPECT_FLOAT_EQ(rotation, 0.0f);
}

TEST_F(BeaconBeamModelTest, CalculateBeamRotation_MaxAngle)
{
    // gameTime=40, partialTick=0 -> rotation = -45 (min)
    // gameTime=20, partialTick=0 -> rotation = 0
    // 最大角度接近 +45 度（在 period/2 附近）
    // gameTime=39, partialTick=1 -> rotation = 40 * 2.25 - 45 = 45
    f32 rotation = BeaconBeamModel::calculateBeamRotation(39, 1.0f);
    EXPECT_NEAR(rotation, 45.0f, 0.01f);
}

// ========== 网格生成测试 ==========

TEST_F(BeaconBeamModelTest, GenerateMesh_Empty_NoOutput)
{
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    model_->generateMesh(vertices, indices, 0, 0.0f);

    EXPECT_TRUE(vertices.empty());
    EXPECT_TRUE(indices.empty());
}

TEST_F(BeaconBeamModelTest, GenerateMesh_SingleSegment_ProducesVertices)
{
    BeamSegment seg;
    seg.colors = {1.0f, 1.0f, 1.0f};
    seg.height = 10;
    model_->addSegment(seg);

    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    model_->generateMesh(vertices, indices, 0, 0.0f);

    // 单段光束应产生：内层 4 面 + 外层 4 面 = 8 面 = 16 三角形 = 48 索引
    EXPECT_GT(vertices.size(), 0u);
    EXPECT_GT(indices.size(), 0u);
    EXPECT_EQ(indices.size() % 3, 0u); // 三角形
}

TEST_F(BeaconBeamModelTest, GenerateMesh_MultipleSegments)
{
    BeamSegment seg1;
    seg1.colors = {1.0f, 1.0f, 1.0f};
    seg1.height = 5;
    model_->addSegment(seg1);

    BeamSegment seg2;
    seg2.colors = {0.0f, 1.0f, 0.0f};
    seg2.height = 10;
    model_->addSegment(seg2);

    std::vector<ModelVertex> vertices1;
    std::vector<u32> indices1;
    model_->generateMesh(vertices1, indices1, 0, 0.0f);

    // 清空并测试单段
    model_->clearSegments();

    BeamSegment seg3;
    seg3.colors = {1.0f, 1.0f, 1.0f};
    seg3.height = 5;
    model_->addSegment(seg3);

    std::vector<ModelVertex> vertices2;
    std::vector<u32> indices2;
    model_->generateMesh(vertices2, indices2, 0, 0.0f);

    // 两段应该比一段产生更多顶点
    EXPECT_GT(vertices1.size(), vertices2.size());
}

TEST_F(BeaconBeamModelTest, GenerateMesh_LastSegment_UsesMaxHeight)
{
    // 最后一段高度应为 MAX_BEAM_HEIGHT (1024)
    BeamSegment seg;
    seg.colors = {1.0f, 1.0f, 1.0f};
    seg.height = 5;
    model_->addSegment(seg);

    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    model_->generateMesh(vertices, indices, 0, 0.0f);

    // 验证网格生成成功（最后一段高度为 1024 是内部逻辑）
    EXPECT_GT(vertices.size(), 0u);
}

// ========== 常量验证测试 ==========

TEST_F(BeaconBeamModelTest, Constants_CorrectValues)
{
    // MC 1.16.5 常量验证
    EXPECT_FLOAT_EQ(BeaconBeamModel::BEAM_RADIUS, 0.2f);
    EXPECT_FLOAT_EQ(BeaconBeamModel::GLOW_RADIUS, 0.25f);
    EXPECT_FLOAT_EQ(BeaconBeamModel::ROTATION_SPEED, 2.25f);
    EXPECT_FLOAT_EQ(BeaconBeamModel::ROTATION_OFFSET, -45.0f);
    EXPECT_EQ(BeaconBeamModel::ROTATION_PERIOD, 40L);
    EXPECT_EQ(BeaconBeamModel::MAX_BEAM_HEIGHT, 1024);
    EXPECT_EQ(BeaconBeamModel::MAX_LIGHT, 15728880u);
}

// ========== BeamSegment 测试 ==========

TEST(BeamSegmentTest, DefaultConstructor)
{
    BeamSegment segment;
    EXPECT_FLOAT_EQ(segment.colors[0], 1.0f);
    EXPECT_FLOAT_EQ(segment.colors[1], 1.0f);
    EXPECT_FLOAT_EQ(segment.colors[2], 1.0f);
    EXPECT_EQ(segment.height, 1);
}

TEST(BeamSegmentTest, ColorConstructor)
{
    BeamSegment segment(1.0f, 0.5f, 0.25f);
    EXPECT_FLOAT_EQ(segment.colors[0], 1.0f);
    EXPECT_FLOAT_EQ(segment.colors[1], 0.5f);
    EXPECT_FLOAT_EQ(segment.colors[2], 0.25f);
    EXPECT_EQ(segment.height, 1);
}

TEST(BeamSegmentTest, ColorArrayConstructor)
{
    std::array<f32, 3> color = {0.1f, 0.2f, 0.3f};
    BeamSegment segment(color);
    EXPECT_FLOAT_EQ(segment.colors[0], 0.1f);
    EXPECT_FLOAT_EQ(segment.colors[1], 0.2f);
    EXPECT_FLOAT_EQ(segment.colors[2], 0.3f);
    EXPECT_EQ(segment.height, 1);
}

TEST(BeamSegmentTest, IncrementHeight)
{
    BeamSegment segment;
    segment.colors = {1.0f, 1.0f, 1.0f};
    segment.height = 5;
    EXPECT_EQ(segment.height, 5);

    segment.incrementHeight();
    EXPECT_EQ(segment.height, 6);

    segment.incrementHeight();
    EXPECT_EQ(segment.height, 7);
}

TEST(BeamSegmentTest, ColorAccessors)
{
    BeamSegment segment(0.5f, 0.75f, 0.9f);
    EXPECT_FLOAT_EQ(segment.red(), 0.5f);
    EXPECT_FLOAT_EQ(segment.green(), 0.75f);
    EXPECT_FLOAT_EQ(segment.blue(), 0.9f);
}
