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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file ShulkerBoxBlockTest.cpp
 * @brief 16色潜影盒方块单元测试
 *
 * 测试内容：
 * - 无色潜影盒和16色潜影盒的注册
 * - ShulkerBoxBlock::getColor() 颜色访问
 * - ShulkerBoxBlock::isShulkerBox() 类型检查（包括所有变体）
 * - BlockTags::SHULKER_BOXES 包含所有变体
 * - 方块实体类型正确
 * - 比较器输入覆盖
 */

#include <gtest/gtest.h>

#include "common/util/color/DyeColor.hpp"
#include "common/world/block/registry/ColoredBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/blocks/ShulkerBoxBlock.hpp"
#include "world/blockentity/BlockEntityType.hpp"

using namespace mc;
using namespace mc::blocks;
using namespace mc::block_registry;

class ShulkerBoxBlockTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

// ========== isShulkerBox 类型检查测试 ==========

TEST_F(ShulkerBoxBlockTest, IsShulkerBox_UncoloredShulkerBox_ReturnsTrue)
{
    ASSERT_NE(VanillaBlocks::SHULKER_BOX, nullptr);
    EXPECT_TRUE(ShulkerBoxBlock::isShulkerBox(*VanillaBlocks::SHULKER_BOX));
}

TEST_F(ShulkerBoxBlockTest, IsShulkerBox_ColoredShulkerBox_ReturnsTrue)
{
    ASSERT_NE(ColoredBlocks::RED_SHULKER_BOX, nullptr);
    EXPECT_TRUE(ShulkerBoxBlock::isShulkerBox(*ColoredBlocks::RED_SHULKER_BOX));

    ASSERT_NE(ColoredBlocks::BLUE_SHULKER_BOX, nullptr);
    EXPECT_TRUE(ShulkerBoxBlock::isShulkerBox(*ColoredBlocks::BLUE_SHULKER_BOX));

    ASSERT_NE(ColoredBlocks::WHITE_SHULKER_BOX, nullptr);
    EXPECT_TRUE(ShulkerBoxBlock::isShulkerBox(*ColoredBlocks::WHITE_SHULKER_BOX));
}

TEST_F(ShulkerBoxBlockTest, IsShulkerBox_NonShulkerBox_ReturnsFalse)
{
    EXPECT_FALSE(ShulkerBoxBlock::isShulkerBox(*VanillaBlocks::STONE));
    EXPECT_FALSE(ShulkerBoxBlock::isShulkerBox(*VanillaBlocks::DIRT));
}

// ========== 16色潜影盒注册测试 ==========

TEST_F(ShulkerBoxBlockTest, AllColoredShulkerBoxesRegistered)
{
    // 验证所有16色潜影盒都已注册
    ASSERT_NE(ColoredBlocks::WHITE_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::ORANGE_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::MAGENTA_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::LIGHT_BLUE_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::YELLOW_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::LIME_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::PINK_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::GRAY_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::LIGHT_GRAY_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::CYAN_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::PURPLE_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::BLUE_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::BROWN_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::GREEN_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::RED_SHULKER_BOX, nullptr);
    ASSERT_NE(ColoredBlocks::BLACK_SHULKER_BOX, nullptr);
}

TEST_F(ShulkerBoxBlockTest, ColoredShulkerBoxes_HaveCorrectColors)
{
    // 验证每个颜色潜影盒的 getColor() 返回正确的 DyeColor
    auto verifyColor = [](Block* block, DyeColor expectedColor) {
        ASSERT_NE(block, nullptr);
        const auto* shulkerBox = dynamic_cast<const ShulkerBoxBlock*>(block);
        ASSERT_NE(shulkerBox, nullptr);
        ASSERT_TRUE(shulkerBox->getColor().has_value());
        EXPECT_EQ(shulkerBox->getColor().value(), expectedColor);
    };

    verifyColor(ColoredBlocks::WHITE_SHULKER_BOX, DyeColor::White);
    verifyColor(ColoredBlocks::ORANGE_SHULKER_BOX, DyeColor::Orange);
    verifyColor(ColoredBlocks::MAGENTA_SHULKER_BOX, DyeColor::Magenta);
    verifyColor(ColoredBlocks::LIGHT_BLUE_SHULKER_BOX, DyeColor::LightBlue);
    verifyColor(ColoredBlocks::YELLOW_SHULKER_BOX, DyeColor::Yellow);
    verifyColor(ColoredBlocks::LIME_SHULKER_BOX, DyeColor::Lime);
    verifyColor(ColoredBlocks::PINK_SHULKER_BOX, DyeColor::Pink);
    verifyColor(ColoredBlocks::GRAY_SHULKER_BOX, DyeColor::Gray);
    verifyColor(ColoredBlocks::LIGHT_GRAY_SHULKER_BOX, DyeColor::LightGray);
    verifyColor(ColoredBlocks::CYAN_SHULKER_BOX, DyeColor::Cyan);
    verifyColor(ColoredBlocks::PURPLE_SHULKER_BOX, DyeColor::Purple);
    verifyColor(ColoredBlocks::BLUE_SHULKER_BOX, DyeColor::Blue);
    verifyColor(ColoredBlocks::BROWN_SHULKER_BOX, DyeColor::Brown);
    verifyColor(ColoredBlocks::GREEN_SHULKER_BOX, DyeColor::Green);
    verifyColor(ColoredBlocks::RED_SHULKER_BOX, DyeColor::Red);
    verifyColor(ColoredBlocks::BLACK_SHULKER_BOX, DyeColor::Black);
}

TEST_F(ShulkerBoxBlockTest, UncoloredShulkerBox_HasNoColor)
{
    ASSERT_NE(VanillaBlocks::SHULKER_BOX, nullptr);
    const auto* shulkerBox = dynamic_cast<const ShulkerBoxBlock*>(VanillaBlocks::SHULKER_BOX);
    ASSERT_NE(shulkerBox, nullptr);
    EXPECT_FALSE(shulkerBox->getColor().has_value());
}

// ========== 方块实体类型测试 ==========

TEST_F(ShulkerBoxBlockTest, AllShulkerBoxes_HaveCorrectBlockEntityType)
{
    const auto* uncoloredBox = dynamic_cast<const ShulkerBoxBlock*>(VanillaBlocks::SHULKER_BOX);
    ASSERT_NE(uncoloredBox, nullptr);
    EXPECT_EQ(uncoloredBox->getBlockEntityType(), BlockEntityType::ShulkerBox);

    // 抽样检查几个颜色变体
    const auto* redBox = dynamic_cast<const ShulkerBoxBlock*>(ColoredBlocks::RED_SHULKER_BOX);
    ASSERT_NE(redBox, nullptr);
    EXPECT_EQ(redBox->getBlockEntityType(), BlockEntityType::ShulkerBox);

    const auto* blueBox = dynamic_cast<const ShulkerBoxBlock*>(ColoredBlocks::BLUE_SHULKER_BOX);
    ASSERT_NE(blueBox, nullptr);
    EXPECT_EQ(blueBox->getBlockEntityType(), BlockEntityType::ShulkerBox);
}

TEST_F(ShulkerBoxBlockTest, AllShulkerBoxes_HaveBlockEntity)
{
    const auto* uncoloredBox = dynamic_cast<const ShulkerBoxBlock*>(VanillaBlocks::SHULKER_BOX);
    ASSERT_NE(uncoloredBox, nullptr);
    EXPECT_TRUE(uncoloredBox->hasBlockEntity());

    const auto* redBox = dynamic_cast<const ShulkerBoxBlock*>(ColoredBlocks::RED_SHULKER_BOX);
    ASSERT_NE(redBox, nullptr);
    EXPECT_TRUE(redBox->hasBlockEntity());
}

// ========== 比较器输入覆盖测试 ==========

TEST_F(ShulkerBoxBlockTest, AllShulkerBoxes_HaveComparatorInputOverride)
{
    const auto* uncoloredBox = dynamic_cast<const ShulkerBoxBlock*>(VanillaBlocks::SHULKER_BOX);
    ASSERT_NE(uncoloredBox, nullptr);
    const BlockState& state = uncoloredBox->defaultState();
    EXPECT_TRUE(uncoloredBox->hasComparatorInputOverride(state));

    const auto* redBox = dynamic_cast<const ShulkerBoxBlock*>(ColoredBlocks::RED_SHULKER_BOX);
    ASSERT_NE(redBox, nullptr);
    EXPECT_TRUE(redBox->hasComparatorInputOverride(state));
}

TEST_F(ShulkerBoxBlockTest, AllShulkerBoxes_CannotProvidePower)
{
    const auto* uncoloredBox = dynamic_cast<const ShulkerBoxBlock*>(VanillaBlocks::SHULKER_BOX);
    ASSERT_NE(uncoloredBox, nullptr);
    const BlockState& state = uncoloredBox->defaultState();
    EXPECT_FALSE(uncoloredBox->canProvidePower(state));

    const auto* redBox = dynamic_cast<const ShulkerBoxBlock*>(ColoredBlocks::RED_SHULKER_BOX);
    ASSERT_NE(redBox, nullptr);
    EXPECT_FALSE(redBox->canProvidePower(state));
}

// ========== BlockTags::SHULKER_BOXES 标签测试 ==========

TEST_F(ShulkerBoxBlockTest, BlockTagShulkerBoxes_ContainsUncolored)
{
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "shulker_box")));
}

TEST_F(ShulkerBoxBlockTest, BlockTagShulkerBoxes_ContainsAllColoredVariants)
{
    // 验证 BlockTags::SHULKER_BOXES 包含所有 16 色变体
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "white_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "orange_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "magenta_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "light_blue_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "yellow_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "lime_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "pink_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "gray_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "light_gray_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "cyan_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "purple_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "blue_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "brown_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "green_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "red_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "black_shulker_box")));
}

TEST_F(ShulkerBoxBlockTest, BlockTagShulkerBoxes_DoesNotContainNonShulkerBlocks)
{
    EXPECT_FALSE(BlockTags::SHULKER_BOXES().contains(*VanillaBlocks::STONE));
    EXPECT_FALSE(BlockTags::SHULKER_BOXES().contains(*VanillaBlocks::CHEST));
}

TEST_F(ShulkerBoxBlockTest, BlockTagShulkerBoxes_ContainsAll17Variants)
{
    // 无色 + 16色 = 17 个潜影盒
    const auto& blockIds = BlockTags::SHULKER_BOXES().getBlockIds();
    EXPECT_EQ(blockIds.size(), 17u);
}

// ========== 方块注册资源路径测试 ==========

TEST_F(ShulkerBoxBlockTest, ColoredShulkerBoxes_HaveCorrectResourceLocations)
{
    EXPECT_EQ(
        ColoredBlocks::WHITE_SHULKER_BOX->blockLocation(), ResourceLocation("minecraft", "white_shulker_box"));
    EXPECT_EQ(ColoredBlocks::RED_SHULKER_BOX->blockLocation(), ResourceLocation("minecraft", "red_shulker_box"));
    EXPECT_EQ(
        ColoredBlocks::BLACK_SHULKER_BOX->blockLocation(), ResourceLocation("minecraft", "black_shulker_box"));
}

TEST_F(ShulkerBoxBlockTest, UncoloredShulkerBox_HasCorrectResourceLocation)
{
    EXPECT_EQ(VanillaBlocks::SHULKER_BOX->blockLocation(), ResourceLocation("minecraft", "shulker_box"));
}
