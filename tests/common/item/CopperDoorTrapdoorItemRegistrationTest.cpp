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
// 铜门和铜活板门物品注册测试
// MC 1.21+ 新增 8 种铜门和 8 种铜活板门（含氧化/涂蜡变种）
// ============================================================================

class CopperDoorTrapdoorItemRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

// ========== 铜门物品注册 ==========

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperDoors_StaticPointersNotNull)
{
    // 所有 8 种铜门的静态指针应不为空
    ASSERT_NE(Items::COPPER_DOOR, nullptr);
    ASSERT_NE(Items::EXPOSED_COPPER_DOOR, nullptr);
    ASSERT_NE(Items::WEATHERED_COPPER_DOOR, nullptr);
    ASSERT_NE(Items::OXIDIZED_COPPER_DOOR, nullptr);
    ASSERT_NE(Items::WAXED_COPPER_DOOR, nullptr);
    ASSERT_NE(Items::WAXED_EXPOSED_COPPER_DOOR, nullptr);
    ASSERT_NE(Items::WAXED_WEATHERED_COPPER_DOOR, nullptr);
    ASSERT_NE(Items::WAXED_OXIDIZED_COPPER_DOOR, nullptr);
}

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperDoors_CorrectItemIds)
{
    EXPECT_EQ(Items::COPPER_DOOR->itemLocation(), ResourceLocation("minecraft", "copper_door"));
    EXPECT_EQ(Items::EXPOSED_COPPER_DOOR->itemLocation(), ResourceLocation("minecraft", "exposed_copper_door"));
    EXPECT_EQ(Items::WEATHERED_COPPER_DOOR->itemLocation(), ResourceLocation("minecraft", "weathered_copper_door"));
    EXPECT_EQ(Items::OXIDIZED_COPPER_DOOR->itemLocation(), ResourceLocation("minecraft", "oxidized_copper_door"));
    EXPECT_EQ(Items::WAXED_COPPER_DOOR->itemLocation(), ResourceLocation("minecraft", "waxed_copper_door"));
    EXPECT_EQ(
        Items::WAXED_EXPOSED_COPPER_DOOR->itemLocation(), ResourceLocation("minecraft", "waxed_exposed_copper_door"));
    EXPECT_EQ(Items::WAXED_WEATHERED_COPPER_DOOR->itemLocation(),
        ResourceLocation("minecraft", "waxed_weathered_copper_door"));
    EXPECT_EQ(
        Items::WAXED_OXIDIZED_COPPER_DOOR->itemLocation(), ResourceLocation("minecraft", "waxed_oxidized_copper_door"));
}

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperDoors_MaxStackSize64)
{
    // MC 原版门的堆叠数为 64（1.21+）
    EXPECT_EQ(Items::COPPER_DOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::EXPOSED_COPPER_DOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::WEATHERED_COPPER_DOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::OXIDIZED_COPPER_DOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::WAXED_COPPER_DOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::WAXED_EXPOSED_COPPER_DOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::WAXED_WEATHERED_COPPER_DOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::WAXED_OXIDIZED_COPPER_DOOR->maxStackSize(), 64);
}

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperDoors_BlockItemMapping)
{
    // 所有铜门方块应有对应的 BlockItem
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::COPPER_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::EXPOSED_COPPER_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::WEATHERED_COPPER_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::OXIDIZED_COPPER_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::WAXED_COPPER_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::WAXED_EXPOSED_COPPER_DOOR->blockId()), nullptr);
    EXPECT_NE(
        BlockItemRegistry::instance().getBlockItem(CopperBlocks::WAXED_WEATHERED_COPPER_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::WAXED_OXIDIZED_COPPER_DOOR->blockId()), nullptr);
}

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperDoors_RegistryLookup)
{
    const char* names[] = {
        "copper_door",
        "exposed_copper_door",
        "weathered_copper_door",
        "oxidized_copper_door",
        "waxed_copper_door",
        "waxed_exposed_copper_door",
        "waxed_weathered_copper_door",
        "waxed_oxidized_copper_door",
    };
    for (const char* name : names) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing copper door item: minecraft:" << name;
    }
}

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperDoors_AreBlockItems)
{
    const char* names[] = {
        "copper_door",
        "exposed_copper_door",
        "weathered_copper_door",
        "oxidized_copper_door",
        "waxed_copper_door",
        "waxed_exposed_copper_door",
        "waxed_weathered_copper_door",
        "waxed_oxidized_copper_door",
    };
    for (const char* name : names) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Missing copper door item: minecraft:" << name;
        auto* blockItem = dynamic_cast<BlockItem*>(item);
        EXPECT_NE(blockItem, nullptr) << "Copper door item should be a BlockItem: minecraft:" << name;
    }
}

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperDoors_ReverseMapping)
{
    const Block* copperDoorBlocks[] = {
        CopperBlocks::COPPER_DOOR,
        CopperBlocks::EXPOSED_COPPER_DOOR,
        CopperBlocks::WEATHERED_COPPER_DOOR,
        CopperBlocks::OXIDIZED_COPPER_DOOR,
        CopperBlocks::WAXED_COPPER_DOOR,
        CopperBlocks::WAXED_EXPOSED_COPPER_DOOR,
        CopperBlocks::WAXED_WEATHERED_COPPER_DOOR,
        CopperBlocks::WAXED_OXIDIZED_COPPER_DOOR,
    };
    for (const Block* block : copperDoorBlocks) {
        const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(block->blockId());
        ASSERT_NE(blockItem, nullptr) << "Missing BlockItem for block: " << block->blockLocation().toString();
        const Block* reverseBlock = BlockItemRegistry::instance().getBlock(blockItem->itemId());
        EXPECT_EQ(reverseBlock, block) << "Reverse mapping mismatch for block: " << block->blockLocation().toString();
    }
}

// ========== 铜活板门物品注册 ==========

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperTrapdoors_StaticPointersNotNull)
{
    // 所有 8 种铜活板门的静态指针应不为空
    ASSERT_NE(Items::COPPER_TRAPDOOR, nullptr);
    ASSERT_NE(Items::EXPOSED_COPPER_TRAPDOOR, nullptr);
    ASSERT_NE(Items::WEATHERED_COPPER_TRAPDOOR, nullptr);
    ASSERT_NE(Items::OXIDIZED_COPPER_TRAPDOOR, nullptr);
    ASSERT_NE(Items::WAXED_COPPER_TRAPDOOR, nullptr);
    ASSERT_NE(Items::WAXED_EXPOSED_COPPER_TRAPDOOR, nullptr);
    ASSERT_NE(Items::WAXED_WEATHERED_COPPER_TRAPDOOR, nullptr);
    ASSERT_NE(Items::WAXED_OXIDIZED_COPPER_TRAPDOOR, nullptr);
}

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperTrapdoors_CorrectItemIds)
{
    EXPECT_EQ(Items::COPPER_TRAPDOOR->itemLocation(), ResourceLocation("minecraft", "copper_trapdoor"));
    EXPECT_EQ(Items::EXPOSED_COPPER_TRAPDOOR->itemLocation(), ResourceLocation("minecraft", "exposed_copper_trapdoor"));
    EXPECT_EQ(
        Items::WEATHERED_COPPER_TRAPDOOR->itemLocation(), ResourceLocation("minecraft", "weathered_copper_trapdoor"));
    EXPECT_EQ(
        Items::OXIDIZED_COPPER_TRAPDOOR->itemLocation(), ResourceLocation("minecraft", "oxidized_copper_trapdoor"));
    EXPECT_EQ(Items::WAXED_COPPER_TRAPDOOR->itemLocation(), ResourceLocation("minecraft", "waxed_copper_trapdoor"));
    EXPECT_EQ(Items::WAXED_EXPOSED_COPPER_TRAPDOOR->itemLocation(),
        ResourceLocation("minecraft", "waxed_exposed_copper_trapdoor"));
    EXPECT_EQ(Items::WAXED_WEATHERED_COPPER_TRAPDOOR->itemLocation(),
        ResourceLocation("minecraft", "waxed_weathered_copper_trapdoor"));
    EXPECT_EQ(Items::WAXED_OXIDIZED_COPPER_TRAPDOOR->itemLocation(),
        ResourceLocation("minecraft", "waxed_oxidized_copper_trapdoor"));
}

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperTrapdoors_MaxStackSize64)
{
    // MC 原版活板门堆叠数为 64
    EXPECT_EQ(Items::COPPER_TRAPDOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::EXPOSED_COPPER_TRAPDOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::WEATHERED_COPPER_TRAPDOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::OXIDIZED_COPPER_TRAPDOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::WAXED_COPPER_TRAPDOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::WAXED_EXPOSED_COPPER_TRAPDOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::WAXED_WEATHERED_COPPER_TRAPDOOR->maxStackSize(), 64);
    EXPECT_EQ(Items::WAXED_OXIDIZED_COPPER_TRAPDOOR->maxStackSize(), 64);
}

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperTrapdoors_BlockItemMapping)
{
    // 所有铜活板门方块应有对应的 BlockItem
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::COPPER_TRAPDOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::EXPOSED_COPPER_TRAPDOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::WEATHERED_COPPER_TRAPDOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::OXIDIZED_COPPER_TRAPDOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(CopperBlocks::WAXED_COPPER_TRAPDOOR->blockId()), nullptr);
    EXPECT_NE(
        BlockItemRegistry::instance().getBlockItem(CopperBlocks::WAXED_EXPOSED_COPPER_TRAPDOOR->blockId()), nullptr);
    EXPECT_NE(
        BlockItemRegistry::instance().getBlockItem(CopperBlocks::WAXED_WEATHERED_COPPER_TRAPDOOR->blockId()), nullptr);
    EXPECT_NE(
        BlockItemRegistry::instance().getBlockItem(CopperBlocks::WAXED_OXIDIZED_COPPER_TRAPDOOR->blockId()), nullptr);
}

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperTrapdoors_RegistryLookup)
{
    const char* names[] = {
        "copper_trapdoor",
        "exposed_copper_trapdoor",
        "weathered_copper_trapdoor",
        "oxidized_copper_trapdoor",
        "waxed_copper_trapdoor",
        "waxed_exposed_copper_trapdoor",
        "waxed_weathered_copper_trapdoor",
        "waxed_oxidized_copper_trapdoor",
    };
    for (const char* name : names) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing copper trapdoor item: minecraft:" << name;
    }
}

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperTrapdoors_AreBlockItems)
{
    const char* names[] = {
        "copper_trapdoor",
        "exposed_copper_trapdoor",
        "weathered_copper_trapdoor",
        "oxidized_copper_trapdoor",
        "waxed_copper_trapdoor",
        "waxed_exposed_copper_trapdoor",
        "waxed_weathered_copper_trapdoor",
        "waxed_oxidized_copper_trapdoor",
    };
    for (const char* name : names) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        ASSERT_NE(item, nullptr) << "Missing copper trapdoor item: minecraft:" << name;
        auto* blockItem = dynamic_cast<BlockItem*>(item);
        EXPECT_NE(blockItem, nullptr) << "Copper trapdoor item should be a BlockItem: minecraft:" << name;
    }
}

TEST_F(CopperDoorTrapdoorItemRegistrationTest, CopperTrapdoors_ReverseMapping)
{
    const Block* copperTrapdoorBlocks[] = {
        CopperBlocks::COPPER_TRAPDOOR,
        CopperBlocks::EXPOSED_COPPER_TRAPDOOR,
        CopperBlocks::WEATHERED_COPPER_TRAPDOOR,
        CopperBlocks::OXIDIZED_COPPER_TRAPDOOR,
        CopperBlocks::WAXED_COPPER_TRAPDOOR,
        CopperBlocks::WAXED_EXPOSED_COPPER_TRAPDOOR,
        CopperBlocks::WAXED_WEATHERED_COPPER_TRAPDOOR,
        CopperBlocks::WAXED_OXIDIZED_COPPER_TRAPDOOR,
    };
    for (const Block* block : copperTrapdoorBlocks) {
        const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(block->blockId());
        ASSERT_NE(blockItem, nullptr) << "Missing BlockItem for block: " << block->blockLocation().toString();
        const Block* reverseBlock = BlockItemRegistry::instance().getBlock(blockItem->itemId());
        EXPECT_EQ(reverseBlock, block) << "Reverse mapping mismatch for block: " << block->blockLocation().toString();
    }
}
