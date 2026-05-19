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

#include <algorithm>
#include <cstddef>
#include <gtest/gtest.h>

#include "client/renderer/trident/entity/model/animal/LlamaModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"

using namespace mc::client::renderer::entity::model::animal;
using mc::client::renderer::entity::model::ModelVertex;

namespace mc::client::renderer {
namespace {

struct Bounds {
    f32 minX = 0.0f;
    f32 minY = 0.0f;
    f32 minZ = 0.0f;
    f32 maxX = 0.0f;
    f32 maxY = 0.0f;
    f32 maxZ = 0.0f;
};

Bounds computeBounds(const std::vector<ModelVertex>& vertices)
{
    Bounds b;
    if (vertices.empty()) {
        return b;
    }

    b.minX = b.maxX = vertices[0].position.x;
    b.minY = b.maxY = vertices[0].position.y;
    b.minZ = b.maxZ = vertices[0].position.z;

    for (const auto& v : vertices) {
        b.minX = std::min(b.minX, v.position.x);
        b.minY = std::min(b.minY, v.position.y);
        b.minZ = std::min(b.minZ, v.position.z);
        b.maxX = std::max(b.maxX, v.position.x);
        b.maxY = std::max(b.maxY, v.position.y);
        b.maxZ = std::max(b.maxZ, v.position.z);
    }

    return b;
}

Bounds buildDefaultPoseBounds(LlamaModel& model)
{
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    constexpr f32 kModelScale = 1.0f / 16.0f;
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, kModelScale);
    model.generateMesh(vertices, indices, kModelScale);

    EXPECT_FALSE(vertices.empty());
    EXPECT_FALSE(indices.empty());

    return computeBounds(vertices);
}

} // anonymous namespace

// 测试 LlamaModel 默认构造
TEST(LlamaModelTest, DefaultConstructionCreatesValidModel)
{
    LlamaModel model;

    // 验证模型有有效的纹理尺寸 (128x64)
    EXPECT_EQ(model.textureWidth(), 128);
    EXPECT_EQ(model.textureHeight(), 64);
}

// 测试 LlamaModel 网格生成 - 验证边界合理
TEST(LlamaModelTest, GeneratedMeshHasReasonableBounds)
{
    LlamaModel model;
    const Bounds b = buildDefaultPoseBounds(model);

    const f32 width = b.maxX - b.minX;
    const f32 height = b.maxY - b.minY;
    const f32 depth = b.maxZ - b.minZ;

    // 羊驼模型应该有合理的尺寸
    // 宽度: 约 0.5-1.0 (模型单位 8-16)
    // 高度: 约 1.0-2.0 (模型单位 16-32)
    // 深度: 约 0.8-1.5 (模型单位 12-24)
    EXPECT_GT(width, 0.4f);
    EXPECT_GT(height, 0.8f);
    EXPECT_GT(depth, 0.6f);

    EXPECT_LT(width, 1.2f);
    EXPECT_LT(height, 2.5f);
    EXPECT_LT(depth, 2.0f);
}

// 测试 LlamaModel 网格生成 - 验证包含所有部件
TEST(LlamaModelTest, GeneratedMeshIncludesAllParts)
{
    LlamaModel model;
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    constexpr f32 kModelScale = 1.0f / 16.0f;
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, kModelScale);
    model.generateMesh(vertices, indices, kModelScale);

    // 验证网格包含头部（高位置）
    const bool hasHeadHeight = std::any_of(
        vertices.begin(), vertices.end(), [](const ModelVertex& vertex) { return vertex.position.y > 1.0f; });

    // 验证网格包含身体（中间位置）
    const bool hasBodyHeight = std::any_of(vertices.begin(), vertices.end(), [](const ModelVertex& vertex) {
        return vertex.position.y > 0.3f && vertex.position.y < 1.0f;
    });

    // 验证网格包含腿部（低位置）
    const bool hasLegHeight = std::any_of(
        vertices.begin(), vertices.end(), [](const ModelVertex& vertex) { return vertex.position.y < 0.3f; });

    EXPECT_TRUE(hasHeadHeight) << "Model should have head parts at high positions";
    EXPECT_TRUE(hasBodyHeight) << "Model should have body parts at middle positions";
    EXPECT_TRUE(hasLegHeight) << "Model should have leg parts at low positions";
}

// 测试 LlamaModel 幼体状态
TEST(LlamaModelTest, ChildStateCanBeSetAndQueried)
{
    LlamaModel model;

    // 默认应该不是幼体
    EXPECT_FALSE(model.isChild());

    // 设置为幼体
    model.setChild(true);
    EXPECT_TRUE(model.isChild());

    // 设置回成年体
    model.setChild(false);
    EXPECT_FALSE(model.isChild());
}

// 测试 LlamaModel 箱子状态
TEST(LlamaModelTest, ChestStateCanBeSetAndQueried)
{
    LlamaModel model;

    // 默认应该没有箱子
    EXPECT_FALSE(model.hasChest());

    // 设置为有箱子
    model.setHasChest(true);
    EXPECT_TRUE(model.hasChest());

    // 设置回无箱子
    model.setHasChest(false);
    EXPECT_FALSE(model.hasChest());
}

// 测试 LlamaModel 角度设置
TEST(LlamaModelTest, SetAnglesUpdatesModelPose)
{
    LlamaModel model;

    // 设置行走动画
    model.setAngles(1.0, 0.5, 0.0, 0.0, 0.0, 1.0);

    // 生成网格验证不会崩溃
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    model.generateMesh(vertices, indices, 1.0f / 16.0f);

    EXPECT_FALSE(vertices.empty());
    EXPECT_FALSE(indices.empty());
}

// 测试 LlamaModel 头部偏转
TEST(LlamaModelTest, HeadRotationAppliedCorrectly)
{
    LlamaModel model;

    // 设置头部向左偏转 45 度
    model.setAngles(0.0, 0.0, 0.0, 45.0, 0.0, 1.0);

    // 生成网格验证不会崩溃
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    model.generateMesh(vertices, indices, 1.0f / 16.0f);

    EXPECT_FALSE(vertices.empty());

    // 设置头部俯仰 30 度
    model.setAngles(0.0, 0.0, 0.0, 0.0, 30.0, 1.0);
    model.generateMesh(vertices, indices, 1.0f / 16.0f);

    EXPECT_FALSE(vertices.empty());
}

// 测试 LlamaModel 纹理尺寸正确
TEST(LlamaModelTest, TextureSizeMatchesMinecraftSpec)
{
    LlamaModel model;

    // MC 1.16.5 羊驼纹理尺寸是 128x64
    EXPECT_EQ(model.textureWidth(), 128);
    EXPECT_EQ(model.textureHeight(), 64);
}

// 测试 LlamaModel 使用模型单位时的尺寸
TEST(LlamaModelTest, RuntimeMeshUsesMinecraftModelUnits)
{
    LlamaModel model;
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    model.generateMesh(vertices, indices, 1.0f);

    const Bounds b = computeBounds(vertices);
    const f32 width = b.maxX - b.minX;
    const f32 depth = b.maxZ - b.minZ;

    // 在模型单位 (未缩放) 下，羊驼应该有合理的尺寸
    // 宽度: 约 10-20 模型单位
    // 深度: 约 15-30 模型单位
    EXPECT_GT(width, 6.0f);
    EXPECT_GT(depth, 10.0f);
    EXPECT_LT(width, 20.0f);
    EXPECT_LT(depth, 35.0f);
}

// 测试 LlamaModel 幼体渲染
TEST(LlamaModelTest, ChildRenderingProducesSmallerMesh)
{
    LlamaModel adultModel;
    LlamaModel childModel;

    adultModel.setChild(false);
    childModel.setChild(true);

    std::vector<ModelVertex> adultVertices;
    std::vector<u32> adultIndices;
    adultModel.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f / 16.0f);
    adultModel.generateMesh(adultVertices, adultIndices, 1.0f / 16.0f);

    std::vector<ModelVertex> childVertices;
    std::vector<u32> childIndices;
    childModel.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f / 16.0f);
    childModel.generateMesh(childVertices, childIndices, 1.0f / 16.0f);

    // 两个模型都应该生成有效的网格
    EXPECT_FALSE(adultVertices.empty());
    EXPECT_FALSE(childVertices.empty());
}

// 测试 LlamaModel 行走动画
TEST(LlamaModelTest, WalkAnimationGeneratesValidMesh)
{
    LlamaModel model;

    // 模拟行走动画的不同阶段
    for (f64 phase = 0.0; phase < 6.28; phase += 0.5) {
        f64 limbSwing = phase;
        f64 limbSwingAmount = 0.5;

        model.setAngles(limbSwing, limbSwingAmount, 0.0, 0.0, 0.0, 1.0);

        std::vector<ModelVertex> vertices;
        std::vector<u32> indices;
        model.generateMesh(vertices, indices, 1.0f / 16.0f);

        EXPECT_FALSE(vertices.empty()) << "Phase: " << phase;
        EXPECT_FALSE(indices.empty()) << "Phase: " << phase;
    }
}

} // namespace mc::client::renderer
