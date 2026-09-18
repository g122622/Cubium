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

#include "client/renderer/trident/firstperson/ItemInHandRenderer.hpp"
#include "client/renderer/trident/firstperson/MatrixStack.hpp"
#include "client/resource/ItemModelLoader.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItem.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc::client::renderer::trident::firstperson;
using namespace mc::client::renderer;
using namespace mc;

class ItemInHandRendererTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        renderer = std::make_unique<ItemInHandRenderer>();
        renderer->initialize();
        stack = std::make_unique<MatrixStack>();
    }

    void TearDown() override
    {
        renderer.reset();
        stack.reset();
    }

    std::unique_ptr<ItemInHandRenderer> renderer;
    std::unique_ptr<MatrixStack> stack;
};

// ============================================================================
// 初始化测试
// ============================================================================

TEST(ItemInHandRendererConstruction, InitialState_NotInitialized)
{
    auto fresh = std::make_unique<ItemInHandRenderer>();
    EXPECT_FALSE(fresh->isInitialized());
}

TEST(ItemInHandRendererConstruction, Initialize_Success)
{
    auto fresh = std::make_unique<ItemInHandRenderer>();
    auto result = fresh->initialize();
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(fresh->isInitialized());
}

TEST_F(ItemInHandRendererTest, GetTransforms_ReturnsReference)
{
    // 不需要初始化就能获取变换
    const ItemCameraTransforms& transforms = renderer->transforms();
    EXPECT_TRUE(transforms.getTransform(TransformType::None).isDefault());
}

// ============================================================================
// 方块物品检测测试
// ============================================================================

TEST_F(ItemInHandRendererTest, IsBlockItem_EmptyStack_ReturnsFalse)
{
    ItemStack emptyStack;
    EXPECT_FALSE(ItemInHandRenderer::isBlockItem(emptyStack));
}

TEST_F(ItemInHandRendererTest, IsBlockItem_NullItem_ReturnsFalse)
{
    // ItemStack 默认构造为空，getItem() 返回 nullptr
    ItemStack stack;
    EXPECT_FALSE(ItemInHandRenderer::isBlockItem(stack));
}

// ============================================================================
// 模型获取测试
// ============================================================================

TEST_F(ItemInHandRendererTest, GetItemModel_EmptyStack_ReturnsNull)
{
    ItemStack emptyStack;
    const auto* model = ItemInHandRenderer::getItemModel(emptyStack);
    EXPECT_EQ(model, nullptr);
}

// ============================================================================
// 默认变换测试
// ============================================================================

TEST_F(ItemInHandRendererTest, ApplyDefaultTransform_ThirdPersonRight)
{
    renderer->applyDefaultTransform(*stack, TransformType::ThirdPersonRightHand, false);

    const Matrix4f& matrix = stack->last();
    // 验证缩放
    EXPECT_NEAR(matrix(0, 0), 0.55f, 0.001f);
    EXPECT_NEAR(matrix(1, 1), 0.55f, 0.001f);
    EXPECT_NEAR(matrix(2, 2), 0.55f, 0.001f);
    // 验证平移（像素单位除以 16）
    EXPECT_NEAR(matrix(1, 3), 3.0f / 16.0f, 0.001f);
    EXPECT_NEAR(matrix(2, 3), 1.0f / 16.0f, 0.001f);
}

TEST_F(ItemInHandRendererTest, ApplyDefaultTransform_ThirdPersonLeft)
{
    renderer->applyDefaultTransform(*stack, TransformType::ThirdPersonLeftHand, true);

    const Matrix4f& matrix = stack->last();
    // 验证缩放
    EXPECT_NEAR(matrix(0, 0), 0.55f, 0.001f);
    EXPECT_NEAR(matrix(1, 1), 0.55f, 0.001f);
    EXPECT_NEAR(matrix(2, 2), 0.55f, 0.001f);
}

TEST_F(ItemInHandRendererTest, ApplyDefaultTransform_FirstPersonRight)
{
    renderer->applyDefaultTransform(*stack, TransformType::FirstPersonRightHand, false);

    const Matrix4f& matrix = stack->last();

    // 第一人称右手有 Y 轴 45 度旋转，所以不能简单验证对角线
    // 验证平移分量（像素单位除以 16）
    Vector3f translation = matrix.translation();
    EXPECT_NEAR(translation.x, 1.13f / 16.0f, 0.01f);
    EXPECT_NEAR(translation.y, 3.2f / 16.0f, 0.01f);
    EXPECT_NEAR(translation.z, 1.13f / 16.0f, 0.01f);

    // 验证矩阵不是单位矩阵（有变换应用）
    EXPECT_FALSE(std::abs(matrix(0, 0) - 1.0f) < 0.001f && std::abs(matrix(1, 1) - 1.0f) < 0.001f &&
        std::abs(matrix(2, 2) - 1.0f) < 0.001f);
}

TEST_F(ItemInHandRendererTest, ApplyDefaultTransform_FirstPersonLeft)
{
    renderer->applyDefaultTransform(*stack, TransformType::FirstPersonLeftHand, true);

    const Matrix4f& matrix = stack->last();

    // 左手平移 X 为负值（像素单位除以 16）
    EXPECT_NEAR(matrix(0, 3), -1.13f / 16.0f, 0.01f);
    EXPECT_NEAR(matrix(1, 3), 3.2f / 16.0f, 0.01f);
    EXPECT_NEAR(matrix(2, 3), 1.13f / 16.0f, 0.01f);
}

TEST_F(ItemInHandRendererTest, ApplyDefaultTransform_Gui)
{
    renderer->applyDefaultTransform(*stack, TransformType::Gui, false);

    const Matrix4f& matrix = stack->last();

    // GUI 有 X 轴 30 度和 Y 轴 225 度旋转，所以不能简单验证对角线
    // 验证矩阵不是单位矩阵（有变换应用）
    EXPECT_FALSE(std::abs(matrix(0, 0) - 1.0f) < 0.001f && std::abs(matrix(1, 1) - 1.0f) < 0.001f &&
        std::abs(matrix(2, 2) - 1.0f) < 0.001f);
}

TEST_F(ItemInHandRendererTest, ApplyDefaultTransform_Ground)
{
    renderer->applyDefaultTransform(*stack, TransformType::Ground, false);

    const Matrix4f& matrix = stack->last();

    // 验证缩放
    EXPECT_NEAR(matrix(0, 0), 0.25f, 0.001f);
    EXPECT_NEAR(matrix(1, 1), 0.25f, 0.001f);
    EXPECT_NEAR(matrix(2, 2), 0.25f, 0.001f);
}

TEST_F(ItemInHandRendererTest, ApplyDefaultTransform_Fixed)
{
    renderer->applyDefaultTransform(*stack, TransformType::Fixed, false);

    const Matrix4f& matrix = stack->last();

    // 验证缩放
    EXPECT_NEAR(matrix(0, 0), 0.5f, 0.001f);
    EXPECT_NEAR(matrix(1, 1), 0.5f, 0.001f);
    EXPECT_NEAR(matrix(2, 2), 0.5f, 0.001f);
}

TEST_F(ItemInHandRendererTest, ApplyDefaultTransform_Head)
{
    renderer->applyDefaultTransform(*stack, TransformType::Head, false);

    const Matrix4f& matrix = stack->last();

    // 头部变换：Y轴旋转180度，所以矩阵的 X 和 Z 对角线会变成 -1
    // 旋转 180 度后: cos(180°) = -1, sin(180°) = 0
    EXPECT_NEAR(matrix(0, 0), -1.0f, 0.001f);
    EXPECT_NEAR(matrix(1, 1), 1.0f, 0.001f); // Y 轴不受影响
    EXPECT_NEAR(matrix(2, 2), -1.0f, 0.001f);
}

TEST_F(ItemInHandRendererTest, ApplyDefaultTransform_None_NoChange)
{
    Matrix4f before = stack->last();
    renderer->applyDefaultTransform(*stack, TransformType::None, false);
    Matrix4f after = stack->last();

    // None 类型应该不改变矩阵
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_NEAR(after(i, j), before(i, j), 0.001f);
        }
    }
}

// ============================================================================
// applyTransform 测试
// ============================================================================

TEST_F(ItemInHandRendererTest, ApplyTransform_EmptyStack_UsesDefault)
{
    ItemStack emptyStack;
    bool result = renderer->applyTransform(*stack, emptyStack, TransformType::Gui, false);

    // 空物品堆应该使用默认变换
    EXPECT_FALSE(result);

    // 验证矩阵已被修改（有旋转）
    const Matrix4f& matrix = stack->last();
    EXPECT_FALSE(std::abs(matrix(0, 0) - 1.0f) < 0.001f && std::abs(matrix(1, 1) - 1.0f) < 0.001f &&
        std::abs(matrix(2, 2) - 1.0f) < 0.001f);
}

// ============================================================================
// TransformType 辅助函数测试
// ============================================================================

TEST_F(ItemInHandRendererTest, IsFirstPerson_TrueForFirstPersonTypes)
{
    EXPECT_TRUE(isFirstPerson(TransformType::FirstPersonLeftHand));
    EXPECT_TRUE(isFirstPerson(TransformType::FirstPersonRightHand));
    EXPECT_FALSE(isFirstPerson(TransformType::ThirdPersonLeftHand));
    EXPECT_FALSE(isFirstPerson(TransformType::ThirdPersonRightHand));
    EXPECT_FALSE(isFirstPerson(TransformType::Gui));
}

TEST_F(ItemInHandRendererTest, IsThirdPerson_TrueForThirdPersonTypes)
{
    EXPECT_TRUE(isThirdPerson(TransformType::ThirdPersonLeftHand));
    EXPECT_TRUE(isThirdPerson(TransformType::ThirdPersonRightHand));
    EXPECT_FALSE(isThirdPerson(TransformType::FirstPersonLeftHand));
    EXPECT_FALSE(isThirdPerson(TransformType::FirstPersonRightHand));
    EXPECT_FALSE(isThirdPerson(TransformType::Gui));
}

TEST_F(ItemInHandRendererTest, IsLeftHand_TrueForLeftHandTypes)
{
    EXPECT_TRUE(isLeftHand(TransformType::FirstPersonLeftHand));
    EXPECT_TRUE(isLeftHand(TransformType::ThirdPersonLeftHand));
    EXPECT_FALSE(isLeftHand(TransformType::FirstPersonRightHand));
    EXPECT_FALSE(isLeftHand(TransformType::ThirdPersonRightHand));
    EXPECT_FALSE(isLeftHand(TransformType::Gui));
}

// ============================================================================
// ItemCameraTransforms 测试
// ============================================================================

TEST_F(ItemInHandRendererTest, ItemCameraTransforms_GetTransform)
{
    ItemCameraTransforms transforms;

    // 获取各类型变换
    const ItemTransform& gui = transforms.getTransform(TransformType::Gui);
    const ItemTransform& ground = transforms.getTransform(TransformType::Ground);
    const ItemTransform& fixed = transforms.getTransform(TransformType::Fixed);

    // 默认构造应该都是默认值
    EXPECT_TRUE(gui.isDefault());
    EXPECT_TRUE(ground.isDefault());
    EXPECT_TRUE(fixed.isDefault());
}

TEST_F(ItemInHandRendererTest, ItemCameraTransforms_HasCustomTransform)
{
    ItemCameraTransforms transforms;

    // 默认构造应该没有自定义变换
    EXPECT_FALSE(transforms.hasCustomTransform(TransformType::Gui));
    EXPECT_FALSE(transforms.hasCustomTransform(TransformType::Ground));
}

TEST_F(ItemInHandRendererTest, ItemTransform_IsDefault)
{
    // 默认构造的变换应该是默认值
    ItemTransform identity;
    EXPECT_TRUE(identity.isDefault());

    // 有任何变化的变换应该不是默认值
    ItemTransform translated(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    EXPECT_FALSE(translated.isDefault());

    ItemTransform rotated(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_FALSE(rotated.isDefault());

    ItemTransform scaled(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 1.0f, 1.0f);
    EXPECT_FALSE(scaled.isDefault());
}

TEST_F(ItemInHandRendererTest, ItemTransform_Apply)
{
    // 使用无旋转的变换来测试平移和缩放
    ItemTransform transform(0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f, 0.5f, 0.5f, 0.5f);
    MatrixStack testStack;

    transform.apply(testStack);

    // 验证矩阵被修改
    const Matrix4f& matrix = testStack.last();

    // 无旋转时，缩放直接应用于对角线
    EXPECT_NEAR(matrix(0, 0), 0.5f, 0.01f);
    EXPECT_NEAR(matrix(1, 1), 0.5f, 0.01f);
    EXPECT_NEAR(matrix(2, 2), 0.5f, 0.01f);

    // ItemTransform::apply 不除以 16，直接使用平移值
    Vector3f translation = matrix.translation();
    EXPECT_NEAR(translation.x, 1.0f, 0.01f);
    EXPECT_NEAR(translation.y, 2.0f, 0.01f);
    EXPECT_NEAR(translation.z, 3.0f, 0.01f);
}

// ============================================================================
// 销毁测试
// ============================================================================

TEST_F(ItemInHandRendererTest, Destroy_ResetsState)
{
    renderer->destroy();
    EXPECT_FALSE(renderer->isInitialized());
}
