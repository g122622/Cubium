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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

using namespace mc;

class RawOreItemRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

// ============================================================================
// RAW_IRON 物品测试
// ============================================================================

TEST_F(RawOreItemRegistrationTest, RawIron_StaticPointerNotNull)
{
    ASSERT_NE(Items::RAW_IRON, nullptr) << "RAW_IRON should be registered";
}

TEST_F(RawOreItemRegistrationTest, RawIron_CorrectItemId)
{
    EXPECT_EQ(Items::RAW_IRON->itemLocation(), ResourceLocation("minecraft", "raw_iron"));
}

TEST_F(RawOreItemRegistrationTest, RawIron_MaxStackSize64)
{
    EXPECT_EQ(Items::RAW_IRON->maxStackSize(), 64);
}

TEST_F(RawOreItemRegistrationTest, RawIron_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "raw_iron"));
    EXPECT_EQ(item, Items::RAW_IRON);
}

// ============================================================================
// RAW_COPPER 物品测试
// ============================================================================

TEST_F(RawOreItemRegistrationTest, RawCopper_StaticPointerNotNull)
{
    ASSERT_NE(Items::RAW_COPPER, nullptr) << "RAW_COPPER should be registered";
}

TEST_F(RawOreItemRegistrationTest, RawCopper_CorrectItemId)
{
    EXPECT_EQ(Items::RAW_COPPER->itemLocation(), ResourceLocation("minecraft", "raw_copper"));
}

TEST_F(RawOreItemRegistrationTest, RawCopper_MaxStackSize64)
{
    EXPECT_EQ(Items::RAW_COPPER->maxStackSize(), 64);
}

TEST_F(RawOreItemRegistrationTest, RawCopper_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "raw_copper"));
    EXPECT_EQ(item, Items::RAW_COPPER);
}

// ============================================================================
// RAW_GOLD 物品测试
// ============================================================================

TEST_F(RawOreItemRegistrationTest, RawGold_StaticPointerNotNull)
{
    ASSERT_NE(Items::RAW_GOLD, nullptr) << "RAW_GOLD should be registered";
}

TEST_F(RawOreItemRegistrationTest, RawGold_CorrectItemId)
{
    EXPECT_EQ(Items::RAW_GOLD->itemLocation(), ResourceLocation("minecraft", "raw_gold"));
}

TEST_F(RawOreItemRegistrationTest, RawGold_MaxStackSize64)
{
    EXPECT_EQ(Items::RAW_GOLD->maxStackSize(), 64);
}

TEST_F(RawOreItemRegistrationTest, RawGold_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "raw_gold"));
    EXPECT_EQ(item, Items::RAW_GOLD);
}

// ============================================================================
// RAW_IRON_BLOCK 方块物品测试
// ============================================================================

TEST_F(RawOreItemRegistrationTest, RawIronBlock_StaticPointerNotNull)
{
    ASSERT_NE(Items::RAW_IRON_BLOCK, nullptr) << "RAW_IRON_BLOCK should be registered";
}

TEST_F(RawOreItemRegistrationTest, RawIronBlock_CorrectItemId)
{
    EXPECT_EQ(Items::RAW_IRON_BLOCK->itemLocation(), ResourceLocation("minecraft", "raw_iron_block"));
}

TEST_F(RawOreItemRegistrationTest, RawIronBlock_MaxStackSize64)
{
    EXPECT_EQ(Items::RAW_IRON_BLOCK->maxStackSize(), 64);
}

TEST_F(RawOreItemRegistrationTest, RawIronBlock_BlockItemMapping)
{
    ASSERT_NE(VanillaBlocks::RAW_IRON_BLOCK, nullptr);
    auto* blockItem = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::RAW_IRON_BLOCK->blockId());
    EXPECT_NE(blockItem, nullptr) << "RAW_IRON_BLOCK should have a BlockItem mapping";
}

TEST_F(RawOreItemRegistrationTest, RawIronBlock_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "raw_iron_block"));
    EXPECT_EQ(item, Items::RAW_IRON_BLOCK);
}

// ============================================================================
// RAW_COPPER_BLOCK 方块物品测试
// ============================================================================

TEST_F(RawOreItemRegistrationTest, RawCopperBlock_StaticPointerNotNull)
{
    ASSERT_NE(Items::RAW_COPPER_BLOCK, nullptr) << "RAW_COPPER_BLOCK should be registered";
}

TEST_F(RawOreItemRegistrationTest, RawCopperBlock_CorrectItemId)
{
    EXPECT_EQ(Items::RAW_COPPER_BLOCK->itemLocation(), ResourceLocation("minecraft", "raw_copper_block"));
}

TEST_F(RawOreItemRegistrationTest, RawCopperBlock_MaxStackSize64)
{
    EXPECT_EQ(Items::RAW_COPPER_BLOCK->maxStackSize(), 64);
}

TEST_F(RawOreItemRegistrationTest, RawCopperBlock_BlockItemMapping)
{
    ASSERT_NE(VanillaBlocks::RAW_COPPER_BLOCK, nullptr);
    auto* blockItem = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::RAW_COPPER_BLOCK->blockId());
    EXPECT_NE(blockItem, nullptr) << "RAW_COPPER_BLOCK should have a BlockItem mapping";
}

TEST_F(RawOreItemRegistrationTest, RawCopperBlock_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "raw_copper_block"));
    EXPECT_EQ(item, Items::RAW_COPPER_BLOCK);
}

// ============================================================================
// RAW_GOLD_BLOCK 方块物品测试
// ============================================================================

TEST_F(RawOreItemRegistrationTest, RawGoldBlock_StaticPointerNotNull)
{
    ASSERT_NE(Items::RAW_GOLD_BLOCK, nullptr) << "RAW_GOLD_BLOCK should be registered";
}

TEST_F(RawOreItemRegistrationTest, RawGoldBlock_CorrectItemId)
{
    EXPECT_EQ(Items::RAW_GOLD_BLOCK->itemLocation(), ResourceLocation("minecraft", "raw_gold_block"));
}

TEST_F(RawOreItemRegistrationTest, RawGoldBlock_MaxStackSize64)
{
    EXPECT_EQ(Items::RAW_GOLD_BLOCK->maxStackSize(), 64);
}

TEST_F(RawOreItemRegistrationTest, RawGoldBlock_BlockItemMapping)
{
    ASSERT_NE(VanillaBlocks::RAW_GOLD_BLOCK, nullptr);
    auto* blockItem = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::RAW_GOLD_BLOCK->blockId());
    EXPECT_NE(blockItem, nullptr) << "RAW_GOLD_BLOCK should have a BlockItem mapping";
}

TEST_F(RawOreItemRegistrationTest, RawGoldBlock_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "raw_gold_block"));
    EXPECT_EQ(item, Items::RAW_GOLD_BLOCK);
}

// ============================================================================
// TURTLE_SCUTE 重命名测试
// ============================================================================

TEST_F(RawOreItemRegistrationTest, TurtleScute_StaticPointerNotNull)
{
    ASSERT_NE(Items::TURTLE_SCUTE, nullptr) << "TURTLE_SCUTE should be registered";
}

TEST_F(RawOreItemRegistrationTest, TurtleScute_CorrectItemId)
{
    // MC 1.20.5+ 将 scute 重命名为 turtle_scute
    EXPECT_EQ(Items::TURTLE_SCUTE->itemLocation(), ResourceLocation("minecraft", "turtle_scute"));
}

TEST_F(RawOreItemRegistrationTest, TurtleScute_MaxStackSize64)
{
    EXPECT_EQ(Items::TURTLE_SCUTE->maxStackSize(), 64);
}

TEST_F(RawOreItemRegistrationTest, TurtleScute_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "turtle_scute"));
    EXPECT_EQ(item, Items::TURTLE_SCUTE);
}

// ============================================================================
// 批量验证 - 确保所有新增物品都能通过注册表查找到
// ============================================================================

TEST_F(RawOreItemRegistrationTest, AllRawOreItems_RegistryLookup)
{
    const char* itemNames[] = {
        "raw_iron", "raw_copper", "raw_gold", "raw_iron_block", "raw_copper_block", "raw_gold_block", "turtle_scute"};

    for (const char* name : itemNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing item: minecraft:" << name;
    }
}
