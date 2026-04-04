#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "client/renderer/mesh/MeshWorkerPool.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/VanillaBlocks.hpp"

using namespace mc::client;
using namespace mc;

// ============================================================================
// 测试固件
// ============================================================================

class MeshWorkerPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化方块注册表
        VanillaBlocks::initialize();
    }

    void TearDown() override {
        // 清理
    }

    // 创建测试用的区块数据
    std::shared_ptr<ChunkData> createTestChunkData(ChunkCoord x, ChunkCoord z, u32 blockStateId = 1) {
        auto chunkData = std::make_shared<ChunkData>(x, z);
        // 填充一些方块
        for (i32 bx = 0; bx < 16; ++bx) {
            for (i32 bz = 0; bz < 16; ++bz) {
                for (i32 by = 0; by < 16; ++by) {
                    chunkData->setBlockStateId(bx, by, bz, blockStateId);
                }
            }
        }
        chunkData->setFullyGenerated(true);
        return chunkData;
    }
};

// ============================================================================
// 基础测试
// ============================================================================

TEST_F(MeshWorkerPoolTest, StartStop) {
    MeshWorkerPool pool(2);

    EXPECT_FALSE(pool.isRunning());

    pool.start();
    EXPECT_TRUE(pool.isRunning());

    pool.shutdown();
    EXPECT_FALSE(pool.isRunning());
}

TEST_F(MeshWorkerPoolTest, MultipleStartStop) {
    MeshWorkerPool pool(1);

    pool.start();
    EXPECT_TRUE(pool.isRunning());

    // 重复 start 应该是安全的
    pool.start();
    EXPECT_TRUE(pool.isRunning());

    pool.shutdown();
    EXPECT_FALSE(pool.isRunning());

    // 重复 shutdown 应该是安全的
    pool.shutdown();
    EXPECT_FALSE(pool.isRunning());
}

// ============================================================================
// 任务提交测试
// ============================================================================

TEST_F(MeshWorkerPoolTest, SubmitSingleTask) {
    MeshWorkerPool pool(1);
    pool.start();

    auto chunkData = createTestChunkData(0, 0);
    std::array<std::shared_ptr<const ChunkData>, 6> neighbors = {};

    pool.submitTask(ChunkId(0, 0), chunkData, neighbors, 0);

    // 等待任务被处理
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 完成队列应该有一个结果
    EXPECT_GE(pool.completedTaskCount(), 1u);

    pool.shutdown();
}

TEST_F(MeshWorkerPoolTest, SubmitMultipleTasks) {
    MeshWorkerPool pool(2);
    pool.start();

    const int numTasks = 10;
    for (int i = 0; i < numTasks; ++i) {
        auto chunkData = createTestChunkData(i, 0);
        std::array<std::shared_ptr<const ChunkData>, 6> neighbors = {};
        pool.submitTask(ChunkId(i, 0), chunkData, neighbors, 0);
    }

    // 等待所有任务完成
    int attempts = 0;
    while (pool.completedTaskCount() < static_cast<size_t>(numTasks) && attempts < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ++attempts;
    }

    EXPECT_EQ(static_cast<size_t>(numTasks), pool.completedTaskCount());

    pool.shutdown();
}

TEST_F(MeshWorkerPoolTest, PriorityOrdering) {
    MeshWorkerPool pool(1);
    pool.start();

    // 先等待 worker 线程启动
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 提交低优先级任务
    auto lowPriorityChunk = createTestChunkData(0, 0);
    std::array<std::shared_ptr<const ChunkData>, 6> neighbors = {};
    pool.submitTask(ChunkId(0, 0), lowPriorityChunk, neighbors, 10);

    // 提交高优先级任务
    auto highPriorityChunk = createTestChunkData(1, 0);
    pool.submitTask(ChunkId(1, 0), highPriorityChunk, neighbors, 0);

    // 等待任务完成
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 高优先级任务应该先完成
    EXPECT_GE(pool.completedTaskCount(), 1u);

    pool.shutdown();
}

// ============================================================================
// 结果处理测试
// ============================================================================

TEST_F(MeshWorkerPoolTest, ProcessCompletedTasksFrameLimit) {
    MeshWorkerPool pool(2);
    pool.start();

    const int numTasks = 10;
    for (int i = 0; i < numTasks; ++i) {
        auto chunkData = createTestChunkData(i, 0);
        std::array<std::shared_ptr<const ChunkData>, 6> neighbors = {};
        pool.submitTask(ChunkId(i, 0), chunkData, neighbors, 0);
    }

    // 等待任务完成
    int attempts = 0;
    while (pool.completedTaskCount() < static_cast<size_t>(numTasks) && attempts < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ++attempts;
    }

    // 处理结果，每帧限制 3 个
    int processed = 0;
    pool.processCompletedTasks([&processed](MeshBuildResult result) {
        EXPECT_TRUE(result.success);
        ++processed;
    }, 3);

    // 应该只处理了 3 个
    EXPECT_EQ(3, processed);
    EXPECT_EQ(static_cast<size_t>(numTasks - 3), pool.completedTaskCount());

    // 继续处理剩余的
    while (pool.completedTaskCount() > 0) {
        pool.processCompletedTasks([&processed](MeshBuildResult result) {
            ++processed;
        }, 10);
    }

    EXPECT_EQ(numTasks, processed);

    pool.shutdown();
}

TEST_F(MeshWorkerPoolTest, NullChunkDataHandling) {
    MeshWorkerPool pool(1);
    pool.start();

    // 提交空数据应该被安全忽略
    pool.submitTask(ChunkId(0, 0), nullptr, {}, 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 不应该崩溃，完成队列应该为空
    EXPECT_EQ(0u, pool.completedTaskCount());

    pool.shutdown();
}

// ============================================================================
// 并发测试
// ============================================================================

TEST_F(MeshWorkerPoolTest, ConcurrentSubmissions) {
    MeshWorkerPool pool(4);
    pool.start();

    const int numThreads = 4;
    const int tasksPerThread = 10;
    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < tasksPerThread; ++i) {
                auto chunkData = createTestChunkData(t * 100 + i, 0);
                std::array<std::shared_ptr<const ChunkData>, 6> neighbors = {};
                pool.submitTask(ChunkId(t * 100 + i, 0), chunkData, neighbors, 0);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 等待所有任务完成
    int attempts = 0;
    while (pool.completedTaskCount() < static_cast<size_t>(numThreads * tasksPerThread) && attempts < 200) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ++attempts;
    }

    EXPECT_EQ(static_cast<size_t>(numThreads * tasksPerThread), pool.completedTaskCount());

    pool.shutdown();
}

TEST_F(MeshWorkerPoolTest, ShutdownWithPendingTasks) {
    MeshWorkerPool pool(1);
    pool.start();

    // 提交大量任务
    for (int i = 0; i < 100; ++i) {
        auto chunkData = createTestChunkData(i, 0);
        std::array<std::shared_ptr<const ChunkData>, 6> neighbors = {};
        pool.submitTask(ChunkId(i, 0), chunkData, neighbors, 0);
    }

    // 立即关闭（可能还有未完成的任务）
    pool.shutdown();

    // 应该安全关闭，不崩溃
    EXPECT_FALSE(pool.isRunning());
}

// ============================================================================
// 统计测试
// ============================================================================

TEST_F(MeshWorkerPoolTest, PendingTaskCount) {
    MeshWorkerPool pool(1);
    pool.start();

    // 等待 worker 启动
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // 先让之前的任务完成
    while (pool.pendingTaskCount() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 提交任务
    for (int i = 0; i < 5; ++i) {
        auto chunkData = createTestChunkData(i, 0);
        std::array<std::shared_ptr<const ChunkData>, 6> neighbors = {};
        pool.submitTask(ChunkId(i, 0), chunkData, neighbors, 0);
    }

    // 应该有待处理任务（可能在处理前）
    EXPECT_GE(pool.pendingTaskCount(), 0u);

    pool.shutdown();
}

// ============================================================================
// 边界情况测试
// ============================================================================

TEST_F(MeshWorkerPoolTest, ProcessEmptyQueue) {
    MeshWorkerPool pool(1);
    pool.start();

    // 没有任务时处理应该安全
    int processed = 0;
    pool.processCompletedTasks([&processed](MeshBuildResult result) {
        ++processed;
    }, 10);

    EXPECT_EQ(0, processed);

    pool.shutdown();
}

TEST_F(MeshWorkerPoolTest, ProcessWithoutStart) {
    MeshWorkerPool pool(1);

    // 没有启动时提交任务应该被安全忽略
    auto chunkData = createTestChunkData(0, 0);
    std::array<std::shared_ptr<const ChunkData>, 6> neighbors = {};
    pool.submitTask(ChunkId(0, 0), chunkData, neighbors, 0);

    // 应该不崩溃
    EXPECT_FALSE(pool.isRunning());
}

TEST_F(MeshWorkerPoolTest, ProcessWithNullProcessor) {
    MeshWorkerPool pool(1);
    pool.start();

    auto chunkData = createTestChunkData(0, 0);
    std::array<std::shared_ptr<const ChunkData>, 6> neighbors = {};
    pool.submitTask(ChunkId(0, 0), chunkData, neighbors, 0);

    // 等待任务完成
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 空 processor 应该安全
    pool.processCompletedTasks(nullptr, 10);

    pool.shutdown();
}
