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
 * @file MudCaveMangroveItemRegistrationTest.cpp
 * @brief 泥巴/洞穴/红树林方块物品注册测试
 *
 * 验证以下方块的 BlockItem 注册正确性：
 * - 泥巴系列方块（1.19 荒野更新）：mud, packed_mud, mud_bricks, mud_brick_stairs/slab/wall
 * - 洞穴方块（1.17+）：moss_block, moss_carpet, rooted_dirt, hanging_roots,
 *   azalea, flowering_azalea, azalea_leaves, flowering_azalea_leaves
 * - 红树林方块（1.19+）：mangrove_log, mangrove_wood, stripped_mangrove_log/wood,
 *   mangrove_planks, mangrove_leaves, mangrove_propagule, mangrove_roots, muddy_mangrove_roots
 */

#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "world/block/registry/VanillaBlocks.hpp"
#include <gtest/gtest.h>

using namespace mc;

// ============================================================================
// 泥巴/洞穴/红树林方块物品注册测试
// ============================================================================

class MudCaveMangroveItemRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

// ========== 泥巴系列方块物品 ==========

TEST_F(MudCaveMangroveItemRegistrationTest, MudBlocks_HaveBlockItems)
{
    // 泥巴 - 使用自定义 MudBlock 类
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MUD->blockId()), nullptr)
        << "mud should have a BlockItem";
    // 泥坯
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PACKED_MUD->blockId()), nullptr)
        << "packed_mud should have a BlockItem";
    // 泥砖
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MUD_BRICKS->blockId()), nullptr)
        << "mud_bricks should have a BlockItem";
    // 泥砖楼梯
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MUD_BRICK_STAIRS->blockId()), nullptr)
        << "mud_brick_stairs should have a BlockItem";
    // 泥砖台阶
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MUD_BRICK_SLAB->blockId()), nullptr)
        << "mud_brick_slab should have a BlockItem";
    // 泥砖墙
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MUD_BRICK_WALL->blockId()), nullptr)
        << "mud_brick_wall should have a BlockItem";
}

TEST_F(MudCaveMangroveItemRegistrationTest, MudBlocks_RegistryLookup)
{
    // 通过 ResourceLocation 查找泥巴系列方块物品
    const char* mudBlockNames[] = {
        "mud",
        "packed_mud",
        "mud_bricks",
        "mud_brick_stairs",
        "mud_brick_slab",
        "mud_brick_wall",
    };

    for (const char* name : mudBlockNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing block item: minecraft:" << name;
    }
}

// ========== 洞穴方块物品 ==========

TEST_F(MudCaveMangroveItemRegistrationTest, CaveBlocks_HaveBlockItems)
{
    // 苔藓方块
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MOSS_BLOCK->blockId()), nullptr)
        << "moss_block should have a BlockItem";
    // 苔藓地毯
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MOSS_CARPET->blockId()), nullptr)
        << "moss_carpet should have a BlockItem";
    // 生根泥土
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::ROOTED_DIRT->blockId()), nullptr)
        << "rooted_dirt should have a BlockItem";
    // 垂根
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::HANGING_ROOTS->blockId()), nullptr)
        << "hanging_roots should have a BlockItem";
    // 杜鹃花丛
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::AZALEA->blockId()), nullptr)
        << "azalea should have a BlockItem";
    // 盛开的杜鹃花丛
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::FLOWERING_AZALEA->blockId()), nullptr)
        << "flowering_azalea should have a BlockItem";
    // 杜鹃树叶
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::AZALEA_LEAVES->blockId()), nullptr)
        << "azalea_leaves should have a BlockItem";
    // 盛开的杜鹃树叶
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::FLOWERING_AZALEA_LEAVES->blockId()), nullptr)
        << "flowering_azalea_leaves should have a BlockItem";
}

TEST_F(MudCaveMangroveItemRegistrationTest, CaveBlocks_RegistryLookup)
{
    const char* caveBlockNames[] = {
        "moss_block",
        "moss_carpet",
        "rooted_dirt",
        "hanging_roots",
        "azalea",
        "flowering_azalea",
        "azalea_leaves",
        "flowering_azalea_leaves",
    };

    for (const char* name : caveBlockNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing block item: minecraft:" << name;
    }
}

// ========== 红树林方块物品 ==========

TEST_F(MudCaveMangroveItemRegistrationTest, MangroveBlocks_HaveBlockItems)
{
    // 红树原木
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_LOG->blockId()), nullptr)
        << "mangrove_log should have a BlockItem";
    // 红树木
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_WOOD->blockId()), nullptr)
        << "mangrove_wood should have a BlockItem";
    // 剥皮红树原木
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::STRIPPED_MANGROVE_LOG->blockId()), nullptr)
        << "stripped_mangrove_log should have a BlockItem";
    // 剥皮红树木
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::STRIPPED_MANGROVE_WOOD->blockId()), nullptr)
        << "stripped_mangrove_wood should have a BlockItem";
    // 红树木板
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_PLANKS->blockId()), nullptr)
        << "mangrove_planks should have a BlockItem";
    // 红树树叶
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_LEAVES->blockId()), nullptr)
        << "mangrove_leaves should have a BlockItem";
    // 红树胎生苗
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_PROPAGULE->blockId()), nullptr)
        << "mangrove_propagule should have a BlockItem";
    // 红树根
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_ROOTS->blockId()), nullptr)
        << "mangrove_roots should have a BlockItem";
    // 沾泥红树根
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MUDDY_MANGROVE_ROOTS->blockId()), nullptr)
        << "muddy_mangrove_roots should have a BlockItem";
}

TEST_F(MudCaveMangroveItemRegistrationTest, MangroveBlocks_RegistryLookup)
{
    const char* mangroveBlockNames[] = {
        "mangrove_log",
        "mangrove_wood",
        "stripped_mangrove_log",
        "stripped_mangrove_wood",
        "mangrove_planks",
        "mangrove_leaves",
        "mangrove_propagule",
        "mangrove_roots",
        "muddy_mangrove_roots",
    };

    for (const char* name : mangroveBlockNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing block item: minecraft:" << name;
    }
}
