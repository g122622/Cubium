#include <gtest/gtest.h>

#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockTags.hpp"
#include "world/block/VanillaBlocks.hpp"

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

TEST_F(BlockTagsTest, DirtContainsFarmland)
{
    EXPECT_TRUE(BlockTags::DIRT().contains(*VanillaBlocks::FARMLAND));
}

TEST_F(BlockTagsTest, SandContainsSand)
{
    EXPECT_TRUE(BlockTags::SAND().contains(*VanillaBlocks::SAND));
}

TEST_F(BlockTagsTest, SandContainsRedSand)
{
    EXPECT_TRUE(BlockTags::SAND().contains(*VanillaBlocks::RED_SAND));
}

TEST_F(BlockTagsTest, SoulFireBaseBlocksContainsSoulSand)
{
    EXPECT_TRUE(BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(*VanillaBlocks::SOUL_SAND));
}

TEST_F(BlockTagsTest, SoulFireBaseBlocksContainsSoulSoil)
{
    EXPECT_TRUE(BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(*VanillaBlocks::SOUL_SOIL));
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
