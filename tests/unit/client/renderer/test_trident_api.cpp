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

#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/api/TridentApi.hpp"
#include <gtest/gtest.h>

#undef EXPECT_FLOAT_EQ
#define EXPECT_FLOAT_EQ EXPECT_DOUBLE_EQ

using namespace mc;
// api 层的 BlendState/DepthState/RenderState/RenderType/Face/BlockGeometry 等类型
// 在本测试中被大量使用，统一引入 api 命名空间。api::Vertex 已删除，Vertex 解析到
// mc::Vertex 无歧义；Face/BlockGeometry 解析到 api 版（与 mc 版定义平行，行为一致）。
// 注意 TextureRegion：mc::（MeshTypes.hpp 精简版）与 api::（含 full()/width()/operator==）
// 各有一份且非镜像（api 版是活跃契约，mc 版供 ChunkTextureAtlas 内部用）。本测试的
// TextureRegionTest 测 api 版，用别名 api:: 全限定消歧。
using namespace mc::client::renderer::api;
namespace api = mc::client::renderer::api;

// ============================================================================
// Types 测试
// ============================================================================

class TypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TypesTest, VertexDefaultValues)
{
    Vertex v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
    EXPECT_FLOAT_EQ(v.u, 0.0f);
    EXPECT_FLOAT_EQ(v.v, 0.0f);
    EXPECT_EQ(v.color, 0xFFFFFFFFu);
    EXPECT_EQ(v.light, 255u);
}

TEST_F(TypesTest, VertexParameterizedConstructor)
{
    Vertex v(1.0f, 2.0f, 3.0f, 0.5f, 0.5f, 0x80808080, 128);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);
    EXPECT_FLOAT_EQ(v.u, 0.5f);
    EXPECT_FLOAT_EQ(v.v, 0.5f);
    EXPECT_EQ(v.color, 0x80808080u);
    EXPECT_EQ(v.light, 128u);
}

TEST_F(TypesTest, FaceEnumValues)
{
    EXPECT_EQ(static_cast<u8>(Face::Bottom), 0u);
    EXPECT_EQ(static_cast<u8>(Face::Top), 1u);
    EXPECT_EQ(static_cast<u8>(Face::North), 2u);
    EXPECT_EQ(static_cast<u8>(Face::South), 3u);
    EXPECT_EQ(static_cast<u8>(Face::West), 4u);
    EXPECT_EQ(static_cast<u8>(Face::East), 5u);
    EXPECT_EQ(static_cast<u8>(Face::Count), 6u);
}

// ============================================================================
// BlockGeometry 测试
// ============================================================================

class BlockGeometryTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(BlockGeometryTest, Constants)
{
    EXPECT_EQ(BlockGeometry::VERTICES_PER_FACE, 4u);
    EXPECT_EQ(BlockGeometry::INDICES_PER_FACE, 6u);
}

TEST_F(BlockGeometryTest, GetFaceNormal)
{
    // 测试每个面的法线
    auto bottomNormal = BlockGeometry::getFaceNormal(Face::Bottom);
    EXPECT_FLOAT_EQ(bottomNormal[0], 0.0f);
    EXPECT_FLOAT_EQ(bottomNormal[1], -1.0f);
    EXPECT_FLOAT_EQ(bottomNormal[2], 0.0f);

    auto topNormal = BlockGeometry::getFaceNormal(Face::Top);
    EXPECT_FLOAT_EQ(topNormal[0], 0.0f);
    EXPECT_FLOAT_EQ(topNormal[1], 1.0f);
    EXPECT_FLOAT_EQ(topNormal[2], 0.0f);

    auto northNormal = BlockGeometry::getFaceNormal(Face::North);
    EXPECT_FLOAT_EQ(northNormal[0], 0.0f);
    EXPECT_FLOAT_EQ(northNormal[1], 0.0f);
    EXPECT_FLOAT_EQ(northNormal[2], -1.0f);

    auto southNormal = BlockGeometry::getFaceNormal(Face::South);
    EXPECT_FLOAT_EQ(southNormal[0], 0.0f);
    EXPECT_FLOAT_EQ(southNormal[1], 0.0f);
    EXPECT_FLOAT_EQ(southNormal[2], 1.0f);

    auto westNormal = BlockGeometry::getFaceNormal(Face::West);
    EXPECT_FLOAT_EQ(westNormal[0], -1.0f);
    EXPECT_FLOAT_EQ(westNormal[1], 0.0f);
    EXPECT_FLOAT_EQ(westNormal[2], 0.0f);

    auto eastNormal = BlockGeometry::getFaceNormal(Face::East);
    EXPECT_FLOAT_EQ(eastNormal[0], 1.0f);
    EXPECT_FLOAT_EQ(eastNormal[1], 0.0f);
    EXPECT_FLOAT_EQ(eastNormal[2], 0.0f);
}

TEST_F(BlockGeometryTest, GetFaceVertices)
{
    // 测试顶面顶点 (Y+)
    auto topVertices = BlockGeometry::getFaceVertices(Face::Top);
    EXPECT_EQ(topVertices.size(), 12u); // 4 vertices * 3 components

    // 顶面应该在 Y=1 平面
    EXPECT_FLOAT_EQ(topVertices[1], 1.0f);  // v0.y
    EXPECT_FLOAT_EQ(topVertices[4], 1.0f);  // v1.y
    EXPECT_FLOAT_EQ(topVertices[7], 1.0f);  // v2.y
    EXPECT_FLOAT_EQ(topVertices[10], 1.0f); // v3.y
}

TEST_F(BlockGeometryTest, GetFaceIndices)
{
    auto indices = BlockGeometry::getFaceIndices();
    EXPECT_EQ(indices.size(), 6u);

    // 标准四边形索引: 0-1-2, 0-2-3
    EXPECT_EQ(indices[0], 0u);
    EXPECT_EQ(indices[1], 1u);
    EXPECT_EQ(indices[2], 2u);
    EXPECT_EQ(indices[3], 0u);
    EXPECT_EQ(indices[4], 2u);
    EXPECT_EQ(indices[5], 3u);
}

TEST_F(BlockGeometryTest, GetFaceDirection)
{
    auto bottomDir = BlockGeometry::getFaceDirection(Face::Bottom);
    EXPECT_EQ(bottomDir[0], 0);
    EXPECT_EQ(bottomDir[1], -1);
    EXPECT_EQ(bottomDir[2], 0);

    auto topDir = BlockGeometry::getFaceDirection(Face::Top);
    EXPECT_EQ(topDir[0], 0);
    EXPECT_EQ(topDir[1], 1);
    EXPECT_EQ(topDir[2], 0);
}

TEST_F(BlockGeometryTest, ShouldRenderFace)
{
    // 如果邻居不透明，不渲染该面
    EXPECT_FALSE(BlockGeometry::shouldRenderFace(Face::Top, true));

    // 如果邻居透明，渲染该面
    EXPECT_TRUE(BlockGeometry::shouldRenderFace(Face::Top, false));
}

// ============================================================================
// BlendMode 测试
// ============================================================================

class BlendStateTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(BlendStateTest, DisabledState)
{
    auto state = BlendState::disabled();
    EXPECT_FALSE(state.enabled);
}

TEST_F(BlendStateTest, AlphaBlend)
{
    auto state = BlendState::alpha();
    EXPECT_TRUE(state.enabled);
    EXPECT_EQ(state.srcColor, BlendFactor::SrcAlpha);
    EXPECT_EQ(state.dstColor, BlendFactor::OneMinusSrcAlpha);
}

TEST_F(BlendStateTest, AdditiveBlend)
{
    auto state = BlendState::additive();
    EXPECT_TRUE(state.enabled);
    EXPECT_EQ(state.srcColor, BlendFactor::SrcAlpha);
    EXPECT_EQ(state.dstColor, BlendFactor::One);
}

TEST_F(BlendStateTest, PremultipliedBlend)
{
    auto state = BlendState::premultiplied();
    EXPECT_TRUE(state.enabled);
    EXPECT_EQ(state.srcColor, BlendFactor::One);
    EXPECT_EQ(state.dstColor, BlendFactor::OneMinusSrcAlpha);
}

TEST_F(BlendStateTest, EqualityComparison)
{
    auto state1 = BlendState::alpha();
    auto state2 = BlendState::alpha();
    auto state3 = BlendState::additive();

    EXPECT_EQ(state1, state2);
    EXPECT_NE(state1, state3);
}

// ============================================================================
// DepthState 测试
// ============================================================================

class DepthStateTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(DepthStateTest, DisabledState)
{
    auto state = DepthState::disabled();
    EXPECT_FALSE(state.testEnabled);
    EXPECT_FALSE(state.writeEnabled);
}

TEST_F(DepthStateTest, ReadOnlyState)
{
    auto state = DepthState::readOnly();
    EXPECT_TRUE(state.testEnabled);
    EXPECT_FALSE(state.writeEnabled);
    EXPECT_EQ(state.compareOp, CompareOp::Less);
}

TEST_F(DepthStateTest, ReadWriteState)
{
    auto state = DepthState::readWrite();
    EXPECT_TRUE(state.testEnabled);
    EXPECT_TRUE(state.writeEnabled);
    EXPECT_EQ(state.compareOp, CompareOp::Less);
}

TEST_F(DepthStateTest, EqualState)
{
    auto state = DepthState::equal();
    EXPECT_TRUE(state.testEnabled);
    EXPECT_FALSE(state.writeEnabled);
    EXPECT_EQ(state.compareOp, CompareOp::Equal);
}

// ============================================================================
// CullMode/RasterizerState 测试
// ============================================================================

class RasterizerStateTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(RasterizerStateTest, DefaultState)
{
    auto state = RasterizerState::defaults();
    EXPECT_EQ(state.cullMode, CullMode::Back);
    EXPECT_EQ(state.frontFace, FrontFace::Clockwise);
    EXPECT_EQ(state.polygonMode, PolygonMode::Fill);
}

TEST_F(RasterizerStateTest, DoubleSidedState)
{
    auto state = RasterizerState::doubleSided();
    EXPECT_EQ(state.cullMode, CullMode::None);
}

TEST_F(RasterizerStateTest, WireframeState)
{
    auto state = RasterizerState::wireframe();
    EXPECT_EQ(state.polygonMode, PolygonMode::Line);
    EXPECT_EQ(state.cullMode, CullMode::None);
}

// ============================================================================
// RenderState 测试
// ============================================================================

class RenderStateTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(RenderStateTest, SolidState)
{
    auto state = RenderState::solid();
    EXPECT_FALSE(state.blend.enabled);
    EXPECT_TRUE(state.depth.testEnabled);
    EXPECT_TRUE(state.depth.writeEnabled);
    EXPECT_EQ(state.rasterizer.cullMode, CullMode::Back);
}

TEST_F(RenderStateTest, TranslucentState)
{
    auto state = RenderState::translucent();
    EXPECT_TRUE(state.blend.enabled);
    EXPECT_TRUE(state.depth.testEnabled);
    EXPECT_FALSE(state.depth.writeEnabled);
    EXPECT_EQ(state.rasterizer.cullMode, CullMode::None);
}

TEST_F(RenderStateTest, LinesState)
{
    auto state = RenderState::lines();
    EXPECT_TRUE(state.blend.enabled);
    EXPECT_TRUE(state.depth.testEnabled);
    EXPECT_TRUE(state.depth.writeEnabled);
    EXPECT_EQ(state.rasterizer.polygonMode, PolygonMode::Line);
}

// ============================================================================
// RenderType 测试
// ============================================================================

class RenderTypeTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(RenderTypeTest, SolidRenderType)
{
    auto rt = RenderType::solid();
    EXPECT_TRUE(rt.isValid());
    EXPECT_EQ(rt.name(), std::string("solid"));
    EXPECT_EQ(rt.sortIndex(), 0);
    EXPECT_FALSE(rt.needsSorting());
}

TEST_F(RenderTypeTest, TranslucentRenderType)
{
    auto rt = RenderType::translucent();
    EXPECT_TRUE(rt.isValid());
    EXPECT_EQ(rt.name(), std::string("translucent"));
    EXPECT_EQ(rt.sortIndex(), 100);
    EXPECT_TRUE(rt.needsSorting());
}

TEST_F(RenderTypeTest, RenderTypeComparison)
{
    auto solid = RenderType::solid();
    auto cutout = RenderType::cutout();
    auto translucent = RenderType::translucent();

    EXPECT_TRUE(solid.shouldRenderBefore(cutout));
    EXPECT_TRUE(cutout.shouldRenderBefore(translucent));
    EXPECT_TRUE(solid.shouldRenderBefore(translucent));
}

TEST_F(RenderTypeTest, EntityRenderType)
{
    auto rt = RenderType::entitySolid(mc::ResourceLocation("minecraft:textures/entity/pig.png"));
    EXPECT_TRUE(rt.isValid());
    EXPECT_EQ(rt.name(), std::string("entity_solid"));
    EXPECT_EQ(rt.texture().toString(), std::string("minecraft:textures/entity/pig.png"));
}

// ============================================================================
// TextureRegion 测试
// ============================================================================

class TextureRegionTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TextureRegionTest, DefaultValues)
{
    api::TextureRegion region;
    EXPECT_FLOAT_EQ(region.u0, 0.0f);
    EXPECT_FLOAT_EQ(region.v0, 0.0f);
    EXPECT_FLOAT_EQ(region.u1, 1.0f);
    EXPECT_FLOAT_EQ(region.v1, 1.0f);
}

TEST_F(TextureRegionTest, ParameterizedConstructor)
{
    api::TextureRegion region(0.25f, 0.5f, 0.75f, 1.0f);
    EXPECT_FLOAT_EQ(region.u0, 0.25f);
    EXPECT_FLOAT_EQ(region.v0, 0.5f);
    EXPECT_FLOAT_EQ(region.u1, 0.75f);
    EXPECT_FLOAT_EQ(region.v1, 1.0f);
}

TEST_F(TextureRegionTest, WidthHeight)
{
    api::TextureRegion region(0.0f, 0.0f, 0.5f, 0.5f);
    EXPECT_FLOAT_EQ(region.width(), 0.5f);
    EXPECT_FLOAT_EQ(region.height(), 0.5f);
}

TEST_F(TextureRegionTest, FullRegion)
{
    auto region = api::TextureRegion::full();
    EXPECT_FLOAT_EQ(region.u0, 0.0f);
    EXPECT_FLOAT_EQ(region.v0, 0.0f);
    EXPECT_FLOAT_EQ(region.u1, 1.0f);
    EXPECT_FLOAT_EQ(region.v1, 1.0f);
}

TEST_F(TextureRegionTest, EqualityComparison)
{
    api::TextureRegion r1(0.0f, 0.0f, 1.0f, 1.0f);
    api::TextureRegion r2(0.0f, 0.0f, 1.0f, 1.0f);
    api::TextureRegion r3(0.5f, 0.5f, 1.0f, 1.0f);

    EXPECT_EQ(r1, r2);
    EXPECT_NE(r1, r3);
}

// ============================================================================
// MeshData 测试
// ============================================================================

class MeshDataTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(MeshDataTest, Clear)
{
    MeshData mesh;
    mesh.vertices.push_back(Vertex());
    mesh.indices.push_back(0);

    mesh.clear();
    EXPECT_TRUE(mesh.vertices.empty());
    EXPECT_TRUE(mesh.indices.empty());
}

TEST_F(MeshDataTest, Reserve)
{
    MeshData mesh;
    mesh.reserve(100, 200);
    // reserve 不改变 size，只改变 capacity
    EXPECT_TRUE(mesh.vertices.empty());
    EXPECT_TRUE(mesh.indices.empty());
}

TEST_F(MeshDataTest, AddFace)
{
    MeshData mesh;
    std::array<Vertex, 4> faceVertices = {Vertex(0, 0, 0, 0, 0, 0xFFFFFFFF, 255),
        Vertex(1, 0, 0, 1, 0, 0xFFFFFFFF, 255),
        Vertex(1, 1, 0, 1, 1, 0xFFFFFFFF, 255),
        Vertex(0, 1, 0, 0, 1, 0xFFFFFFFF, 255)};

    mesh.addFace(faceVertices, 0);
    EXPECT_EQ(mesh.vertexCount(), 4u);
    EXPECT_EQ(mesh.indexCount(), 6u);
}

// ============================================================================
// CameraConfig 测试
// ============================================================================

class CameraConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(CameraConfigTest, DefaultValues)
{
    mc::client::renderer::api::CameraConfig config;
    // CameraConfig 字段均为 f64，使用 EXPECT_DOUBLE_EQ 避免 float→double 提升导致的精度不一致。
    EXPECT_DOUBLE_EQ(config.fov, 70.0);
    EXPECT_DOUBLE_EQ(config.aspectRatio, 16.0 / 9.0);
    EXPECT_DOUBLE_EQ(config.nearPlane, 0.1);
    EXPECT_DOUBLE_EQ(config.farPlane, 1000.0);
    EXPECT_EQ(config.projectionMode, mc::client::renderer::api::ProjectionMode::Perspective);
}
