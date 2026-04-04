#include <gtest/gtest.h>
#include "server/world/ChunkWorkerPool.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/chunk/ChunkStatus.hpp"
#include <atomic>
#include <chrono>

using namespace mc;
using namespace mc::server;

// ============================================================================
// ChunkWorkerPool 测试固件
// ============================================================================

class ChunkWorkerPoolTest : public ::testing::Test {
protected:
    void SetUp() override {
        // ChunkStatus 是静态初始化的，无需手动初始化
    }

    void TearDown() override {
    }
};

// ============================================================================
// 构造和生命周期测试
// ============================================================================

TEST_F(ChunkWorkerPoolTest, DefaultConstructor) {
    ChunkWorkerPool pool;
    EXPECT_FALSE(pool.isRunning());
    EXPECT_GT(pool.threadCount(), 0);  // 自动检测线程数
}

TEST_F(ChunkWorkerPoolTest, StartStop) {
    ChunkWorkerPool pool(2);

    EXPECT_FALSE(pool.isRunning());

    pool.start();
    EXPECT_TRUE(pool.isRunning());

    pool.shutdown();
    EXPECT_FALSE(pool.isRunning());
}

TEST_F(ChunkWorkerPoolTest, DoubleStart) {
    ChunkWorkerPool pool(2);

    pool.start();
    EXPECT_TRUE(pool.isRunning());

    // 重复启动应该无效
    pool.start();
    EXPECT_TRUE(pool.isRunning());

    pool.shutdown();
}

TEST_F(ChunkWorkerPoolTest, DoubleShutdown) {
    ChunkWorkerPool pool(2);

    pool.start();
    pool.shutdown();
    EXPECT_FALSE(pool.isRunning());

    // 重复关闭应该安全
    pool.shutdown();
    EXPECT_FALSE(pool.isRunning());
}

// ============================================================================
// 任务提交测试
// ============================================================================

TEST_F(ChunkWorkerPoolTest, SubmitGenerateBasic) {
    ChunkWorkerPool pool(2);
    pool.setGenerator([](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
        (void)cancelSignal;
        // 简单的生成器，标记为完成
        chunk.setChunkStatus(ChunkStatuses::FULL);
    });
    pool.start();

    std::atomic<bool> completed{false};
    ChunkPrimer* resultChunk = nullptr;

    pool.submitGenerate(0, 0, ChunkStatuses::FULL,
        [&](bool success, ChunkPrimer* chunk) {
            completed = true;
            resultChunk = chunk;
        },
        0);

    // 等待完成
    for (int i = 0; i < 100 && !completed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(completed);
    EXPECT_NE(resultChunk, nullptr);
    if (resultChunk) {
        EXPECT_EQ(resultChunk->x(), 0);
        EXPECT_EQ(resultChunk->z(), 0);
    }

    pool.shutdown();
}

TEST_F(ChunkWorkerPoolTest, SubmitGenerateMultiple) {
    ChunkWorkerPool pool(4);
    pool.setGenerator([](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
        (void)cancelSignal;
        chunk.setChunkStatus(ChunkStatuses::FULL);
    });
    pool.start();

    std::atomic<int> completedCount{0};
    const int numChunks = 10;

    for (int i = 0; i < numChunks; ++i) {
        pool.submitGenerate(i, i, ChunkStatuses::FULL,
            [&](bool success, ChunkPrimer* chunk) {
                completedCount++;
            },
            0);
    }

    // 等待所有完成
    for (int i = 0; i < 200 && completedCount < numChunks; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(completedCount, numChunks);
    EXPECT_EQ(pool.pendingTaskCount(), 0);

    pool.shutdown();
}

TEST_F(ChunkWorkerPoolTest, SubmitGeneratePriority) {
    ChunkWorkerPool pool(1);  // 单线程确保顺序执行
    pool.setGenerator([](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
        (void)cancelSignal;
        // 短暂延迟以便测试优先级
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        chunk.setChunkStatus(ChunkStatuses::FULL);
    });
    pool.start();

    std::vector<int> executionOrder;
    std::mutex orderMutex;

    auto callback = [&](bool success, ChunkPrimer* chunk) {
        std::lock_guard<std::mutex> lock(orderMutex);
        executionOrder.push_back(chunk ? chunk->x() : -1);
    };

    // 提交三个任务，优先级 2, 0, 1（0 最高）
    pool.submitGenerate(1, 0, ChunkStatuses::FULL, callback, 2);  // 低优先级
    pool.submitGenerate(2, 0, ChunkStatuses::FULL, callback, 0);  // 高优先级
    pool.submitGenerate(3, 0, ChunkStatuses::FULL, callback, 1);  // 中优先级

    // 等待完成
    for (int i = 0; i < 100 && executionOrder.size() < 3; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 高优先级任务应该先执行
    EXPECT_EQ(executionOrder.size(), 3);
    if (executionOrder.size() >= 3) {
        EXPECT_EQ(executionOrder[0], 2);  // 优先级 0 最高
    }

    pool.shutdown();
}

TEST_F(ChunkWorkerPoolTest, SubmitGenerateWhenNotRunning) {
    ChunkWorkerPool pool(2);
    // 不启动

    std::atomic<bool> completed{false};
    pool.submitGenerate(0, 0, ChunkStatuses::FULL,
        [&](bool success, ChunkPrimer* chunk) {
            completed = true;
            EXPECT_FALSE(success);  // 应该失败
            EXPECT_EQ(chunk, nullptr);
        });

    // 回调应该立即执行并标记失败
    EXPECT_TRUE(completed);
}

TEST_F(ChunkWorkerPoolTest, SubmitTaskCustom) {
    ChunkWorkerPool pool(2);
    pool.start();

    std::atomic<bool> completed{false};

    ChunkTask task(ChunkTask::Type::Generate, 5, 10, &ChunkStatuses::FULL, 0);

    pool.submitTask(std::move(task),
        [](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
            (void)cancelSignal;
            chunk.setChunkStatus(ChunkStatuses::FULL);
        },
        [&](bool success, ChunkPrimer* chunk) {
            completed = true;
            EXPECT_TRUE(success);
            EXPECT_NE(chunk, nullptr);
            if (chunk) {
                EXPECT_EQ(chunk->x(), 5);
                EXPECT_EQ(chunk->z(), 10);
            }
        });

    for (int i = 0; i < 100 && !completed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(completed);
    pool.shutdown();
}

// ============================================================================
// 异常处理测试
// ============================================================================

TEST_F(ChunkWorkerPoolTest, GeneratorException) {
    ChunkWorkerPool pool(2);
    pool.setGenerator([](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
        (void)cancelSignal;
        throw std::runtime_error("Test exception");
    });
    pool.start();

    std::atomic<bool> completed{false};
    std::atomic<bool> success{true};

    pool.submitGenerate(0, 0, ChunkStatuses::FULL,
        [&](bool s, ChunkPrimer* chunk) {
            completed = true;
            success = s;
        });

    for (int i = 0; i < 100 && !completed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(completed);
    EXPECT_FALSE(success);  // 应该失败

    pool.shutdown();
}

// ============================================================================
// 统计测试
// ============================================================================

TEST_F(ChunkWorkerPoolTest, PendingTaskCount) {
    ChunkWorkerPool pool(1);  // 单线程
    pool.setGenerator([](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
        (void)cancelSignal;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        chunk.setChunkStatus(ChunkStatuses::FULL);
    });
    pool.start();

    // 提交多个任务
    for (int i = 0; i < 5; ++i) {
        pool.submitGenerate(i, 0, ChunkStatuses::FULL, nullptr, 0);
    }

    // 应该有待处理任务
    EXPECT_GT(pool.pendingTaskCount(), 0);

    pool.shutdown();
    EXPECT_EQ(pool.pendingTaskCount(), 0);
}

// ============================================================================
// 线程安全测试
// ============================================================================

TEST_F(ChunkWorkerPoolTest, ConcurrentSubmissions) {
    ChunkWorkerPool pool(4);
    pool.setGenerator([](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
        (void)cancelSignal;
        chunk.setChunkStatus(ChunkStatuses::FULL);
    });
    pool.start();

    std::atomic<int> completedCount{0};
    std::vector<std::thread> threads;

    // 多个线程同时提交任务
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&pool, &completedCount, t]() {
            for (int i = 0; i < 10; ++i) {
                pool.submitGenerate(t * 10 + i, 0, ChunkStatuses::FULL,
                    [&](bool success, ChunkPrimer* chunk) {
                        completedCount++;
                    });
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    // 等待所有完成
    for (int i = 0; i < 200 && completedCount < 40; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_EQ(completedCount, 40);
    pool.shutdown();
}

TEST_F(ChunkWorkerPoolTest, CancelTokenSkipsExecution) {
    ChunkWorkerPool pool(1);
    pool.setGenerator([](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
        if (cancelSignal.load(std::memory_order_acquire)) {
            return;
        }
        chunk.setChunkStatus(targetStatus);
    });
    pool.start();

    auto token = std::make_shared<std::atomic<bool>>(true);
    std::atomic<bool> completed{false};
    std::atomic<bool> success{true};

    pool.submitGenerate(0, 0, ChunkStatuses::FULL,
        [&](bool s, ChunkPrimer* chunk) {
            completed = true;
            success = s;
            EXPECT_EQ(chunk, nullptr);
        },
        token,
        0);

    for (int i = 0; i < 100 && !completed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(completed);
    EXPECT_FALSE(success);

    pool.shutdown();
}

TEST_F(ChunkWorkerPoolTest, PruneCancelledTasksRemovesQueueEntries) {
    ChunkWorkerPool pool(1);
    pool.setGenerator([](ChunkPrimer& chunk, const ChunkStatus& targetStatus, const std::atomic<bool>& cancelSignal) {
        if (cancelSignal.load(std::memory_order_acquire)) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        chunk.setChunkStatus(targetStatus);
    });
    pool.start();

    auto tokenA = std::make_shared<std::atomic<bool>>(false);
    auto tokenB = std::make_shared<std::atomic<bool>>(false);

    pool.submitGenerate(1, 0, ChunkStatuses::FULL, nullptr, tokenA, 0);
    pool.submitGenerate(2, 0, ChunkStatuses::FULL, nullptr, tokenB, 1);

    tokenB->store(true, std::memory_order_release);
    pool.pruneCancelledTasks();

    EXPECT_LE(pool.pendingTaskCount(), 1u);

    pool.shutdown();
}
