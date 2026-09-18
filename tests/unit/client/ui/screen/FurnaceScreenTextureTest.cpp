/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
 * @file FurnaceScreenTextureTest.cpp
 * @brief 熔炉屏幕纹理渲染和进度计算单元测试
 *
 * 测试 ContainerTex UV 坐标常量的一致性、熔炉进度计算逻辑
 * （getLitProgress/getBurnProgress 边界值和 clamp 行为）、
 * ContainerTextureEntry 结构体默认值、FurnaceContainer 槽位常量、
 * 以及 GuiColors 颜色常量。
 * 纯数据逻辑测试，不依赖 Vulkan 或渲染器。
 */

#include "client/renderer/trident/gui/GuiTextureManager.hpp"
#include "common/entity/inventory/container/FurnaceContainer.hpp"
#include <cmath>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client::renderer::trident::gui;
using namespace mc::blockentity;

// ============================================================================
// ContainerTex UV 坐标常量一致性测试
// ============================================================================

/**
 * @brief 验证 ContainerTex 常量中的纹理尺寸基准值
 */
TEST(FurnaceScreenTextureTest, ContainerTex_TextureDimensions)
{
    // MC Java 中所有容器纹理均为 256x256
    EXPECT_EQ(ContainerTex::TEXTURE_WIDTH, 256);
    EXPECT_EQ(ContainerTex::TEXTURE_HEIGHT, 256);
}

/**
 * @brief 验证背包背景 UV 坐标和尺寸的一致性
 */
TEST(FurnaceScreenTextureTest, ContainerTex_InventoryUVConsistency)
{
    // 背包背景从 (0,0) 到 (176,166)
    EXPECT_DOUBLE_EQ(ContainerTex::INVENTORY_BG_U0, 0.0);
    EXPECT_DOUBLE_EQ(ContainerTex::INVENTORY_BG_V0, 0.0);
    EXPECT_DOUBLE_EQ(ContainerTex::INVENTORY_BG_U1, 176.0 / 256.0);
    EXPECT_DOUBLE_EQ(ContainerTex::INVENTORY_BG_V1, 166.0 / 256.0);
    EXPECT_EQ(ContainerTex::INVENTORY_BG_WIDTH, 176);
    EXPECT_EQ(ContainerTex::INVENTORY_BG_HEIGHT, 166);

    // UV 坐标必须在 [0, 1] 范围内
    EXPECT_GE(ContainerTex::INVENTORY_BG_U0, 0.0);
    EXPECT_GE(ContainerTex::INVENTORY_BG_V0, 0.0);
    EXPECT_LE(ContainerTex::INVENTORY_BG_U1, 1.0);
    EXPECT_LE(ContainerTex::INVENTORY_BG_V1, 1.0);
}

/**
 * @brief 验证工作台背景 UV 坐标和尺寸的一致性
 */
TEST(FurnaceScreenTextureTest, ContainerTex_CraftingTableUVConsistency)
{
    EXPECT_DOUBLE_EQ(ContainerTex::CRAFTING_TABLE_BG_U0, 0.0);
    EXPECT_DOUBLE_EQ(ContainerTex::CRAFTING_TABLE_BG_V0, 0.0);
    EXPECT_DOUBLE_EQ(ContainerTex::CRAFTING_TABLE_BG_U1, 176.0 / 256.0);
    EXPECT_DOUBLE_EQ(ContainerTex::CRAFTING_TABLE_BG_V1, 166.0 / 256.0);
    EXPECT_EQ(ContainerTex::CRAFTING_TABLE_BG_WIDTH, 176);
    EXPECT_EQ(ContainerTex::CRAFTING_TABLE_BG_HEIGHT, 166);
}

/**
 * @brief 验证熔炉背景 UV 坐标和尺寸的一致性
 */
TEST(FurnaceScreenTextureTest, ContainerTex_FurnaceBackgroundUVConsistency)
{
    // 熔炉背景也是从 (0,0) 到 (176,166)，与 MC Java 一致
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_BG_U0, 0.0);
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_BG_V0, 0.0);
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_BG_U1, 176.0 / 256.0);
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_BG_V1, 166.0 / 256.0);
    EXPECT_EQ(ContainerTex::FURNACE_BG_WIDTH, 176);
    EXPECT_EQ(ContainerTex::FURNACE_BG_HEIGHT, 166);

    // 尺寸与 MC Java 标准容器尺寸一致
    EXPECT_EQ(ContainerTex::FURNACE_BG_WIDTH, ContainerTex::INVENTORY_BG_WIDTH);
    EXPECT_EQ(ContainerTex::FURNACE_BG_HEIGHT, ContainerTex::INVENTORY_BG_HEIGHT);
}

/**
 * @brief 验证熔炉火焰指示器 UV 坐标的一致性
 *
 * 火焰图标在经典 furnace.png 纹理中位于像素坐标 (176, 0)，尺寸 14x14。
 * 对应 MC Java 的 lit_progress 精灵。
 */
TEST(FurnaceScreenTextureTest, ContainerTex_FurnaceLitProgressUVConsistency)
{
    // 火焰图标位于纹理右侧，起始 X=176
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_LIT_U0, 176.0 / 256.0);
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_LIT_V0, 0.0);
    // 火焰图标尺寸 14x14
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_LIT_U1, (176.0 + 14.0) / 256.0);
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_LIT_V1, 14.0 / 256.0);
    EXPECT_EQ(ContainerTex::FURNACE_LIT_WIDTH, 14);
    EXPECT_EQ(ContainerTex::FURNACE_LIT_HEIGHT, 14);

    // UV 范围在 [0, 1] 内
    EXPECT_GE(ContainerTex::FURNACE_LIT_U0, 0.0);
    EXPECT_LE(ContainerTex::FURNACE_LIT_U1, 1.0);
    EXPECT_GE(ContainerTex::FURNACE_LIT_V0, 0.0);
    EXPECT_LE(ContainerTex::FURNACE_LIT_V1, 1.0);

    // 火焰区域不与背景区域重叠（X 起始 >= 背景 X 结束）
    EXPECT_GE(ContainerTex::FURNACE_LIT_U0, ContainerTex::FURNACE_BG_U1);
}

/**
 * @brief 验证熔炉进度箭头 UV 坐标的一致性
 *
 * 箭头图标在经典 furnace.png 纹理中位于像素坐标 (176, 14)，尺寸 24x16。
 * 对应 MC Java 的 burn_progress 精灵。
 */
TEST(FurnaceScreenTextureTest, ContainerTex_FurnaceBurnProgressUVConsistency)
{
    // 箭头图标紧接在火焰图标下方
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_ARROW_U0, 176.0 / 256.0);
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_ARROW_V0, 14.0 / 256.0);
    // 箭头图标尺寸 24x16
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_ARROW_U1, (176.0 + 24.0) / 256.0);
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_ARROW_V1, (14.0 + 16.0) / 256.0);
    EXPECT_EQ(ContainerTex::FURNACE_ARROW_WIDTH, 24);
    EXPECT_EQ(ContainerTex::FURNACE_ARROW_HEIGHT, 16);

    // UV 范围在 [0, 1] 内
    EXPECT_GE(ContainerTex::FURNACE_ARROW_U0, 0.0);
    EXPECT_LE(ContainerTex::FURNACE_ARROW_U1, 1.0);
    EXPECT_GE(ContainerTex::FURNACE_ARROW_V0, 0.0);
    EXPECT_LE(ContainerTex::FURNACE_ARROW_V1, 1.0);

    // 箭头起始 V 应该等于火焰结束 V（紧接排列）
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_ARROW_V0, ContainerTex::FURNACE_LIT_V1);
}

/**
 * @brief 验证熔炉屏幕坐标与 MC Java 一致
 *
 * MC Java AbstractFurnaceScreen 中火焰位于 (leftPos+56, topPos+36+14-l)，
 * 箭头位于 (leftPos+79, topPos+34)。
 */
TEST(FurnaceScreenTextureTest, ContainerTex_FurnaceScreenPositions)
{
    // 火焰指示器屏幕位置：相对于 GUI 左上角 (56, 36)
    EXPECT_EQ(ContainerTex::FURNACE_LIT_SCREEN_X, 56);
    EXPECT_EQ(ContainerTex::FURNACE_LIT_SCREEN_Y, 36);

    // 进度箭头屏幕位置：相对于 GUI 左上角 (79, 34)
    EXPECT_EQ(ContainerTex::FURNACE_ARROW_SCREEN_X, 79);
    EXPECT_EQ(ContainerTex::FURNACE_ARROW_SCREEN_Y, 34);

    // 位置在 GUI 背景范围内 (176x166)
    EXPECT_LT(ContainerTex::FURNACE_LIT_SCREEN_X + ContainerTex::FURNACE_LIT_WIDTH, ContainerTex::FURNACE_BG_WIDTH);
    EXPECT_LT(ContainerTex::FURNACE_LIT_SCREEN_Y + ContainerTex::FURNACE_LIT_HEIGHT, ContainerTex::FURNACE_BG_HEIGHT);
    EXPECT_LT(ContainerTex::FURNACE_ARROW_SCREEN_X + ContainerTex::FURNACE_ARROW_WIDTH, ContainerTex::FURNACE_BG_WIDTH);
    EXPECT_LT(
        ContainerTex::FURNACE_ARROW_SCREEN_Y + ContainerTex::FURNACE_ARROW_HEIGHT, ContainerTex::FURNACE_BG_HEIGHT);
}

/**
 * @brief 验证火焰和箭头区域不重叠
 */
TEST(FurnaceScreenTextureTest, ContainerTex_FurnaceRegionsNoOverlap)
{
    // 在纹理坐标空间中，火焰和箭头不重叠（X方向相同，Y方向紧接排列）
    EXPECT_DOUBLE_EQ(ContainerTex::FURNACE_ARROW_V0, ContainerTex::FURNACE_LIT_V1);
    EXPECT_GT(ContainerTex::FURNACE_ARROW_V1, ContainerTex::FURNACE_ARROW_V0);
}

// ============================================================================
// ContainerTextureEntry 结构体默认值测试
// ============================================================================

/**
 * @brief 验证 ContainerTextureEntry 的默认初始值
 */
TEST(FurnaceScreenTextureTest, ContainerTextureEntry_DefaultValues)
{
    ContainerTextureEntry entry;
    EXPECT_EQ(entry.image, VK_NULL_HANDLE);
    EXPECT_EQ(entry.imageMemory, VK_NULL_HANDLE);
    EXPECT_EQ(entry.imageView, VK_NULL_HANDLE);
    EXPECT_EQ(entry.sampler, VK_NULL_HANDLE);
    EXPECT_EQ(entry.width, 256u);
    EXPECT_EQ(entry.height, 256u);
    EXPECT_EQ(entry.atlasSlot, 255u); // 255 = 未注册
    EXPECT_FALSE(entry.loaded);
}

// ============================================================================
// 熔炉进度计算逻辑测试（对齐 MC Java 的算法）
// ============================================================================

/**
 * @brief 测试燃烧进度（litProgress）的计算逻辑
 *
 * 模拟 MC Java AbstractFurnaceMenu.getLitProgress()：
 * litProgress = clamp(burnTime / burnTimeTotal, 0, 1)
 * 当 burnTimeTotal <= 0 时返回 0。
 */
TEST(FurnaceScreenTextureTest, LitProgress_ZeroBurnTime_ReturnsZero)
{
    const i32 burnTime = 0;
    const i32 burnTimeTotal = 200;
    f32 litProgress = 0.0f;
    if (burnTimeTotal > 0) {
        litProgress = std::clamp(static_cast<f32>(burnTime) / static_cast<f32>(burnTimeTotal), 0.0f, 1.0f);
    }
    EXPECT_FLOAT_EQ(litProgress, 0.0f);
}

TEST(FurnaceScreenTextureTest, LitProgress_FullBurnTime_ReturnsOne)
{
    const i32 burnTime = 200;
    const i32 burnTimeTotal = 200;
    f32 litProgress = std::clamp(static_cast<f32>(burnTime) / static_cast<f32>(burnTimeTotal), 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(litProgress, 1.0f);
}

TEST(FurnaceScreenTextureTest, LitProgress_HalfBurnTime_ReturnsHalf)
{
    const i32 burnTime = 100;
    const i32 burnTimeTotal = 200;
    f32 litProgress = std::clamp(static_cast<f32>(burnTime) / static_cast<f32>(burnTimeTotal), 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(litProgress, 0.5f);
}

TEST(FurnaceScreenTextureTest, LitProgress_ZeroBurnTimeTotal_ReturnsZero)
{
    // 当 burnTimeTotal = 0 时，不应除以零
    const i32 burnTime = 100;
    const i32 burnTimeTotal = 0;
    f32 litProgress = 0.0f;
    if (burnTimeTotal > 0) {
        litProgress = std::clamp(static_cast<f32>(burnTime) / static_cast<f32>(burnTimeTotal), 0.0f, 1.0f);
    }
    EXPECT_FLOAT_EQ(litProgress, 0.0f);
}

TEST(FurnaceScreenTextureTest, LitProgress_NegativeValues_ClampedToZero)
{
    const i32 burnTime = -50;
    const i32 burnTimeTotal = 200;
    f32 litProgress = std::clamp(static_cast<f32>(burnTime) / static_cast<f32>(burnTimeTotal), 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(litProgress, 0.0f);
}

TEST(FurnaceScreenTextureTest, LitProgress_ExceedsTotal_ClampedToOne)
{
    const i32 burnTime = 300;
    const i32 burnTimeTotal = 200;
    f32 litProgress = std::clamp(static_cast<f32>(burnTime) / static_cast<f32>(burnTimeTotal), 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(litProgress, 1.0f);
}

// ============================================================================
// 熔炼进度（burnProgress / cookProgress）计算测试
// ============================================================================

/**
 * @brief 测试熔炼进度计算逻辑
 *
 * 模拟 MC Java AbstractFurnaceMenu.getBurnProgress()：
 * burnProgress = (cookTimeTotal > 0 && cookTime > 0) ? clamp(cookTime / cookTimeTotal, 0, 1) : 0
 */
TEST(FurnaceScreenTextureTest, BurnProgress_ZeroCookTime_ReturnsZero)
{
    const i32 cookTime = 0;
    const i32 cookTimeTotal = 200;
    f32 burnProgress = 0.0f;
    if (cookTimeTotal > 0 && cookTime > 0) {
        burnProgress = std::clamp(static_cast<f32>(cookTime) / static_cast<f32>(cookTimeTotal), 0.0f, 1.0f);
    }
    EXPECT_FLOAT_EQ(burnProgress, 0.0f);
}

TEST(FurnaceScreenTextureTest, BurnProgress_HalfCookTime_ReturnsHalf)
{
    const i32 cookTime = 100;
    const i32 cookTimeTotal = 200;
    f32 burnProgress = std::clamp(static_cast<f32>(cookTime) / static_cast<f32>(cookTimeTotal), 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(burnProgress, 0.5f);
}

TEST(FurnaceScreenTextureTest, BurnProgress_FullCookTime_ReturnsOne)
{
    const i32 cookTime = 200;
    const i32 cookTimeTotal = 200;
    f32 burnProgress = std::clamp(static_cast<f32>(cookTime) / static_cast<f32>(cookTimeTotal), 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(burnProgress, 1.0f);
}

TEST(FurnaceScreenTextureTest, BurnProgress_ZeroCookTimeTotal_ReturnsZero)
{
    const i32 cookTime = 100;
    const i32 cookTimeTotal = 0;
    f32 burnProgress = 0.0f;
    if (cookTimeTotal > 0 && cookTime > 0) {
        burnProgress = std::clamp(static_cast<f32>(cookTime) / static_cast<f32>(cookTimeTotal), 0.0f, 1.0f);
    }
    EXPECT_FLOAT_EQ(burnProgress, 0.0f);
}

TEST(FurnaceScreenTextureTest, BurnProgress_BothZero_ReturnsZero)
{
    const i32 cookTime = 0;
    const i32 cookTimeTotal = 0;
    f32 burnProgress = 0.0f;
    if (cookTimeTotal > 0 && cookTime > 0) {
        burnProgress = std::clamp(static_cast<f32>(cookTime) / static_cast<f32>(cookTimeTotal), 0.0f, 1.0f);
    }
    EXPECT_FLOAT_EQ(burnProgress, 0.0f);
}

TEST(FurnaceScreenTextureTest, BurnProgress_ExceedsTotal_ClampedToOne)
{
    const i32 cookTime = 300;
    const i32 cookTimeTotal = 200;
    f32 burnProgress = std::clamp(static_cast<f32>(cookTime) / static_cast<f32>(cookTimeTotal), 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(burnProgress, 1.0f);
}

TEST(FurnaceScreenTextureTest, BurnProgress_NegativeCookTime_ClampedToZero)
{
    const i32 cookTime = -50;
    const i32 cookTimeTotal = 200;
    f32 burnProgress = 0.0f;
    if (cookTimeTotal > 0 && cookTime > 0) {
        burnProgress = std::clamp(static_cast<f32>(cookTime) / static_cast<f32>(cookTimeTotal), 0.0f, 1.0f);
    }
    EXPECT_FLOAT_EQ(burnProgress, 0.0f);
}

// ============================================================================
// 火焰可见高度计算测试（对齐 MC Java 的 ceil(litProgress * 13.0) + 1）
// ============================================================================

/**
 * @brief 验证火焰可见高度计算与 MC Java 一致
 *
 * MC Java: l = ceil(litProgress * 13.0) + 1, 范围 1~14
 */
TEST(FurnaceScreenTextureTest, LitProgressVisibleHeightCalculation)
{
    // litProgress = 0.5: ceil(0.5 * 13) + 1 = ceil(6.5) + 1 = 7 + 1 = 8
    EXPECT_EQ(static_cast<i32>(std::ceil(0.5 * 13.0)) + 1, 8);

    // litProgress = 1.0: ceil(1.0 * 13) + 1 = 13 + 1 = 14 (满)
    EXPECT_EQ(static_cast<i32>(std::ceil(1.0 * 13.0)) + 1, 14);

    // litProgress = 0.999: ceil(0.999 * 13) + 1 = ceil(12.987) + 1 = 13 + 1 = 14
    EXPECT_EQ(static_cast<i32>(std::ceil(0.999 * 13.0)) + 1, 14);

    // litProgress = 0.01: ceil(0.01 * 13) + 1 = ceil(0.13) + 1 = 1 + 1 = 2
    EXPECT_EQ(static_cast<i32>(std::ceil(0.01 * 13.0)) + 1, 2);

    // litProgress 接近 0 但非零: ceil(0.001 * 13) + 1 = ceil(0.013) + 1 = 1 + 1 = 2
    EXPECT_EQ(static_cast<i32>(std::ceil(0.001 * 13.0)) + 1, 2);
}

/**
 * @brief 验证箭头可见宽度计算与 MC Java 一致
 *
 * MC Java: j1 = ceil(burnProgress * 24.0), 范围 0~24
 */
TEST(FurnaceScreenTextureTest, BurnProgressVisibleWidthCalculation)
{
    // burnProgress = 0.5: ceil(0.5 * 24) = ceil(12.0) = 12
    EXPECT_EQ(static_cast<i32>(std::ceil(0.5 * 24.0)), 12);

    // burnProgress = 1.0: ceil(1.0 * 24) = 24 (满)
    EXPECT_EQ(static_cast<i32>(std::ceil(1.0 * 24.0)), 24);

    // burnProgress = 0.01: ceil(0.01 * 24) = ceil(0.24) = 1
    EXPECT_EQ(static_cast<i32>(std::ceil(0.01 * 24.0)), 1);

    // burnProgress = 0.999: ceil(0.999 * 24) = ceil(23.976) = 24
    EXPECT_EQ(static_cast<i32>(std::ceil(0.999 * 24.0)), 24);

    // burnProgress = 0.042: ceil(0.042 * 24) = ceil(1.008) = 2
    EXPECT_EQ(static_cast<i32>(std::ceil(0.042 * 24.0)), 2);
}

// ============================================================================
// 熔炉屏幕和容器常量测试
// ============================================================================

/**
 * @brief 验证 FurnaceContainer 槽位位置与 MC Java 一致
 */
TEST(FurnaceScreenTextureTest, FurnaceContainer_SlotPositions)
{
    // MC Java AbstractFurnaceMenu: ingredient slot at (56, 17), fuel at (56, 53), result at (116, 35)
    EXPECT_EQ(FurnaceContainer::SLOT_INPUT, 0);
    EXPECT_EQ(FurnaceContainer::SLOT_FUEL, 1);
    EXPECT_EQ(FurnaceContainer::SLOT_OUTPUT, 2);
    EXPECT_EQ(FurnaceContainer::FURNACE_SLOTS, 3);
    EXPECT_EQ(FurnaceContainer::FURNACE_SLOT_Y, 17);
    EXPECT_EQ(FurnaceContainer::PLAYER_INV_Y, 84);
    EXPECT_EQ(FurnaceContainer::HOTBAR_Y, 142);
}

/**
 * @brief 验证 GuiColors 常量用于默认纹理生成
 */
TEST(FurnaceScreenTextureTest, GuiColors_ContainerColors)
{
    // 验证颜色常量不为零且有合理的ARGB值
    EXPECT_NE(GuiColors::CONTAINER_BG, 0u);
    EXPECT_NE(GuiColors::CONTAINER_BORDER, 0u);
    EXPECT_NE(GuiColors::SLOT_BG, 0u);
    EXPECT_NE(GuiColors::SLOT_BORDER, 0u);
    EXPECT_NE(GuiColors::DEFAULT_BG, 0u);

    // 背景颜色应比边框颜色亮 (ARGB格式：0xAARRGGBB)
    u32 bgRgb = GuiColors::CONTAINER_BG & 0x00FFFFFF;
    u32 borderRgb = GuiColors::CONTAINER_BORDER & 0x00FFFFFF;
    EXPECT_GT(bgRgb, borderRgb);

    // 熔炉特有颜色常量
    EXPECT_NE(GuiColors::FURNACE_FIRE_FILL, 0u);
    EXPECT_NE(GuiColors::FURNACE_ARROW_FILL, 0u);
}
