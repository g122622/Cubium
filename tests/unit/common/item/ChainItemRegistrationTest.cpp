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

#include "common/world/block/registry/CopperBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/items/block/BlockItem.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::block_registry;

// ============================================================================
// 铁锁链和铜锁链方块物品注册测试
// MC 1.21+ 将 minecraft:chain 重命名为 minecraft:iron_chain
// ============================================================================

class ChainItemRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

// ========== 铁锁链物品注册 ==========

TEST_F(ChainItemRegistrationTest, IronChain_BlockItemMapping)
{
    // 铁锁链应注册为 BlockItem
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHAIN->blockId()), nullptr);
}

TEST_F(ChainItemRegistrationTest, IronChain_RegistryLookup)
{
    // MC 1.21+ 重命名为 iron_chain
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_chain"));
    EXPECT_NE(item, nullptr) << "Missing chain item: minecraft:iron_chain";
}

TEST_F(ChainItemRegistrationTest, IronChain_OldNameNotRegistered)
{
    // 旧名称 chain 不应存在
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "chain"));
    EXPECT_EQ(item, nullptr) << "Old 'minecraft:chain' should not be registered (renamed to iron_chain in MC 1.21+)";
}

TEST_F(ChainItemRegistrationTest, IronChain_MaxStackSize)
{
    // MC 原版锁链最大堆叠数为64
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_chain"));
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(item->maxStackSize(), 64);
}

TEST_F(ChainItemRegistrationTest, IronChain_IsBlockItem)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "iron_chain"));
    ASSERT_NE(item, nullptr);
    auto* blockItem = dynamic_cast<BlockItem*>(item);
    EXPECT_NE(blockItem, nullptr) << "Iron chain item should be a BlockItem";
}

TEST_F(ChainItemRegistrationTest, IronChain_ReverseMapping)
{
    const Block* block = VanillaBlocks::CHAIN;
    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(block->blockId());
    ASSERT_NE(blockItem, nullptr);
    const Block* reverseBlock = BlockItemRegistry::instance().getBlock(blockItem->itemId());
    EXPECT_EQ(reverseBlock, block);
}

// ========== 铜锁链物品注册（含氧化和涂蜡变种） ==========

TEST_F(ChainItemRegistrationTest, CopperChain_BlockItemMapping)
{
    // 所有铜锁链变体都应注册为 BlockItem
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::COPPER_CHAIN->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::EXPOSED_COPPER_CHAIN->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::WEATHERED_COPPER_CHAIN->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::OXIDIZED_COPPER_CHAIN->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::WAXED_COPPER_CHAIN->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::WAXED_EXPOSED_COPPER_CHAIN->blockId()), nullptr);
    EXPECT_NE(
        BlockItemRegistry::instance().getBlockItem(CopperBlocks::WAXED_WEATHERED_COPPER_CHAIN->blockId()), nullptr);
    EXPECT_NE(
        BlockItemRegistry::instance().getBlockItem(CopperBlocks::WAXED_OXIDIZED_COPPER_CHAIN->blockId()), nullptr);
}

TEST_F(ChainItemRegistrationTest, CopperChain_RegistryLookup)
{
    const char* copperChainNames[] = {
        "copper_chain",
        "exposed_copper_chain",
        "weathered_copper_chain",
        "oxidized_copper_chain",
        "waxed_copper_chain",
        "waxed_exposed_copper_chain",
        "waxed_weathered_copper_chain",
        "waxed_oxidized_copper_chain",
    };
    for (const char* name : copperChainNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing copper chain block item: minecraft:" << name;
    }
}

TEST_F(ChainItemRegistrationTest, CopperChain_MaxStackSize)
{
    // MC 原版铜锁链最大堆叠数为64
    const char* copperChainNames[] = {
        "copper_chain",
        "exposed_copper_chain",
        "weathered_copper_chain",
        "oxidized_copper_chain",
        "waxed_copper_chain",
        "waxed_exposed_copper_chain",
        "waxed_weathered_copper_chain",
        "waxed_oxidized_copper_chain",
    };
    for (const char* name : copperChainNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Missing copper chain block item: minecraft:" << name;
        EXPECT_EQ(item->maxStackSize(), 64) << "Copper chain item should have max stack size 64: minecraft:" << name;
    }
}

TEST_F(ChainItemRegistrationTest, CopperChain_AreBlockItems)
{
    const char* copperChainNames[] = {
        "copper_chain",
        "exposed_copper_chain",
        "weathered_copper_chain",
        "oxidized_copper_chain",
        "waxed_copper_chain",
        "waxed_exposed_copper_chain",
        "waxed_weathered_copper_chain",
        "waxed_oxidized_copper_chain",
    };
    for (const char* name : copperChainNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Missing copper chain block item: minecraft:" << name;
        auto* blockItem = dynamic_cast<BlockItem*>(item);
        EXPECT_NE(blockItem, nullptr) << "Copper chain item should be a BlockItem: minecraft:" << name;
    }
}

TEST_F(ChainItemRegistrationTest, CopperChain_ReverseMapping)
{
    const Block* copperChainBlocks[] = {
        CopperBlocks::COPPER_CHAIN,
        CopperBlocks::EXPOSED_COPPER_CHAIN,
        CopperBlocks::WEATHERED_COPPER_CHAIN,
        CopperBlocks::OXIDIZED_COPPER_CHAIN,
        CopperBlocks::WAXED_COPPER_CHAIN,
        CopperBlocks::WAXED_EXPOSED_COPPER_CHAIN,
        CopperBlocks::WAXED_WEATHERED_COPPER_CHAIN,
        CopperBlocks::WAXED_OXIDIZED_COPPER_CHAIN,
    };
    for (const Block* block : copperChainBlocks) {
        const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(block->blockId());
        ASSERT_NE(blockItem, nullptr) << "Missing BlockItem for block: " << block->blockLocation().toString();
        const Block* reverseBlock = BlockItemRegistry::instance().getBlock(blockItem->itemId());
        EXPECT_EQ(reverseBlock, block) << "Reverse mapping mismatch for block: " << block->blockLocation().toString();
    }
}
