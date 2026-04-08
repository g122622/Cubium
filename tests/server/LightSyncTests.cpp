#include <gtest/gtest.h>

#include "../lighting/LightingTestHelpers.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/lighting/LightType.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "server/sync/LightSyncManager.hpp"
#include "server/world/ServerWorld.hpp"

using namespace mc;
using namespace mc::server;
using namespace mc::server::sync;
using namespace lighting_test;

namespace {

class LightSyncTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        ensureVanillaBlocksInitialized();
    }

    static void prepareWorld(ServerWorld& world)
    {
        world.setLightManager(std::make_unique<WorldLightManager>(&world, true, true));
    }
};

} // namespace

TEST_F(LightSyncTests, InitializeChunkLightingCopiesSectionData)
{
    ServerWorld world;
    prepareWorld(world);

    ChunkData* chunk = world.getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    ChunkSection* section = chunk->createSection(0);
    ASSERT_NE(section, nullptr);

    const NibbleArray skyData = NibbleArray::filled(11);
    const NibbleArray blockData = NibbleArray::filled(4);
    section->skyLightNibble() = skyData;
    section->blockLightNibble() = blockData;

    LightSyncManager lightSyncManager(*world.lightManager(), *world.chunkManager());
    lightSyncManager.initializeChunkLighting(0, 0);

    const SectionPos sectionPos(0, 0, 0);
    EXPECT_EQ(world.lightManager()->getData(LightType::SKY, sectionPos), skyData.data());
    EXPECT_EQ(world.lightManager()->getData(LightType::BLOCK, sectionPos), blockData.data());
}

TEST_F(LightSyncTests, MarkLightChangedSyncsDataBackToChunk)
{
    ServerWorld world;
    prepareWorld(world);

    ChunkData* chunk = world.getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    ChunkSection* section = chunk->createSection(0);
    ASSERT_NE(section, nullptr);

    LightSyncManager lightSyncManager(*world.lightManager(), *world.chunkManager());
    const SectionPos sectionPos(0, 0, 0);

    const NibbleArray skyData = NibbleArray::filled(13);
    const NibbleArray blockData = NibbleArray::filled(6);
    world.lightManager()->setData(LightType::SKY, sectionPos, skyData, false);
    world.lightManager()->setData(LightType::BLOCK, sectionPos, blockData, false);

    lightSyncManager.markLightChanged(LightType::SKY, sectionPos);
    lightSyncManager.markLightChanged(LightType::BLOCK, sectionPos);

    EXPECT_TRUE(chunk->isDirty());
    EXPECT_EQ(section->skyLightNibble().data(), skyData.data());
    EXPECT_EQ(section->blockLightNibble().data(), blockData.data());
}
