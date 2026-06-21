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
 * LIABILITY, WHETHER AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
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
// 木材变种方块物品注册测试
// ============================================================================

class WoodVariantBlockItemTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 确保注册表已初始化
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
    }
};

// ========== 楼梯方块物品映射 ==========

TEST_F(WoodVariantBlockItemTest, Stairs_AllWoodTypesHaveBlockItems)
{
    // 主世界6种木材
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::SPRUCE_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BIRCH_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::JUNGLE_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::ACACIA_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DARK_OAK_STAIRS->blockId()), nullptr);
    // 新增木材类型
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHERRY_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BAMBOO_STAIRS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BAMBOO_MOSAIC_STAIRS->blockId()), nullptr);
}

// ========== 台阶方块物品映射 ==========

TEST_F(WoodVariantBlockItemTest, Slabs_AllWoodTypesHaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_SLAB->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_SLAB->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHERRY_SLAB->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_SLAB->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BAMBOO_SLAB->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BAMBOO_MOSAIC_SLAB->blockId()), nullptr);
}

// ========== 栅栏方块物品映射 ==========

TEST_F(WoodVariantBlockItemTest, Fences_AllWoodTypesHaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_FENCE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::SPRUCE_FENCE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BIRCH_FENCE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::JUNGLE_FENCE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::ACACIA_FENCE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DARK_OAK_FENCE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_FENCE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHERRY_FENCE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_FENCE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BAMBOO_FENCE->blockId()), nullptr);
}

// ========== 栅栏门方块物品映射 ==========

TEST_F(WoodVariantBlockItemTest, FenceGates_AllWoodTypesHaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_FENCE_GATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_FENCE_GATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHERRY_FENCE_GATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_FENCE_GATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BAMBOO_FENCE_GATE->blockId()), nullptr);
}

// ========== 门方块物品映射 ==========

TEST_F(WoodVariantBlockItemTest, Doors_AllWoodTypesHaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::SPRUCE_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BIRCH_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::JUNGLE_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::ACACIA_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DARK_OAK_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHERRY_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_DOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BAMBOO_DOOR->blockId()), nullptr);
}

// ========== 活板门方块物品映射 ==========

TEST_F(WoodVariantBlockItemTest, Trapdoors_AllWoodTypesHaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_TRAPDOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_TRAPDOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHERRY_TRAPDOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_TRAPDOOR->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BAMBOO_TRAPDOOR->blockId()), nullptr);
}

// ========== 按钮方块物品映射 ==========

TEST_F(WoodVariantBlockItemTest, Buttons_AllWoodTypesHaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_BUTTON->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_BUTTON->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHERRY_BUTTON->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BAMBOO_BUTTON->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_BUTTON->blockId()), nullptr);
}

// ========== 压力板方块物品映射 ==========

TEST_F(WoodVariantBlockItemTest, PressurePlates_AllWoodTypesHaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_PRESSURE_PLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::SPRUCE_PRESSURE_PLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BIRCH_PRESSURE_PLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::JUNGLE_PRESSURE_PLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::ACACIA_PRESSURE_PLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::DARK_OAK_PRESSURE_PLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CRIMSON_PRESSURE_PLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::WARPED_PRESSURE_PLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_PRESSURE_PLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHERRY_PRESSURE_PLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BAMBOO_PRESSURE_PLATE->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_PRESSURE_PLATE->blockId()), nullptr);
}

// ========== 告示牌物品和方块映射 ==========

TEST_F(WoodVariantBlockItemTest, Signs_AllWoodTypesRegisteredInItems)
{
    // 告示牌物品应在 ItemRegistry 中注册
    EXPECT_NE(Items::OAK_SIGN, nullptr);
    EXPECT_NE(Items::SPRUCE_SIGN, nullptr);
    EXPECT_NE(Items::BIRCH_SIGN, nullptr);
    EXPECT_NE(Items::JUNGLE_SIGN, nullptr);
    EXPECT_NE(Items::ACACIA_SIGN, nullptr);
    EXPECT_NE(Items::DARK_OAK_SIGN, nullptr);
    EXPECT_NE(Items::CRIMSON_SIGN, nullptr);
    EXPECT_NE(Items::WARPED_SIGN, nullptr);
    EXPECT_NE(Items::MANGROVE_SIGN, nullptr);
    EXPECT_NE(Items::CHERRY_SIGN, nullptr);
    EXPECT_NE(Items::BAMBOO_SIGN, nullptr);
    EXPECT_NE(Items::PALE_OAK_SIGN, nullptr);
}

TEST_F(WoodVariantBlockItemTest, Signs_BlockItemRegistryMappingsExist)
{
    // 站立告示牌的方块→物品映射应存在
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_SIGN->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_SIGN->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHERRY_SIGN->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BAMBOO_SIGN->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_SIGN->blockId()), nullptr);
}

TEST_F(WoodVariantBlockItemTest, Signs_WallSignMapsToSameItem)
{
    // 墙壁告示牌应映射到与站立告示牌相同的物品
    const BlockItem* standingItem = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_SIGN->blockId());
    const BlockItem* wallItem = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_WALL_SIGN->blockId());
    ASSERT_NE(standingItem, nullptr);
    ASSERT_NE(wallItem, nullptr);
    EXPECT_EQ(standingItem, wallItem) << "站立告示牌和墙壁告示牌应映射到相同的物品";

    // 新增木材类型
    const BlockItem* mangroveStanding =
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_SIGN->blockId());
    const BlockItem* mangroveWall =
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_WALL_SIGN->blockId());
    ASSERT_NE(mangroveStanding, nullptr);
    ASSERT_NE(mangroveWall, nullptr);
    EXPECT_EQ(mangroveStanding, mangroveWall) << "红树木站立告示牌和墙壁告示牌应映射到相同的物品";
}

// ========== 悬挂告示牌物品和方块映射 ==========

TEST_F(WoodVariantBlockItemTest, HangingSigns_AllWoodTypesRegisteredInItems)
{
    EXPECT_NE(Items::OAK_HANGING_SIGN, nullptr);
    EXPECT_NE(Items::SPRUCE_HANGING_SIGN, nullptr);
    EXPECT_NE(Items::BIRCH_HANGING_SIGN, nullptr);
    EXPECT_NE(Items::JUNGLE_HANGING_SIGN, nullptr);
    EXPECT_NE(Items::ACACIA_HANGING_SIGN, nullptr);
    EXPECT_NE(Items::DARK_OAK_HANGING_SIGN, nullptr);
    EXPECT_NE(Items::CRIMSON_HANGING_SIGN, nullptr);
    EXPECT_NE(Items::WARPED_HANGING_SIGN, nullptr);
    EXPECT_NE(Items::MANGROVE_HANGING_SIGN, nullptr);
    EXPECT_NE(Items::CHERRY_HANGING_SIGN, nullptr);
    EXPECT_NE(Items::BAMBOO_HANGING_SIGN, nullptr);
    EXPECT_NE(Items::PALE_OAK_HANGING_SIGN, nullptr);
}

TEST_F(WoodVariantBlockItemTest, HangingSigns_BlockItemRegistryMappingsExist)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_HANGING_SIGN->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_HANGING_SIGN->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHERRY_HANGING_SIGN->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::BAMBOO_HANGING_SIGN->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::PALE_OAK_HANGING_SIGN->blockId()), nullptr);
}

TEST_F(WoodVariantBlockItemTest, HangingSigns_WallHangingSignMapsToSameItem)
{
    // 悬挂告示牌：墙壁变体应映射到与悬挂变体相同的物品
    const BlockItem* oakHanging =
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_HANGING_SIGN->blockId());
    const BlockItem* oakWallHanging =
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::OAK_WALL_HANGING_SIGN->blockId());
    ASSERT_NE(oakHanging, nullptr);
    ASSERT_NE(oakWallHanging, nullptr);
    EXPECT_EQ(oakHanging, oakWallHanging) << "悬挂告示牌和墙壁悬挂告示牌应映射到相同的物品";

    // 新增木材类型
    const BlockItem* cherryHanging =
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHERRY_HANGING_SIGN->blockId());
    const BlockItem* cherryWallHanging =
        BlockItemRegistry::instance().getBlockItem(VanillaBlocks::CHERRY_WALL_HANGING_SIGN->blockId());
    ASSERT_NE(cherryHanging, nullptr);
    ASSERT_NE(cherryWallHanging, nullptr);
    EXPECT_EQ(cherryHanging, cherryWallHanging) << "樱花木悬挂告示牌和墙壁悬挂告示牌应映射到相同的物品";
}

// ========== 木质压力板物品注册 ==========

TEST_F(WoodVariantBlockItemTest, PressurePlateItems_AllWoodTypesRegisteredInItems)
{
    EXPECT_NE(Items::OAK_PRESSURE_PLATE, nullptr);
    EXPECT_NE(Items::SPRUCE_PRESSURE_PLATE, nullptr);
    EXPECT_NE(Items::BIRCH_PRESSURE_PLATE, nullptr);
    EXPECT_NE(Items::JUNGLE_PRESSURE_PLATE, nullptr);
    EXPECT_NE(Items::ACACIA_PRESSURE_PLATE, nullptr);
    EXPECT_NE(Items::DARK_OAK_PRESSURE_PLATE, nullptr);
    EXPECT_NE(Items::CRIMSON_PRESSURE_PLATE, nullptr);
    EXPECT_NE(Items::WARPED_PRESSURE_PLATE, nullptr);
    EXPECT_NE(Items::MANGROVE_PRESSURE_PLATE, nullptr);
    EXPECT_NE(Items::CHERRY_PRESSURE_PLATE, nullptr);
    EXPECT_NE(Items::BAMBOO_PRESSURE_PLATE, nullptr);
    EXPECT_NE(Items::PALE_OAK_PRESSURE_PLATE, nullptr);
}

// ========== 木质按钮物品注册 ==========

TEST_F(WoodVariantBlockItemTest, ButtonItems_NewWoodTypesRegisteredInItems)
{
    EXPECT_NE(Items::MANGROVE_BUTTON, nullptr);
    EXPECT_NE(Items::CHERRY_BUTTON, nullptr);
    EXPECT_NE(Items::BAMBOO_BUTTON, nullptr);
    EXPECT_NE(Items::PALE_OAK_BUTTON, nullptr);
}

// ========== 木质门物品注册 ==========

TEST_F(WoodVariantBlockItemTest, DoorItems_AllWoodTypesRegisteredInItems)
{
    EXPECT_NE(Items::OAK_DOOR, nullptr);
    EXPECT_NE(Items::SPRUCE_DOOR, nullptr);
    EXPECT_NE(Items::BIRCH_DOOR, nullptr);
    EXPECT_NE(Items::JUNGLE_DOOR, nullptr);
    EXPECT_NE(Items::ACACIA_DOOR, nullptr);
    EXPECT_NE(Items::DARK_OAK_DOOR, nullptr);
    EXPECT_NE(Items::MANGROVE_DOOR, nullptr);
    EXPECT_NE(Items::CHERRY_DOOR, nullptr);
    EXPECT_NE(Items::PALE_OAK_DOOR, nullptr);
    EXPECT_NE(Items::BAMBOO_DOOR, nullptr);
}

// ========== 物品与方块的双向映射 ==========

TEST_F(WoodVariantBlockItemTest, BlockToItemAndItemToBlockAreConsistent)
{
    // 对于通过 registerSimpleBlock 注册的方块，方块→物品和物品→方块映射都应存在
    const BlockItem* stairItem = BlockItemRegistry::instance().getBlockItem(VanillaBlocks::MANGROVE_STAIRS->blockId());
    ASSERT_NE(stairItem, nullptr);
    const Block* block = BlockItemRegistry::instance().getBlock(stairItem->itemId());
    EXPECT_EQ(block, VanillaBlocks::MANGROVE_STAIRS) << "物品→方块映射应指回原方块";
}

TEST_F(WoodVariantBlockItemTest, SignItemPointsToStandingSignBlock)
{
    // 告示牌物品应指向站立告示牌方块（WallOrFloorItem 的地板变体）
    const Block* block = BlockItemRegistry::instance().getBlock(Items::MANGROVE_SIGN->itemId());
    EXPECT_EQ(block, VanillaBlocks::MANGROVE_SIGN) << "告示牌物品应映射到站立告示牌方块";
}

TEST_F(WoodVariantBlockItemTest, HangingSignItemPointsToHangingSignBlock)
{
    const Block* block = BlockItemRegistry::instance().getBlock(Items::CHERRY_HANGING_SIGN->itemId());
    EXPECT_EQ(block, VanillaBlocks::CHERRY_HANGING_SIGN) << "悬挂告示牌物品应映射到悬挂告示牌方块";
}
