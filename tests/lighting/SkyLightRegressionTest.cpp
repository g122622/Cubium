#include <gtest/gtest.h>

#include "LightingTestHelpers.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"

using namespace mc;
using namespace lighting_test;

namespace {

class SkyLightRegressionTest : public ::testing::Test {
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

TEST_F(SkyLightRegressionTest, EmptyColumnStaysFullyLit)
{
    TestLightingProvider provider(0, 16, 1, true);
    WorldLightManager lightManager(&provider, true, true);

    prepareChunk(lightManager);

    lightManager.checkBlock(8, 8, 8);
    EXPECT_EQ(lightManager.tick(64, true, false), 0);

    EXPECT_EQ(lightManager.getSkyLight(8, 15, 8), 15);
    EXPECT_EQ(lightManager.getSkyLight(8, 0, 8), 15);
}

TEST_F(SkyLightRegressionTest, SingleRoofBlockStillAllowsLateralSkyLight)
{
    TestLightingProvider provider(0, 16, 1, true);
    WorldLightManager lightManager(&provider, true, true);

    prepareChunk(lightManager);
    provider.setBlock(8, 15, 8, &VanillaBlocks::STONE->defaultState());

    lightManager.checkBlock(8, 15, 8);
    EXPECT_EQ(lightManager.tick(64, true, false), 0);

    EXPECT_EQ(lightManager.getSkyLight(8, 15, 8), 0);
    EXPECT_EQ(lightManager.getSkyLight(8, 14, 8), 14);
}

TEST_F(SkyLightRegressionTest, RemovingRoofRestoresSkyLight)
{
    TestLightingProvider provider(0, 16, 1, true);
    WorldLightManager lightManager(&provider, true, true);

    prepareChunk(lightManager);
    provider.setBlock(8, 15, 8, &VanillaBlocks::STONE->defaultState());

    lightManager.checkBlock(8, 15, 8);
    EXPECT_EQ(lightManager.tick(64, true, false), 0);
    EXPECT_EQ(lightManager.getSkyLight(8, 14, 8), 14);

    provider.setBlock(8, 15, 8, &VanillaBlocks::AIR->defaultState());
    lightManager.checkBlock(8, 15, 8);
    EXPECT_EQ(lightManager.tick(64, true, false), 0);

    EXPECT_EQ(lightManager.getSkyLight(8, 15, 8), 15);
    EXPECT_EQ(lightManager.getSkyLight(8, 14, 8), 15);
}
