#include <gtest/gtest.h>

#include "LightingTestHelpers.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"

using namespace mc;
using namespace lighting_test;

namespace {

class LightUpdateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ensureVanillaBlocksInitialized();
    }
};

} // namespace

TEST_F(LightUpdateTest, TickWithoutWorkReturnsBudget)
{
    TestLightingProvider provider(0, 16, 1, true);
    WorldLightManager lightManager(&provider, true, true);

    EXPECT_FALSE(lightManager.hasLightWork());
    EXPECT_EQ(lightManager.tick(32, true, true), 32);
}

TEST_F(LightUpdateTest, CheckBlockMarksWorkAndTickClearsIt)
{
    TestLightingProvider provider(0, 16, 1, true);
    WorldLightManager lightManager(&provider, true, true);

    lightManager.checkBlock(0, 0, 0);
    EXPECT_TRUE(lightManager.hasLightWork());

    EXPECT_EQ(lightManager.tick(32, false, false), 0);
    EXPECT_FALSE(lightManager.hasLightWork());
}

TEST_F(LightUpdateTest, BlockEmissionIncreaseTriggersChangedSection)
{
    TestLightingProvider provider(0, 16, 1, true);
    WorldLightManager lightManager(&provider, true, true);
    const ChunkPos chunkPos(0, 0);
    const SectionPos sectionPos(0, 0, 0);

    lightManager.updateSectionStatus(sectionPos, false);
    lightManager.enableLightSources(chunkPos, true);

    provider.setBlock(8, 8, 8, &VanillaBlocks::LANTERN->defaultState());

    lightManager.checkBlock(8, 8, 8);
    lightManager.onBlockEmissionIncrease(8, 8, 8, 15);

    EXPECT_TRUE(lightManager.hasLightWork());
    EXPECT_EQ(lightManager.tick(64, false, true), 0);
    EXPECT_FALSE(lightManager.hasLightWork());

    EXPECT_EQ(lightManager.getBlockLight(8, 8, 8), 15);
    EXPECT_EQ(lightManager.getBlockLight(9, 8, 8), 14);
    EXPECT_EQ(provider.changedSections().size(), 1u);
    EXPECT_TRUE(provider.hasChangedSection(LightType::BLOCK, sectionPos));
}
