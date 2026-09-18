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
#include "item/items/block/BlockItem.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// 铁砧和避雷针方块物品注册测试
// ============================================================================

class AnvilLightningRodItemRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

// ========== 铁砧物品注册 ==========

TEST_F(AnvilLightningRodItemRegistrationTest, AnvilItems_BlockItemMapping)
{
    // 铁砧三个变体都应注册为 BlockItem
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::ANVIL->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHIPPED_ANVIL->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DAMAGED_ANVIL->blockId()), nullptr);
}

TEST_F(AnvilLightningRodItemRegistrationTest, AnvilItems_RegistryLookup)
{
    const char* anvilNames[] = {
        "anvil",
        "chipped_anvil",
        "damaged_anvil",
    };
    for (const char* name : anvilNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing anvil block item: minecraft:" << name;
    }
}

TEST_F(AnvilLightningRodItemRegistrationTest, AnvilItems_MaxStackSizeIsOne)
{
    // MC 原版铁砧最大堆叠数为1
    const char* anvilNames[] = {
        "anvil",
        "chipped_anvil",
        "damaged_anvil",
    };
    for (const char* name : anvilNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Missing anvil block item: minecraft:" << name;
        EXPECT_EQ(item->maxStackSize(), 1) << "Anvil item should have max stack size 1: minecraft:" << name;
    }
}

TEST_F(AnvilLightningRodItemRegistrationTest, AnvilItems_AreBlockItems)
{
    // 铁砧物品应为 BlockItem 实例
    const char* anvilNames[] = {
        "anvil",
        "chipped_anvil",
        "damaged_anvil",
    };
    for (const char* name : anvilNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Missing anvil block item: minecraft:" << name;
        auto* blockItem = dynamic_cast<BlockItem*>(item);
        EXPECT_NE(blockItem, nullptr) << "Anvil item should be a BlockItem: minecraft:" << name;
    }
}

TEST_F(AnvilLightningRodItemRegistrationTest, AnvilItems_ReverseMapping)
{
    // BlockItemRegistry 双向映射：通过方块ID查找物品，通过物品ID查找方块
    const Block* anvilBlocks[] = {
        VanillaBlocks::ANVIL,
        VanillaBlocks::CHIPPED_ANVIL,
        VanillaBlocks::DAMAGED_ANVIL,
    };
    for (const Block* block : anvilBlocks) {
        const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(block->blockId());
        ASSERT_NE(blockItem, nullptr) << "Missing BlockItem for block: " << block->blockLocation().toString();
        const Block* reverseBlock = BlockItemRegistry::instance().getBlock(blockItem->itemId());
        EXPECT_EQ(reverseBlock, block) << "Reverse mapping mismatch for block: " << block->blockLocation().toString();
    }
}

// ========== 避雷针物品注册 ==========

TEST_F(AnvilLightningRodItemRegistrationTest, LightningRodItems_BlockItemMapping)
{
    // 避雷针8个变体都应注册为 BlockItem
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::LIGHTNING_ROD->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::EXPOSED_LIGHTNING_ROD->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::WEATHERED_LIGHTNING_ROD->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OXIDIZED_LIGHTNING_ROD->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::WAXED_LIGHTNING_ROD->blockId()), nullptr);
    EXPECT_NE(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::WAXED_EXPOSED_LIGHTNING_ROD->blockId()), nullptr);
    EXPECT_NE(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::WAXED_WEATHERED_LIGHTNING_ROD->blockId()), nullptr);
    EXPECT_NE(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::WAXED_OXIDIZED_LIGHTNING_ROD->blockId()), nullptr);
}

TEST_F(AnvilLightningRodItemRegistrationTest, LightningRodItems_RegistryLookup)
{
    const char* lightningRodNames[] = {
        "lightning_rod",
        "exposed_lightning_rod",
        "weathered_lightning_rod",
        "oxidized_lightning_rod",
        "waxed_lightning_rod",
        "waxed_exposed_lightning_rod",
        "waxed_weathered_lightning_rod",
        "waxed_oxidized_lightning_rod",
    };
    for (const char* name : lightningRodNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing lightning rod block item: minecraft:" << name;
    }
}

TEST_F(AnvilLightningRodItemRegistrationTest, LightningRodItems_MaxStackSizeIs64)
{
    // MC 原版避雷针最大堆叠数为64
    const char* lightningRodNames[] = {
        "lightning_rod",
        "exposed_lightning_rod",
        "weathered_lightning_rod",
        "oxidized_lightning_rod",
        "waxed_lightning_rod",
        "waxed_exposed_lightning_rod",
        "waxed_weathered_lightning_rod",
        "waxed_oxidized_lightning_rod",
    };
    for (const char* name : lightningRodNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Missing lightning rod block item: minecraft:" << name;
        EXPECT_EQ(item->maxStackSize(), 64) << "Lightning rod item should have max stack size 64: minecraft:" << name;
    }
}

TEST_F(AnvilLightningRodItemRegistrationTest, LightningRodItems_AreBlockItems)
{
    const char* lightningRodNames[] = {
        "lightning_rod",
        "exposed_lightning_rod",
        "weathered_lightning_rod",
        "oxidized_lightning_rod",
        "waxed_lightning_rod",
        "waxed_exposed_lightning_rod",
        "waxed_weathered_lightning_rod",
        "waxed_oxidized_lightning_rod",
    };
    for (const char* name : lightningRodNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Missing lightning rod block item: minecraft:" << name;
        auto* blockItem = dynamic_cast<BlockItem*>(item);
        EXPECT_NE(blockItem, nullptr) << "Lightning rod item should be a BlockItem: minecraft:" << name;
    }
}

TEST_F(AnvilLightningRodItemRegistrationTest, LightningRodItems_ReverseMapping)
{
    const Block* lightningRodBlocks[] = {
        VanillaBlocks::LIGHTNING_ROD,
        VanillaBlocks::EXPOSED_LIGHTNING_ROD,
        VanillaBlocks::WEATHERED_LIGHTNING_ROD,
        VanillaBlocks::OXIDIZED_LIGHTNING_ROD,
        VanillaBlocks::WAXED_LIGHTNING_ROD,
        VanillaBlocks::WAXED_EXPOSED_LIGHTNING_ROD,
        VanillaBlocks::WAXED_WEATHERED_LIGHTNING_ROD,
        VanillaBlocks::WAXED_OXIDIZED_LIGHTNING_ROD,
    };
    for (const Block* block : lightningRodBlocks) {
        const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(block->blockId());
        ASSERT_NE(blockItem, nullptr) << "Missing BlockItem for block: " << block->blockLocation().toString();
        const Block* reverseBlock = BlockItemRegistry::instance().getBlock(blockItem->itemId());
        EXPECT_EQ(reverseBlock, block) << "Reverse mapping mismatch for block: " << block->blockLocation().toString();
    }
}
