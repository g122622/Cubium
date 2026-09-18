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

#include <gtest/gtest.h>

#include "item/Items.hpp"
#include "item/core/ItemRegistry.hpp"
#include "item/items/block/BlockItemRegistry.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/Material.hpp"
#include "world/block/registry/VanillaBlocks.hpp"

using namespace mc;

// ============================================================================
// 新增楼梯、台阶、墙方块注册测试
//
// 验证范围：
// 1) 方块静态指针不为 nullptr
// 2) 方块属性（硬度/抗性/材质/工具）与 MC 原版一致
// 3) BlockItem 注册与方块对应
// 4) 标签包含正确的方块
// ============================================================================

class BuildingVariantRegistrationTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        Items::initialize();
        BlockItemRegistry::instance().initializeVanillaBlockItems();
        BlockTags::initialize();
    }
};

// ========== 楼梯方块指针非空验证 ==========

TEST_F(BuildingVariantRegistrationTest, Stairs_PointersNotNull)
{
    // 花岗岩系列楼梯
    ASSERT_NE(VanillaBlocks::GRANITE_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::POLISHED_GRANITE_STAIRS, nullptr);
    // 闪长岩系列楼梯
    ASSERT_NE(VanillaBlocks::DIORITE_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::POLISHED_DIORITE_STAIRS, nullptr);
    // 安山岩系列楼梯
    ASSERT_NE(VanillaBlocks::ANDESITE_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::POLISHED_ANDESITE_STAIRS, nullptr);
    // 砖与苔石圆石楼梯
    ASSERT_NE(VanillaBlocks::BRICK_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::MOSSY_COBBLESTONE_STAIRS, nullptr);
    // 石英与紫珀楼梯
    ASSERT_NE(VanillaBlocks::QUARTZ_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::SMOOTH_QUARTZ_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::PURPUR_STAIRS, nullptr);
    // 红砂岩楼梯
    ASSERT_NE(VanillaBlocks::RED_SANDSTONE_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::SMOOTH_RED_SANDSTONE_STAIRS, nullptr);
    // 下界砖与末地石砖楼梯
    ASSERT_NE(VanillaBlocks::NETHER_BRICK_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::RED_NETHER_BRICK_STAIRS, nullptr);
    ASSERT_NE(VanillaBlocks::END_STONE_BRICK_STAIRS, nullptr);
}

// ========== 台阶方块指针非空验证 ==========

TEST_F(BuildingVariantRegistrationTest, Slabs_PointersNotNull)
{
    // 花岗岩系列台阶
    ASSERT_NE(VanillaBlocks::GRANITE_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::POLISHED_GRANITE_SLAB, nullptr);
    // 闪长岩系列台阶
    ASSERT_NE(VanillaBlocks::DIORITE_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::POLISHED_DIORITE_SLAB, nullptr);
    // 安山岩系列台阶
    ASSERT_NE(VanillaBlocks::ANDESITE_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::POLISHED_ANDESITE_SLAB, nullptr);
    // 砖与苔石圆石台阶
    ASSERT_NE(VanillaBlocks::BRICK_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::MOSSY_COBBLESTONE_SLAB, nullptr);
    // 石英与紫珀台阶
    ASSERT_NE(VanillaBlocks::QUARTZ_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::SMOOTH_QUARTZ_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::PURPUR_SLAB, nullptr);
    // 红砂岩台阶
    ASSERT_NE(VanillaBlocks::RED_SANDSTONE_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::SMOOTH_RED_SANDSTONE_SLAB, nullptr);
    // 切制砂岩台阶
    ASSERT_NE(VanillaBlocks::CUT_SANDSTONE_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::CUT_RED_SANDSTONE_SLAB, nullptr);
    // 平滑石头与石化橡木台阶
    ASSERT_NE(VanillaBlocks::SMOOTH_STONE_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::PETRIFIED_OAK_SLAB, nullptr);
    // 下界砖与末地石砖台阶
    ASSERT_NE(VanillaBlocks::NETHER_BRICK_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::RED_NETHER_BRICK_SLAB, nullptr);
    ASSERT_NE(VanillaBlocks::END_STONE_BRICK_SLAB, nullptr);
}

// ========== 墙方块指针非空验证 ==========

TEST_F(BuildingVariantRegistrationTest, Walls_PointersNotNull)
{
    ASSERT_NE(VanillaBlocks::MOSSY_COBBLESTONE_WALL, nullptr);
    ASSERT_NE(VanillaBlocks::BRICK_WALL, nullptr);
    ASSERT_NE(VanillaBlocks::PRISMARINE_WALL, nullptr);
    ASSERT_NE(VanillaBlocks::SANDSTONE_WALL, nullptr);
    ASSERT_NE(VanillaBlocks::RED_SANDSTONE_WALL, nullptr);
    ASSERT_NE(VanillaBlocks::NETHER_BRICK_WALL, nullptr);
    ASSERT_NE(VanillaBlocks::RED_NETHER_BRICK_WALL, nullptr);
    ASSERT_NE(VanillaBlocks::END_STONE_BRICK_WALL, nullptr);
    ASSERT_NE(VanillaBlocks::GRANITE_WALL, nullptr);
    ASSERT_NE(VanillaBlocks::DIORITE_WALL, nullptr);
    ASSERT_NE(VanillaBlocks::ANDESITE_WALL, nullptr);
}

// ========== 基础方块指针验证 ==========

TEST_F(BuildingVariantRegistrationTest, BaseBlocks_PointersNotNull)
{
    ASSERT_NE(VanillaBlocks::NETHER_BRICKS, nullptr);
    ASSERT_NE(VanillaBlocks::RED_NETHER_BRICKS, nullptr);
    ASSERT_NE(VanillaBlocks::SMOOTH_QUARTZ, nullptr);
}

// ========== 楼梯方块属性验证（与 MC 原版对照） ==========

TEST_F(BuildingVariantRegistrationTest, Stairs_PropertiesMatchMC)
{
    // 花岗岩楼梯: 硬度1.5, 抗性6.0, 石镐
    EXPECT_FLOAT_EQ(VanillaBlocks::GRANITE_STAIRS->hardness(), 1.5f);
    EXPECT_FLOAT_EQ(VanillaBlocks::GRANITE_STAIRS->resistance(), 6.0f);
    EXPECT_EQ(&VanillaBlocks::GRANITE_STAIRS->material(), &Material::ROCK);

    // 砖楼梯: 硬度2.0, 抗性6.0
    EXPECT_FLOAT_EQ(VanillaBlocks::BRICK_STAIRS->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::BRICK_STAIRS->resistance(), 6.0f);
    EXPECT_EQ(&VanillaBlocks::BRICK_STAIRS->material(), &Material::ROCK);

    // 石英楼梯: 硬度0.8, 抗性0.8
    EXPECT_FLOAT_EQ(VanillaBlocks::QUARTZ_STAIRS->hardness(), 0.8f);
    EXPECT_FLOAT_EQ(VanillaBlocks::QUARTZ_STAIRS->resistance(), 0.8f);

    // 下界砖楼梯: 硬度2.0, 抗性6.0, 石镐, requiresTool
    EXPECT_FLOAT_EQ(VanillaBlocks::NETHER_BRICK_STAIRS->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::NETHER_BRICK_STAIRS->resistance(), 6.0f);
    EXPECT_EQ(&VanillaBlocks::NETHER_BRICK_STAIRS->material(), &Material::ROCK);

    // 末地石砖楼梯: 硬度3.0, 抗性9.0
    EXPECT_FLOAT_EQ(VanillaBlocks::END_STONE_BRICK_STAIRS->hardness(), 3.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::END_STONE_BRICK_STAIRS->resistance(), 9.0f);

    // 紫珀楼梯: 硬度1.5, 抗性6.0
    EXPECT_FLOAT_EQ(VanillaBlocks::PURPUR_STAIRS->hardness(), 1.5f);
    EXPECT_FLOAT_EQ(VanillaBlocks::PURPUR_STAIRS->resistance(), 6.0f);
}

// ========== 台阶方块属性验证 ==========

TEST_F(BuildingVariantRegistrationTest, Slabs_PropertiesMatchMC)
{
    // 花岗岩台阶: 硬度1.5, 抗性6.0
    EXPECT_FLOAT_EQ(VanillaBlocks::GRANITE_SLAB->hardness(), 1.5f);
    EXPECT_FLOAT_EQ(VanillaBlocks::GRANITE_SLAB->resistance(), 6.0f);
    EXPECT_EQ(&VanillaBlocks::GRANITE_SLAB->material(), &Material::ROCK);

    // 切制砂岩台阶: 硬度2.0, 抗性6.0
    EXPECT_FLOAT_EQ(VanillaBlocks::CUT_SANDSTONE_SLAB->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::CUT_SANDSTONE_SLAB->resistance(), 6.0f);

    // 平滑石头台阶: 硬度2.0, 抗性6.0
    EXPECT_FLOAT_EQ(VanillaBlocks::SMOOTH_STONE_SLAB->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::SMOOTH_STONE_SLAB->resistance(), 6.0f);

    // 石化橡木台阶: 硬度2.0, 抗性2.0, 可燃
    EXPECT_FLOAT_EQ(VanillaBlocks::PETRIFIED_OAK_SLAB->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::PETRIFIED_OAK_SLAB->resistance(), 2.0f);
    EXPECT_EQ(&VanillaBlocks::PETRIFIED_OAK_SLAB->material(), &Material::WOOD);

    // 下界砖台阶: 硬度2.0, 抗性6.0
    EXPECT_FLOAT_EQ(VanillaBlocks::NETHER_BRICK_SLAB->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::NETHER_BRICK_SLAB->resistance(), 6.0f);
}

// ========== 墙方块属性验证 ==========

TEST_F(BuildingVariantRegistrationTest, Walls_PropertiesMatchMC)
{
    // 苔石圆石墙: 硬度2.0, 抗性6.0
    EXPECT_FLOAT_EQ(VanillaBlocks::MOSSY_COBBLESTONE_WALL->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::MOSSY_COBBLESTONE_WALL->resistance(), 6.0f);

    // 砖墙: 硬度2.0, 抗性6.0
    EXPECT_FLOAT_EQ(VanillaBlocks::BRICK_WALL->hardness(), 2.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::BRICK_WALL->resistance(), 6.0f);

    // 砂岩墙: 硬度0.8, 抗性0.8
    EXPECT_FLOAT_EQ(VanillaBlocks::SANDSTONE_WALL->hardness(), 0.8f);

    // 末地石砖墙: 硬度3.0, 抗性9.0
    EXPECT_FLOAT_EQ(VanillaBlocks::END_STONE_BRICK_WALL->hardness(), 3.0f);
    EXPECT_FLOAT_EQ(VanillaBlocks::END_STONE_BRICK_WALL->resistance(), 9.0f);

    // 花岗岩墙: 硬度1.5, 抗性6.0
    EXPECT_FLOAT_EQ(VanillaBlocks::GRANITE_WALL->hardness(), 1.5f);
    EXPECT_FLOAT_EQ(VanillaBlocks::GRANITE_WALL->resistance(), 6.0f);
}

// ========== BlockItem 映射验证 ==========

TEST_F(BuildingVariantRegistrationTest, Stairs_HaveBlockItems)
{
    // 通过 BlockItemRegistry 验证楼梯方块都有对应的物品
    const char* stairNames[] = {
        "granite_stairs",
        "polished_granite_stairs",
        "diorite_stairs",
        "polished_diorite_stairs",
        "andesite_stairs",
        "polished_andesite_stairs",
        "brick_stairs",
        "mossy_cobblestone_stairs",
        "quartz_stairs",
        "smooth_quartz_stairs",
        "purpur_stairs",
        "red_sandstone_stairs",
        "smooth_red_sandstone_stairs",
        "nether_brick_stairs",
        "red_nether_brick_stairs",
        "end_stone_brick_stairs",
    };
    for (const char* name : stairNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing stair block item: minecraft:" << name;
    }
}

TEST_F(BuildingVariantRegistrationTest, Slabs_HaveBlockItems)
{
    const char* slabNames[] = {
        "granite_slab",
        "polished_granite_slab",
        "diorite_slab",
        "polished_diorite_slab",
        "andesite_slab",
        "polished_andesite_slab",
        "brick_slab",
        "mossy_cobblestone_slab",
        "quartz_slab",
        "smooth_quartz_slab",
        "purpur_slab",
        "red_sandstone_slab",
        "smooth_red_sandstone_slab",
        "cut_sandstone_slab",
        "cut_red_sandstone_slab",
        "smooth_stone_slab",
        "petrified_oak_slab",
        "nether_brick_slab",
        "red_nether_brick_slab",
        "end_stone_brick_slab",
    };
    for (const char* name : slabNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing slab block item: minecraft:" << name;
    }
}

TEST_F(BuildingVariantRegistrationTest, Walls_HaveBlockItems)
{
    const char* wallNames[] = {
        "mossy_cobblestone_wall",
        "brick_wall",
        "prismarine_wall",
        "sandstone_wall",
        "red_sandstone_wall",
        "nether_brick_wall",
        "red_nether_brick_wall",
        "end_stone_brick_wall",
        "granite_wall",
        "diorite_wall",
        "andesite_wall",
    };
    for (const char* name : wallNames) {
        auto* item = ItemRegistry::instance().getItem(ResourceLocation("minecraft", name));
        EXPECT_NE(item, nullptr) << "Missing wall block item: minecraft:" << name;
    }
}

TEST_F(BuildingVariantRegistrationTest, BaseBlocks_HaveBlockItems)
{
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::NETHER_BRICKS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::RED_NETHER_BRICKS->blockId()), nullptr);
    EXPECT_NE(BlockItemRegistry::instance().getBlockItem(VanillaBlocks::SMOOTH_QUARTZ->blockId()), nullptr);
}

// ========== 标签验证 ==========

TEST_F(BuildingVariantRegistrationTest, StairsTagContainsNewStairs)
{
    auto& stairsTag = BlockTags::STAIRS();
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::GRANITE_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::POLISHED_GRANITE_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::DIORITE_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::POLISHED_DIORITE_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::ANDESITE_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::POLISHED_ANDESITE_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::BRICK_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::MOSSY_COBBLESTONE_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::QUARTZ_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::SMOOTH_QUARTZ_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::PURPUR_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::RED_SANDSTONE_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::SMOOTH_RED_SANDSTONE_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::NETHER_BRICK_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::RED_NETHER_BRICK_STAIRS));
    EXPECT_TRUE(stairsTag.contains(*VanillaBlocks::END_STONE_BRICK_STAIRS));
}

TEST_F(BuildingVariantRegistrationTest, SlabsTagContainsNewSlabs)
{
    auto& slabsTag = BlockTags::SLABS();
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::GRANITE_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::POLISHED_GRANITE_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::DIORITE_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::POLISHED_DIORITE_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::ANDESITE_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::POLISHED_ANDESITE_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::BRICK_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::MOSSY_COBBLESTONE_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::QUARTZ_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::SMOOTH_QUARTZ_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::PURPUR_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::RED_SANDSTONE_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::SMOOTH_RED_SANDSTONE_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::CUT_SANDSTONE_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::CUT_RED_SANDSTONE_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::SMOOTH_STONE_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::PETRIFIED_OAK_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::NETHER_BRICK_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::RED_NETHER_BRICK_SLAB));
    EXPECT_TRUE(slabsTag.contains(*VanillaBlocks::END_STONE_BRICK_SLAB));
}

TEST_F(BuildingVariantRegistrationTest, WallsTagContainsNewWalls)
{
    auto& wallsTag = BlockTags::WALLS();
    EXPECT_TRUE(wallsTag.contains(*VanillaBlocks::MOSSY_COBBLESTONE_WALL));
    EXPECT_TRUE(wallsTag.contains(*VanillaBlocks::BRICK_WALL));
    EXPECT_TRUE(wallsTag.contains(*VanillaBlocks::PRISMARINE_WALL));
    EXPECT_TRUE(wallsTag.contains(*VanillaBlocks::SANDSTONE_WALL));
    EXPECT_TRUE(wallsTag.contains(*VanillaBlocks::RED_SANDSTONE_WALL));
    EXPECT_TRUE(wallsTag.contains(*VanillaBlocks::NETHER_BRICK_WALL));
    EXPECT_TRUE(wallsTag.contains(*VanillaBlocks::RED_NETHER_BRICK_WALL));
    EXPECT_TRUE(wallsTag.contains(*VanillaBlocks::END_STONE_BRICK_WALL));
    EXPECT_TRUE(wallsTag.contains(*VanillaBlocks::GRANITE_WALL));
    EXPECT_TRUE(wallsTag.contains(*VanillaBlocks::DIORITE_WALL));
    EXPECT_TRUE(wallsTag.contains(*VanillaBlocks::ANDESITE_WALL));
    // 注意: polished_granite/diorite/andesite_wall 不在 MC 原版 walls 标签中
}

// ========== BlockRegistry ResourceLocation 查找验证 ==========

TEST_F(BuildingVariantRegistrationTest, Stairs_RegistryLookup)
{
    // 通过 ResourceLocation 验证方块注册正确
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:granite_stairs")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:brick_stairs")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:nether_brick_stairs")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:end_stone_brick_stairs")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:purpur_stairs")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:quartz_stairs")), nullptr);
}

TEST_F(BuildingVariantRegistrationTest, Slabs_RegistryLookup)
{
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:granite_slab")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:brick_slab")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:nether_brick_slab")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:end_stone_brick_slab")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:purpur_slab")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:smooth_stone_slab")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:petrified_oak_slab")), nullptr);
}

TEST_F(BuildingVariantRegistrationTest, Walls_RegistryLookup)
{
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:mossy_cobblestone_wall")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:brick_wall")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:nether_brick_wall")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:end_stone_brick_wall")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:granite_wall")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:prismarine_wall")), nullptr);
}

TEST_F(BuildingVariantRegistrationTest, BaseBlocks_RegistryLookup)
{
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:nether_bricks")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:red_nether_bricks")), nullptr);
    EXPECT_NE(BlockRegistry::instance().getBlock(ResourceLocation("minecraft:smooth_quartz")), nullptr);
}
