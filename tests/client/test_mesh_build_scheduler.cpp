/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, without limitation the rights
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

#include "client/renderer/mesh/MeshBuildScheduler.hpp"
#include "client/renderer/mesh/MeshDataPool.hpp"
#include "client/renderer/mesh/MeshResultQueue.hpp"
#include "common/core/Constants.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <chrono>
#include <thread>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::client;
using namespace mc::util;

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
    request.chunkId = ChunkId(x, z, 0);
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
        viewState.cameraPosition, viewState.cameraPosition + viewState.cameraForward, glm::vec3(0.0f, 1.0f, 0.0f));

    viewState.viewProjectionMatrix = projection * view;
    viewState.renderDistanceChunks = renderDistanceChunks;
    viewState.minBuildHeight = mc::world::MIN_BUILD_HEIGHT;
    viewState.maxBuildHeight = mc::world::MAX_BUILD_HEIGHT;
    return viewState;
}

} // namespace

class MeshBuildSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(MeshBuildSchedulerTest, LatestTaskForChunkWins)
{
    UniversalWorkerPool pool{1, "TestCompute", 900};
    pool.start();

    auto dataPool = std::make_shared<MeshDataPool>();
    auto resultQueue = std::make_shared<MeshResultQueue>();
    MeshBuildScheduler scheduler(pool, dataPool, resultQueue, createSchedulerConfig(1));
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
            [&completedTaskIds](MeshWorkerResult&& result) { completedTaskIds.push_back(result.taskId); }, 8);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    ASSERT_EQ(completedTaskIds.size(), 1u);
    EXPECT_EQ(completedTaskIds.front(), secondTaskId);

    scheduler.shutdown();
    pool.shutdown();
}

TEST_F(MeshBuildSchedulerTest, FrustumVisibleChunkHasHigherPriority)
{
    UniversalWorkerPool pool{1, "TestCompute", 900};
    pool.start();

    auto dataPool = std::make_shared<MeshDataPool>();
    auto resultQueue = std::make_shared<MeshResultQueue>();
    MeshBuildScheduler scheduler(pool, dataPool, resultQueue, createSchedulerConfig(1));
    scheduler.setViewState(createViewState(12));

    const ChunkId behindChunk(0, -1, 0);
    const ChunkId frontChunk(0, 1, 0);

    scheduler.submit(makeRequest(behindChunk.x, behindChunk.z));
    scheduler.submit(makeRequest(frontChunk.x, frontChunk.z));

    std::vector<ChunkId> completedChunks;
    for (i32 attempt = 0; attempt < 200 && completedChunks.empty(); ++attempt) {
        scheduler.tick();
        scheduler.drainCompleted(
            [&completedChunks](MeshWorkerResult&& result) { completedChunks.push_back(result.chunkId); }, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    ASSERT_FALSE(completedChunks.empty());
    EXPECT_EQ(completedChunks.front(), frontChunk);

    scheduler.shutdown();
    pool.shutdown();
}

TEST_F(MeshBuildSchedulerTest, OutOfRangePendingTaskCancelledBeforeDispatch)
{
    UniversalWorkerPool pool{1, "TestCompute", 900};
    pool.start();

    auto dataPool = std::make_shared<MeshDataPool>();
    auto resultQueue = std::make_shared<MeshResultQueue>();
    MeshBuildScheduler scheduler(pool, dataPool, resultQueue, createSchedulerConfig(1));
    scheduler.setViewState(createViewState(2));

    const u64 taskId = scheduler.submit(makeRequest(32, 32));
    ASSERT_NE(taskId, 0u);

    scheduler.tick();

    const MeshSchedulerStats stats = scheduler.stats();
    EXPECT_EQ(stats.trackedTaskCount, 0u);
    EXPECT_EQ(stats.pendingTaskCount, 0u);
    EXPECT_GE(stats.cancelledTaskCount, 1u);
    EXPECT_EQ(pool.pendingTaskCount(), 0u);
    EXPECT_FALSE(scheduler.isTaskTracked(taskId));

    scheduler.shutdown();
    pool.shutdown();
}
