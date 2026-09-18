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

#include "client/renderer/mesh/MeshBuildTask.hpp"
#include "client/renderer/mesh/MeshDataPool.hpp"
#include "client/renderer/mesh/MeshResultQueue.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>
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

void waitUntilQueueSizeAtLeast(MeshResultQueue& queue, size_t expectedCount)
{
    i32 attempts = 0;
    while (queue.size() < expectedCount && attempts < 200) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ++attempts;
    }
}

struct MeshTaskFixture {
    explicit MeshTaskFixture(i32 threadCount)
        : pool(threadCount, "TestCompute", 900)
    {}

    UniversalWorkerPool pool;
    std::shared_ptr<MeshDataPool> dataPool = std::make_shared<MeshDataPool>();
    std::shared_ptr<MeshResultQueue> resultQueue = std::make_shared<MeshResultQueue>();

    void start() { pool.start(); }

    void submitTask(ChunkCoord x, ChunkCoord z, u64 taskId, std::shared_ptr<std::atomic<bool>> abortSignal = nullptr)
    {
        // abortSignal 作为取消令牌经 pool.submit 传入（execute 收到的即该令牌）；
        // 不传时池内用静态 neverAbort，任务不可被取消。
        auto token = abortSignal ? std::move(abortSignal) : std::make_shared<std::atomic<bool>>(false);
        auto task = std::make_unique<MeshBuildTask>(ChunkId(x, z, 0),
            taskId,
            createTestChunkData(x, z),
            std::array<std::shared_ptr<const ChunkData>, 6>{},
            dataPool,
            std::weak_ptr<MeshResultQueue>(resultQueue));
        pool.submit(std::move(task), nullptr, TaskPriority::Normal, std::move(token));
    }
};

} // namespace

class MeshBuildTaskTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(MeshBuildTaskTest, StartStop)
{
    UniversalWorkerPool pool{2, "TestCompute", 900};

    EXPECT_FALSE(pool.isRunning());

    pool.start();
    EXPECT_TRUE(pool.isRunning());

    pool.shutdown();
    EXPECT_FALSE(pool.isRunning());
}

TEST_F(MeshBuildTaskTest, SubmitAndDrainSingleTask)
{
    MeshTaskFixture fx{1};
    fx.start();

    fx.submitTask(0, 0, 42);
    waitUntilQueueSizeAtLeast(*fx.resultQueue, 1);

    i32 callbackCount = 0;
    fx.resultQueue->drain(
        [&callbackCount](MeshWorkerResult&& result) {
            EXPECT_EQ(result.chunkId, ChunkId(0, 0, 0));
            EXPECT_EQ(result.taskId, 42u);
            ++callbackCount;
        },
        8);

    EXPECT_EQ(callbackCount, 1);
    EXPECT_EQ(fx.resultQueue->size(), 0u);

    fx.pool.shutdown();
}

TEST_F(MeshBuildTaskTest, DrainRespectsMaxCount)
{
    MeshTaskFixture fx{2};
    fx.start();

    for (i32 i = 0; i < 6; ++i) {
        fx.submitTask(i, 0, static_cast<u64>(i));
    }

    waitUntilQueueSizeAtLeast(*fx.resultQueue, 6);

    i32 callbackCount = 0;
    fx.resultQueue->drain([&callbackCount](MeshWorkerResult&&) { ++callbackCount; }, 3);

    EXPECT_EQ(callbackCount, 3);
    EXPECT_EQ(fx.resultQueue->size(), 3u);

    fx.pool.shutdown();
}

TEST_F(MeshBuildTaskTest, SubmitWithoutStartIgnored)
{
    UniversalWorkerPool pool{1, "TestCompute", 900};
    auto dataPool = std::make_shared<MeshDataPool>();
    auto resultQueue = std::make_shared<MeshResultQueue>();

    auto task = std::make_unique<MeshBuildTask>(ChunkId(0, 0, 0),
        1u,
        createTestChunkData(0, 0),
        std::array<std::shared_ptr<const ChunkData>, 6>{},
        dataPool,
        std::weak_ptr<MeshResultQueue>(resultQueue));
    pool.submit(std::move(task), nullptr, TaskPriority::Normal, nullptr);

    EXPECT_FALSE(pool.isRunning());
    EXPECT_EQ(resultQueue->size(), 0u);
}

TEST_F(MeshBuildTaskTest, PreCancelledTaskReturnsCancelledResult)
{
    MeshTaskFixture fx{1};
    fx.start();

    auto abortSignal = std::make_shared<std::atomic<bool>>(false);
    abortSignal->store(true, std::memory_order::release);
    fx.submitTask(2, 3, 7, abortSignal);
    waitUntilQueueSizeAtLeast(*fx.resultQueue, 1);

    bool gotCancelled = false;
    fx.resultQueue->drain(
        [&gotCancelled](MeshWorkerResult&& result) {
            gotCancelled = result.cancelled;
            EXPECT_FALSE(result.success);
        },
        2);

    EXPECT_TRUE(gotCancelled);

    fx.pool.shutdown();
}

TEST_F(MeshBuildTaskTest, ConcurrentSubmission)
{
    MeshTaskFixture fx{4};
    fx.start();

    constexpr i32 THREAD_COUNT = 4;
    constexpr i32 TASKS_PER_THREAD = 8;

    std::vector<std::thread> submitThreads;
    submitThreads.reserve(THREAD_COUNT);

    for (i32 threadIndex = 0; threadIndex < THREAD_COUNT; ++threadIndex) {
        submitThreads.emplace_back([&fx, threadIndex, TASKS_PER_THREAD]() {
            for (i32 i = 0; i < TASKS_PER_THREAD; ++i) {
                fx.submitTask(threadIndex * 100 + i, threadIndex, static_cast<u64>(threadIndex * 100 + i));
            }
        });
    }

    for (std::thread& submitThread : submitThreads) {
        submitThread.join();
    }

    const size_t expectedCount = static_cast<size_t>(THREAD_COUNT * TASKS_PER_THREAD);
    waitUntilQueueSizeAtLeast(*fx.resultQueue, expectedCount);

    size_t drainedCount = 0;
    while (fx.resultQueue->size() > 0) {
        fx.resultQueue->drain([&drainedCount](MeshWorkerResult&&) { ++drainedCount; }, 16);
    }

    EXPECT_EQ(drainedCount, expectedCount);

    fx.pool.shutdown();
}
