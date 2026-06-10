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

#include "client/renderer/mesh/MeshWorkerPool.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

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

void waitUntilCompletedAtLeast(MeshWorkerPool& pool, size_t expectedCount)
{
    i32 attempts = 0;
    while (pool.completedTaskCount() < expectedCount && attempts < 200) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++attempts;
    }
}

MeshWorkerTask makeTask(ChunkCoord x, ChunkCoord z)
{
    MeshWorkerTask task;
    task.chunkId = ChunkId(x, z, 0);
    task.taskId = static_cast<u64>((x + 2048) * 4096 + (z + 2048));
    task.chunkData = createTestChunkData(x, z);
    task.neighbors = {};
    task.cancelSignal = std::make_shared<std::atomic<bool>>(false);
    return task;
}

} // namespace

class MeshWorkerPoolTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(MeshWorkerPoolTest, StartStop)
{
    MeshWorkerPool pool(2);

    EXPECT_FALSE(pool.isRunning());

    pool.start();
    EXPECT_TRUE(pool.isRunning());

    pool.shutdown();
    EXPECT_FALSE(pool.isRunning());
}

TEST_F(MeshWorkerPoolTest, SubmitAndDrainSingleTask)
{
    MeshWorkerPool pool(1);
    pool.start();

    pool.submit(makeTask(0, 0));
    waitUntilCompletedAtLeast(pool, 1);

    i32 callbackCount = 0;
    pool.drainCompleted(
        [&callbackCount](MeshWorkerResult&& result) {
            EXPECT_EQ(result.chunkId, ChunkId(0, 0, 0));
            ++callbackCount;
        },
        8);

    EXPECT_EQ(callbackCount, 1);
    EXPECT_EQ(pool.completedTaskCount(), 0u);

    pool.shutdown();
}

TEST_F(MeshWorkerPoolTest, DrainRespectsMaxCount)
{
    MeshWorkerPool pool(2);
    pool.start();

    for (i32 i = 0; i < 6; ++i) {
        pool.submit(makeTask(i, 0));
    }

    waitUntilCompletedAtLeast(pool, 6);

    i32 callbackCount = 0;
    pool.drainCompleted([&callbackCount](MeshWorkerResult&&) { ++callbackCount; }, 3);

    EXPECT_EQ(callbackCount, 3);
    EXPECT_EQ(pool.completedTaskCount(), 3u);

    pool.shutdown();
}

TEST_F(MeshWorkerPoolTest, SubmitWithoutStartIgnored)
{
    MeshWorkerPool pool(1);

    pool.submit(makeTask(0, 0));

    EXPECT_FALSE(pool.isRunning());
    EXPECT_EQ(pool.queuedTaskCount(), 0u);
    EXPECT_EQ(pool.completedTaskCount(), 0u);
}

TEST_F(MeshWorkerPoolTest, PreCancelledTaskReturnsCancelledResult)
{
    MeshWorkerPool pool(1);
    pool.start();

    MeshWorkerTask task = makeTask(2, 3);
    task.cancelSignal->store(true, std::memory_order_release);

    pool.submit(std::move(task));
    waitUntilCompletedAtLeast(pool, 1);

    bool gotCancelled = false;
    pool.drainCompleted(
        [&gotCancelled](MeshWorkerResult&& result) {
            gotCancelled = result.cancelled;
            EXPECT_FALSE(result.success);
        },
        2);

    EXPECT_TRUE(gotCancelled);

    pool.shutdown();
}

TEST_F(MeshWorkerPoolTest, ConcurrentSubmission)
{
    MeshWorkerPool pool(4);
    pool.start();

    constexpr i32 THREAD_COUNT = 4;
    constexpr i32 TASKS_PER_THREAD = 8;

    std::vector<std::thread> submitThreads;
    submitThreads.reserve(THREAD_COUNT);

    for (i32 threadIndex = 0; threadIndex < THREAD_COUNT; ++threadIndex) {
        submitThreads.emplace_back([&pool, threadIndex, TASKS_PER_THREAD]() {
            for (i32 i = 0; i < TASKS_PER_THREAD; ++i) {
                pool.submit(makeTask(threadIndex * 100 + i, threadIndex));
            }
        });
    }

    for (std::thread& submitThread : submitThreads) {
        submitThread.join();
    }

    const size_t expectedCount = static_cast<size_t>(THREAD_COUNT * TASKS_PER_THREAD);
    waitUntilCompletedAtLeast(pool, expectedCount);

    size_t drainedCount = 0;
    while (pool.completedTaskCount() > 0) {
        pool.drainCompleted([&drainedCount](MeshWorkerResult&&) { ++drainedCount; }, 16);
    }

    EXPECT_EQ(drainedCount, expectedCount);

    pool.shutdown();
}
