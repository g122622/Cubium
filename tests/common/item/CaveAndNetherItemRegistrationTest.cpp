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

/**
 * @file CaveAndNetherItemRegistrationTest.cpp
 * @brief 洞穴与下界新增方块物品注册测试
 *
 * 验证以下方块的 BlockItem 注册正确性：
 * - 孢子花（1.17 洞穴与山崖）
 * - 粉红色花瓣（1.20.3 足迹与故事）
 * - 绯红木门、诡异木门（1.16 下界更新）
 * - 绯红木活板门、诡异木活板门（1.16 下界更新）
 */

#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/items/block/BlockItem.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// 洞穴与下界新增方块物品注册测试
// ============================================================================

class CaveAndNetherItemRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

// ========== 孢子花方块物品 ==========

TEST_F(CaveAndNetherItemRegistrationTest, SporeBlossom_HasBlockItem)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::SPORE_BLOSSOM->blockId()), nullptr)
        << "spore_blossom should have a BlockItem";
}

TEST_F(CaveAndNetherItemRegistrationTest, SporeBlossom_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "spore_blossom"));
    EXPECT_NE(item, nullptr) << "Missing block item: minecraft:spore_blossom";
}

TEST_F(CaveAndNetherItemRegistrationTest, SporeBlossom_IsBlockItem)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "spore_blossom"));
    ASSERT_NE(item, nullptr);
    auto* blockItem = dynamic_cast<const BlockItem*>(item);
    EXPECT_NE(blockItem, nullptr) << "spore_blossom should be a BlockItem";
}

// ========== 粉红色花瓣方块物品 ==========

TEST_F(CaveAndNetherItemRegistrationTest, PinkPetals_HasBlockItem)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PINK_PETALS->blockId()), nullptr)
        << "pink_petals should have a BlockItem";
}

TEST_F(CaveAndNetherItemRegistrationTest, PinkPetals_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "pink_petals"));
    EXPECT_NE(item, nullptr) << "Missing block item: minecraft:pink_petals";
}

TEST_F(CaveAndNetherItemRegistrationTest, PinkPetals_IsBlockItem)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "pink_petals"));
    ASSERT_NE(item, nullptr);
    auto* blockItem = dynamic_cast<const BlockItem*>(item);
    EXPECT_NE(blockItem, nullptr) << "pink_petals should be a BlockItem";
}

// ========== 绯红木门物品 ==========

TEST_F(CaveAndNetherItemRegistrationTest, CrimsonDoor_StaticPointerNotNull)
{
    ASSERT_NE(Items::CRIMSON_DOOR, nullptr) << "CRIMSON_DOOR should be registered";
}

TEST_F(CaveAndNetherItemRegistrationTest, CrimsonDoor_CorrectItemId)
{
    EXPECT_EQ(Items::CRIMSON_DOOR->itemLocation(), ResourceLocation("minecraft", "crimson_door"));
}

TEST_F(CaveAndNetherItemRegistrationTest, CrimsonDoor_MaxStackSize64)
{
    EXPECT_EQ(Items::CRIMSON_DOOR->maxStackSize(), 64);
}

TEST_F(CaveAndNetherItemRegistrationTest, CrimsonDoor_BlockItemMapping)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CRIMSON_DOOR->blockId()), nullptr)
        << "crimson_door block should have a BlockItem";
}

TEST_F(CaveAndNetherItemRegistrationTest, CrimsonDoor_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "crimson_door"));
    EXPECT_EQ(item, Items::CRIMSON_DOOR);
}

// ========== 诡异木门物品 ==========

TEST_F(CaveAndNetherItemRegistrationTest, WarpedDoor_StaticPointerNotNull)
{
    ASSERT_NE(Items::WARPED_DOOR, nullptr) << "WARPED_DOOR should be registered";
}

TEST_F(CaveAndNetherItemRegistrationTest, WarpedDoor_CorrectItemId)
{
    EXPECT_EQ(Items::WARPED_DOOR->itemLocation(), ResourceLocation("minecraft", "warped_door"));
}

TEST_F(CaveAndNetherItemRegistrationTest, WarpedDoor_MaxStackSize64)
{
    EXPECT_EQ(Items::WARPED_DOOR->maxStackSize(), 64);
}

TEST_F(CaveAndNetherItemRegistrationTest, WarpedDoor_BlockItemMapping)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::WARPED_DOOR->blockId()), nullptr)
        << "warped_door block should have a BlockItem";
}

TEST_F(CaveAndNetherItemRegistrationTest, WarpedDoor_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "warped_door"));
    EXPECT_EQ(item, Items::WARPED_DOOR);
}

// ========== 绯红木活板门物品 ==========

TEST_F(CaveAndNetherItemRegistrationTest, CrimsonTrapdoor_StaticPointerNotNull)
{
    ASSERT_NE(Items::CRIMSON_TRAPDOOR, nullptr) << "CRIMSON_TRAPDOOR should be registered";
}

TEST_F(CaveAndNetherItemRegistrationTest, CrimsonTrapdoor_CorrectItemId)
{
    EXPECT_EQ(Items::CRIMSON_TRAPDOOR->itemLocation(), ResourceLocation("minecraft", "crimson_trapdoor"));
}

TEST_F(CaveAndNetherItemRegistrationTest, CrimsonTrapdoor_MaxStackSize64)
{
    EXPECT_EQ(Items::CRIMSON_TRAPDOOR->maxStackSize(), 64);
}

TEST_F(CaveAndNetherItemRegistrationTest, CrimsonTrapdoor_BlockItemMapping)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CRIMSON_TRAPDOOR->blockId()), nullptr)
        << "crimson_trapdoor block should have a BlockItem";
}

TEST_F(CaveAndNetherItemRegistrationTest, CrimsonTrapdoor_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "crimson_trapdoor"));
    EXPECT_EQ(item, Items::CRIMSON_TRAPDOOR);
}

// ========== 诡异木活板门物品 ==========

TEST_F(CaveAndNetherItemRegistrationTest, WarpedTrapdoor_StaticPointerNotNull)
{
    ASSERT_NE(Items::WARPED_TRAPDOOR, nullptr) << "WARPED_TRAPDOOR should be registered";
}

TEST_F(CaveAndNetherItemRegistrationTest, WarpedTrapdoor_CorrectItemId)
{
    EXPECT_EQ(Items::WARPED_TRAPDOOR->itemLocation(), ResourceLocation("minecraft", "warped_trapdoor"));
}

TEST_F(CaveAndNetherItemRegistrationTest, WarpedTrapdoor_MaxStackSize64)
{
    EXPECT_EQ(Items::WARPED_TRAPDOOR->maxStackSize(), 64);
}

TEST_F(CaveAndNetherItemRegistrationTest, WarpedTrapdoor_BlockItemMapping)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::WARPED_TRAPDOOR->blockId()), nullptr)
        << "warped_trapdoor block should have a BlockItem";
}

TEST_F(CaveAndNetherItemRegistrationTest, WarpedTrapdoor_RegistryLookup)
{
    auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", "warped_trapdoor"));
    EXPECT_EQ(item, Items::WARPED_TRAPDOOR);
}

// ========== 批量验证 - 确保所有新增物品都能通过注册表查找到 ==========

TEST_F(CaveAndNetherItemRegistrationTest, AllItems_RegistryLookup)
{
    const char* itemNames[] = {
        "spore_blossom",
        "pink_petals",
        "crimson_door",
        "warped_door",
        "crimson_trapdoor",
        "warped_trapdoor",
    };

    for (const char* name : itemNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing item: minecraft:" << name;
    }
}
