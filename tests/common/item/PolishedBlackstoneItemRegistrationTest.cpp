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

#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// 磨制黑石按钮、磨制黑石压力板、镶金黑石物品注册测试
// ============================================================================

class PolishedBlackstoneItemRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

// ========== 磨制黑石按钮（通过 Items:: 静态指针） ==========

TEST_F(PolishedBlackstoneItemRegistrationTest, PolishedBlackstoneButton_StaticPointerNotNull)
{
    ASSERT_NE(Items::POLISHED_BLACKSTONE_BUTTON, nullptr);
}

TEST_F(PolishedBlackstoneItemRegistrationTest, PolishedBlackstoneButton_CorrectItemId)
{
    EXPECT_EQ(
        Items::POLISHED_BLACKSTONE_BUTTON->itemLocation(), ResourceLocation("minecraft:polished_blackstone_button"));
}

TEST_F(PolishedBlackstoneItemRegistrationTest, PolishedBlackstoneButton_MaxStackSize64)
{
    EXPECT_EQ(Items::POLISHED_BLACKSTONE_BUTTON->maxStackSize(), 64);
}

TEST_F(PolishedBlackstoneItemRegistrationTest, PolishedBlackstoneButton_BlockItemMapping)
{
    // 按钮物品应通过 BlockItemRegistry 映射到对应的方块
    EXPECT_NE(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BUTTON->blockId()), nullptr);
}

TEST_F(PolishedBlackstoneItemRegistrationTest, PolishedBlackstoneButton_RegistryLookup)
{
    // 通过 ResourceLocation 查找应返回与 Items:: 静态指针相同的物品
    EXPECT_EQ(ItemRegistry::instance().getItem(ResourceLocation("minecraft:polished_blackstone_button")),
        Items::POLISHED_BLACKSTONE_BUTTON);
}

TEST_F(PolishedBlackstoneItemRegistrationTest, PolishedBlackstoneButton_BlockItemReverseMapping)
{
    // 物品反向映射到方块
    const BlockItem* item =
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BUTTON->blockId());
    ASSERT_NE(item, nullptr);
    const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
    EXPECT_EQ(block, VanillaBlocks::POLISHED_BLACKSTONE_BUTTON);
}

// ========== 磨制黑石压力板（通过 Items:: 静态指针） ==========

TEST_F(PolishedBlackstoneItemRegistrationTest, PolishedBlackstonePressurePlate_StaticPointerNotNull)
{
    ASSERT_NE(Items::POLISHED_BLACKSTONE_PRESSURE_PLATE, nullptr);
}

TEST_F(PolishedBlackstoneItemRegistrationTest, PolishedBlackstonePressurePlate_CorrectItemId)
{
    EXPECT_EQ(Items::POLISHED_BLACKSTONE_PRESSURE_PLATE->itemLocation(),
        ResourceLocation("minecraft:polished_blackstone_pressure_plate"));
}

TEST_F(PolishedBlackstoneItemRegistrationTest, PolishedBlackstonePressurePlate_MaxStackSize64)
{
    EXPECT_EQ(Items::POLISHED_BLACKSTONE_PRESSURE_PLATE->maxStackSize(), 64);
}

TEST_F(PolishedBlackstoneItemRegistrationTest, PolishedBlackstonePressurePlate_BlockItemMapping)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_PRESSURE_PLATE->blockId()),
        nullptr);
}

TEST_F(PolishedBlackstoneItemRegistrationTest, PolishedBlackstonePressurePlate_RegistryLookup)
{
    EXPECT_EQ(ItemRegistry::instance().getItem(ResourceLocation("minecraft:polished_blackstone_pressure_plate")),
        Items::POLISHED_BLACKSTONE_PRESSURE_PLATE);
}

TEST_F(PolishedBlackstoneItemRegistrationTest, PolishedBlackstonePressurePlate_BlockItemReverseMapping)
{
    const BlockItem* item =
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_PRESSURE_PLATE->blockId());
    ASSERT_NE(item, nullptr);
    const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
    EXPECT_EQ(block, VanillaBlocks::POLISHED_BLACKSTONE_PRESSURE_PLATE);
}

// ========== 镶金黑石（通过 Items:: 静态指针） ==========

TEST_F(PolishedBlackstoneItemRegistrationTest, GildedBlackstone_StaticPointerNotNull)
{
    ASSERT_NE(Items::GILDED_BLACKSTONE, nullptr);
}

TEST_F(PolishedBlackstoneItemRegistrationTest, GildedBlackstone_CorrectItemId)
{
    EXPECT_EQ(Items::GILDED_BLACKSTONE->itemLocation(), ResourceLocation("minecraft:gilded_blackstone"));
}

TEST_F(PolishedBlackstoneItemRegistrationTest, GildedBlackstone_MaxStackSize64)
{
    EXPECT_EQ(Items::GILDED_BLACKSTONE->maxStackSize(), 64);
}

TEST_F(PolishedBlackstoneItemRegistrationTest, GildedBlackstone_BlockItemMapping)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::GILDED_BLACKSTONE->blockId()), nullptr);
}

TEST_F(PolishedBlackstoneItemRegistrationTest, GildedBlackstone_RegistryLookup)
{
    EXPECT_EQ(
        ItemRegistry::instance().getItem(ResourceLocation("minecraft:gilded_blackstone")), Items::GILDED_BLACKSTONE);
}

TEST_F(PolishedBlackstoneItemRegistrationTest, GildedBlackstone_BlockItemReverseMapping)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::GILDED_BLACKSTONE->blockId());
    ASSERT_NE(item, nullptr);
    const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
    EXPECT_EQ(block, VanillaBlocks::GILDED_BLACKSTONE);
}

// ========== 批量注册验证：通过 ItemRegistry 查找所有磨制黑石相关物品 ==========

TEST_F(PolishedBlackstoneItemRegistrationTest, AllPolishedBlackstoneItems_RegistryLookup)
{
    const char* polishedBlackstoneItemNames[] = {
        "polished_blackstone_button",
        "polished_blackstone_pressure_plate",
        "gilded_blackstone",
        // 已有物品（验证未被破坏）
        "blackstone",
        "polished_blackstone",
        "stone_button",
    };
    for (const char* name : polishedBlackstoneItemNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing block item: minecraft:" << name;
    }
}
