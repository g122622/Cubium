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
// 黑石建筑方块（楼梯/台阶/墙）物品注册测试
//
// 涵盖 9 个黑石变体 BlockItem：
//   - blackstone_stairs / blackstone_slab / blackstone_wall
//   - polished_blackstone_stairs / polished_blackstone_slab / polished_blackstone_wall
//   - polished_blackstone_brick_stairs / polished_blackstone_brick_slab / polished_blackstone_brick_wall
//
// 这些物品在 BlockItemRegistry::initializeVanillaBlockItems() 中通过 registerSimpleBlock 注册，
// 不挂在 Items:: 静态指针上，因此通过 BlockItemRegistry / ItemRegistry 反查校验。
// ============================================================================

class BlackstoneVariantItemRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

// ========== 静态方块指针非空（前置条件） ==========

TEST_F(BlackstoneVariantItemRegistrationTest, BlackstoneVariantBlocks_AreNotNull)
{
    ASSERT_NE(VanillaBlocks::BLACKSTONE_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::BLACKSTONE_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::BLACKSTONE_WALL, nullptr);
    ASSERT_NE(VanillaBlocks::POLISHED_BLACKSTONE_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::POLISHED_BLACKSTONE_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::POLISHED_BLACKSTONE_WALL, nullptr);
    ASSERT_NE(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_WALL, nullptr);
}

// ========== BlockItemMapping：方块 id -> BlockItem* ==========

TEST_F(BlackstoneVariantItemRegistrationTest, BlackstoneStairs_HaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BLACKSTONE_STAIRS->blockId()), nullptr);
    EXPECT_NE(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_STAIRS->blockId()),
        nullptr);
}

TEST_F(BlackstoneVariantItemRegistrationTest, BlackstoneSlabs_HaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BLACKSTONE_SLAB->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_SLAB->blockId()), nullptr);
    EXPECT_NE(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_SLAB->blockId()), nullptr);
}

TEST_F(BlackstoneVariantItemRegistrationTest, BlackstoneWalls_HaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BLACKSTONE_WALL->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_WALL->blockId()), nullptr);
    EXPECT_NE(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_WALL->blockId()), nullptr);
}

// ========== BlockItemReverseMapping：物品 id -> 方块* ==========

TEST_F(BlackstoneVariantItemRegistrationTest, BlackstoneStairs_ReverseMapping)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BLACKSTONE_STAIRS->blockId());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(BlockItemRegistry::instance().getBlock(item->itemId()), VanillaBlocks::BLACKSTONE_STAIRS);

    item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_STAIRS->blockId());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(BlockItemRegistry::instance().getBlock(item->itemId()), VanillaBlocks::POLISHED_BLACKSTONE_STAIRS);

    item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_STAIRS->blockId());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(BlockItemRegistry::instance().getBlock(item->itemId()), VanillaBlocks::POLISHED_BLACKSTONE_BRICK_STAIRS);
}

TEST_F(BlackstoneVariantItemRegistrationTest, BlackstoneSlabs_ReverseMapping)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BLACKSTONE_SLAB->blockId());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(BlockItemRegistry::instance().getBlock(item->itemId()), VanillaBlocks::BLACKSTONE_SLAB);

    item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_SLAB->blockId());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(BlockItemRegistry::instance().getBlock(item->itemId()), VanillaBlocks::POLISHED_BLACKSTONE_SLAB);

    item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_SLAB->blockId());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(BlockItemRegistry::instance().getBlock(item->itemId()), VanillaBlocks::POLISHED_BLACKSTONE_BRICK_SLAB);
}

TEST_F(BlackstoneVariantItemRegistrationTest, BlackstoneWalls_ReverseMapping)
{
    const BlockItem* item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BLACKSTONE_WALL->blockId());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(BlockItemRegistry::instance().getBlock(item->itemId()), VanillaBlocks::BLACKSTONE_WALL);

    item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_WALL->blockId());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(BlockItemRegistry::instance().getBlock(item->itemId()), VanillaBlocks::POLISHED_BLACKSTONE_WALL);

    item = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_WALL->blockId());
    ASSERT_NE(item, nullptr);
    EXPECT_EQ(BlockItemRegistry::instance().getBlock(item->itemId()), VanillaBlocks::POLISHED_BLACKSTONE_BRICK_WALL);
}

// ========== MaxStackSize：所有 9 个变体均应为 64 ==========

TEST_F(BlackstoneVariantItemRegistrationTest, BlackstoneVariantItems_MaxStackSize64)
{
    EXPECT_EQ(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BLACKSTONE_STAIRS->blockId())->maxStackSize(), 64);
    EXPECT_EQ(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BLACKSTONE_SLAB->blockId())->maxStackSize(), 64);
    EXPECT_EQ(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BLACKSTONE_WALL->blockId())->maxStackSize(), 64);
    EXPECT_EQ(BlockItemRegistry::instance()
                  .getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_STAIRS->blockId())
                  ->maxStackSize(),
        64);
    EXPECT_EQ(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_SLAB->blockId())->maxStackSize(),
        64);
    EXPECT_EQ(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_WALL->blockId())->maxStackSize(),
        64);
    EXPECT_EQ(BlockItemRegistry::instance()
                  .getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_STAIRS->blockId())
                  ->maxStackSize(),
        64);
    EXPECT_EQ(BlockItemRegistry::instance()
                  .getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_SLAB->blockId())
                  ->maxStackSize(),
        64);
    EXPECT_EQ(BlockItemRegistry::instance()
                  .getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_WALL->blockId())
                  ->maxStackSize(),
        64);
}

// ========== ItemLocation：物品 id 应与方块 id 一致 ==========

TEST_F(BlackstoneVariantItemRegistrationTest, BlackstoneVariantItems_CorrectItemLocation)
{
    EXPECT_EQ(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BLACKSTONE_STAIRS->blockId())->itemLocation(),
        ResourceLocation("minecraft:blackstone_stairs"));
    EXPECT_EQ(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BLACKSTONE_SLAB->blockId())->itemLocation(),
        ResourceLocation("minecraft:blackstone_slab"));
    EXPECT_EQ(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BLACKSTONE_WALL->blockId())->itemLocation(),
        ResourceLocation("minecraft:blackstone_wall"));
    EXPECT_EQ(BlockItemRegistry::instance()
                  .getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_STAIRS->blockId())
                  ->itemLocation(),
        ResourceLocation("minecraft:polished_blackstone_stairs"));
    EXPECT_EQ(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_SLAB->blockId())->itemLocation(),
        ResourceLocation("minecraft:polished_blackstone_slab"));
    EXPECT_EQ(
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_WALL->blockId())->itemLocation(),
        ResourceLocation("minecraft:polished_blackstone_wall"));
    EXPECT_EQ(BlockItemRegistry::instance()
                  .getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_STAIRS->blockId())
                  ->itemLocation(),
        ResourceLocation("minecraft:polished_blackstone_brick_stairs"));
    EXPECT_EQ(BlockItemRegistry::instance()
                  .getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_SLAB->blockId())
                  ->itemLocation(),
        ResourceLocation("minecraft:polished_blackstone_brick_slab"));
    EXPECT_EQ(BlockItemRegistry::instance()
                  .getBlockItem(VanillaBlocks::POLISHED_BLACKSTONE_BRICK_WALL->blockId())
                  ->itemLocation(),
        ResourceLocation("minecraft:polished_blackstone_brick_wall"));
}

// ========== 批量注册验证：通过 ItemRegistry 查找所有黑石变体物品 ==========

TEST_F(BlackstoneVariantItemRegistrationTest, AllBlackstoneVariantItems_RegistryLookup)
{
    const char* blackstoneVariantItemNames[] = {
        "blackstone_stairs",
        "blackstone_slab",
        "blackstone_wall",
        "polished_blackstone_stairs",
        "polished_blackstone_slab",
        "polished_blackstone_wall",
        "polished_blackstone_brick_stairs",
        "polished_blackstone_brick_slab",
        "polished_blackstone_brick_wall",
    };
    for (const char* name : blackstoneVariantItemNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing block item: minecraft:" << name;
    }
}

// ========== 已有黑石基础方块物品未被破坏（回归测试） ==========

TEST_F(BlackstoneVariantItemRegistrationTest, ExistingBlackstoneBaseItems_StillRegistered)
{
    // blackstone / polished_blackstone / gilded_blackstone 在新增变体之前已注册，
    // 此处确认新增注册未破坏它们的注册关系。
    EXPECT_NE(ItemRegistry::instance().getItem(ResourceLocation("minecraft:blackstone")), nullptr);
    EXPECT_NE(ItemRegistry::instance().getItem(ResourceLocation("minecraft:polished_blackstone")), nullptr);
    EXPECT_NE(ItemRegistry::instance().getItem(ResourceLocation("minecraft:gilded_blackstone")), nullptr);
}
