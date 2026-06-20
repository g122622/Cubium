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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER INFORMATION
 * CONTAINED IN THE SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "client/renderer/api/texture/TextureRegion.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/item/ItemMeshBuilder.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"

#include <cmath>

using namespace mc::client::renderer::entity::item;
using namespace mc::client::renderer::entity::model;

namespace mc::client::renderer::trident::item {
namespace {

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 计算网格中的四边形数量
 */
u32 countQuads(const std::vector<u32>& indices)
{
    return static_cast<u32>(indices.size()) / 6;
}

/**
 * @brief 收集网格中所有唯一的法线方向
 *
 * @param vertices 顶点数组
 * @param indices 索引数组
 * @return 去重后的法线方向列表
 */
std::vector<mc::Vector3f> collectUniqueNormals(
    const std::vector<ModelVertex>& vertices, const std::vector<u32>& indices)
{
    std::vector<mc::Vector3f> normals;
    for (size_t i = 0; i < indices.size(); i += 3) {
        if (indices[i] >= vertices.size() || indices[i + 1] >= vertices.size() || indices[i + 2] >= vertices.size()) {
            continue;
        }
        const auto& v0 = vertices[indices[i]];
        mc::Vector3f n = v0.normal;

        // 检查是否已存在相近法线
        bool found = false;
        for (const auto& existing : normals) {
            float dx = existing.x - n.x;
            float dy = existing.y - n.y;
            float dz = existing.z - n.z;
            if (dx * dx + dy * dy + dz * dz < 0.01f) {
                found = true;
                break;
            }
        }
        if (!found) {
            normals.push_back(n);
        }
    }
    return normals;
}

/**
 * @brief 检查网格中是否包含指定法线方向的四边形
 */
bool hasNormalDirection(
    const std::vector<ModelVertex>& vertices, const std::vector<u32>& indices, float nx, float ny, float nz)
{
    for (size_t i = 0; i < indices.size(); i += 3) {
        if (indices[i] >= vertices.size()) continue;
        const auto& v = vertices[indices[i]];
        float dx = v.normal.x - nx;
        float dy = v.normal.y - ny;
        float dz = v.normal.z - nz;
        if (dx * dx + dy * dy + dz * dz < 0.01f) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 注册一个测试用的 Item 并返回引用
 *
 * 使用 ResourceLocation("test:xxx") 避免与 vanilla 物品冲突
 */
mc::Item& registerTestItem(const mc::ResourceLocation& id)
{
    return mc::ItemRegistry::instance().registerItem(id, mc::ItemProperties().maxStackSize(64));
}

// ============================================================================
// 回退网格测试：验证完整的6面立方体
// ============================================================================

class FallbackMeshTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保物品注册表已初始化
        auto& registry = mc::ItemRegistry::instance();
        (void)registry;
    }
};

/**
 * @brief 空物品堆应该返回空网格
 */
TEST_F(FallbackMeshTest, EmptyItemStackReturnsEmptyMesh)
{
    mc::ItemStack emptyStack;
    auto [vertices, indices] = ItemMeshBuilder::buildHeldItemMesh(emptyStack, ItemTransformType::Gui);
    EXPECT_TRUE(vertices.empty());
    EXPECT_TRUE(indices.empty());
}

/**
 * @brief buildIconMesh 生成四边形
 */
TEST_F(FallbackMeshTest, BuildIconMeshProducesQuad)
{
    mc::client::renderer::api::TextureRegion region{0.0, 0.0, 1.0, 1.0};
    auto [vertices, indices] = ItemMeshBuilder::buildIconMesh(region, 1.0);

    EXPECT_EQ(vertices.size(), 4u);
    EXPECT_EQ(indices.size(), 6u);
    EXPECT_EQ(countQuads(indices), 1u);
}

/**
 * @brief buildIconMesh 法线朝 +Z 方向
 */
TEST_F(FallbackMeshTest, BuildIconMeshNormalsFaceForward)
{
    mc::client::renderer::api::TextureRegion region{0.0, 0.0, 1.0, 1.0};
    auto [vertices, indices] = ItemMeshBuilder::buildIconMesh(region, 1.0);

    for (const auto& v : vertices) {
        EXPECT_NEAR(v.normal.x, 0.0f, 0.001f);
        EXPECT_NEAR(v.normal.y, 0.0f, 0.001f);
        EXPECT_NEAR(v.normal.z, 1.0f, 0.001f);
    }
}

/**
 * @brief buildIconMesh UV 坐标正确映射
 */
TEST_F(FallbackMeshTest, BuildIconMeshUVCoordinatesCorrect)
{
    mc::client::renderer::api::TextureRegion region{0.25, 0.25, 0.75, 0.75};
    auto [vertices, indices] = ItemMeshBuilder::buildIconMesh(region, 1.0);

    ASSERT_EQ(vertices.size(), 4u);
    // 验证 UV 覆盖了 region 的范围
    bool hasU0 = false, hasU1 = false, hasV0 = false, hasV1 = false;
    for (const auto& v : vertices) {
        if (std::abs(v.texCoord.x - 0.25f) < 0.001f) hasU0 = true;
        if (std::abs(v.texCoord.x - 0.75f) < 0.001f) hasU1 = true;
        if (std::abs(v.texCoord.y - 0.25f) < 0.001f) hasV0 = true;
        if (std::abs(v.texCoord.y - 0.75f) < 0.001f) hasV1 = true;
    }
    EXPECT_TRUE(hasU0);
    EXPECT_TRUE(hasU1);
    EXPECT_TRUE(hasV0);
    EXPECT_TRUE(hasV1);
}

// ============================================================================
// 回退网格 6 面立方体验证
// 使用 buildIconMesh 测试公开接口，然后通过回退路径间接测试 6 面
// 由于 _buildFallbackMesh 是私有的，我们通过观察回退路径来验证
// ============================================================================

/**
 * @brief buildIconMesh 的网格尺寸参数正确
 */
TEST_F(FallbackMeshTest, BuildIconMeshSizeParameterCorrect)
{
    mc::client::renderer::api::TextureRegion region{0.0, 0.0, 1.0, 1.0};
    f64 size = 2.0;
    auto [vertices, indices] = ItemMeshBuilder::buildIconMesh(region, size);

    ASSERT_EQ(vertices.size(), 4u);
    // halfSize = size * 0.5 = 1.0
    // 验证顶点位置在 [-1, 1] 范围内
    for (const auto& v : vertices) {
        EXPECT_GE(v.position.x, -1.01f);
        EXPECT_LE(v.position.x, 1.01f);
        EXPECT_GE(v.position.y, -1.01f);
        EXPECT_LE(v.position.y, 1.01f);
        EXPECT_NEAR(v.position.z, 0.0f, 0.001f); // icon mesh 在 Z=0 平面
    }
}

/**
 * @brief buildGroundItemMesh 对空物品堆返回空网格
 */
TEST_F(FallbackMeshTest, BuildGroundItemMeshEmptyReturnsEmpty)
{
    mc::ItemStack emptyStack;
    auto [vertices, indices] = ItemMeshBuilder::buildGroundItemMesh(emptyStack, 0.0);
    EXPECT_TRUE(vertices.empty());
    EXPECT_TRUE(indices.empty());
}

/**
 * @brief buildHeadMesh 对空物品堆返回空网格
 */
TEST_F(FallbackMeshTest, BuildHeadMeshEmptyReturnsEmpty)
{
    mc::ItemStack emptyStack;
    auto [vertices, indices] = ItemMeshBuilder::buildHeadMesh(emptyStack);
    EXPECT_TRUE(vertices.empty());
    EXPECT_TRUE(indices.empty());
}

/**
 * @brief buildArmorMesh 对空物品堆返回空网格
 */
TEST_F(FallbackMeshTest, BuildArmorMeshEmptyReturnsEmpty)
{
    mc::ItemStack emptyStack;
    std::array<f64, 16> identityMatrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    auto [vertices, indices] = ItemMeshBuilder::buildArmorMesh(emptyStack, 0, identityMatrix);
    EXPECT_TRUE(vertices.empty());
    EXPECT_TRUE(indices.empty());
}

// ============================================================================
// getItemTransform 测试
// ============================================================================

/**
 * @brief None 变换类型返回近似单位矩阵
 */
TEST(ItemTransformTest, NoneTransformReturnsIdentityLike)
{
    auto transform = ItemMeshBuilder::getItemTransform(ItemTransformType::None, 0.0f, 0.0f, true);

    // None 类型应该返回单位矩阵
    EXPECT_NEAR(transform[0], 1.0, 0.001);
    EXPECT_NEAR(transform[5], 1.0, 0.001);
    EXPECT_NEAR(transform[10], 1.0, 0.001);
    EXPECT_NEAR(transform[15], 1.0, 0.001);
}

/**
 * @brief GUI 变换有缩放
 */
TEST(ItemTransformTest, GuiTransformHasScale)
{
    auto transform = ItemMeshBuilder::getItemTransform(ItemTransformType::Gui, 0.0f, 0.0f, true);

    constexpr f64 ITEM_GUI_SCALE = 1.0 / 32.0;
    EXPECT_NEAR(transform[0], ITEM_GUI_SCALE, 0.001);
    EXPECT_NEAR(transform[5], -ITEM_GUI_SCALE, 0.001);
    EXPECT_NEAR(transform[10], ITEM_GUI_SCALE, 0.001);
}

/**
 * @brief Ground 变换有 Y 方向偏移
 */
TEST(ItemTransformTest, GroundTransformHasYOffset)
{
    auto transform = ItemMeshBuilder::getItemTransform(ItemTransformType::Ground, 0.0f, 0.0f, true);

    EXPECT_NEAR(transform[7], 0.1, 0.001);
}

/**
 * @brief FirstPersonRightHand 变换有 Z 方向偏移
 */
TEST(ItemTransformTest, FirstPersonRightHandHasZOffset)
{
    auto transform = ItemMeshBuilder::getItemTransform(ItemTransformType::FirstPersonRightHand, 0.0f, 0.0f, true);

    EXPECT_NEAR(transform[11], -0.72, 0.01);
}

/**
 * @brief ThirdPersonRightHand 有 Y 偏移
 */
TEST(ItemTransformTest, ThirdPersonRightHandHasYOffset)
{
    auto transform = ItemMeshBuilder::getItemTransform(ItemTransformType::ThirdPersonRightHand, 0.0f, 0.0f, true);

    EXPECT_NEAR(transform[7], 0.16, 0.01);
}

// ============================================================================
// setItemTextureAtlas 测试
// ============================================================================

TEST(ItemTextureAtlasSetterTest, SetNullPointerAllowed)
{
    // 设置 nullptr 不应该崩溃
    ItemMeshBuilder::setItemTextureAtlas(nullptr);
    ItemMeshBuilder::setItemTextureAtlas(nullptr);
}

// ============================================================================
// 法线变换数学验证（独立于 ItemMeshBuilder 内部实现）
// 验证逆法线矩阵变换的数学正确性
// ============================================================================

/**
 * @brief 验证逆法线矩阵对均匀缩放保持法线方向不变
 *
 * 对于均匀缩放矩阵 S = scale(s, s, s)，
 * 逆转置矩阵 = (S^{-1})^T = (1/s) * I
 * 变换后的法线归一化后方向不变
 */
TEST(NormalTransformMathTest, UniformScalePreservesDirection)
{
    // 均匀缩放 2x
    f64 s = 2.0;
    f64 invS = 1.0 / s;
    // 逆转置矩阵 = diag(invS, invS, invS)
    f64 normalIn[] = {0.0, 1.0, 0.0};
    f64 normalOut[3] = {normalIn[0] * invS, normalIn[1] * invS, normalIn[2] * invS};
    // 归一化
    f64 len = std::sqrt(normalOut[0] * normalOut[0] + normalOut[1] * normalOut[1] + normalOut[2] * normalOut[2]);
    normalOut[0] /= len;
    normalOut[1] /= len;
    normalOut[2] /= len;

    EXPECT_NEAR(normalOut[0], 0.0, 0.001);
    EXPECT_NEAR(normalOut[1], 1.0, 0.001);
    EXPECT_NEAR(normalOut[2], 0.0, 0.001);
}

/**
 * @brief 验证逆法线矩阵对非均匀缩放正确调整法线方向
 *
 * 对于非均匀缩放 S = diag(1, 2, 1)，
 * 法线 (1,1,0) 经过逆转置变换后应该是 (2, 0.5, 0) 方向（归一化后）
 * 而非简单乘法得到的 (1, 2, 0) 方向
 */
TEST(NormalTransformMathTest, NonUniformScaleCorrectsNormalDirection)
{
    // 非均匀缩放：Y 轴放大 2 倍
    // scale(1, 2, 1) 的逆矩阵 = scale(1, 0.5, 1)
    // 逆转置 = scale(1, 0.5, 1)（对角矩阵的逆等于转置）
    // 法线 (1/sqrt(2), 1/sqrt(2), 0) 变换后 = (1/sqrt(2), 0.5/sqrt(2), 0)
    // 归一化 = (2/sqrt(5), 1/sqrt(5), 0)
    f64 invSx = 1.0, invSy = 0.5, invSz = 1.0;
    f64 nx = 1.0 / std::sqrt(2.0);
    f64 ny = 1.0 / std::sqrt(2.0);
    f64 nz = 0.0;

    f64 outX = nx * invSx;
    f64 outY = ny * invSy;
    f64 outZ = nz * invSz;

    f64 len = std::sqrt(outX * outX + outY * outY + outZ * outZ);
    outX /= len;
    outY /= len;
    outZ /= len;

    // 期望方向: (2/sqrt(5), 1/sqrt(5), 0)
    f64 expectedX = 2.0 / std::sqrt(5.0);
    f64 expectedY = 1.0 / std::sqrt(5.0);
    EXPECT_NEAR(outX, expectedX, 0.01);
    EXPECT_NEAR(outY, expectedY, 0.01);
    EXPECT_NEAR(outZ, 0.0, 0.01);

    // 与简单矩阵乘法的错误结果对比
    // 错误方法：直接用 scale 矩阵乘法 = (1/sqrt(2), 2/sqrt(2), 0)
    // 归一化 = (1/sqrt(5), 2/sqrt(5), 0) - 这是错误的
    f64 wrongX = nx * 1.0;
    f64 wrongY = ny * 2.0;
    f64 wrongZ = nz * 1.0;
    f64 wrongLen = std::sqrt(wrongX * wrongX + wrongY * wrongY + wrongZ * wrongZ);
    wrongX /= wrongLen;
    wrongY /= wrongLen;

    // 确认两种方法得到不同结果（证明简单乘法是错误的）
    bool sameDirection = (std::abs(outX - wrongX) < 0.01 && std::abs(outY - wrongY) < 0.01);
    EXPECT_FALSE(sameDirection);
}

/**
 * @brief 验证单位矩阵的法线变换保持不变
 */
TEST(NormalTransformMathTest, IdentityMatrixPreservesNormals)
{
    // 单位矩阵的逆法线矩阵 = 单位矩阵
    f64 normalIn[] = {0.0f, 1.0f, 0.0f};
    // 单位矩阵变换后法线不变
    f64 normalOut[] = {normalIn[0], normalIn[1], normalIn[2]};

    EXPECT_NEAR(normalOut[0], 0.0, 0.001);
    EXPECT_NEAR(normalOut[1], 1.0, 0.001);
    EXPECT_NEAR(normalOut[2], 0.0, 0.001);

    // 另一个法线方向
    f64 normalIn2[] = {1.0f, 0.0f, 0.0f};
    EXPECT_NEAR(normalIn2[0], 1.0, 0.001);
    EXPECT_NEAR(normalIn2[1], 0.0, 0.001);
    EXPECT_NEAR(normalIn2[2], 0.0, 0.001);
}

// ============================================================================
// 回退网格6面立方体验证
// 通过注册真实 Item 但不初始化 ItemModelCache，触发 _buildFallbackMesh 路径
// _buildFallbackMesh 在 ItemModelCache::getItemModel() 返回 nullptr 时被调用
// ============================================================================

class FallbackCubeTest : public ::testing::Test {
protected:
    static mc::Item* s_testItem;

    static void SetUpTestSuite()
    {
        // 注册一个测试物品（不需要 Items::initialize()，只注册一个即可）
        s_testItem = &registerTestItem(mc::ResourceLocation("test", "fallback_cube_item"));
    }

    void SetUp() override
    {
        // 确保 s_itemTextureAtlas 为 nullptr，使纹理解析路径走空分支
        ItemMeshBuilder::setItemTextureAtlas(nullptr);
    }

    void TearDown() override { ItemMeshBuilder::setItemTextureAtlas(nullptr); }
};

mc::Item* FallbackCubeTest::s_testItem = nullptr;

/**
 * @brief buildHeldItemMesh 对有真实Item的ItemStack应生成6面回退立方体
 *
 * 当 ItemModelCache 未初始化时，getItemModel() 返回 nullptr，
 * _build3DItemMesh 会调用 _buildFallbackMesh 生成6面立方体。
 */
TEST_F(FallbackCubeTest, HeldItemMeshGeneratesSixFaceCube)
{
    mc::ItemStack stack(*s_testItem, 1);
    auto [vertices, indices] = ItemMeshBuilder::buildHeldItemMesh(stack, ItemTransformType::Gui);

    // 6面 × 4顶点 = 24 顶点
    EXPECT_EQ(vertices.size(), 24u);
    // 6面 × 6索引 = 36 索引
    EXPECT_EQ(indices.size(), 36u);
    // 6个四边形
    EXPECT_EQ(countQuads(indices), 6u);
}

/**
 * @brief 回退立方体包含所有6个法线方向
 */
TEST_F(FallbackCubeTest, FallbackCubeHasAllSixNormals)
{
    mc::ItemStack stack(*s_testItem, 1);
    auto [vertices, indices] = ItemMeshBuilder::buildHeldItemMesh(stack, ItemTransformType::Gui);

    ASSERT_FALSE(vertices.empty());
    ASSERT_FALSE(indices.empty());

    // 验证6个法线方向都存在
    EXPECT_TRUE(hasNormalDirection(vertices, indices, 0.0f, 0.0f, 1.0f)) << "Missing South (Z+) normal";
    EXPECT_TRUE(hasNormalDirection(vertices, indices, 0.0f, 0.0f, -1.0f)) << "Missing North (Z-) normal";
    EXPECT_TRUE(hasNormalDirection(vertices, indices, 1.0f, 0.0f, 0.0f)) << "Missing East (X+) normal";
    EXPECT_TRUE(hasNormalDirection(vertices, indices, -1.0f, 0.0f, 0.0f)) << "Missing West (X-) normal";
    EXPECT_TRUE(hasNormalDirection(vertices, indices, 0.0f, 1.0f, 0.0f)) << "Missing Up (Y+) normal";
    EXPECT_TRUE(hasNormalDirection(vertices, indices, 0.0f, -1.0f, 0.0f)) << "Missing Down (Y-) normal";
}

/**
 * @brief 回退立方体恰好有6个唯一法线方向
 */
TEST_F(FallbackCubeTest, FallbackCubeHasExactlySixUniqueNormals)
{
    mc::ItemStack stack(*s_testItem, 1);
    auto [vertices, indices] = ItemMeshBuilder::buildHeldItemMesh(stack, ItemTransformType::Gui);

    auto normals = collectUniqueNormals(vertices, indices);
    EXPECT_EQ(normals.size(), 6u);
}

/**
 * @brief 回退立方体顶点位置范围在 [-0.5, 0.5] 内
 *
 * _buildFallbackMesh 使用 halfSize = ITEM_SCALE * 16.0 * 0.5 = 0.5
 * 所以立方体范围是 [-0.5, 0.5]
 */
TEST_F(FallbackCubeTest, FallbackCubeVertexPositionsInExpectedRange)
{
    mc::ItemStack stack(*s_testItem, 1);
    auto [vertices, indices] = ItemMeshBuilder::buildHeldItemMesh(stack, ItemTransformType::Gui);

    for (const auto& v : vertices) {
        EXPECT_GE(v.position.x, -0.51f) << "Vertex X below -0.5";
        EXPECT_LE(v.position.x, 0.51f) << "Vertex X above 0.5";
        EXPECT_GE(v.position.y, -0.51f) << "Vertex Y below -0.5";
        EXPECT_LE(v.position.y, 0.51f) << "Vertex Y above 0.5";
        EXPECT_GE(v.position.z, -0.51f) << "Vertex Z below -0.5";
        EXPECT_LE(v.position.z, 0.51f) << "Vertex Z above 0.5";
    }
}

/**
 * @brief 回退立方体每个面的UV坐标覆盖完整纹理范围 [0,1]
 */
TEST_F(FallbackCubeTest, FallbackCubeUVsCoverFullTextureRange)
{
    mc::ItemStack stack(*s_testItem, 1);
    auto [vertices, indices] = ItemMeshBuilder::buildHeldItemMesh(stack, ItemTransformType::Gui);

    ASSERT_EQ(vertices.size(), 24u);

    // 检查存在 u=0 和 u=1 的顶点
    bool hasU0 = false, hasU1 = false, hasV0 = false, hasV1 = false;
    for (const auto& v : vertices) {
        if (std::abs(v.texCoord.x - 0.0f) < 0.001f) hasU0 = true;
        if (std::abs(v.texCoord.x - 1.0f) < 0.001f) hasU1 = true;
        if (std::abs(v.texCoord.y - 0.0f) < 0.001f) hasV0 = true;
        if (std::abs(v.texCoord.y - 1.0f) < 0.001f) hasV1 = true;
    }
    EXPECT_TRUE(hasU0) << "No vertex with U=0 found";
    EXPECT_TRUE(hasU1) << "No vertex with U=1 found";
    EXPECT_TRUE(hasV0) << "No vertex with V=0 found";
    EXPECT_TRUE(hasV1) << "No vertex with V=1 found";
}

/**
 * @brief buildGroundItemMesh 同样能触发回退网格6面立方体
 */
TEST_F(FallbackCubeTest, GroundItemMeshGeneratesSixFaceCube)
{
    mc::ItemStack stack(*s_testItem, 1);
    auto [vertices, indices] = ItemMeshBuilder::buildGroundItemMesh(stack, 0.0);

    // 6面 × 4顶点 = 24 顶点
    EXPECT_EQ(vertices.size(), 24u);
    // 6面 × 6索引 = 36 索引
    EXPECT_EQ(indices.size(), 36u);

    // 验证法线方向
    EXPECT_TRUE(hasNormalDirection(vertices, indices, 0.0f, 0.0f, 1.0f));
    EXPECT_TRUE(hasNormalDirection(vertices, indices, 0.0f, 0.0f, -1.0f));
    EXPECT_TRUE(hasNormalDirection(vertices, indices, 1.0f, 0.0f, 0.0f));
    EXPECT_TRUE(hasNormalDirection(vertices, indices, -1.0f, 0.0f, 0.0f));
    EXPECT_TRUE(hasNormalDirection(vertices, indices, 0.0f, 1.0f, 0.0f));
    EXPECT_TRUE(hasNormalDirection(vertices, indices, 0.0f, -1.0f, 0.0f));
}

// ============================================================================
// 纹理解析路径测试
// 验证 ItemTextureAtlas 为 nullptr 时 _buildGeneratedMesh 的安全行为
// ============================================================================

/**
 * @brief 当 ItemTextureAtlas 为 nullptr 时，纹理解析路径不会崩溃
 *
 * setItemTextureAtlas(nullptr) 后，_buildGeneratedMesh 应安全跳过纹理解析，
 * 使用默认 UV 坐标或不生成纹理层。
 */
TEST(ItemTextureResolutionTest, NullAtlasDoesNotCrashGeneratedMesh)
{
    // 确保 atlas 为 nullptr
    ItemMeshBuilder::setItemTextureAtlas(nullptr);

    // 注册一个测试物品（即使有 atlas 为空，buildHeldItemMesh 仍应正常工作）
    auto& testItem = registerTestItem(mc::ResourceLocation("test", "null_atlas_item"));
    mc::ItemStack stack(testItem, 1);

    // 不应崩溃 - 如果 ItemModelCache 未初始化，走回退路径
    auto [vertices, indices] = ItemMeshBuilder::buildHeldItemMesh(stack, ItemTransformType::Gui);

    // 回退路径应成功生成网格
    EXPECT_FALSE(vertices.empty());
    EXPECT_FALSE(indices.empty());
}

} // namespace
} // namespace mc::client::renderer::trident::item
