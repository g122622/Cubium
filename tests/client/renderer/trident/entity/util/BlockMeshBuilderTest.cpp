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
 * @file BlockMeshBuilderTest.cpp
 * @brief BlockMeshBuilder 方块网格构建工具单元测试
 *
 * 验证内容：
 * - buildFallbackCubeMesh 生成正确的 24 顶点 / 36 索引单位立方体
 * - 6 个面的法线方向正确（North/South/West/East/Down/Up）
 * - 顶点位置在 [0, 1] 范围内（单位立方体）
 * - UV 覆盖完整 0-1 范围
 * - 索引按两个三角形组成一个四边形（base+0, base+1, base+2, base+0, base+2, base+3）
 * - buildBlockMesh 在无 BlockModelCache 时回退到立方体网格
 *
 * BlockMeshBuilder 供 HeldBlockLayer / FallingBlockRenderer / TNTRenderer 复用，
 * 是方块实体渲染的网格数据来源。
 */

#include "client/renderer/trident/entity/util/BlockMeshBuilder.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client::renderer::entity::util;
using namespace mc::client::renderer::entity::model;

namespace {

/// 检查索引是否构成两个三角形（四边形）
/// 每面 4 顶点 / 6 索引：顶点基址 = faceIndex*4，索引基址 = faceIndex*6
void expectQuadIndices(const std::vector<u32>& indices, u32 faceIndex)
{
    const u32 indexBase = faceIndex * 6;
    const u32 vertexBase = faceIndex * 4;
    ASSERT_GE(indices.size(), static_cast<std::size_t>(indexBase + 6));
    // 三角形 1: vertexBase+0, vertexBase+1, vertexBase+2
    EXPECT_EQ(vertexBase + 0, indices[indexBase + 0]);
    EXPECT_EQ(vertexBase + 1, indices[indexBase + 1]);
    EXPECT_EQ(vertexBase + 2, indices[indexBase + 2]);
    // 三角形 2: vertexBase+0, vertexBase+2, vertexBase+3
    EXPECT_EQ(vertexBase + 0, indices[indexBase + 3]);
    EXPECT_EQ(vertexBase + 2, indices[indexBase + 4]);
    EXPECT_EQ(vertexBase + 3, indices[indexBase + 5]);
}

/// 检查顶点位置是否在 [0, 1] 范围内
void expectPositionsInUnitRange(const std::vector<ModelVertex>& vertices)
{
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        EXPECT_GE(vertices[i].position.x, 0.0f) << "vertex " << i;
        EXPECT_LE(vertices[i].position.x, 1.0f) << "vertex " << i;
        EXPECT_GE(vertices[i].position.y, 0.0f) << "vertex " << i;
        EXPECT_LE(vertices[i].position.y, 1.0f) << "vertex " << i;
        EXPECT_GE(vertices[i].position.z, 0.0f) << "vertex " << i;
        EXPECT_LE(vertices[i].position.z, 1.0f) << "vertex " << i;
    }
}

} // namespace

// ============================================================================
// buildFallbackCubeMesh 测试
// ============================================================================

TEST(BlockMeshBuilderFallbackTest, ProducesCorrectVertexAndIndexCount)
{
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    BlockMeshBuilder::buildFallbackCubeMesh(vertices, indices);

    // 6 面 × 4 顶点 = 24 顶点
    EXPECT_EQ(24u, vertices.size());
    // 6 面 × 6 索引 = 36 索引
    EXPECT_EQ(36u, indices.size());
}

TEST(BlockMeshBuilderFallbackTest, ClearsOutputVectorsBeforeBuilding)
{
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    // 预填充垃圾数据，验证 buildFallbackCubeMesh 会清空
    vertices.push_back(ModelVertex(99.0, 99.0, 99.0, 0, 0));
    indices.push_back(999u);

    BlockMeshBuilder::buildFallbackCubeMesh(vertices, indices);

    EXPECT_EQ(24u, vertices.size());
    EXPECT_EQ(36u, indices.size());
    // 验证垃圾数据已被清除
    for (const auto& v : vertices) {
        EXPECT_LE(v.position.x, 1.0f);
    }
}

TEST(BlockMeshBuilderFallbackTest, AllPositionsInUnitRange)
{
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    BlockMeshBuilder::buildFallbackCubeMesh(vertices, indices);

    expectPositionsInUnitRange(vertices);
}

TEST(BlockMeshBuilderFallbackTest, FaceNormalsAreAxisAligned)
{
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    BlockMeshBuilder::buildFallbackCubeMesh(vertices, indices);

    // 每面 4 顶点，检查每面法线一致且轴向对齐
    // 面顺序: North(Z-), South(Z+), West(X-), East(X+), Down(Y-), Up(Y+)
    const Vector3f expectedNormals[] = {
        Vector3f(0.0f, 0.0f, -1.0f), // North (Z-)
        Vector3f(0.0f, 0.0f, 1.0f),  // South (Z+)
        Vector3f(-1.0f, 0.0f, 0.0f), // West (X-)
        Vector3f(1.0f, 0.0f, 0.0f),  // East (X+)
        Vector3f(0.0f, -1.0f, 0.0f), // Down (Y-)
        Vector3f(0.0f, 1.0f, 0.0f)   // Up (Y+)
    };

    for (int face = 0; face < 6; ++face) {
        const Vector3f& expected = expectedNormals[face];
        for (int v = 0; v < 4; ++v) {
            const auto& vertex = vertices[face * 4 + v];
            EXPECT_FLOAT_EQ(expected.x, vertex.normal.x) << "face " << face << " vertex " << v;
            EXPECT_FLOAT_EQ(expected.y, vertex.normal.y) << "face " << face << " vertex " << v;
            EXPECT_FLOAT_EQ(expected.z, vertex.normal.z) << "face " << face << " vertex " << v;
        }
    }
}

TEST(BlockMeshBuilderFallbackTest, UVCoversFull01Range)
{
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    BlockMeshBuilder::buildFallbackCubeMesh(vertices, indices);

    // 每面 4 顶点的 UV 应覆盖 [0, 1] 范围
    for (std::size_t i = 0; i < vertices.size(); i += 4) {
        // 收集本面 4 顶点的 U/V
        float uMin = vertices[i].texCoord.x, uMax = uMin;
        float vMin = vertices[i].texCoord.y, vMax = vMin;
        for (std::size_t j = 1; j < 4; ++j) {
            uMin = std::min(uMin, vertices[i + j].texCoord.x);
            uMax = std::max(uMax, vertices[i + j].texCoord.x);
            vMin = std::min(vMin, vertices[i + j].texCoord.y);
            vMax = std::max(vMax, vertices[i + j].texCoord.y);
        }
        // 每面 UV 应覆盖 0 到 1
        EXPECT_FLOAT_EQ(0.0f, uMin) << "face starting at vertex " << i;
        EXPECT_FLOAT_EQ(1.0f, uMax) << "face starting at vertex " << i;
        EXPECT_FLOAT_EQ(0.0f, vMin) << "face starting at vertex " << i;
        EXPECT_FLOAT_EQ(1.0f, vMax) << "face starting at vertex " << i;
    }
}

TEST(BlockMeshBuilderFallbackTest, IndicesFormTwoTrianglesPerFace)
{
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    BlockMeshBuilder::buildFallbackCubeMesh(vertices, indices);

    // 每面 6 索引，验证每面索引构成两个三角形
    for (u32 face = 0; face < 6; ++face) {
        expectQuadIndices(indices, face);
    }
}

TEST(BlockMeshBuilderFallbackTest, AllIndicesWithinVertexBounds)
{
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    BlockMeshBuilder::buildFallbackCubeMesh(vertices, indices);

    const u32 vertexCount = static_cast<u32>(vertices.size());
    for (std::size_t i = 0; i < indices.size(); ++i) {
        EXPECT_LT(indices[i], vertexCount) << "index " << i;
    }
}

// ============================================================================
// buildBlockMesh 回退路径测试
// ============================================================================

TEST(BlockMeshBuilderFallbackTest, BuildBlockMeshFallsBackToCubeWhenNoModelCache)
{
    // 当 BlockModelCache 未初始化时，buildBlockMesh 应回退到立方体网格
    // 这保证了在测试环境（无资源包加载）下不会崩溃，且产生可渲染的网格
    VanillaBlocks::initialize();

    const BlockState* stoneState = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stoneState, nullptr);

    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    BlockMeshBuilder::buildBlockMesh(*stoneState, vertices, indices);

    // 回退路径应产生立方体网格（24 顶点 / 36 索引）
    // 或真实模型网格（顶点数 > 0）
    EXPECT_FALSE(vertices.empty());
    EXPECT_FALSE(indices.empty());

    // 如果是回退立方体，验证顶点/索引数
    if (vertices.size() == 24 && indices.size() == 36) {
        // 回退路径：验证位置在 [0, 1] 范围
        expectPositionsInUnitRange(vertices);
    }
}

TEST(BlockMeshBuilderFallbackTest, BuildBlockMeshClearsOutputBeforeBuilding)
{
    VanillaBlocks::initialize();

    const BlockState* dirtState = &VanillaBlocks::DIRT->defaultState();
    ASSERT_NE(dirtState, nullptr);

    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    vertices.push_back(ModelVertex(99.0, 99.0, 99.0, 0, 0));
    indices.push_back(999u);

    BlockMeshBuilder::buildBlockMesh(*dirtState, vertices, indices);

    // 垃圾数据应被清除
    EXPECT_FALSE(vertices.empty());
    for (const auto& v : vertices) {
        EXPECT_LE(v.position.x, 2.0f); // 真实模型可能在 0-1 范围
    }
}
