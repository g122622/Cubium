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

// D12 第一人称手持物品 3D 升级测试。
//
// 验证 ItemMeshBuilder::buildHeldItemMesh 的"不烘焙变换"原始变体（bakeTransforms=false）
// 与默认烘焙变体（bakeTransforms=true）的几何差异，确保第一人称路径能在矩阵栈上
// 由 ItemInHandRenderer::applyTransform 单独施加 display 变换，不会出现双重施加。
//
// 由于单元测试不初始化 ItemModelCache，getItemModel() 返回 nullptr，会触发
// _buildFallbackMesh 6 面立方体路径——这与既有 ItemMeshBuilderTest 的回退测试一致，
// 足以覆盖"原始变体跳过 getItemTransform 摄像机矩阵"这一核心逻辑。

#include <gtest/gtest.h>

#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/item/ItemMeshBuilder.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"

#include <cmath>

using namespace mc::client::renderer::entity::item;
using namespace mc::client::renderer::entity::model;

namespace mc::client::renderer::trident::firstperson {
namespace {

mc::Item& registerTestItem(const mc::ResourceLocation& id)
{
    return mc::ItemRegistry::instance().registerItem(id, mc::ItemProperties().maxStackSize(64));
}

class HeldItemMesh3DTest : public ::testing::Test {
protected:
    static mc::Item* s_testItem;

    static void SetUpTestSuite() { s_testItem = &registerTestItem(mc::ResourceLocation("test", "held_mesh_3d_item")); }

    void SetUp() override { ItemMeshBuilder::setItemTextureAtlas(nullptr); }

    void TearDown() override { ItemMeshBuilder::setItemTextureAtlas(nullptr); }
};

mc::Item* HeldItemMesh3DTest::s_testItem = nullptr;

// 原始变体（bakeTransforms=false）也应生成 6 面回退立方体——即拓扑与默认变体一致。
TEST_F(HeldItemMesh3DTest, RawVariantGeneratesFallbackCubeTopology)
{
    mc::ItemStack stack(*s_testItem, 1);
    auto [vertices, indices] =
        ItemMeshBuilder::buildHeldItemMesh(stack, ItemTransformType::FirstPersonRightHand, false);

    EXPECT_EQ(vertices.size(), 24u);
    EXPECT_EQ(indices.size(), 36u);
}

// 原始变体跳过 getItemTransform 摄像机矩阵，顶点应停留在回退立方体的原始 [-0.5,0.5] 范围；
// 默认变体经过 FirstPersonRightHand 变换（含 Z=-0.72 平移与 45° 旋转）后顶点 Z 会显著偏移。
// 以此区分两者，证明 bakeTransforms=false 确实未施加摄像机变换。
TEST_F(HeldItemMesh3DTest, RawVariantSkipsCameraTransformBaking)
{
    mc::ItemStack stack(*s_testItem, 1);

    auto [rawVerts, rawIndices] =
        ItemMeshBuilder::buildHeldItemMesh(stack, ItemTransformType::FirstPersonRightHand, false);
    auto [bakedVerts, bakedIndices] =
        ItemMeshBuilder::buildHeldItemMesh(stack, ItemTransformType::FirstPersonRightHand, true);

    ASSERT_FALSE(rawVerts.empty());
    ASSERT_FALSE(bakedVerts.empty());

    // 原始变体顶点 Z 全部在 [-0.5, 0.5]（回退立方体原始范围）。
    for (const auto& v : rawVerts) {
        EXPECT_GE(v.position.z, -0.51f);
        EXPECT_LE(v.position.z, 0.51f);
    }

    // 默认变体经过 FirstPersonRightHand 变换后，必然存在顶点 Z < -0.5
    // （Z=-0.72 平移将立方体推到相机后方）。
    bool hasShiftedZ = false;
    for (const auto& v : bakedVerts) {
        if (v.position.z < -0.55f) {
            hasShiftedZ = true;
            break;
        }
    }
    EXPECT_TRUE(hasShiftedZ) << "Baked variant should have vertices shifted behind camera (Z<-0.55)";
}

// 左手原始变体应与右手原始变体几何相同（镜像只发生在 getItemTransform 摄像机矩阵中，
// bakeTransforms=false 跳过该矩阵，故左右手原始几何一致）。
TEST_F(HeldItemMesh3DTest, RawVariantLeftHandMatchesRightHandGeometry)
{
    mc::ItemStack stack(*s_testItem, 1);

    auto [rightVerts, rightIndices] =
        ItemMeshBuilder::buildHeldItemMesh(stack, ItemTransformType::FirstPersonRightHand, false);
    auto [leftVerts, leftIndices] =
        ItemMeshBuilder::buildHeldItemMesh(stack, ItemTransformType::FirstPersonLeftHand, false);

    ASSERT_EQ(rightVerts.size(), leftVerts.size());
    for (size_t i = 0; i < rightVerts.size(); ++i) {
        EXPECT_FLOAT_EQ(rightVerts[i].position.x, leftVerts[i].position.x);
        EXPECT_FLOAT_EQ(rightVerts[i].position.y, leftVerts[i].position.y);
        EXPECT_FLOAT_EQ(rightVerts[i].position.z, leftVerts[i].position.z);
    }
    (void)rightIndices;
    (void)leftIndices;
}

// 空物品堆在原始变体下同样返回空网格。
TEST_F(HeldItemMesh3DTest, EmptyItemStackRawReturnsEmpty)
{
    mc::ItemStack emptyStack;
    auto [vertices, indices] =
        ItemMeshBuilder::buildHeldItemMesh(emptyStack, ItemTransformType::FirstPersonRightHand, false);
    EXPECT_TRUE(vertices.empty());
    EXPECT_TRUE(indices.empty());
}

} // namespace
} // namespace mc::client::renderer::trident::firstperson
