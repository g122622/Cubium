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
// 深板岩矿石、建筑方块、凝灰岩、树脂、苍白花园及材料物品注册测试
// ============================================================================

class DeepslateItemRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

// ========== 深板岩矿石物品（通过 Items:: 静态指针） ==========

TEST_F(DeepslateItemRegistrationTest, DeepslateOreItems_StaticPointersNotNull)
{
    ASSERT_NE(Items::DEEPSLATE_COAL_ORE, nullptr);
    ASSERT_NE(Items::DEEPSLATE_IRON_ORE, nullptr);
    ASSERT_NE(Items::DEEPSLATE_COPPER_ORE, nullptr);
    ASSERT_NE(Items::DEEPSLATE_GOLD_ORE, nullptr);
    ASSERT_NE(Items::DEEPSLATE_DIAMOND_ORE, nullptr);
    ASSERT_NE(Items::DEEPSLATE_LAPIS_ORE, nullptr);
    ASSERT_NE(Items::DEEPSLATE_EMERALD_ORE, nullptr);
    ASSERT_NE(Items::DEEPSLATE_REDSTONE_ORE, nullptr);
}

TEST_F(DeepslateItemRegistrationTest, DeepslateOreItems_CorrectItemIds)
{
    EXPECT_EQ(Items::DEEPSLATE_COAL_ORE->itemLocation(), ResourceLocation("minecraft:deepslate_coal_ore"));
    EXPECT_EQ(Items::DEEPSLATE_IRON_ORE->itemLocation(), ResourceLocation("minecraft:deepslate_iron_ore"));
    EXPECT_EQ(Items::DEEPSLATE_COPPER_ORE->itemLocation(), ResourceLocation("minecraft:deepslate_copper_ore"));
    EXPECT_EQ(Items::DEEPSLATE_GOLD_ORE->itemLocation(), ResourceLocation("minecraft:deepslate_gold_ore"));
    EXPECT_EQ(Items::DEEPSLATE_DIAMOND_ORE->itemLocation(), ResourceLocation("minecraft:deepslate_diamond_ore"));
    EXPECT_EQ(Items::DEEPSLATE_LAPIS_ORE->itemLocation(), ResourceLocation("minecraft:deepslate_lapis_ore"));
    EXPECT_EQ(Items::DEEPSLATE_EMERALD_ORE->itemLocation(), ResourceLocation("minecraft:deepslate_emerald_ore"));
    EXPECT_EQ(Items::DEEPSLATE_REDSTONE_ORE->itemLocation(), ResourceLocation("minecraft:deepslate_redstone_ore"));
}

TEST_F(DeepslateItemRegistrationTest, DeepslateOreItems_MaxStackSize64)
{
    EXPECT_EQ(Items::DEEPSLATE_COAL_ORE->maxStackSize(), 64);
    EXPECT_EQ(Items::DEEPSLATE_IRON_ORE->maxStackSize(), 64);
    EXPECT_EQ(Items::DEEPSLATE_COPPER_ORE->maxStackSize(), 64);
    EXPECT_EQ(Items::DEEPSLATE_GOLD_ORE->maxStackSize(), 64);
    EXPECT_EQ(Items::DEEPSLATE_DIAMOND_ORE->maxStackSize(), 64);
    EXPECT_EQ(Items::DEEPSLATE_LAPIS_ORE->maxStackSize(), 64);
    EXPECT_EQ(Items::DEEPSLATE_EMERALD_ORE->maxStackSize(), 64);
    EXPECT_EQ(Items::DEEPSLATE_REDSTONE_ORE->maxStackSize(), 64);
}

TEST_F(DeepslateItemRegistrationTest, DeepslateOreItems_BlockItemMapping)
{
    // 深板岩矿石物品应通过 BlockItemRegistry 映射到对应的方块
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_COAL_ORE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_IRON_ORE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_COPPER_ORE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_GOLD_ORE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_DIAMOND_ORE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_LAPIS_ORE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_EMERALD_ORE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_REDSTONE_ORE->blockId()), nullptr);
}

TEST_F(DeepslateItemRegistrationTest, DeepslateOreItems_RegistryLookup)
{
    // 通过 ResourceLocation 查找应返回与 Items:: 静态指针相同的物品
    EXPECT_EQ(
        ItemRegistry::instance().getItem(ResourceLocation("minecraft:deepslate_coal_ore")), Items::DEEPSLATE_COAL_ORE);
    EXPECT_EQ(
        ItemRegistry::instance().getItem(ResourceLocation("minecraft:deepslate_iron_ore")), Items::DEEPSLATE_IRON_ORE);
    EXPECT_EQ(ItemRegistry::instance().getItem(ResourceLocation("minecraft:deepslate_copper_ore")),
        Items::DEEPSLATE_COPPER_ORE);
    EXPECT_EQ(
        ItemRegistry::instance().getItem(ResourceLocation("minecraft:deepslate_gold_ore")), Items::DEEPSLATE_GOLD_ORE);
    EXPECT_EQ(ItemRegistry::instance().getItem(ResourceLocation("minecraft:deepslate_diamond_ore")),
        Items::DEEPSLATE_DIAMOND_ORE);
    EXPECT_EQ(ItemRegistry::instance().getItem(ResourceLocation("minecraft:deepslate_lapis_ore")),
        Items::DEEPSLATE_LAPIS_ORE);
    EXPECT_EQ(ItemRegistry::instance().getItem(ResourceLocation("minecraft:deepslate_emerald_ore")),
        Items::DEEPSLATE_EMERALD_ORE);
    EXPECT_EQ(ItemRegistry::instance().getItem(ResourceLocation("minecraft:deepslate_redstone_ore")),
        Items::DEEPSLATE_REDSTONE_ORE);
}

// ========== 深板岩建筑方块物品（通过 BlockItemRegistry） ==========

TEST_F(DeepslateItemRegistrationTest, DeepslateBuildingBlocks_HaveBlockItems)
{
    // 基础深板岩方块
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::COBBLED_DEEPSLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_DEEPSLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_BRICKS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_TILES->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHISELED_DEEPSLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CRACKED_DEEPSLATE_BRICKS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CRACKED_DEEPSLATE_TILES->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::REINFORCED_DEEPSLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::INFESTED_DEEPSLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::SMOOTH_BASALT->blockId()), nullptr);
}

TEST_F(DeepslateItemRegistrationTest, DeepslateStairs_HaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::COBBLED_DEEPSLATE_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_DEEPSLATE_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_BRICK_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_TILE_STAIRS->blockId()), nullptr);
}

TEST_F(DeepslateItemRegistrationTest, DeepslateSlabs_HaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::COBBLED_DEEPSLATE_SLAB->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_DEEPSLATE_SLAB->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_BRICK_SLAB->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_TILE_SLAB->blockId()), nullptr);
}

TEST_F(DeepslateItemRegistrationTest, DeepslateWalls_HaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::COBBLED_DEEPSLATE_WALL->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_DEEPSLATE_WALL->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_BRICK_WALL->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DEEPSLATE_TILE_WALL->blockId()), nullptr);
}

// ========== 凝灰岩方块物品 ==========

TEST_F(DeepslateItemRegistrationTest, TuffBlocks_HaveBlockItems)
{
    // 基础凝灰岩方块
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::TUFF->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_TUFF->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::TUFF_BRICKS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHISELED_TUFF->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHISELED_TUFF_BRICKS->blockId()), nullptr);
}

TEST_F(DeepslateItemRegistrationTest, TuffStairs_HaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::TUFF_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_TUFF_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::TUFF_BRICK_STAIRS->blockId()), nullptr);
}

TEST_F(DeepslateItemRegistrationTest, TuffSlabs_HaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::TUFF_SLAB->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_TUFF_SLAB->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::TUFF_BRICK_SLAB->blockId()), nullptr);
}

TEST_F(DeepslateItemRegistrationTest, TuffWalls_HaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::TUFF_WALL->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_TUFF_WALL->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::TUFF_BRICK_WALL->blockId()), nullptr);
}

// ========== 树脂方块物品 ==========

TEST_F(DeepslateItemRegistrationTest, ResinBlocks_HaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::RESIN_CLUMP->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::RESIN_BLOCK->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::RESIN_BRICKS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHISELED_RESIN_BRICKS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::RESIN_BRICK_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::RESIN_BRICK_SLAB->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::RESIN_BRICK_WALL->blockId()), nullptr);
}

// ========== 苍白花园方块物品 ==========

TEST_F(DeepslateItemRegistrationTest, PaleOakWoodBlocks_HaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_LOG->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_WOOD->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::STRIPPED_PALE_OAK_LOG->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::STRIPPED_PALE_OAK_WOOD->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_PLANKS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_LEAVES->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_SAPLING->blockId()), nullptr);
}

TEST_F(DeepslateItemRegistrationTest, PaleGardenNatureBlocks_HaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_MOSS_BLOCK->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_MOSS_CARPET->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_HANGING_MOSS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OPEN_EYEBLOSSOM->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CLOSED_EYEBLOSSOM->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CREAKING_HEART->blockId()), nullptr);
}

// ========== 材料物品（非方块物品） ==========

TEST_F(DeepslateItemRegistrationTest, MaterialItems_RegisteredCorrectly)
{
    // 砖（合成材料）
    ASSERT_NE(Items::BRICK, nullptr);
    EXPECT_EQ(Items::BRICK->itemLocation(), ResourceLocation("minecraft:brick"));
    EXPECT_EQ(Items::BRICK->maxStackSize(), 64);

    // 树脂砖（合成材料）
    ASSERT_NE(Items::RESIN_BRICK, nullptr);
    EXPECT_EQ(Items::RESIN_BRICK->itemLocation(), ResourceLocation("minecraft:resin_brick"));
    EXPECT_EQ(Items::RESIN_BRICK->maxStackSize(), 64);

    // 紫水晶碎片
    ASSERT_NE(Items::AMETHYST_SHARD, nullptr);
    EXPECT_EQ(Items::AMETHYST_SHARD->itemLocation(), ResourceLocation("minecraft:amethyst_shard"));
    EXPECT_EQ(Items::AMETHYST_SHARD->maxStackSize(), 64);
}

// ========== 通过 ItemRegistry 查找验证所有新增物品 ==========

TEST_F(DeepslateItemRegistrationTest, DeepslateBuildingBlocks_RegistryLookup)
{
    // 验证通过 ResourceLocation 可以查找深板岩建筑方块物品
    const char* deepslateBlockNames[] = {
        "deepslate",
        "cobbled_deepslate",
        "polished_deepslate",
        "deepslate_bricks",
        "deepslate_tiles",
        "chiseled_deepslate",
        "cracked_deepslate_bricks",
        "cracked_deepslate_tiles",
        "reinforced_deepslate",
        "infested_deepslate",
        "smooth_basalt",
    };
    for (const char* name : deepslateBlockNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing block item: minecraft:" << name;
    }
}

TEST_F(DeepslateItemRegistrationTest, DeepslateStairsSlabsWalls_RegistryLookup)
{
    const char* deepslateStairsSlabsWalls[] = {
        "cobbled_deepslate_stairs",
        "cobbled_deepslate_slab",
        "cobbled_deepslate_wall",
        "polished_deepslate_stairs",
        "polished_deepslate_slab",
        "polished_deepslate_wall",
        "deepslate_brick_stairs",
        "deepslate_brick_slab",
        "deepslate_brick_wall",
        "deepslate_tile_stairs",
        "deepslate_tile_slab",
        "deepslate_tile_wall",
    };
    for (const char* name : deepslateStairsSlabsWalls) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing block item: minecraft:" << name;
    }
}

TEST_F(DeepslateItemRegistrationTest, TuffBlocks_RegistryLookup)
{
    const char* tuffBlockNames[] = {
        "tuff",
        "polished_tuff",
        "tuff_bricks",
        "chiseled_tuff",
        "chiseled_tuff_bricks",
        "tuff_stairs",
        "tuff_slab",
        "tuff_wall",
        "polished_tuff_stairs",
        "polished_tuff_slab",
        "polished_tuff_wall",
        "tuff_brick_stairs",
        "tuff_brick_slab",
        "tuff_brick_wall",
    };
    for (const char* name : tuffBlockNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing block item: minecraft:" << name;
    }
}

TEST_F(DeepslateItemRegistrationTest, ResinBlocks_RegistryLookup)
{
    const char* resinBlockNames[] = {
        "resin_clump",
        "resin_block",
        "resin_bricks",
        "chiseled_resin_bricks",
        "resin_brick_stairs",
        "resin_brick_slab",
        "resin_brick_wall",
    };
    for (const char* name : resinBlockNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing block item: minecraft:" << name;
    }
}

TEST_F(DeepslateItemRegistrationTest, PaleGardenBlocks_RegistryLookup)
{
    const char* paleGardenBlockNames[] = {
        "pale_oak_log",
        "pale_oak_wood",
        "stripped_pale_oak_log",
        "stripped_pale_oak_wood",
        "pale_oak_planks",
        "pale_oak_leaves",
        "pale_oak_sapling",
        "pale_moss_block",
        "pale_moss_carpet",
        "pale_hanging_moss",
        "open_eyeblossom",
        "closed_eyeblossom",
        "creaking_heart",
    };
    for (const char* name : paleGardenBlockNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing block item: minecraft:" << name;
    }
}
