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

#include "common/world/block/registry/ColoredBlocks.hpp"
#include "common/world/block/registry/CopperBlocks.hpp"
#include "common/world/block/registry/NetherBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"

using namespace mc;

class BlockTagsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BlockTags::initialize();
    }
};

// ============================================================================
// VALID_SWEET_BERRY_BUSH_GROUND Tests
// ============================================================================

TEST_F(BlockTagsTest, ValidSweetBerryBushGroundContainsGrassBlock)
{
    EXPECT_TRUE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(*VanillaBlocks::GRASS_BLOCK));
}

TEST_F(BlockTagsTest, ValidSweetBerryBushGroundContainsDirt)
{
    EXPECT_TRUE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(*VanillaBlocks::DIRT));
}

TEST_F(BlockTagsTest, ValidSweetBerryBushGroundContainsCoarseDirt)
{
    EXPECT_TRUE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(*VanillaBlocks::COARSE_DIRT));
}

TEST_F(BlockTagsTest, ValidSweetBerryBushGroundContainsPodzol)
{
    EXPECT_TRUE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(*VanillaBlocks::PODZOL));
}

TEST_F(BlockTagsTest, ValidSweetBerryBushGroundContainsFarmland)
{
    EXPECT_TRUE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(*VanillaBlocks::FARMLAND));
}

TEST_F(BlockTagsTest, ValidSweetBerryBushGroundDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(*VanillaBlocks::STONE));
}

TEST_F(BlockTagsTest, ValidSweetBerryBushGroundDoesNotContainSand)
{
    EXPECT_FALSE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(*VanillaBlocks::SAND));
}

TEST_F(BlockTagsTest, ValidSweetBerryBushGroundDoesNotContainGravel)
{
    EXPECT_FALSE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(*VanillaBlocks::GRAVEL));
}

TEST_F(BlockTagsTest, ValidSweetBerryBushGroundDoesNotContainWater)
{
    EXPECT_FALSE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(*VanillaBlocks::WATER));
}

TEST_F(BlockTagsTest, ValidSweetBerryBushGroundIdIsCorrect)
{
    EXPECT_EQ(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().getId(),
        ResourceLocation("minecraft", "valid_sweet_berry_bush_ground"));
}

// ============================================================================
// Other BlockTags Tests
// ============================================================================

TEST_F(BlockTagsTest, LogsContainsOakLog)
{
    EXPECT_TRUE(BlockTags::LOGS().contains(*VanillaBlocks::OAK_LOG));
}

TEST_F(BlockTagsTest, LogsContainsSpruceLog)
{
    EXPECT_TRUE(BlockTags::LOGS().contains(*VanillaBlocks::SPRUCE_LOG));
}

TEST_F(BlockTagsTest, LogsContainsBirchLog)
{
    EXPECT_TRUE(BlockTags::LOGS().contains(*VanillaBlocks::BIRCH_LOG));
}

TEST_F(BlockTagsTest, JungleLogsContainsJungleLog)
{
    EXPECT_TRUE(BlockTags::JUNGLE_LOGS().contains(*VanillaBlocks::JUNGLE_LOG));
}

TEST_F(BlockTagsTest, JungleLogsDoesNotContainOakLog)
{
    EXPECT_FALSE(BlockTags::JUNGLE_LOGS().contains(*VanillaBlocks::OAK_LOG));
}

TEST_F(BlockTagsTest, DirtContainsDirt)
{
    EXPECT_TRUE(BlockTags::DIRT().contains(*VanillaBlocks::DIRT));
}

TEST_F(BlockTagsTest, DirtContainsGrassBlock)
{
    EXPECT_TRUE(BlockTags::DIRT().contains(*VanillaBlocks::GRASS_BLOCK));
}

TEST_F(BlockTagsTest, DirtContainsPodzol)
{
    EXPECT_TRUE(BlockTags::DIRT().contains(*VanillaBlocks::PODZOL));
}

TEST_F(BlockTagsTest, DirtDoesNotContainFarmland)
{
    // MC 1.21.11 中 farmland 不在 dirt 标签中
    EXPECT_FALSE(BlockTags::DIRT().contains(*VanillaBlocks::FARMLAND));
}

TEST_F(BlockTagsTest, SandContainsSand)
{
    EXPECT_TRUE(BlockTags::SAND().contains(*VanillaBlocks::SAND));
}

TEST_F(BlockTagsTest, SandContainsRedSand)
{
    EXPECT_TRUE(BlockTags::SAND().contains(*VanillaBlocks::RED_SAND));
}

TEST_F(BlockTagsTest, SandContainsSuspiciousSand)
{
    // MC 1.21.11: suspicious_sand 在 sand 标签中
    EXPECT_TRUE(BlockTags::SAND().contains(ResourceLocation("minecraft", "suspicious_sand")));
}

TEST_F(BlockTagsTest, SandDoesNotContainSoulSand)
{
    // MC 1.21.11: soul_sand 不在 sand 标签中（soul_sand 属于 soul_fire_base_blocks）
    EXPECT_FALSE(BlockTags::SAND().contains(ResourceLocation("minecraft", "soul_sand")));
}

TEST_F(BlockTagsTest, SoulFireBaseBlocksContainsSoulSand)
{
    EXPECT_TRUE(BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(*VanillaBlocks::SOUL_SAND));
}

TEST_F(BlockTagsTest, SoulFireBaseBlocksContainsSoulSoil)
{
    EXPECT_TRUE(BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(*VanillaBlocks::SOUL_SOIL));
}

// ============================================================================
// CAMPFIRES Tests
// ============================================================================

TEST_F(BlockTagsTest, CampfiresContainsCampfire)
{
    EXPECT_TRUE(BlockTags::CAMPFIRES().contains(*block_registry::NetherBlocks::CAMPFIRE));
}

TEST_F(BlockTagsTest, CampfiresContainsSoulCampfire)
{
    EXPECT_TRUE(BlockTags::CAMPFIRES().contains(*block_registry::NetherBlocks::SOUL_CAMPFIRE));
}

TEST_F(BlockTagsTest, CampfiresDoesNotContainTorch)
{
    EXPECT_FALSE(BlockTags::CAMPFIRES().contains(*VanillaBlocks::TORCH));
}

TEST_F(BlockTagsTest, CampfiresIdIsCorrect)
{
    EXPECT_EQ(BlockTags::CAMPFIRES().getId(), ResourceLocation("minecraft", "campfires"));
}

TEST_F(BlockTagsTest, LeavesContainsOakLeaves)
{
    EXPECT_TRUE(BlockTags::LEAVES().contains(*VanillaBlocks::OAK_LEAVES));
}

TEST_F(BlockTagsTest, BambooPlantableOnContainsGrassBlock)
{
    EXPECT_TRUE(BlockTags::BAMBOO_PLANTABLE_ON().contains(*VanillaBlocks::GRASS_BLOCK));
}

TEST_F(BlockTagsTest, BambooPlantableOnContainsDirt)
{
    EXPECT_TRUE(BlockTags::BAMBOO_PLANTABLE_ON().contains(*VanillaBlocks::DIRT));
}

TEST_F(BlockTagsTest, BambooPlantableOnContainsSand)
{
    EXPECT_TRUE(BlockTags::BAMBOO_PLANTABLE_ON().contains(*VanillaBlocks::SAND));
}

TEST_F(BlockTagsTest, BambooPlantableOnContainsBamboo)
{
    EXPECT_TRUE(BlockTags::BAMBOO_PLANTABLE_ON().contains(*VanillaBlocks::BAMBOO));
}

// 对齐 vanilla 1.21.11 bamboo_plantable_on.json（#dirt + #sand + bamboo/bamboo_sapling/gravel/
// suspicious_gravel）。验证 #dirt 展开的新成员（mycelium/rooted_dirt/moss_block/pale_moss_block/
// mud/muddy_mangrove_roots）与 #sand 的 suspicious_sand、显式的 suspicious_gravel 均在标签内。
TEST_F(BlockTagsTest, BambooPlantableOnContainsMycelium)
{
    EXPECT_TRUE(BlockTags::BAMBOO_PLANTABLE_ON().contains(*VanillaBlocks::MYCELIUM));
}

TEST_F(BlockTagsTest, BambooPlantableOnContainsRootedDirt)
{
    EXPECT_TRUE(BlockTags::BAMBOO_PLANTABLE_ON().contains(*VanillaBlocks::ROOTED_DIRT));
}

TEST_F(BlockTagsTest, BambooPlantableOnContainsMossBlock)
{
    EXPECT_TRUE(BlockTags::BAMBOO_PLANTABLE_ON().contains(*VanillaBlocks::MOSS_BLOCK));
}

TEST_F(BlockTagsTest, BambooPlantableOnContainsPaleMossBlock)
{
    EXPECT_TRUE(BlockTags::BAMBOO_PLANTABLE_ON().contains(*VanillaBlocks::PALE_MOSS_BLOCK));
}

TEST_F(BlockTagsTest, BambooPlantableOnContainsMud)
{
    EXPECT_TRUE(BlockTags::BAMBOO_PLANTABLE_ON().contains(*VanillaBlocks::MUD));
}

TEST_F(BlockTagsTest, BambooPlantableOnContainsMuddyMangroveRoots)
{
    EXPECT_TRUE(BlockTags::BAMBOO_PLANTABLE_ON().contains(*VanillaBlocks::MUDDY_MANGROVE_ROOTS));
}

TEST_F(BlockTagsTest, BambooPlantableOnContainsSuspiciousSand)
{
    EXPECT_TRUE(BlockTags::BAMBOO_PLANTABLE_ON().contains(*VanillaBlocks::SUSPICIOUS_SAND));
}

TEST_F(BlockTagsTest, BambooPlantableOnContainsSuspiciousGravel)
{
    EXPECT_TRUE(BlockTags::BAMBOO_PLANTABLE_ON().contains(*VanillaBlocks::SUSPICIOUS_GRAVEL));
}

// farmland 不在 vanilla bamboo_plantable_on 中（原 Cubium 实现误含，已修复移除）。
TEST_F(BlockTagsTest, BambooPlantableOnDoesNotContainFarmland)
{
    EXPECT_FALSE(BlockTags::BAMBOO_PLANTABLE_ON().contains(*VanillaBlocks::FARMLAND));
}

// ============================================================================
// BlockState Contains Tests
// ============================================================================

TEST_F(BlockTagsTest, ContainsWorksWithBlockState)
{
    const BlockState& grassState = VanillaBlocks::GRASS_BLOCK->defaultState();
    EXPECT_TRUE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(grassState));

    const BlockState& stoneState = VanillaBlocks::STONE->defaultState();
    EXPECT_FALSE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(stoneState));
}

TEST_F(BlockTagsTest, ContainsWorksWithResourceLocation)
{
    EXPECT_TRUE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(ResourceLocation("minecraft", "grass_block")));
    EXPECT_TRUE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(ResourceLocation("minecraft", "dirt")));
    EXPECT_FALSE(BlockTags::VALID_SWEET_BERRY_BUSH_GROUND().contains(ResourceLocation("minecraft", "stone")));
}

// ============================================================================
// WALL_CORALS and UNDERWATER_BONEMEALS Tests (for BoneMealItem::growSeagrass)
// ============================================================================

TEST_F(BlockTagsTest, WallCoralsTagContainsExpectedBlocks)
{
    // 检查是否包含墙珊瑚
    EXPECT_TRUE(BlockTags::WALL_CORALS().contains(ResourceLocation("minecraft", "tube_coral_wall_fan")));
    EXPECT_TRUE(BlockTags::WALL_CORALS().contains(ResourceLocation("minecraft", "brain_coral_wall_fan")));
    EXPECT_TRUE(BlockTags::WALL_CORALS().contains(ResourceLocation("minecraft", "bubble_coral_wall_fan")));
    EXPECT_TRUE(BlockTags::WALL_CORALS().contains(ResourceLocation("minecraft", "fire_coral_wall_fan")));
    EXPECT_TRUE(BlockTags::WALL_CORALS().contains(ResourceLocation("minecraft", "horn_coral_wall_fan")));

    // 检查是否包含死珊瑚墙扇
    EXPECT_TRUE(BlockTags::WALL_CORALS().contains(ResourceLocation("minecraft", "dead_tube_coral_wall_fan")));
    EXPECT_TRUE(BlockTags::WALL_CORALS().contains(ResourceLocation("minecraft", "dead_brain_coral_wall_fan")));
    EXPECT_TRUE(BlockTags::WALL_CORALS().contains(ResourceLocation("minecraft", "dead_bubble_coral_wall_fan")));
    EXPECT_TRUE(BlockTags::WALL_CORALS().contains(ResourceLocation("minecraft", "dead_fire_coral_wall_fan")));
    EXPECT_TRUE(BlockTags::WALL_CORALS().contains(ResourceLocation("minecraft", "dead_horn_coral_wall_fan")));
}

TEST_F(BlockTagsTest, UnderwaterBonemealsTagContainsExpectedBlocks)
{
    // 检查是否包含海草和海带
    EXPECT_TRUE(BlockTags::UNDERWATER_BONEMEALS().contains(ResourceLocation("minecraft", "seagrass")));
    EXPECT_TRUE(BlockTags::UNDERWATER_BONEMEALS().contains(ResourceLocation("minecraft", "kelp")));

    // 检查是否包含珊瑚扇
    EXPECT_TRUE(BlockTags::UNDERWATER_BONEMEALS().contains(ResourceLocation("minecraft", "tube_coral_fan")));
    EXPECT_TRUE(BlockTags::UNDERWATER_BONEMEALS().contains(ResourceLocation("minecraft", "brain_coral_fan")));
    EXPECT_TRUE(BlockTags::UNDERWATER_BONEMEALS().contains(ResourceLocation("minecraft", "bubble_coral_fan")));
    EXPECT_TRUE(BlockTags::UNDERWATER_BONEMEALS().contains(ResourceLocation("minecraft", "fire_coral_fan")));
    EXPECT_TRUE(BlockTags::UNDERWATER_BONEMEALS().contains(ResourceLocation("minecraft", "horn_coral_fan")));
}

// ============================================================================
// STRIDER_WARM_BLOCKS Tests
// ============================================================================

TEST_F(BlockTagsTest, StriderWarmBlocksContainsLava)
{
    // 参考 MC 1.16.5: BlockTags.STRIDER_WARM_BLOCKS 只包含熔岩
    EXPECT_TRUE(BlockTags::STRIDER_WARM_BLOCKS().contains(ResourceLocation("minecraft", "lava")));
}

TEST_F(BlockTagsTest, StriderWarmBlocksDoesNotContainWater)
{
    EXPECT_FALSE(BlockTags::STRIDER_WARM_BLOCKS().contains(ResourceLocation("minecraft", "water")));
}

TEST_F(BlockTagsTest, StriderWarmBlocksDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::STRIDER_WARM_BLOCKS().contains(ResourceLocation("minecraft", "stone")));
}

TEST_F(BlockTagsTest, StriderWarmBlocksIdIsCorrect)
{
    EXPECT_EQ(BlockTags::STRIDER_WARM_BLOCKS().getId(), ResourceLocation("minecraft", "strider_warm_blocks"));
}

// ============================================================================
// WOOL_CARPETS 标签测试
// ============================================================================

TEST_F(BlockTagsTest, WoolCarpetsContainsWhiteCarpet)
{
    EXPECT_TRUE(BlockTags::WOOL_CARPETS().contains(ResourceLocation("minecraft", "white_carpet")));
}

TEST_F(BlockTagsTest, WoolCarpetsContainsBlackCarpet)
{
    EXPECT_TRUE(BlockTags::WOOL_CARPETS().contains(ResourceLocation("minecraft", "black_carpet")));
}

TEST_F(BlockTagsTest, WoolCarpetsDoesNotContainWhiteWool)
{
    // WOOL_CARPETS 不包含羊毛方块，只有地毯
    EXPECT_FALSE(BlockTags::WOOL_CARPETS().contains(ResourceLocation("minecraft", "white_wool")));
}

TEST_F(BlockTagsTest, WoolCarpetsIdIsCorrect)
{
    EXPECT_EQ(BlockTags::WOOL_CARPETS().getId(), ResourceLocation("minecraft", "wool_carpets"));
}

// ============================================================================
// DAMPENS_VIBRATIONS 标签测试 - 包含羊毛和地毯
// ============================================================================

TEST_F(BlockTagsTest, DampensVibrationsContainsWhiteWool)
{
    EXPECT_TRUE(BlockTags::DAMPENS_VIBRATIONS().contains(ResourceLocation("minecraft", "white_wool")));
}

TEST_F(BlockTagsTest, DampensVibrationsContainsBlackWool)
{
    EXPECT_TRUE(BlockTags::DAMPENS_VIBRATIONS().contains(ResourceLocation("minecraft", "black_wool")));
}

TEST_F(BlockTagsTest, DampensVibrationsContainsWhiteCarpet)
{
    // MC 原版 DAMPENS_VIBRATIONS 包含地毯方块
    EXPECT_TRUE(BlockTags::DAMPENS_VIBRATIONS().contains(ResourceLocation("minecraft", "white_carpet")));
}

TEST_F(BlockTagsTest, DampensVibrationsContainsBlackCarpet)
{
    EXPECT_TRUE(BlockTags::DAMPENS_VIBRATIONS().contains(ResourceLocation("minecraft", "black_carpet")));
}

TEST_F(BlockTagsTest, DampensVibrationsDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::DAMPENS_VIBRATIONS().contains(ResourceLocation("minecraft", "stone")));
}

TEST_F(BlockTagsTest, DampensVibrationsIdIsCorrect)
{
    EXPECT_EQ(BlockTags::DAMPENS_VIBRATIONS().getId(), ResourceLocation("minecraft", "dampens_vibrations"));
}

// ============================================================================
// STAIRS 标签测试
// ============================================================================

TEST_F(BlockTagsTest, StairsTagContainsStoneBrickStairs)
{
    if (!VanillaBlocks::STONE_BRICK_STAIRS) {
        GTEST_SKIP() << "STONE_BRICK_STAIRS block not registered";
    }
    EXPECT_TRUE(BlockTags::STAIRS().contains(*VanillaBlocks::STONE_BRICK_STAIRS));
}

TEST_F(BlockTagsTest, StairsTagContainsMossyStoneBrickStairs)
{
    if (!VanillaBlocks::MOSSY_STONE_BRICK_STAIRS) {
        GTEST_SKIP() << "MOSSY_STONE_BRICK_STAIRS block not registered";
    }
    EXPECT_TRUE(BlockTags::STAIRS().contains(*VanillaBlocks::MOSSY_STONE_BRICK_STAIRS));
}

TEST_F(BlockTagsTest, StairsTagDoesNotContainStoneBricks)
{
    // 石砖不是楼梯
    if (!VanillaBlocks::STONE_BRICKS) {
        GTEST_SKIP() << "STONE_BRICKS block not registered";
    }
    EXPECT_FALSE(BlockTags::STAIRS().contains(*VanillaBlocks::STONE_BRICKS));
}

TEST_F(BlockTagsTest, StairsTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::STAIRS().getId(), ResourceLocation("minecraft", "stairs"));
}

TEST_F(BlockTagsTest, StairsTagContainsResourceLocation)
{
    EXPECT_TRUE(BlockTags::STAIRS().contains(ResourceLocation("minecraft", "oak_stairs")));
    EXPECT_TRUE(BlockTags::STAIRS().contains(ResourceLocation("minecraft", "stone_brick_stairs")));
    EXPECT_TRUE(BlockTags::STAIRS().contains(ResourceLocation("minecraft", "mossy_stone_brick_stairs")));
    EXPECT_FALSE(BlockTags::STAIRS().contains(ResourceLocation("minecraft", "stone_bricks")));
}

// ============================================================================
// SLABS 标签测试
// ============================================================================

TEST_F(BlockTagsTest, SlabsTagContainsStoneBrickSlab)
{
    if (!VanillaBlocks::STONE_BRICK_SLAB) {
        GTEST_SKIP() << "STONE_BRICK_SLAB block not registered";
    }
    EXPECT_TRUE(BlockTags::SLABS().contains(*VanillaBlocks::STONE_BRICK_SLAB));
}

TEST_F(BlockTagsTest, SlabsTagContainsMossyStoneBrickSlab)
{
    if (!VanillaBlocks::MOSSY_STONE_BRICK_SLAB) {
        GTEST_SKIP() << "MOSSY_STONE_BRICK_SLAB block not registered";
    }
    EXPECT_TRUE(BlockTags::SLABS().contains(*VanillaBlocks::MOSSY_STONE_BRICK_SLAB));
}

TEST_F(BlockTagsTest, SlabsTagDoesNotContainStoneBrickStairs)
{
    // 楼梯不是台阶
    if (!VanillaBlocks::STONE_BRICK_STAIRS) {
        GTEST_SKIP() << "STONE_BRICK_STAIRS block not registered";
    }
    EXPECT_FALSE(BlockTags::SLABS().contains(*VanillaBlocks::STONE_BRICK_STAIRS));
}

TEST_F(BlockTagsTest, SlabsTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::SLABS().getId(), ResourceLocation("minecraft", "slabs"));
}

TEST_F(BlockTagsTest, SlabsTagContainsResourceLocation)
{
    EXPECT_TRUE(BlockTags::SLABS().contains(ResourceLocation("minecraft", "oak_slab")));
    EXPECT_TRUE(BlockTags::SLABS().contains(ResourceLocation("minecraft", "stone_brick_slab")));
    EXPECT_FALSE(BlockTags::SLABS().contains(ResourceLocation("minecraft", "stone_brick_stairs")));
}

// ============================================================================
// WALLS 标签测试
// ============================================================================

TEST_F(BlockTagsTest, WallsTagContainsStoneBrickWall)
{
    if (!VanillaBlocks::STONE_BRICK_WALL) {
        GTEST_SKIP() << "STONE_BRICK_WALL block not registered";
    }
    EXPECT_TRUE(BlockTags::WALLS().contains(*VanillaBlocks::STONE_BRICK_WALL));
}

TEST_F(BlockTagsTest, WallsTagContainsMossyStoneBrickWall)
{
    if (!VanillaBlocks::MOSSY_STONE_BRICK_WALL) {
        GTEST_SKIP() << "MOSSY_STONE_BRICK_WALL block not registered";
    }
    EXPECT_TRUE(BlockTags::WALLS().contains(*VanillaBlocks::MOSSY_STONE_BRICK_WALL));
}

TEST_F(BlockTagsTest, WallsTagDoesNotContainStoneBricks)
{
    // 石砖不是墙
    if (!VanillaBlocks::STONE_BRICKS) {
        GTEST_SKIP() << "STONE_BRICKS block not registered";
    }
    EXPECT_FALSE(BlockTags::WALLS().contains(*VanillaBlocks::STONE_BRICKS));
}

TEST_F(BlockTagsTest, WallsTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::WALLS().getId(), ResourceLocation("minecraft", "walls"));
}

TEST_F(BlockTagsTest, WallsTagContainsResourceLocation)
{
    EXPECT_TRUE(BlockTags::WALLS().contains(ResourceLocation("minecraft", "cobblestone_wall")));
    EXPECT_TRUE(BlockTags::WALLS().contains(ResourceLocation("minecraft", "stone_brick_wall")));
    EXPECT_TRUE(BlockTags::WALLS().contains(ResourceLocation("minecraft", "mossy_stone_brick_wall")));
    EXPECT_FALSE(BlockTags::WALLS().contains(ResourceLocation("minecraft", "stone_bricks")));
}

// ============================================================================
// COMBINATION_STEP_SOUND_BLOCKS 标签测试
// ============================================================================

TEST_F(BlockTagsTest, CombinationStepSoundBlocksIdIsCorrect)
{
    EXPECT_EQ(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().getId(),
        ResourceLocation("minecraft", "combination_step_sound_blocks"));
}

TEST_F(BlockTagsTest, CombinationStepSoundBlocksContainsWoolCarpets)
{
    // 羊毛地毯（16色）属于 COMBINATION_STEP_SOUND_BLOCKS
    EXPECT_TRUE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "white_carpet")));
    EXPECT_TRUE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "red_carpet")));
    EXPECT_TRUE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "black_carpet")));
    EXPECT_TRUE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "blue_carpet")));
    EXPECT_TRUE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "green_carpet")));
}

TEST_F(BlockTagsTest, CombinationStepSoundBlocksContainsMossCarpet)
{
    EXPECT_TRUE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "moss_carpet")));
}

TEST_F(BlockTagsTest, CombinationStepSoundBlocksContainsPaleMossCarpet)
{
    EXPECT_TRUE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "pale_moss_carpet")));
}

TEST_F(BlockTagsTest, CombinationStepSoundBlocksContainsSnow)
{
    // 雪层（不是雪块 snow_block）属于 COMBINATION_STEP_SOUND_BLOCKS
    EXPECT_TRUE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "snow")));
}

TEST_F(BlockTagsTest, CombinationStepSoundBlocksContainsNetherVegetation)
{
    // 下界苗、诡异菌索、绯红菌索
    EXPECT_TRUE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "nether_sprouts")));
    EXPECT_TRUE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "warped_roots")));
    EXPECT_TRUE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "crimson_roots")));
}

TEST_F(BlockTagsTest, CombinationStepSoundBlocksContainsResinClump)
{
    EXPECT_TRUE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "resin_clump")));
}

TEST_F(BlockTagsTest, CombinationStepSoundBlocksDoesNotContainSnowBlock)
{
    // 雪块不属于 COMBINATION_STEP_SOUND_BLOCKS
    EXPECT_FALSE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "snow_block")));
}

TEST_F(BlockTagsTest, CombinationStepSoundBlocksDoesNotContainWool)
{
    // 羊毛方块不属于 COMBINATION_STEP_SOUND_BLOCKS（只有地毯属于）
    EXPECT_FALSE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "white_wool")));
}

TEST_F(BlockTagsTest, CombinationStepSoundBlocksDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::COMBINATION_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "stone")));
}

// ============================================================================
// INSIDE_STEP_SOUND_BLOCKS 标签测试
// ============================================================================

TEST_F(BlockTagsTest, InsideStepSoundBlocksIdIsCorrect)
{
    EXPECT_EQ(BlockTags::INSIDE_STEP_SOUND_BLOCKS().getId(), ResourceLocation("minecraft", "inside_step_sound_blocks"));
}

TEST_F(BlockTagsTest, InsideStepSoundBlocksContainsPowderSnow)
{
    EXPECT_TRUE(BlockTags::INSIDE_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "powder_snow")));
}

TEST_F(BlockTagsTest, InsideStepSoundBlocksContainsSculkVein)
{
    EXPECT_TRUE(BlockTags::INSIDE_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "sculk_vein")));
}

TEST_F(BlockTagsTest, InsideStepSoundBlocksContainsGlowLichen)
{
    EXPECT_TRUE(BlockTags::INSIDE_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "glow_lichen")));
}

TEST_F(BlockTagsTest, InsideStepSoundBlocksContainsLilyPad)
{
    EXPECT_TRUE(BlockTags::INSIDE_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "lily_pad")));
}

TEST_F(BlockTagsTest, InsideStepSoundBlocksContainsSmallAmethystBud)
{
    EXPECT_TRUE(BlockTags::INSIDE_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "small_amethyst_bud")));
}

TEST_F(BlockTagsTest, InsideStepSoundBlocksContainsPinkPetals)
{
    EXPECT_TRUE(BlockTags::INSIDE_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "pink_petals")));
}

TEST_F(BlockTagsTest, InsideStepSoundBlocksContainsWildflowers)
{
    EXPECT_TRUE(BlockTags::INSIDE_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "wildflowers")));
}

TEST_F(BlockTagsTest, InsideStepSoundBlocksContainsLeafLitter)
{
    EXPECT_TRUE(BlockTags::INSIDE_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "leaf_litter")));
}

TEST_F(BlockTagsTest, InsideStepSoundBlocksDoesNotContainSnow)
{
    // 雪层属于 COMBINATION_STEP_SOUND_BLOCKS，不属于 INSIDE_STEP_SOUND_BLOCKS
    EXPECT_FALSE(BlockTags::INSIDE_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "snow")));
}

TEST_F(BlockTagsTest, InsideStepSoundBlocksDoesNotContainWhiteCarpet)
{
    // 地毯属于 COMBINATION_STEP_SOUND_BLOCKS，不属于 INSIDE_STEP_SOUND_BLOCKS
    EXPECT_FALSE(BlockTags::INSIDE_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "white_carpet")));
}

TEST_F(BlockTagsTest, InsideStepSoundBlocksDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::INSIDE_STEP_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "stone")));
}

// ============================================================================
// CRYSTAL_SOUND_BLOCKS 标签测试
// ============================================================================

TEST_F(BlockTagsTest, CrystalSoundBlocksIdIsCorrect)
{
    EXPECT_EQ(BlockTags::CRYSTAL_SOUND_BLOCKS().getId(), ResourceLocation("minecraft", "crystal_sound_blocks"));
}

TEST_F(BlockTagsTest, CrystalSoundBlocksContainsAmethystBlock)
{
    EXPECT_TRUE(BlockTags::CRYSTAL_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "amethyst_block")));
}

TEST_F(BlockTagsTest, CrystalSoundBlocksContainsBuddingAmethyst)
{
    EXPECT_TRUE(BlockTags::CRYSTAL_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "budding_amethyst")));
}

TEST_F(BlockTagsTest, CrystalSoundBlocksDoesNotContainSmallAmethystBud)
{
    // 小型紫水晶芽属于 INSIDE_STEP_SOUND_BLOCKS，不属于 CRYSTAL_SOUND_BLOCKS
    EXPECT_FALSE(BlockTags::CRYSTAL_SOUND_BLOCKS().contains(ResourceLocation("minecraft", "small_amethyst_bud")));
}

// ============================================================================
// BARS 标签测试（铁栏杆等，用于墙和玻璃板连接判断）
// ============================================================================

TEST_F(BlockTagsTest, BarsTagContainsIronBars)
{
    EXPECT_TRUE(BlockTags::BARS().contains(ResourceLocation("minecraft", "iron_bars")));
}

TEST_F(BlockTagsTest, BarsTagContainsCopperBarsVariants)
{
    // 铜栏杆变体
    EXPECT_TRUE(BlockTags::BARS().contains(ResourceLocation("minecraft", "copper_bars")));
    EXPECT_TRUE(BlockTags::BARS().contains(ResourceLocation("minecraft", "waxed_copper_bars")));
    EXPECT_TRUE(BlockTags::BARS().contains(ResourceLocation("minecraft", "exposed_copper_bars")));
    EXPECT_TRUE(BlockTags::BARS().contains(ResourceLocation("minecraft", "weathered_copper_bars")));
    EXPECT_TRUE(BlockTags::BARS().contains(ResourceLocation("minecraft", "oxidized_copper_bars")));
}

TEST_F(BlockTagsTest, BarsTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::BARS().getId(), ResourceLocation("minecraft", "bars"));
}

TEST_F(BlockTagsTest, BarsTagDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::BARS().contains(ResourceLocation("minecraft", "stone")));
}

TEST_F(BlockTagsTest, BarsTagDoesNotContainGlassPane)
{
    // 玻璃板不属于 BARS 标签
    EXPECT_FALSE(BlockTags::BARS().contains(ResourceLocation("minecraft", "glass_pane")));
}

// ============================================================================
// SHULKER_BOXES 标签测试（用于连接例外判断）
// ============================================================================

TEST_F(BlockTagsTest, ShulkerBoxesTagContainsShulkerBox)
{
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "shulker_box")));
}

TEST_F(BlockTagsTest, ShulkerBoxesTagContainsColoredVariants)
{
    // 16色潜影盒都应在标签中
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "white_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "orange_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "magenta_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "light_blue_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "yellow_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "lime_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "pink_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "gray_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "light_gray_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "cyan_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "purple_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "blue_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "brown_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "green_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "red_shulker_box")));
    EXPECT_TRUE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "black_shulker_box")));
}

TEST_F(BlockTagsTest, ShulkerBoxesTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::SHULKER_BOXES().getId(), ResourceLocation("minecraft", "shulker_boxes"));
}

TEST_F(BlockTagsTest, ShulkerBoxesTagDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::SHULKER_BOXES().contains(ResourceLocation("minecraft", "stone")));
}

// ============================================================================
// WALL_POST_OVERRIDE 标签测试（放置在墙上时强制显示墙柱的方块）
// ============================================================================

TEST_F(BlockTagsTest, WallPostOverrideTagContainsTorch)
{
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "torch")));
}

TEST_F(BlockTagsTest, WallPostOverrideTagContainsSoulTorch)
{
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "soul_torch")));
}

TEST_F(BlockTagsTest, WallPostOverrideTagContainsRedstoneTorch)
{
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "redstone_torch")));
}

TEST_F(BlockTagsTest, WallPostOverrideTagContainsSigns)
{
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "oak_sign")));
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "oak_wall_sign")));
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "spruce_sign")));
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "birch_sign")));
}

TEST_F(BlockTagsTest, WallPostOverrideTagContainsBanners)
{
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "white_banner")));
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "white_wall_banner")));
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "black_banner")));
}

TEST_F(BlockTagsTest, WallPostOverrideTagContainsPressurePlates)
{
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "stone_pressure_plate")));
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "oak_pressure_plate")));
    EXPECT_TRUE(
        BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "heavy_weighted_pressure_plate")));
    EXPECT_TRUE(
        BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "light_weighted_pressure_plate")));
}

TEST_F(BlockTagsTest, WallPostOverrideTagContainsCactusFlower)
{
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "cactus_flower")));
}

TEST_F(BlockTagsTest, WallPostOverrideTagContainsCopperTorch)
{
    // 铜火把（MC 1.21.2+新增，暂未实现方块但标签已包含）
    EXPECT_TRUE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "copper_torch")));
}

TEST_F(BlockTagsTest, WallPostOverrideTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::WALL_POST_OVERRIDE().getId(), ResourceLocation("minecraft", "wall_post_override"));
}

TEST_F(BlockTagsTest, WallPostOverrideTagDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "stone")));
}

TEST_F(BlockTagsTest, WallPostOverrideTagDoesNotContainWall)
{
    // 墙本身不属于 WALL_POST_OVERRIDE
    EXPECT_FALSE(BlockTags::WALL_POST_OVERRIDE().contains(ResourceLocation("minecraft", "cobblestone_wall")));
}

// ============================================================================
// Block::isExceptionForConnection 测试
// 参考: net.minecraft.block.Block#isExceptionForConnection
// 某些固体方块不应与栅栏、墙、玻璃板建立连接
// ============================================================================

TEST_F(BlockTagsTest, IsExceptionForConnection_Leaves)
{
    // 树叶是连接例外
    if (!VanillaBlocks::OAK_LEAVES) {
        GTEST_SKIP() << "OAK_LEAVES block not registered";
    }
    EXPECT_TRUE(Block::isExceptionForConnection(VanillaBlocks::OAK_LEAVES->defaultState()));
}

TEST_F(BlockTagsTest, IsExceptionForConnection_Barrier)
{
    // 屏障方块是连接例外
    const Block* barrier = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "barrier"));
    if (!barrier) {
        GTEST_SKIP() << "barrier block not registered";
    }
    EXPECT_TRUE(Block::isExceptionForConnection(barrier->defaultState()));
}

TEST_F(BlockTagsTest, IsExceptionForConnection_ShulkerBox)
{
    // 潜影盒是连接例外（通过 SHULKER_BOXES 标签）
    const Block* shulkerBox = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "shulker_box"));
    if (!shulkerBox) {
        GTEST_SKIP() << "shulker_box block not registered";
    }
    EXPECT_TRUE(Block::isExceptionForConnection(shulkerBox->defaultState()));
}

TEST_F(BlockTagsTest, IsExceptionForConnection_Pumpkin)
{
    // 南瓜是连接例外
    const Block* pumpkin = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "pumpkin"));
    if (!pumpkin) {
        GTEST_SKIP() << "pumpkin block not registered";
    }
    EXPECT_TRUE(Block::isExceptionForConnection(pumpkin->defaultState()));
}

TEST_F(BlockTagsTest, IsExceptionForConnection_CarvedPumpkin)
{
    // 雕刻南瓜是连接例外
    const Block* carvedPumpkin = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "carved_pumpkin"));
    if (!carvedPumpkin) {
        GTEST_SKIP() << "carved_pumpkin block not registered";
    }
    EXPECT_TRUE(Block::isExceptionForConnection(carvedPumpkin->defaultState()));
}

TEST_F(BlockTagsTest, IsExceptionForConnection_JackOLantern)
{
    // 南瓜灯是连接例外
    const Block* jackOLantern = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "jack_o_lantern"));
    if (!jackOLantern) {
        GTEST_SKIP() << "jack_o_lantern block not registered";
    }
    EXPECT_TRUE(Block::isExceptionForConnection(jackOLantern->defaultState()));
}

TEST_F(BlockTagsTest, IsExceptionForConnection_Melon)
{
    // 西瓜是连接例外
    const Block* melon = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "melon"));
    if (!melon) {
        GTEST_SKIP() << "melon block not registered";
    }
    EXPECT_TRUE(Block::isExceptionForConnection(melon->defaultState()));
}

TEST_F(BlockTagsTest, IsExceptionForConnection_StoneIsNot)
{
    // 石头不是连接例外
    EXPECT_FALSE(Block::isExceptionForConnection(VanillaBlocks::STONE->defaultState()));
}

TEST_F(BlockTagsTest, IsExceptionForConnection_DirtIsNot)
{
    // 泥土不是连接例外
    EXPECT_FALSE(Block::isExceptionForConnection(VanillaBlocks::DIRT->defaultState()));
}

TEST_F(BlockTagsTest, IsExceptionForConnection_CobblestoneIsNot)
{
    // 圆石不是连接例外
    if (!VanillaBlocks::COBBLESTONE) {
        GTEST_SKIP() << "COBBLESTONE block not registered";
    }
    EXPECT_FALSE(Block::isExceptionForConnection(VanillaBlocks::COBBLESTONE->defaultState()));
}

// ============================================================================
// HOGLIN_REPELLENTS 标签测试（疣猪兽排斥物）
// ============================================================================

TEST_F(BlockTagsTest, HoglinRepellentsIdIsCorrect)
{
    EXPECT_EQ(BlockTags::HOGLIN_REPELLENTS().getId(), ResourceLocation("minecraft", "hoglin_repellents"));
}

TEST_F(BlockTagsTest, HoglinRepellentsContainsWarpedFungus)
{
    EXPECT_TRUE(BlockTags::HOGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "warped_fungus")));
}

TEST_F(BlockTagsTest, HoglinRepellentsDoesNotContainWarpedNylium)
{
    // MC 1.21.11 中 warped_nylium 不在 HOGLIN_REPELLENTS 标签中
    EXPECT_FALSE(BlockTags::HOGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "warped_nylium")));
}

TEST_F(BlockTagsTest, HoglinRepellentsContainsNetherPortal)
{
    EXPECT_TRUE(BlockTags::HOGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "nether_portal")));
}

TEST_F(BlockTagsTest, HoglinRepellentsContainsRespawnAnchor)
{
    EXPECT_TRUE(BlockTags::HOGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "respawn_anchor")));
}

TEST_F(BlockTagsTest, HoglinRepellentsDoesNotContainCrimsonFungus)
{
    // 绯红菌不是疣猪兽排斥物
    EXPECT_FALSE(BlockTags::HOGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "crimson_fungus")));
}

TEST_F(BlockTagsTest, HoglinRepellentsDoesNotContainCrimsonNylium)
{
    // 绯红菌岩不是疣猪兽排斥物（疣猪兽偏好绯红菌岩）
    EXPECT_FALSE(BlockTags::HOGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "crimson_nylium")));
}

TEST_F(BlockTagsTest, HoglinRepellentsDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::HOGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "stone")));
}

// ============================================================================
// PIGLIN_REPELLENTS 标签测试（猪灵排斥物）
// ============================================================================

TEST_F(BlockTagsTest, PiglinRepellentsIdIsCorrect)
{
    EXPECT_EQ(BlockTags::PIGLIN_REPELLENTS().getId(), ResourceLocation("minecraft", "piglin_repellents"));
}

TEST_F(BlockTagsTest, PiglinRepellentsContainsSoulFire)
{
    EXPECT_TRUE(BlockTags::PIGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "soul_fire")));
}

TEST_F(BlockTagsTest, PiglinRepellentsContainsSoulTorch)
{
    EXPECT_TRUE(BlockTags::PIGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "soul_torch")));
}

TEST_F(BlockTagsTest, PiglinRepellentsContainsSoulWallTorch)
{
    EXPECT_TRUE(BlockTags::PIGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "soul_wall_torch")));
}

TEST_F(BlockTagsTest, PiglinRepellentsContainsSoulLantern)
{
    EXPECT_TRUE(BlockTags::PIGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "soul_lantern")));
}

TEST_F(BlockTagsTest, PiglinRepellentsContainsSoulCampfire)
{
    EXPECT_TRUE(BlockTags::PIGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "soul_campfire")));
}

TEST_F(BlockTagsTest, PiglinRepellentsDoesNotContainWarpedFungus)
{
    // MC 1.21.11 中 warped_fungus 不在 PIGLIN_REPELLENTS 标签中，仅存在于 HOGLIN_REPELLENTS
    EXPECT_FALSE(BlockTags::PIGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "warped_fungus")));
}

TEST_F(BlockTagsTest, PiglinRepellentsDoesNotContainSoulSand)
{
    // 灵魂沙不是猪灵排斥物
    EXPECT_FALSE(BlockTags::PIGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "soul_sand")));
}

TEST_F(BlockTagsTest, PiglinRepellentsDoesNotContainSoulSoil)
{
    // 灵魂土不是猪灵排斥物
    EXPECT_FALSE(BlockTags::PIGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "soul_soil")));
}

TEST_F(BlockTagsTest, PiglinRepellentsDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::PIGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "stone")));
}

TEST_F(BlockTagsTest, PiglinRepellentsDoesNotContainCampfire)
{
    // 普通营火不是猪灵排斥物（只有灵魂营火才是）
    EXPECT_FALSE(BlockTags::PIGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "campfire")));
}

TEST_F(BlockTagsTest, PiglinRepellentsDoesNotContainLantern)
{
    // 普通灯笼不是猪灵排斥物（只有灵魂灯笼才是）
    EXPECT_FALSE(BlockTags::PIGLIN_REPELLENTS().contains(ResourceLocation("minecraft", "lantern")));
}

// ========== DOES_NOT_BLOCK_HOPPERS 标签测试 ==========

TEST_F(BlockTagsTest, DoesNotBlockHoppers_ContainsBeehive)
{
    EXPECT_TRUE(BlockTags::DOES_NOT_BLOCK_HOPPERS().contains(ResourceLocation("minecraft", "beehive")));
}

TEST_F(BlockTagsTest, DoesNotBlockHoppers_ContainsBeeNest)
{
    EXPECT_TRUE(BlockTags::DOES_NOT_BLOCK_HOPPERS().contains(ResourceLocation("minecraft", "bee_nest")));
}

TEST_F(BlockTagsTest, DoesNotBlockHoppers_DoesNotContainStone)
{
    // 石头不在漏斗豁免标签中，应阻挡漏斗
    EXPECT_FALSE(BlockTags::DOES_NOT_BLOCK_HOPPERS().contains(ResourceLocation("minecraft", "stone")));
}

TEST_F(BlockTagsTest, DoesNotBlockHoppers_DoesNotContainDirt)
{
    EXPECT_FALSE(BlockTags::DOES_NOT_BLOCK_HOPPERS().contains(ResourceLocation("minecraft", "dirt")));
}

TEST_F(BlockTagsTest, DoesNotBlockHoppers_IdIsCorrect)
{
    EXPECT_EQ(BlockTags::DOES_NOT_BLOCK_HOPPERS().getId(), ResourceLocation("minecraft", "does_not_block_hoppers"));
}

TEST_F(BlockTagsTest, DoesNotBlockHoppers_ContainsBeehiveBlockState)
{
    // 验证使用 BlockState 引用检查也正确工作
    const Block* beehive = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "beehive"));
    if (!beehive) {
        GTEST_SKIP() << "beehive block not registered";
    }
    EXPECT_TRUE(BlockTags::DOES_NOT_BLOCK_HOPPERS().contains(beehive->defaultState()));
}

TEST_F(BlockTagsTest, DoesNotBlockHoppers_ContainsBeeNestBlockState)
{
    const Block* beeNest = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "bee_nest"));
    if (!beeNest) {
        GTEST_SKIP() << "bee_nest block not registered";
    }
    EXPECT_TRUE(BlockTags::DOES_NOT_BLOCK_HOPPERS().contains(beeNest->defaultState()));
}

TEST_F(BlockTagsTest, DoesNotBlockHoppers_StoneBlockStateNotInTag)
{
    const Block* stone = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "stone"));
    if (!stone) {
        GTEST_SKIP() << "stone block not registered";
    }
    EXPECT_FALSE(BlockTags::DOES_NOT_BLOCK_HOPPERS().contains(stone->defaultState()));
}

// ============================================================================
// CHAINS 标签测试（铁锁链和铜锁链）
// 参考: net.minecraft.tags.BlockTags.CHAINS
// ============================================================================

TEST_F(BlockTagsTest, ChainsTagContainsIronChain)
{
    EXPECT_TRUE(BlockTags::CHAINS().contains(ResourceLocation("minecraft", "iron_chain")));
}

TEST_F(BlockTagsTest, ChainsTagContainsCopperChain)
{
    EXPECT_TRUE(BlockTags::CHAINS().contains(ResourceLocation("minecraft", "copper_chain")));
}

TEST_F(BlockTagsTest, ChainsTagContainsExposedCopperChain)
{
    EXPECT_TRUE(BlockTags::CHAINS().contains(ResourceLocation("minecraft", "exposed_copper_chain")));
}

TEST_F(BlockTagsTest, ChainsTagContainsWeatheredCopperChain)
{
    EXPECT_TRUE(BlockTags::CHAINS().contains(ResourceLocation("minecraft", "weathered_copper_chain")));
}

TEST_F(BlockTagsTest, ChainsTagContainsOxidizedCopperChain)
{
    EXPECT_TRUE(BlockTags::CHAINS().contains(ResourceLocation("minecraft", "oxidized_copper_chain")));
}

TEST_F(BlockTagsTest, ChainsTagContainsWaxedCopperChain)
{
    EXPECT_TRUE(BlockTags::CHAINS().contains(ResourceLocation("minecraft", "waxed_copper_chain")));
}

TEST_F(BlockTagsTest, ChainsTagContainsWaxedExposedCopperChain)
{
    EXPECT_TRUE(BlockTags::CHAINS().contains(ResourceLocation("minecraft", "waxed_exposed_copper_chain")));
}

TEST_F(BlockTagsTest, ChainsTagContainsWaxedWeatheredCopperChain)
{
    EXPECT_TRUE(BlockTags::CHAINS().contains(ResourceLocation("minecraft", "waxed_weathered_copper_chain")));
}

TEST_F(BlockTagsTest, ChainsTagContainsWaxedOxidizedCopperChain)
{
    EXPECT_TRUE(BlockTags::CHAINS().contains(ResourceLocation("minecraft", "waxed_oxidized_copper_chain")));
}

TEST_F(BlockTagsTest, ChainsTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::CHAINS().getId(), ResourceLocation("minecraft", "chains"));
}

TEST_F(BlockTagsTest, ChainsTagContainsAllNineVariants)
{
    // CHAINS 标签应包含铁锁链 + 4个铜锁链氧化变种 + 4个涂蜡铜锁链变种 = 9项
    const auto& blockIds = BlockTags::CHAINS().getBlockIds();
    EXPECT_EQ(blockIds.size(), 9u);
}

TEST_F(BlockTagsTest, ChainsTagDoesNotContainIronBars)
{
    // 铁栏杆不属于锁链标签
    EXPECT_FALSE(BlockTags::CHAINS().contains(ResourceLocation("minecraft", "iron_bars")));
}

TEST_F(BlockTagsTest, ChainsTagDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::CHAINS().contains(ResourceLocation("minecraft", "stone")));
}

TEST_F(BlockTagsTest, ChainsTagContainsIronChainBlockPointer)
{
    // 通过 Block* 指针检查
    if (!VanillaBlocks::CHAIN) {
        GTEST_SKIP() << "CHAIN block not registered";
    }
    EXPECT_TRUE(BlockTags::CHAINS().contains(*VanillaBlocks::CHAIN));
}

TEST_F(BlockTagsTest, ChainsTagContainsCopperChainBlockPointer)
{
    // 通过 Block* 指针检查
    if (!block_registry::CopperBlocks::COPPER_CHAIN) {
        GTEST_SKIP() << "COPPER_CHAIN block not registered";
    }
    EXPECT_TRUE(BlockTags::CHAINS().contains(*block_registry::CopperBlocks::COPPER_CHAIN));
}

// ============================================================================
// WOODEN_DOORS 标签测试
// 参考: net.minecraft.tags.BlockTags.WOODEN_DOORS
// ============================================================================

TEST_F(BlockTagsTest, WoodenDoorsContainsOakDoor)
{
    EXPECT_TRUE(BlockTags::WOODEN_DOORS().contains(ResourceLocation("minecraft", "oak_door")));
}

TEST_F(BlockTagsTest, WoodenDoorsContainsCrimsonDoor)
{
    EXPECT_TRUE(BlockTags::WOODEN_DOORS().contains(ResourceLocation("minecraft", "crimson_door")));
}

TEST_F(BlockTagsTest, WoodenDoorsContainsWarpedDoor)
{
    EXPECT_TRUE(BlockTags::WOODEN_DOORS().contains(ResourceLocation("minecraft", "warped_door")));
}

TEST_F(BlockTagsTest, WoodenDoorsDoesNotContainIronDoor)
{
    EXPECT_FALSE(BlockTags::WOODEN_DOORS().contains(ResourceLocation("minecraft", "iron_door")));
}

TEST_F(BlockTagsTest, WoodenDoorsTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::WOODEN_DOORS().getId(), ResourceLocation("minecraft", "wooden_doors"));
}

TEST_F(BlockTagsTest, WoodenDoorsContainsAll12Doors)
{
    const auto& blockIds = BlockTags::WOODEN_DOORS().getBlockIds();
    EXPECT_EQ(blockIds.size(), 12u);
}

// ============================================================================
// DOORS 标签测试
// 参考: net.minecraft.tags.BlockTags.DOORS
// ============================================================================

TEST_F(BlockTagsTest, DoorsContainsOakDoor)
{
    EXPECT_TRUE(BlockTags::DOORS().contains(ResourceLocation("minecraft", "oak_door")));
}

TEST_F(BlockTagsTest, DoorsContainsIronDoor)
{
    EXPECT_TRUE(BlockTags::DOORS().contains(ResourceLocation("minecraft", "iron_door")));
}

TEST_F(BlockTagsTest, DoorsContainsCrimsonDoor)
{
    EXPECT_TRUE(BlockTags::DOORS().contains(ResourceLocation("minecraft", "crimson_door")));
}

TEST_F(BlockTagsTest, DoorsContainsCopperDoor)
{
    EXPECT_TRUE(BlockTags::DOORS().contains(ResourceLocation("minecraft", "copper_door")));
}

TEST_F(BlockTagsTest, DoorsDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::DOORS().contains(ResourceLocation("minecraft", "stone")));
}

TEST_F(BlockTagsTest, DoorsTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::DOORS().getId(), ResourceLocation("minecraft", "doors"));
}

// ============================================================================
// WOODEN_TRAPDOORS 标签测试
// 参考: net.minecraft.tags.BlockTags.WOODEN_TRAPDOORS
// ============================================================================

TEST_F(BlockTagsTest, WoodenTrapdoorsContainsOakTrapdoor)
{
    EXPECT_TRUE(BlockTags::WOODEN_TRAPDOORS().contains(ResourceLocation("minecraft", "oak_trapdoor")));
}

TEST_F(BlockTagsTest, WoodenTrapdoorsContainsCrimsonTrapdoor)
{
    EXPECT_TRUE(BlockTags::WOODEN_TRAPDOORS().contains(ResourceLocation("minecraft", "crimson_trapdoor")));
}

TEST_F(BlockTagsTest, WoodenTrapdoorsContainsWarpedTrapdoor)
{
    EXPECT_TRUE(BlockTags::WOODEN_TRAPDOORS().contains(ResourceLocation("minecraft", "warped_trapdoor")));
}

TEST_F(BlockTagsTest, WoodenTrapdoorsDoesNotContainIronTrapdoor)
{
    EXPECT_FALSE(BlockTags::WOODEN_TRAPDOORS().contains(ResourceLocation("minecraft", "iron_trapdoor")));
}

TEST_F(BlockTagsTest, WoodenTrapdoorsTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::WOODEN_TRAPDOORS().getId(), ResourceLocation("minecraft", "wooden_trapdoors"));
}

TEST_F(BlockTagsTest, WoodenTrapdoorsContainsAll12Trapdoors)
{
    const auto& blockIds = BlockTags::WOODEN_TRAPDOORS().getBlockIds();
    EXPECT_EQ(blockIds.size(), 12u);
}

// ============================================================================
// TRAPDOORS 标签测试
// 参考: net.minecraft.tags.BlockTags.TRAPDOORS
// ============================================================================

TEST_F(BlockTagsTest, TrapdoorsContainsOakTrapdoor)
{
    EXPECT_TRUE(BlockTags::TRAPDOORS().contains(ResourceLocation("minecraft", "oak_trapdoor")));
}

TEST_F(BlockTagsTest, TrapdoorsContainsIronTrapdoor)
{
    EXPECT_TRUE(BlockTags::TRAPDOORS().contains(ResourceLocation("minecraft", "iron_trapdoor")));
}

TEST_F(BlockTagsTest, TrapdoorsContainsWarpedTrapdoor)
{
    EXPECT_TRUE(BlockTags::TRAPDOORS().contains(ResourceLocation("minecraft", "warped_trapdoor")));
}

TEST_F(BlockTagsTest, TrapdoorsContainsCopperTrapdoor)
{
    EXPECT_TRUE(BlockTags::TRAPDOORS().contains(ResourceLocation("minecraft", "copper_trapdoor")));
}

TEST_F(BlockTagsTest, TrapdoorsTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::TRAPDOORS().getId(), ResourceLocation("minecraft", "trapdoors"));
}

// ============================================================================
// NON_FLAMMABLE_WOOD 标签测试
// 参考: net.minecraft.tags.BlockTags.NON_FLAMMABLE_WOOD
// ============================================================================

TEST_F(BlockTagsTest, NonFlammableWoodContainsCrimsonStem)
{
    EXPECT_TRUE(BlockTags::NON_FLAMMABLE_WOOD().contains(ResourceLocation("minecraft", "crimson_stem")));
}

TEST_F(BlockTagsTest, NonFlammableWoodContainsWarpedPlanks)
{
    EXPECT_TRUE(BlockTags::NON_FLAMMABLE_WOOD().contains(ResourceLocation("minecraft", "warped_planks")));
}

TEST_F(BlockTagsTest, NonFlammableWoodContainsCrimsonDoor)
{
    EXPECT_TRUE(BlockTags::NON_FLAMMABLE_WOOD().contains(ResourceLocation("minecraft", "crimson_door")));
}

TEST_F(BlockTagsTest, NonFlammableWoodContainsWarpedTrapdoor)
{
    EXPECT_TRUE(BlockTags::NON_FLAMMABLE_WOOD().contains(ResourceLocation("minecraft", "warped_trapdoor")));
}

TEST_F(BlockTagsTest, NonFlammableWoodContainsCrimsonHyphae)
{
    EXPECT_TRUE(BlockTags::NON_FLAMMABLE_WOOD().contains(ResourceLocation("minecraft", "crimson_hyphae")));
}

TEST_F(BlockTagsTest, NonFlammableWoodContainsStrippedWarpedHyphae)
{
    EXPECT_TRUE(BlockTags::NON_FLAMMABLE_WOOD().contains(ResourceLocation("minecraft", "stripped_warped_hyphae")));
}

TEST_F(BlockTagsTest, NonFlammableWoodContainsCrimsonShelf)
{
    EXPECT_TRUE(BlockTags::NON_FLAMMABLE_WOOD().contains(ResourceLocation("minecraft", "crimson_shelf")));
}

TEST_F(BlockTagsTest, NonFlammableWoodContainsWarpedShelf)
{
    EXPECT_TRUE(BlockTags::NON_FLAMMABLE_WOOD().contains(ResourceLocation("minecraft", "warped_shelf")));
}

TEST_F(BlockTagsTest, NonFlammableWoodDoesNotContainOakPlanks)
{
    EXPECT_FALSE(BlockTags::NON_FLAMMABLE_WOOD().contains(ResourceLocation("minecraft", "oak_planks")));
}

TEST_F(BlockTagsTest, NonFlammableWoodDoesNotContainIronDoor)
{
    EXPECT_FALSE(BlockTags::NON_FLAMMABLE_WOOD().contains(ResourceLocation("minecraft", "iron_door")));
}

TEST_F(BlockTagsTest, NonFlammableWoodTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::NON_FLAMMABLE_WOOD().getId(), ResourceLocation("minecraft", "non_flammable_wood"));
}

// ========== FLOWERS 标签测试 ==========

TEST_F(BlockTagsTest, FlowersTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::FLOWERS().getId(), ResourceLocation("minecraft", "flowers"));
}

TEST_F(BlockTagsTest, FlowersContainsSmallFlowers)
{
    // 小花朵应包含在 flowers 标签中
    EXPECT_TRUE(BlockTags::FLOWERS().contains(ResourceLocation("minecraft", "dandelion")));
    EXPECT_TRUE(BlockTags::FLOWERS().contains(ResourceLocation("minecraft", "poppy")));
    EXPECT_TRUE(BlockTags::FLOWERS().contains(ResourceLocation("minecraft", "wither_rose")));
}

TEST_F(BlockTagsTest, FlowersContainsTallFlowers)
{
    // 高花朵应包含在 flowers 标签中
    EXPECT_TRUE(BlockTags::FLOWERS().contains(ResourceLocation("minecraft", "sunflower")));
    EXPECT_TRUE(BlockTags::FLOWERS().contains(ResourceLocation("minecraft", "lilac")));
    EXPECT_TRUE(BlockTags::FLOWERS().contains(ResourceLocation("minecraft", "pitcher_plant")));
}

TEST_F(BlockTagsTest, FlowersContainsSpecialFlowers)
{
    // 其他花类方块应包含在 flowers 标签中
    EXPECT_TRUE(BlockTags::FLOWERS().contains(ResourceLocation("minecraft", "flowering_azalea")));
    EXPECT_TRUE(BlockTags::FLOWERS().contains(ResourceLocation("minecraft", "chorus_flower")));
    EXPECT_TRUE(BlockTags::FLOWERS().contains(ResourceLocation("minecraft", "spore_blossom")));
    EXPECT_TRUE(BlockTags::FLOWERS().contains(ResourceLocation("minecraft", "cactus_flower")));
    EXPECT_TRUE(BlockTags::FLOWERS().contains(ResourceLocation("minecraft", "wildflowers")));
}

TEST_F(BlockTagsTest, FlowersDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::FLOWERS().contains(ResourceLocation("minecraft", "stone")));
}

// ========== SAPLINGS 标签测试 ==========

TEST_F(BlockTagsTest, SaplingsTagIdIsCorrect)
{
    EXPECT_EQ(BlockTags::SAPLINGS().getId(), ResourceLocation("minecraft", "saplings"));
}

TEST_F(BlockTagsTest, SaplingsContainsBasicSaplings)
{
    EXPECT_TRUE(BlockTags::SAPLINGS().contains(ResourceLocation("minecraft", "oak_sapling")));
    EXPECT_TRUE(BlockTags::SAPLINGS().contains(ResourceLocation("minecraft", "spruce_sapling")));
    EXPECT_TRUE(BlockTags::SAPLINGS().contains(ResourceLocation("minecraft", "birch_sapling")));
    EXPECT_TRUE(BlockTags::SAPLINGS().contains(ResourceLocation("minecraft", "jungle_sapling")));
    EXPECT_TRUE(BlockTags::SAPLINGS().contains(ResourceLocation("minecraft", "acacia_sapling")));
    EXPECT_TRUE(BlockTags::SAPLINGS().contains(ResourceLocation("minecraft", "dark_oak_sapling")));
}

TEST_F(BlockTagsTest, SaplingsContainsNewSaplings)
{
    EXPECT_TRUE(BlockTags::SAPLINGS().contains(ResourceLocation("minecraft", "cherry_sapling")));
    EXPECT_TRUE(BlockTags::SAPLINGS().contains(ResourceLocation("minecraft", "pale_oak_sapling")));
    EXPECT_TRUE(BlockTags::SAPLINGS().contains(ResourceLocation("minecraft", "mangrove_propagule")));
}

TEST_F(BlockTagsTest, SaplingsContainsAzalea)
{
    // 杜鹃花丛在 MC 中属于 saplings 标签
    EXPECT_TRUE(BlockTags::SAPLINGS().contains(ResourceLocation("minecraft", "azalea")));
    EXPECT_TRUE(BlockTags::SAPLINGS().contains(ResourceLocation("minecraft", "flowering_azalea")));
}

TEST_F(BlockTagsTest, SaplingsDoesNotContainStone)
{
    EXPECT_FALSE(BlockTags::SAPLINGS().contains(ResourceLocation("minecraft", "stone")));
}

// ========== LIGHTNING_RODS 更新测试 ==========

TEST_F(BlockTagsTest, LightningRodsContainsOxidizedVariants)
{
    // MC 1.21.11: lightning_rods 标签包含所有氧化变种
    EXPECT_TRUE(BlockTags::LIGHTNING_RODS().contains(ResourceLocation("minecraft", "lightning_rod")));
    EXPECT_TRUE(BlockTags::LIGHTNING_RODS().contains(ResourceLocation("minecraft", "exposed_lightning_rod")));
    EXPECT_TRUE(BlockTags::LIGHTNING_RODS().contains(ResourceLocation("minecraft", "weathered_lightning_rod")));
    EXPECT_TRUE(BlockTags::LIGHTNING_RODS().contains(ResourceLocation("minecraft", "oxidized_lightning_rod")));
}

TEST_F(BlockTagsTest, LightningRodsContainsWaxedVariants)
{
    // MC 1.21.11: lightning_rods 标签包含所有涂蜡变种
    EXPECT_TRUE(BlockTags::LIGHTNING_RODS().contains(ResourceLocation("minecraft", "waxed_lightning_rod")));
    EXPECT_TRUE(BlockTags::LIGHTNING_RODS().contains(ResourceLocation("minecraft", "waxed_exposed_lightning_rod")));
    EXPECT_TRUE(BlockTags::LIGHTNING_RODS().contains(ResourceLocation("minecraft", "waxed_weathered_lightning_rod")));
    EXPECT_TRUE(BlockTags::LIGHTNING_RODS().contains(ResourceLocation("minecraft", "waxed_oxidized_lightning_rod")));
}

// ============================================================================
// BEDS 标签测试
// ============================================================================

TEST_F(BlockTagsTest, BedsTagContainsAllColors)
{
    // minecraft:beds 标签应包含全部 16 色床方块
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::WHITE_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::ORANGE_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::MAGENTA_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::LIGHT_BLUE_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::YELLOW_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::LIME_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::PINK_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::GRAY_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::LIGHT_GRAY_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::CYAN_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::PURPLE_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::BLUE_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::BROWN_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::GREEN_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::RED_BED));
    EXPECT_TRUE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::BLACK_BED));
}

TEST_F(BlockTagsTest, BedsTagDoesNotContainNonBedBlocks)
{
    // 床标签不应包含非床方块
    EXPECT_FALSE(BlockTags::BEDS().contains(*VanillaBlocks::STONE));
    EXPECT_FALSE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::WHITE_WOOL));
    EXPECT_FALSE(BlockTags::BEDS().contains(*block_registry::ColoredBlocks::WHITE_CARPET));
}

TEST_F(BlockTagsTest, BedsTagId)
{
    EXPECT_EQ(BlockTags::BEDS().getId(), ResourceLocation("minecraft", "beds"));
}

// ============================================================================
// BEE_ATTRACTIVE 标签测试
// ============================================================================

TEST_F(BlockTagsTest, BeeAttractiveTagId)
{
    EXPECT_EQ(BlockTags::BEE_ATTRACTIVE().getId(), ResourceLocation("minecraft", "bee_attractive"));
}

TEST_F(BlockTagsTest, BeeAttractiveContainsOpenEyeblossom)
{
    // 开放眼眸花吸引蜜蜂
    Block* openEyeblossom = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "open_eyeblossom"));
    ASSERT_NE(openEyeblossom, nullptr);
    EXPECT_TRUE(BlockTags::BEE_ATTRACTIVE().contains(*openEyeblossom));
}

TEST_F(BlockTagsTest, BeeAttractiveDoesNotContainClosedEyeblossom)
{
    // 闭合眼眸花不吸引蜜蜂（与 MC 1.21.11 数据包一致）
    Block* closedEyeblossom = BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "closed_eyeblossom"));
    ASSERT_NE(closedEyeblossom, nullptr);
    EXPECT_FALSE(BlockTags::BEE_ATTRACTIVE().contains(*closedEyeblossom));
}

TEST_F(BlockTagsTest, BeeAttractiveContainsVanillaFlowers)
{
    // 验证 BEE_ATTRACTIVE 标签包含原版花朵（蒲公英、虞美人、向日葵等）
    EXPECT_TRUE(BlockTags::BEE_ATTRACTIVE().contains(ResourceLocation("minecraft", "dandelion")));
    EXPECT_TRUE(BlockTags::BEE_ATTRACTIVE().contains(ResourceLocation("minecraft", "poppy")));
    EXPECT_TRUE(BlockTags::BEE_ATTRACTIVE().contains(ResourceLocation("minecraft", "blue_orchid")));
    EXPECT_TRUE(BlockTags::BEE_ATTRACTIVE().contains(ResourceLocation("minecraft", "allium")));
    EXPECT_TRUE(BlockTags::BEE_ATTRACTIVE().contains(ResourceLocation("minecraft", "azure_bluet")));
    EXPECT_TRUE(BlockTags::BEE_ATTRACTIVE().contains(ResourceLocation("minecraft", "sunflower")));
    EXPECT_TRUE(BlockTags::BEE_ATTRACTIVE().contains(ResourceLocation("minecraft", "lilac")));
    EXPECT_TRUE(BlockTags::BEE_ATTRACTIVE().contains(ResourceLocation("minecraft", "wither_rose")));
    EXPECT_TRUE(BlockTags::BEE_ATTRACTIVE().contains(ResourceLocation("minecraft", "torchflower")));
    EXPECT_TRUE(BlockTags::BEE_ATTRACTIVE().contains(ResourceLocation("minecraft", "chorus_flower")));
    EXPECT_TRUE(BlockTags::BEE_ATTRACTIVE().contains(ResourceLocation("minecraft", "spore_blossom")));
}

TEST_F(BlockTagsTest, BeeAttractiveDoesNotContainNonFlowerBlocks)
{
    // 非花朵方块不应在 BEE_ATTRACTIVE 标签中
    EXPECT_FALSE(BlockTags::BEE_ATTRACTIVE().contains(*VanillaBlocks::STONE));
    EXPECT_FALSE(BlockTags::BEE_ATTRACTIVE().contains(*VanillaBlocks::GRASS_BLOCK));
    EXPECT_FALSE(BlockTags::BEE_ATTRACTIVE().contains(*VanillaBlocks::DIRT));
}
