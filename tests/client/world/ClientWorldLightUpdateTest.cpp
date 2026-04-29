// 在macOS系统头文件中，BYTE_SIZE被定义为宏，会与NibbleArray的静态常数冲突
// 使用pragma push_macro/pop_macro来暂时屏蔽系统宏
#pragma push_macro("BYTE_SIZE")
#undef BYTE_SIZE

#include <gtest/gtest.h>

#include "client/world/ClientWorld.hpp"
#include "common/network/sync/ChunkSync.hpp"
#include "common/util/NibbleArray.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"

#pragma pop_macro("BYTE_SIZE")

using namespace mc;
using namespace mc::client;

namespace {

MeshSchedulerConfig createSchedulerConfig()
{
    MeshSchedulerConfig config;
    config.maxDispatchedTaskCount = 1;
    config.reprioritizeIntervalFrames = 1;
    config.cameraMoveThreshold = 0.0f;
    config.cameraDirectionDotThreshold = 1.0f;
    config.behindCancelDotThreshold = -0.4f;
    config.behindCancelDistanceChunks = 0.0f;
    return config;
}

std::vector<u8> createSerializedChunkData()
{
    ChunkData chunkData(0, 0);
    const u32 stoneStateId = VanillaBlocks::STONE->defaultState().stateId();
    chunkData.fill(0, ChunkData::HEIGHT, stoneStateId);
    chunkData.setFullyGenerated(true);
    chunkData.setLoaded(true);

    auto serialized = network::ChunkSerializer::serializeChunk(chunkData);
    EXPECT_TRUE(serialized.success());
    if (!serialized.success()) {
        return {};
    }

    return serialized.value();
}

std::vector<u8> createLightData(u8 value)
{
    return std::vector<u8>(NibbleArray::BYTE_SIZE, value);
}

} // namespace

class ClientWorldLightUpdateTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }
};

TEST_F(ClientWorldLightUpdateTest, LightUpdateBurstDoesNotResubmitPendingChunkMesh)
{
    ClientWorld world;
    ASSERT_TRUE(world.initialize(12345).success());

    world.initializeMeshSystem(1, createSchedulerConfig());

    std::vector<u8> chunkBytes = createSerializedChunkData();
    ASSERT_FALSE(chunkBytes.empty());

    world.onChunkData(0, 0, std::move(chunkBytes));

    const MeshBuildScheduler* scheduler = world.meshBuildScheduler();
    ASSERT_NE(scheduler, nullptr);

    const MeshSchedulerStats initialStats = scheduler->stats();
    ASSERT_EQ(initialStats.submittedTaskCount, 1u);
    ASSERT_EQ(initialStats.trackedTaskCount, 1u);

    ClientChunk* chunk = world.getChunk(ChunkId(0, 0));
    ASSERT_NE(chunk, nullptr);

    const u64 initialTaskId = chunk->activeMeshTaskId;
    ASSERT_NE(initialTaskId, 0u);

    const std::vector<u8> skyLight = createLightData(0xFF);
    const std::vector<u8> blockLight = createLightData(0x00);

    for (i32 i = 0; i < 8; ++i) {
        world.onLightUpdate(0, 0, 0, skyLight, blockLight, false);
    }

    const MeshSchedulerStats afterStats = scheduler->stats();
    EXPECT_EQ(afterStats.submittedTaskCount, initialStats.submittedTaskCount);
    EXPECT_EQ(afterStats.trackedTaskCount, initialStats.trackedTaskCount);
    EXPECT_EQ(world.getChunk(ChunkId(0, 0))->activeMeshTaskId, initialTaskId);

    world.shutdownMeshSystem();
}