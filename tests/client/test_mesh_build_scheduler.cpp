#include <gtest/gtest.h>
#include "client/renderer/mesh/MeshBuildScheduler.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include <chrono>
#include <thread>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

using namespace mc;
using namespace mc::client;

namespace {

std::shared_ptr<ChunkData> createTestChunkData(ChunkCoord x, ChunkCoord z)
{
    auto chunkData = std::make_shared<ChunkData>(x, z);
    for (i32 bx = 0; bx < 16; ++bx) {
        for (i32 bz = 0; bz < 16; ++bz) {
            for (i32 by = 0; by < 16; ++by) {
                chunkData->setBlockStateId(bx, by, bz, 1);
            }
        }
    }
    chunkData->setFullyGenerated(true);
    return chunkData;
}

MeshBuildRequest makeRequest(ChunkCoord x, ChunkCoord z)
{
    MeshBuildRequest request;
    request.chunkId = ChunkId(x, z);
    request.chunkData = createTestChunkData(x, z);
    request.neighbors = {};
    return request;
}

MeshSchedulerConfig createSchedulerConfig(i32 maxDispatchedTaskCount)
{
    MeshSchedulerConfig config;
    config.maxDispatchedTaskCount = maxDispatchedTaskCount;
    config.reprioritizeIntervalFrames = 1;
    config.cameraMoveThreshold = 0.0f;
    config.cameraDirectionDotThreshold = 1.0f;
    config.behindCancelDotThreshold = -0.4f;
    config.behindCancelDistanceChunks = 6.0f;
    return config;
}

MeshSchedulerViewState createViewState(i32 renderDistanceChunks)
{
    MeshSchedulerViewState viewState;
    viewState.cameraPosition = glm::vec3(8.0f, 64.0f, 8.0f);
    viewState.cameraForward = glm::vec3(0.0f, 0.0f, 1.0f);

    const glm::mat4 projection = glm::perspective(glm::radians(70.0f), 1.0f, 0.1f, 1024.0f);
    const glm::mat4 view = glm::lookAt(
        viewState.cameraPosition,
        viewState.cameraPosition + viewState.cameraForward,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    viewState.viewProjectionMatrix = projection * view;
    viewState.renderDistanceChunks = renderDistanceChunks;
    viewState.minBuildHeight = 0;
    viewState.maxBuildHeight = 256;
    return viewState;
}

} // namespace

class MeshBuildSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
    }
};

TEST_F(MeshBuildSchedulerTest, LatestTaskForChunkWins)
{
    MeshWorkerPool pool(1);
    pool.start();

    MeshBuildScheduler scheduler(pool, createSchedulerConfig(1));
    scheduler.setViewState(createViewState(12));

    const u64 firstTaskId = scheduler.submit(makeRequest(0, 0));
    const u64 secondTaskId = scheduler.submit(makeRequest(0, 0));

    ASSERT_NE(firstTaskId, 0u);
    ASSERT_NE(secondTaskId, 0u);
    EXPECT_NE(firstTaskId, secondTaskId);

    std::vector<u64> completedTaskIds;
    for (i32 attempt = 0; attempt < 200 && completedTaskIds.empty(); ++attempt) {
        scheduler.tick();
        scheduler.drainCompleted(
            [&completedTaskIds](MeshWorkerResult&& result) {
                completedTaskIds.push_back(result.taskId);
            },
            8
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    ASSERT_EQ(completedTaskIds.size(), 1u);
    EXPECT_EQ(completedTaskIds.front(), secondTaskId);

    pool.shutdown();
}

TEST_F(MeshBuildSchedulerTest, FrustumVisibleChunkHasHigherPriority)
{
    MeshWorkerPool pool(1);
    pool.start();

    MeshBuildScheduler scheduler(pool, createSchedulerConfig(1));
    scheduler.setViewState(createViewState(12));

    const ChunkId behindChunk(0, -1);
    const ChunkId frontChunk(0, 1);

    scheduler.submit(makeRequest(behindChunk.x, behindChunk.z));
    scheduler.submit(makeRequest(frontChunk.x, frontChunk.z));

    std::vector<ChunkId> completedChunks;
    for (i32 attempt = 0; attempt < 200 && completedChunks.empty(); ++attempt) {
        scheduler.tick();
        scheduler.drainCompleted(
            [&completedChunks](MeshWorkerResult&& result) {
                completedChunks.push_back(result.chunkId);
            },
            1
        );
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    ASSERT_FALSE(completedChunks.empty());
    EXPECT_EQ(completedChunks.front(), frontChunk);

    pool.shutdown();
}

TEST_F(MeshBuildSchedulerTest, OutOfRangePendingTaskCancelledBeforeDispatch)
{
    MeshWorkerPool pool(1);
    pool.start();

    MeshBuildScheduler scheduler(pool, createSchedulerConfig(1));
    scheduler.setViewState(createViewState(2));

    const u64 taskId = scheduler.submit(makeRequest(32, 32));
    ASSERT_NE(taskId, 0u);

    scheduler.tick();

    const MeshSchedulerStats stats = scheduler.stats();
    EXPECT_EQ(stats.trackedTaskCount, 0u);
    EXPECT_EQ(stats.pendingTaskCount, 0u);
    EXPECT_GE(stats.cancelledTaskCount, 1u);
    EXPECT_EQ(pool.queuedTaskCount(), 0u);
    EXPECT_FALSE(scheduler.isTaskTracked(taskId));

    pool.shutdown();
}
