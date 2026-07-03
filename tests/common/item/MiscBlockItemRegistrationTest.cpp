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
 * IMPLIED, INCLUDING BUT NOT LIMITED TO ANY KIND OF EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "common/world/block/registry/BambooBlocks.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
#include "common/world/block/registry/TrailsBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/items/block/BlockItem.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::block_registry;

// ============================================================================
// 钟、可疑沙子/沙砾、竹木方块物品注册测试
// ============================================================================

class MiscBlockItemRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

// ========== 钟（Bell）物品注册 ==========

TEST_F(MiscBlockItemRegistrationTest, Bell_StaticPointerNotNull)
{
    ASSERT_NE(Items::BELL, nullptr) << "BELL item should be registered";
}

TEST_F(MiscBlockItemRegistrationTest, Bell_CorrectItemId)
{
    EXPECT_EQ(Items::BELL->itemLocation(), ResourceLocation("minecraft", "bell"));
}

TEST_F(MiscBlockItemRegistrationTest, Bell_MaxStackSize64)
{
    EXPECT_EQ(Items::BELL->maxStackSize(), 64);
}

TEST_F(MiscBlockItemRegistrationTest, Bell_IsBlockItem)
{
    auto* blockItem = dynamic_cast<BlockItem*>(Items::BELL);
    EXPECT_NE(blockItem, nullptr) << "Bell item should be a BlockItem";
}

TEST_F(MiscBlockItemRegistrationTest, Bell_BlockItemMapping)
{
    // 钟方块应有对应的 BlockItem
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(NetherBlocks::BELL->blockId()), nullptr);
}

TEST_F(MiscBlockItemRegistrationTest, Bell_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "bell"));
    EXPECT_NE(item, nullptr) << "Missing item: minecraft:bell";
    EXPECT_EQ(item, Items::BELL);
}

TEST_F(MiscBlockItemRegistrationTest, Bell_ReverseMapping)
{
    const Block* block = NetherBlocks::BELL;
    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(block->blockId());
    ASSERT_NE(blockItem, nullptr) << "Missing BlockItem for bell";
    const Block* reverseBlock = BlockItemRegistry::instance().getBlock(blockItem->itemId());
    EXPECT_EQ(reverseBlock, block);
}

// ========== 可疑沙子（Suspicious Sand）物品注册 ==========

TEST_F(MiscBlockItemRegistrationTest, SuspiciousSand_StaticPointerNotNull)
{
    ASSERT_NE(Items::SUSPICIOUS_SAND, nullptr) << "SUSPICIOUS_SAND item should be registered";
}

TEST_F(MiscBlockItemRegistrationTest, SuspiciousSand_CorrectItemId)
{
    EXPECT_EQ(Items::SUSPICIOUS_SAND->itemLocation(), ResourceLocation("minecraft", "suspicious_sand"));
}

TEST_F(MiscBlockItemRegistrationTest, SuspiciousSand_MaxStackSize64)
{
    EXPECT_EQ(Items::SUSPICIOUS_SAND->maxStackSize(), 64);
}

TEST_F(MiscBlockItemRegistrationTest, SuspiciousSand_IsBlockItem)
{
    auto* blockItem = dynamic_cast<BlockItem*>(Items::SUSPICIOUS_SAND);
    EXPECT_NE(blockItem, nullptr) << "Suspicious sand item should be a BlockItem";
}

TEST_F(MiscBlockItemRegistrationTest, SuspiciousSand_BlockItemMapping)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(TrailsBlocks::SUSPICIOUS_SAND->blockId()), nullptr);
}

TEST_F(MiscBlockItemRegistrationTest, SuspiciousSand_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "suspicious_sand"));
    EXPECT_NE(item, nullptr) << "Missing item: minecraft:suspicious_sand";
    EXPECT_EQ(item, Items::SUSPICIOUS_SAND);
}

TEST_F(MiscBlockItemRegistrationTest, SuspiciousSand_ReverseMapping)
{
    const Block* block = TrailsBlocks::SUSPICIOUS_SAND;
    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(block->blockId());
    ASSERT_NE(blockItem, nullptr) << "Missing BlockItem for suspicious_sand";
    const Block* reverseBlock = BlockItemRegistry::instance().getBlock(blockItem->itemId());
    EXPECT_EQ(reverseBlock, block);
}

// ========== 可疑沙砾（Suspicious Gravel）物品注册 ==========

TEST_F(MiscBlockItemRegistrationTest, SuspiciousGravel_StaticPointerNotNull)
{
    ASSERT_NE(Items::SUSPICIOUS_GRAVEL, nullptr) << "SUSPICIOUS_GRAVEL item should be registered";
}

TEST_F(MiscBlockItemRegistrationTest, SuspiciousGravel_CorrectItemId)
{
    EXPECT_EQ(Items::SUSPICIOUS_GRAVEL->itemLocation(), ResourceLocation("minecraft", "suspicious_gravel"));
}

TEST_F(MiscBlockItemRegistrationTest, SuspiciousGravel_MaxStackSize64)
{
    EXPECT_EQ(Items::SUSPICIOUS_GRAVEL->maxStackSize(), 64);
}

TEST_F(MiscBlockItemRegistrationTest, SuspiciousGravel_IsBlockItem)
{
    auto* blockItem = dynamic_cast<BlockItem*>(Items::SUSPICIOUS_GRAVEL);
    EXPECT_NE(blockItem, nullptr) << "Suspicious gravel item should be a BlockItem";
}

TEST_F(MiscBlockItemRegistrationTest, SuspiciousGravel_BlockItemMapping)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(TrailsBlocks::SUSPICIOUS_GRAVEL->blockId()), nullptr);
}

TEST_F(MiscBlockItemRegistrationTest, SuspiciousGravel_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "suspicious_gravel"));
    EXPECT_NE(item, nullptr) << "Missing item: minecraft:suspicious_gravel";
    EXPECT_EQ(item, Items::SUSPICIOUS_GRAVEL);
}

TEST_F(MiscBlockItemRegistrationTest, SuspiciousGravel_ReverseMapping)
{
    const Block* block = TrailsBlocks::SUSPICIOUS_GRAVEL;
    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(block->blockId());
    ASSERT_NE(blockItem, nullptr) << "Missing BlockItem for suspicious_gravel";
    const Block* reverseBlock = BlockItemRegistry::instance().getBlock(blockItem->itemId());
    EXPECT_EQ(reverseBlock, block);
}

// ========== 竹木块（Bamboo Block）物品注册 ==========

TEST_F(MiscBlockItemRegistrationTest, BambooBlock_StaticPointerNotNull)
{
    ASSERT_NE(Items::BAMBOO_BLOCK, nullptr) << "BAMBOO_BLOCK item should be registered";
    ASSERT_NE(Items::STRIPPED_BAMBOO_BLOCK, nullptr) << "STRIPPED_BAMBOO_BLOCK item should be registered";
}

TEST_F(MiscBlockItemRegistrationTest, BambooBlock_CorrectItemIds)
{
    EXPECT_EQ(Items::BAMBOO_BLOCK->itemLocation(), ResourceLocation("minecraft", "bamboo_block"));
    EXPECT_EQ(Items::STRIPPED_BAMBOO_BLOCK->itemLocation(), ResourceLocation("minecraft", "stripped_bamboo_block"));
}

TEST_F(MiscBlockItemRegistrationTest, BambooBlock_MaxStackSize64)
{
    EXPECT_EQ(Items::BAMBOO_BLOCK->maxStackSize(), 64);
    EXPECT_EQ(Items::STRIPPED_BAMBOO_BLOCK->maxStackSize(), 64);
}

TEST_F(MiscBlockItemRegistrationTest, BambooBlock_BlockItemMapping)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(BambooBlocks::BAMBOO_BLOCK->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(BambooBlocks::STRIPPED_BAMBOO_BLOCK->blockId()), nullptr);
}

TEST_F(MiscBlockItemRegistrationTest, BambooBlock_RegistryLookup)
{
    auto* bambooBlock = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "bamboo_block"));
    EXPECT_NE(bambooBlock, nullptr) << "Missing item: minecraft:bamboo_block";

    auto* strippedBambooBlock =
        ItemRegistry::instance().getItem(ResourceLocation("minecraft", "stripped_bamboo_block"));
    EXPECT_NE(strippedBambooBlock, nullptr) << "Missing item: minecraft:stripped_bamboo_block";
}

TEST_F(MiscBlockItemRegistrationTest, BambooBlock_ReverseMapping)
{
    const Block* blocks[] = {BambooBlocks::BAMBOO_BLOCK, BambooBlocks::STRIPPED_BAMBOO_BLOCK};
    for (const Block* block : blocks) {
        const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(block->blockId());
        ASSERT_NE(blockItem, nullptr) << "Missing BlockItem for block: " << block->blockLocation().toString();
        const Block* reverseBlock = BlockItemRegistry::instance().getBlock(blockItem->itemId());
        EXPECT_EQ(reverseBlock, block) << "Reverse mapping mismatch for block: " << block->blockLocation().toString();
    }
}

// ========== 竹木板和竹马赛克（Bamboo Planks & Bamboo Mosaic）物品注册 ==========

TEST_F(MiscBlockItemRegistrationTest, BambooPlanks_StaticPointerNotNull)
{
    ASSERT_NE(Items::BAMBOO_PLANKS, nullptr) << "BAMBOO_PLANKS item should be registered";
    ASSERT_NE(Items::BAMBOO_MOSAIC, nullptr) << "BAMBOO_MOSAIC item should be registered";
}

TEST_F(MiscBlockItemRegistrationTest, BambooPlanks_CorrectItemIds)
{
    EXPECT_EQ(Items::BAMBOO_PLANKS->itemLocation(), ResourceLocation("minecraft", "bamboo_planks"));
    EXPECT_EQ(Items::BAMBOO_MOSAIC->itemLocation(), ResourceLocation("minecraft", "bamboo_mosaic"));
}

TEST_F(MiscBlockItemRegistrationTest, BambooPlanks_MaxStackSize64)
{
    EXPECT_EQ(Items::BAMBOO_PLANKS->maxStackSize(), 64);
    EXPECT_EQ(Items::BAMBOO_MOSAIC->maxStackSize(), 64);
}

TEST_F(MiscBlockItemRegistrationTest, BambooPlanks_BlockItemMapping)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(BambooBlocks::BAMBOO_PLANKS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(BambooBlocks::BAMBOO_MOSAIC->blockId()), nullptr);
}

TEST_F(MiscBlockItemRegistrationTest, BambooPlanks_RegistryLookup)
{
    auto* bambooPlanks = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "bamboo_planks"));
    EXPECT_NE(bambooPlanks, nullptr) << "Missing item: minecraft:bamboo_planks";

    auto* bambooMosaic = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "bamboo_mosaic"));
    EXPECT_NE(bambooMosaic, nullptr) << "Missing item: minecraft:bamboo_mosaic";
}

TEST_F(MiscBlockItemRegistrationTest, BambooPlanks_ReverseMapping)
{
    const Block* blocks[] = {BambooBlocks::BAMBOO_PLANKS, BambooBlocks::BAMBOO_MOSAIC};
    for (const Block* block : blocks) {
        const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(block->blockId());
        ASSERT_NE(blockItem, nullptr) << "Missing BlockItem for block: " << block->blockLocation().toString();
        const Block* reverseBlock = BlockItemRegistry::instance().getBlock(blockItem->itemId());
        EXPECT_EQ(reverseBlock, block) << "Reverse mapping mismatch for block: " << block->blockLocation().toString();
    }
}
