#include <gtest/gtest.h>

#include "LightingTestHelpers.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"

using namespace mc;
using namespace lighting_test;

namespace {

class BlockLightRegressionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ensureVanillaBlocksInitialized();
    }

    static void prepareChunk(WorldLightManager& lightManager)
    {
        lightManager.updateSectionStatus(SectionPos(0, 0, 0), false);
        lightManager.enableLightSources(ChunkPos(0, 0), true);
    }
};

} // namespace

TEST_F(BlockLightRegressionTest, EmissiveBlockPropagatesToNeighbors)
{
    TestLightingProvider provider(0, 16, 1, true);
    WorldLightManager lightManager(&provider, true, true);

    prepareChunk(lightManager);
    provider.setBlock(8, 8, 8, &VanillaBlocks::LANTERN->defaultState());

    lightManager.checkBlock(8, 8, 8);
    lightManager.onBlockEmissionIncrease(8, 8, 8, 15);
    EXPECT_EQ(lightManager.tick(64, false, true), 0);

    EXPECT_EQ(lightManager.getBlockLight(8, 8, 8), 15);
    EXPECT_EQ(lightManager.getBlockLight(9, 8, 8), 14);
    EXPECT_EQ(lightManager.getBlockLight(10, 8, 8), 13);
}

TEST_F(BlockLightRegressionTest, OpaqueBlockBlocksDirectPathOnly)
{
    TestLightingProvider provider(0, 16, 1, true);
    WorldLightManager lightManager(&provider, true, true);

    prepareChunk(lightManager);
    provider.setBlock(8, 8, 8, &VanillaBlocks::LANTERN->defaultState());
    provider.setBlock(9, 8, 8, &VanillaBlocks::STONE->defaultState());

    lightManager.checkBlock(8, 8, 8);
    lightManager.onBlockEmissionIncrease(8, 8, 8, 15);
    EXPECT_EQ(lightManager.tick(64, false, true), 0);

    EXPECT_EQ(lightManager.getBlockLight(8, 8, 8), 15);
    EXPECT_EQ(lightManager.getBlockLight(9, 8, 8), 0);
    EXPECT_EQ(lightManager.getBlockLight(10, 8, 8), 11);
}

TEST_F(BlockLightRegressionTest, RemovingOpaqueBlockRestoresPropagation)
{
    TestLightingProvider provider(0, 16, 1, true);
    WorldLightManager lightManager(&provider, true, true);

    prepareChunk(lightManager);
    provider.setBlock(8, 8, 8, &VanillaBlocks::LANTERN->defaultState());
    provider.setBlock(9, 8, 8, &VanillaBlocks::STONE->defaultState());

    lightManager.checkBlock(8, 8, 8);
    lightManager.onBlockEmissionIncrease(8, 8, 8, 15);
    EXPECT_EQ(lightManager.tick(64, false, true), 0);
    EXPECT_EQ(lightManager.getBlockLight(10, 8, 8), 11);

    provider.setBlock(9, 8, 8, &VanillaBlocks::AIR->defaultState());
    lightManager.checkBlock(9, 8, 8);
    EXPECT_EQ(lightManager.tick(64, false, true), 0);

    EXPECT_EQ(lightManager.getBlockLight(9, 8, 8), 14);
    EXPECT_EQ(lightManager.getBlockLight(10, 8, 8), 13);
}
