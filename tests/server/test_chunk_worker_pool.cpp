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

#include "common/util/thread/ServerWorkerPool.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/chunk/ChunkStatus.hpp"
#include "server/world/ChunkGenerateTask.hpp"
#include <atomic>
#include <chrono>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::server;
using namespace mc::util;

// ============================================================================
// ChunkGenerateTask 测试固件
// ============================================================================

class ChunkGenerateTaskTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // ChunkStatus 是静态初始化的，无需手动初始化
    }

    void TearDown() override {}
};

// ============================================================================
// ChunkGenerateTask 测试
// ============================================================================

TEST_F(ChunkGenerateTaskTest, BasicConstruction)
{
    ChunkGenerateTask::GeneratorFunc generator =
        [](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
            (void)cancelSignal;
            chunk.setChunkStatus(targetStatus);
        };

    ChunkGenerateTask task(10, 20, ChunkStatuses::FULL, generator);

    EXPECT_EQ(task.x(), 10);
    EXPECT_EQ(task.z(), 20);
    EXPECT_EQ(task.type(), TaskType::ChunkGenerate);
    EXPECT_FALSE(task.description().empty());
}

TEST_F(ChunkGenerateTaskTest, ExecuteSuccess)
{
    ChunkGenerateTask::GeneratorFunc generator =
        [](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
            (void)cancelSignal;
            chunk.setChunkStatus(targetStatus);
        };

    auto task = std::make_unique<ChunkGenerateTask>(5, 7, ChunkStatuses::FULL, generator);

    std::atomic<bool> cancelSignal{false};
    bool result = task->execute(cancelSignal);

    EXPECT_TRUE(result);

    // takeResult() 只能调用一次，移动后结果为空
    auto resultChunk = task->takeResult();
    ASSERT_NE(resultChunk, nullptr);
    EXPECT_EQ(resultChunk->x(), 5);
    EXPECT_EQ(resultChunk->z(), 7);

    // 再次调用返回 nullptr
    EXPECT_EQ(task->takeResult(), nullptr);
}

TEST_F(ChunkGenerateTaskTest, ExecuteCancelled)
{
    ChunkGenerateTask::GeneratorFunc generator =
        [](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
            if (cancelSignal.load(std::memory_order_acquire)) {
                return;
            }
            chunk.setChunkStatus(targetStatus);
        };

    auto task = std::make_unique<ChunkGenerateTask>(1, 2, ChunkStatuses::FULL, generator);

    std::atomic<bool> cancelSignal{true};
    bool result = task->execute(cancelSignal);

    EXPECT_FALSE(result);
    EXPECT_EQ(task->takeResult(), nullptr);
}

TEST_F(ChunkGenerateTaskTest, ExecuteException)
{
    ChunkGenerateTask::GeneratorFunc generator =
        [](ChunkPrimer& chunk, const ChunkStatus&, const std::atomic<bool>&) -> void {
        (void)chunk;
        throw std::runtime_error("Test exception");
    };

    auto task = std::make_unique<ChunkGenerateTask>(0, 0, ChunkStatuses::FULL, generator);

    std::atomic<bool> cancelSignal{false};
    // execute 内部捕获异常并返回 false
    bool result = task->execute(cancelSignal);

    EXPECT_FALSE(result);
    EXPECT_EQ(task->takeResult(), nullptr);
}

TEST_F(ChunkGenerateTaskTest, OnCancel)
{
    ChunkGenerateTask::GeneratorFunc generator =
        [](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
            (void)cancelSignal;
            chunk.setChunkStatus(targetStatus);
        };

    auto task = std::make_unique<ChunkGenerateTask>(3, 4, ChunkStatuses::FULL, generator);

    // onCancel 不应该抛出异常
    EXPECT_NO_THROW(task->onCancel());
}

// ============================================================================
// 与 ServerWorkerPool 集成测试
// ============================================================================

class ChunkGenerateTaskIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ChunkGenerateTaskIntegrationTest, SubmitToPool)
{
    ServerWorkerPool pool(2, "ChunkGenTest");
    pool.start();

    std::atomic<bool> completed{false};
    std::atomic<bool> success{false};
    ChunkGenerateTask* taskPtr = nullptr;

    ChunkGenerateTask::GeneratorFunc generator =
        [](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
            (void)cancelSignal;
            chunk.setChunkStatus(targetStatus);
        };

    auto task = std::make_unique<ChunkGenerateTask>(10, 20, ChunkStatuses::FULL, generator);
    taskPtr = task.get();

    pool.submit(
        std::move(task),
        [&](bool s, ITask* t) {
            completed = true;
            success = s;
            // 验证任务指针
            EXPECT_EQ(t, taskPtr);
        },
        TaskPriority::Normal);

    // 等待完成
    for (int i = 0; i < 100 && !completed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(completed);
    EXPECT_TRUE(success);

    // 获取结果
    auto result = taskPtr->takeResult();
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->x(), 10);
    EXPECT_EQ(result->z(), 20);

    pool.shutdown();
}

TEST_F(ChunkGenerateTaskIntegrationTest, MultipleChunkGenerations)
{
    ServerWorkerPool pool(4, "ChunkGenTest");
    pool.start();

    std::atomic<int> completedCount{0};
    const int numChunks = 10;

    ChunkGenerateTask::GeneratorFunc generator =
        [](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
            (void)cancelSignal;
            chunk.setChunkStatus(targetStatus);
        };

    for (int i = 0; i < numChunks; ++i) {
        auto task = std::make_unique<ChunkGenerateTask>(i, i * 2, ChunkStatuses::FULL, generator);
        pool.submit(std::move(task), [&completedCount](bool, ITask*) { completedCount++; }, TaskPriority::Normal);
    }

    // 等待所有完成
    for (int i = 0; i < 200 && completedCount < numChunks; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(completedCount, numChunks);

    pool.shutdown();
}

TEST_F(ChunkGenerateTaskIntegrationTest, PriorityOrdering)
{
    ServerWorkerPool pool(1, "ChunkGenTest"); // 单线程确保顺序执行
    pool.start();

    std::vector<int> executionOrder;
    std::mutex orderMutex;
    std::atomic<bool> firstStarted{false};

    ChunkGenerateTask::GeneratorFunc generator = [&executionOrder, &orderMutex, &firstStarted](ChunkPrimer& chunk,
                                                     const ChunkStatus& targetStatus,
                                                     const std::atomic<bool>& cancelSignal) {
        (void)cancelSignal;
        firstStarted = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        {
            std::lock_guard<std::mutex> lock(orderMutex);
            executionOrder.push_back(chunk.x());
        }
        chunk.setChunkStatus(targetStatus);
    };

    // 提交第一个任务（等待它开始）
    pool.submit(
        std::make_unique<ChunkGenerateTask>(0, 0, ChunkStatuses::FULL, generator), nullptr, TaskPriority::Normal);

    // 等待第一个任务开始
    for (int i = 0; i < 100 && !firstStarted; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 提交三个任务，优先级不同
    pool.submit(std::make_unique<ChunkGenerateTask>(3, 0, ChunkStatuses::FULL, generator), nullptr, TaskPriority::Low);
    pool.submit(std::make_unique<ChunkGenerateTask>(1, 0, ChunkStatuses::FULL, generator), nullptr, TaskPriority::High);
    pool.submit(
        std::make_unique<ChunkGenerateTask>(2, 0, ChunkStatuses::FULL, generator), nullptr, TaskPriority::Normal);

    pool.waitForCompletion();

    // 高优先级任务应该先执行
    ASSERT_GE(executionOrder.size(), 4);
    EXPECT_EQ(executionOrder[1], 1); // 高优先级第二个执行（第一个是初始任务）
}

TEST_F(ChunkGenerateTaskIntegrationTest, CancelToken)
{
    ServerWorkerPool pool(1, "ChunkGenTest");
    pool.start();

    std::atomic<bool> completed{false};
    std::atomic<bool> success{true};

    ChunkGenerateTask::GeneratorFunc generator =
        [](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
            if (cancelSignal.load(std::memory_order_acquire)) {
                return;
            }
            chunk.setChunkStatus(targetStatus);
        };

    auto cancelToken = std::make_shared<std::atomic<bool>>(true);

    auto task = std::make_unique<ChunkGenerateTask>(5, 5, ChunkStatuses::FULL, generator);
    pool.submit(
        std::move(task),
        [&](bool s, ITask*) {
            completed = true;
            success = s;
        },
        TaskPriority::Normal,
        cancelToken);

    for (int i = 0; i < 100 && !completed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(completed);
    EXPECT_FALSE(success); // 应该因取消而失败

    pool.shutdown();
}
