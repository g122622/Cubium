#include <gtest/gtest.h>

#include "LightingTestHelpers.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"

using namespace mc;
using namespace lighting_test;

namespace {

class LightingTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        ensureVanillaBlocksInitialized();
    }
};

} // namespace

TEST_F(LightingTest, LightTypeValues)
{
    EXPECT_EQ(static_cast<u8>(LightType::SKY), 0);
    EXPECT_EQ(static_cast<u8>(LightType::BLOCK), 1);
}

TEST_F(LightingTest, SWMRNibbleArrayFromVanillaHandlesNullAndEmpty)
{
    NibbleArray emptyArray;

    SWMRNibbleArray fromNull = SWMRNibbleArray::fromVanilla(nullptr);
    SWMRNibbleArray fromEmpty = SWMRNibbleArray::fromVanilla(&emptyArray);

    EXPECT_TRUE(fromNull.isNullNibbleUpdating());
    EXPECT_TRUE(fromNull.isNullNibbleVisible());
    EXPECT_TRUE(fromEmpty.isUninitialisedUpdating());
    EXPECT_TRUE(fromEmpty.isUninitialisedVisible());
}

TEST_F(LightingTest, SWMRNibbleArrayPublishesWrittenValues)
{
    SWMRNibbleArray array;

    EXPECT_TRUE(array.isUninitialisedUpdating());
    EXPECT_FALSE(array.isInitialisedVisible());

    array.set(1, 2, 3, 9);

    EXPECT_TRUE(array.isInitialisedUpdating());
    EXPECT_TRUE(array.isDirty());
    EXPECT_EQ(array.getUpdating(1, 2, 3), 9);
    EXPECT_EQ(array.getVisible(1, 2, 3), 0);

    EXPECT_TRUE(array.updateVisible());
    EXPECT_TRUE(array.isInitialisedVisible());
    EXPECT_EQ(array.getVisible(1, 2, 3), 9);

    const auto vanilla = array.toVanillaNibble();
    ASSERT_TRUE(vanilla.has_value());
    EXPECT_EQ(vanilla->get(1, 2, 3), 9);

    const auto bytes = array.toByteArray();
    EXPECT_EQ(bytes.size(), NibbleArray::BYTE_SIZE);
}

TEST_F(LightingTest, WorldLightManagerStoresAndClearsSectionData)
{
    TestLightingProvider provider(0, 16, 1, true);
    WorldLightManager lightManager(&provider, true, true);
    const SectionPos sectionPos(0, 0, 0);
    const ChunkPos chunkPos(0, 0);

    EXPECT_EQ(lightManager.getSkyLight(0, 0, 0), 15);
    EXPECT_EQ(lightManager.getBlockLight(0, 0, 0), 0);
    EXPECT_EQ(lightManager.getLightSubtracted(BlockPos(0, 0, 0), 3), 12);

    lightManager.updateSectionStatus(sectionPos, false);
    lightManager.enableLightSources(chunkPos, true);

    const NibbleArray skyData = NibbleArray::filled(11);
    const NibbleArray blockData = NibbleArray::filled(4);

    lightManager.setData(LightType::SKY, sectionPos, skyData, false);
    lightManager.setData(LightType::BLOCK, sectionPos, blockData, false);

    EXPECT_EQ(lightManager.getSkyLight(0, 0, 0), 11);
    EXPECT_EQ(lightManager.getBlockLight(0, 0, 0), 4);
    EXPECT_EQ(lightManager.getData(LightType::SKY, sectionPos), skyData.data());
    EXPECT_EQ(lightManager.getData(LightType::BLOCK, sectionPos), blockData.data());

    const String debugInfo = lightManager.getDebugInfo(LightType::SKY, sectionPos);
    EXPECT_NE(debugInfo.find("size=2048"), String::npos);

    lightManager.retainData(chunkPos, false);
    lightManager.enableLightSources(chunkPos, false);

    EXPECT_TRUE(lightManager.getData(LightType::SKY, sectionPos).empty());
    EXPECT_TRUE(lightManager.getData(LightType::BLOCK, sectionPos).empty());
}
