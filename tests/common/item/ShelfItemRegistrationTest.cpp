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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND, EXPRESS OR
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "common/world/block/registry/VanillaBlocks.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/items/block/BlockItem.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// 木质书架物品注册测试
// MC 1.21.4+ 新增 12 种木质书架变体
// ============================================================================

class ShelfItemRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

// ========== 静态指针验证 ==========

TEST_F(ShelfItemRegistrationTest, StaticPointersNotNull)
{
    ASSERT_NE(Items::OAK_SHELF, nullptr);
    ASSERT_NE(Items::SPRUCE_SHELF, nullptr);
    ASSERT_NE(Items::BIRCH_SHELF, nullptr);
    ASSERT_NE(Items::JUNGLE_SHELF, nullptr);
    ASSERT_NE(Items::ACACIA_SHELF, nullptr);
    ASSERT_NE(Items::DARK_OAK_SHELF, nullptr);
    ASSERT_NE(Items::MANGROVE_SHELF, nullptr);
    ASSERT_NE(Items::CHERRY_SHELF, nullptr);
    ASSERT_NE(Items::PALE_OAK_SHELF, nullptr);
    ASSERT_NE(Items::BAMBOO_SHELF, nullptr);
    ASSERT_NE(Items::CRIMSON_SHELF, nullptr);
    ASSERT_NE(Items::WARPED_SHELF, nullptr);
}

// ========== 物品ID验证 ==========

TEST_F(ShelfItemRegistrationTest, CorrectItemIds)
{
    EXPECT_EQ(Items::OAK_SHELF->itemLocation(), ResourceLocation("minecraft", "oak_shelf"));
    EXPECT_EQ(Items::SPRUCE_SHELF->itemLocation(), ResourceLocation("minecraft", "spruce_shelf"));
    EXPECT_EQ(Items::BIRCH_SHELF->itemLocation(), ResourceLocation("minecraft", "birch_shelf"));
    EXPECT_EQ(Items::JUNGLE_SHELF->itemLocation(), ResourceLocation("minecraft", "jungle_shelf"));
    EXPECT_EQ(Items::ACACIA_SHELF->itemLocation(), ResourceLocation("minecraft", "acacia_shelf"));
    EXPECT_EQ(Items::DARK_OAK_SHELF->itemLocation(), ResourceLocation("minecraft", "dark_oak_shelf"));
    EXPECT_EQ(Items::MANGROVE_SHELF->itemLocation(), ResourceLocation("minecraft", "mangrove_shelf"));
    EXPECT_EQ(Items::CHERRY_SHELF->itemLocation(), ResourceLocation("minecraft", "cherry_shelf"));
    EXPECT_EQ(Items::PALE_OAK_SHELF->itemLocation(), ResourceLocation("minecraft", "pale_oak_shelf"));
    EXPECT_EQ(Items::BAMBOO_SHELF->itemLocation(), ResourceLocation("minecraft", "bamboo_shelf"));
    EXPECT_EQ(Items::CRIMSON_SHELF->itemLocation(), ResourceLocation("minecraft", "crimson_shelf"));
    EXPECT_EQ(Items::WARPED_SHELF->itemLocation(), ResourceLocation("minecraft", "warped_shelf"));
}

// ========== 最大堆叠数验证 ==========

TEST_F(ShelfItemRegistrationTest, MaxStackSize64)
{
    EXPECT_EQ(Items::OAK_SHELF->maxStackSize(), 64);
    EXPECT_EQ(Items::MANGROVE_SHELF->maxStackSize(), 64);
    EXPECT_EQ(Items::CRIMSON_SHELF->maxStackSize(), 64);
    EXPECT_EQ(Items::WARPED_SHELF->maxStackSize(), 64);
}

// ========== BlockItem映射验证 ==========

TEST_F(ShelfItemRegistrationTest, BlockItemMapping)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_SHELF->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_SHELF->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CRIMSON_SHELF->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::WARPED_SHELF->blockId()), nullptr);
}

// ========== 注册表查询验证 ==========

TEST_F(ShelfItemRegistrationTest, RegistryLookup)
{
    static const char* shelfNames[] = {"oak_shelf",
        "spruce_shelf",
        "birch_shelf",
        "jungle_shelf",
        "acacia_shelf",
        "dark_oak_shelf",
        "mangrove_shelf",
        "cherry_shelf",
        "pale_oak_shelf",
        "bamboo_shelf",
        "crimson_shelf",
        "warped_shelf"};

    for (const char* name : shelfNames) {
        Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Shelf item '" << name << "' should be registered";
    }
}

// ========== BlockItem类型验证 ==========

TEST_F(ShelfItemRegistrationTest, AreBlockItems)
{
    static const char* shelfNames[] = {"oak_shelf",
        "spruce_shelf",
        "birch_shelf",
        "jungle_shelf",
        "acacia_shelf",
        "dark_oak_shelf",
        "mangrove_shelf",
        "cherry_shelf",
        "pale_oak_shelf",
        "bamboo_shelf",
        "crimson_shelf",
        "warped_shelf"};

    for (const char* name : shelfNames) {
        Item* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr);
        auto* blockItem = dynamic_cast<BlockItem*>(item);
        EXPECT_NE(blockItem, nullptr) << "Shelf item '" << name << "' should be a BlockItem";
    }
}

// ========== 反向映射验证 ==========

TEST_F(ShelfItemRegistrationTest, ReverseMapping)
{
    auto* blockItem = dynamic_cast<BlockItem*>(Items::OAK_SHELF);
    ASSERT_NE(blockItem, nullptr);
    const Block* block = BlockItemRegistry::instance().getBlock(blockItem->itemId());
    EXPECT_NE(block, nullptr);
    EXPECT_EQ(block->blockLocation(), ResourceLocation("minecraft", "oak_shelf"));
}
