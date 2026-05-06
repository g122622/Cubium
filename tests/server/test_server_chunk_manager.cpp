#include <gtest/gtest.h>
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/util/thread/ServerWorkerPool.hpp"
#include <thread>
#include <chrono>

using namespace mc;
using namespace mc::server;

// ============================================================================
// ServerChunkManager 测试固件
// ============================================================================

class ServerChunkManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化方块注册表
        VanillaBlocks::initialize();

        // 创建工作线程池
        m_workerPool = std::make_unique<mc::util::ServerWorkerPool>(2, "TestWorker");

        // 创建测试用的 ServerWorld
        ServerWorldConfig config;
        config.seed = 12345;
        config.viewDistance = 8;
        m_world = std::make_unique<ServerWorld>(config);

        // 创建区块管理器
        auto generator = std::make_unique<NoiseChunkGenerator>(
            config.seed,
            DimensionSettings::overworld()
        );
        m_manager = std::make_unique<ServerChunkManager>(*m_world, std::move(generator));
        m_manager->setWorkerPool(m_workerPool.get());
    }

    void TearDown() override {
        m_manager.reset();
        m_workerPool.reset();
        m_world.reset();
    }

    std::unique_ptr<mc::util::ServerWorkerPool> m_workerPool;
    std::unique_ptr<ServerWorld> m_world;
    std::unique_ptr<ServerChunkManager> m_manager;
};

// ============================================================================
// 构造和生命周期测试
// ============================================================================

TEST_F(ServerChunkManagerTest, Constructor) {
    EXPECT_EQ(m_manager->loadedChunkCount(), 0);
    EXPECT_EQ(m_manager->singleChunkLifecycleManagerCount(), 0);
    EXPECT_FALSE(m_workerPool->isRunning());
}

TEST_F(ServerChunkManagerTest, Initialize) {
    m_workerPool->start();
    auto result = m_manager->initialize();
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(m_workerPool->isRunning());
    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerTest, Shutdown) {
    m_workerPool->start();
    m_manager->initialize();
    EXPECT_TRUE(m_workerPool->isRunning());

    m_manager->shutdown();
    m_workerPool->shutdown();
    EXPECT_FALSE(m_workerPool->isRunning());
    EXPECT_EQ(m_manager->loadedChunkCount(), 0);
}

// ============================================================================
// 同步区块访问测试
// ============================================================================

TEST_F(ServerChunkManagerTest, GetChunk_NotExists) {
    ChunkData* chunk = m_manager->getChunk(0, 0);
    EXPECT_EQ(chunk, nullptr);
}

TEST_F(ServerChunkManagerTest, HasChunk_NotExists) {
    EXPECT_FALSE(m_manager->hasChunk(0, 0));
    EXPECT_FALSE(m_manager->hasChunk(100, 100));
}

TEST_F(ServerChunkManagerTest, GetChunkSync_CreatesChunk) {
    ChunkData* chunk = m_manager->getChunkSync(0, 0);

    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 0);
    EXPECT_EQ(chunk->z(), 0);
    EXPECT_TRUE(m_manager->hasChunk(0, 0));
}

TEST_F(ServerChunkManagerTest, GetChunkSync_ReturnsSameChunk) {
    ChunkData* chunk1 = m_manager->getChunkSync(5, 10);
    ChunkData* chunk2 = m_manager->getChunkSync(5, 10);

    EXPECT_EQ(chunk1, chunk2);
}

TEST_F(ServerChunkManagerTest, GetChunkSync_AfterGeneration) {
    m_manager->getChunkSync(3, 7);

    ChunkData* chunk = m_manager->getChunk(3, 7);
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 3);
    EXPECT_EQ(chunk->z(), 7);
}

TEST_F(ServerChunkManagerTest, GetChunkSync_MultipleChunks) {
    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            ChunkData* chunk = m_manager->getChunkSync(x, z);
            ASSERT_NE(chunk, nullptr) << "Failed to generate chunk (" << x << ", " << z << ")";
        }
    }

    EXPECT_EQ(m_manager->loadedChunkCount(), 25);

    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            EXPECT_TRUE(m_manager->hasChunk(x, z));
        }
    }
}

// ============================================================================
// 异步区块访问测试
// ============================================================================

TEST_F(ServerChunkManagerTest, GetChunkAsync_NotInitialized) {
    // 未初始化 Worker 时，异步生成应该失败或立即返回
    auto future = m_manager->getChunkAsync(0, 0, &ChunkStatuses::FULL);

    // 等待结果
    auto status = future.wait_for(std::chrono::seconds(5));
    EXPECT_NE(status, std::future_status::timeout);

    ChunkData* chunk = future.get();
    // 未启动 Worker 时可能返回 nullptr
    // 这是预期的行为
}

TEST_F(ServerChunkManagerTest, GetChunkAsync_AfterInit) {
    m_workerPool->start();
    m_manager->initialize();

    auto future = m_manager->getChunkAsync(0, 0, &ChunkStatuses::FULL);

    // 等待完成
    auto status = future.wait_for(std::chrono::seconds(10));
    EXPECT_NE(status, std::future_status::timeout);

    ChunkData* chunk = future.get();
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 0);
    EXPECT_EQ(chunk->z(), 0);
    EXPECT_TRUE(m_manager->hasChunk(0, 0));

    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerTest, GetChunkAsync_Callback) {
    m_workerPool->start();
    m_manager->initialize();

    std::atomic<bool> completed{false};
    ChunkData* resultChunk = nullptr;

    m_manager->getChunkAsync(5, 5,
        [&](bool success, ChunkData* chunk) {
            completed = true;
            resultChunk = chunk;
        },
        &ChunkStatuses::FULL);

    // 等待完成
    for (int i = 0; i < 200 && !completed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    EXPECT_TRUE(completed);
    EXPECT_NE(resultChunk, nullptr);

    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerTest, GetChunkAsync_AlreadyCached) {
    m_workerPool->start();
    m_manager->initialize();

    // 先同步生成
    ChunkData* syncChunk = m_manager->getChunkSync(10, 10);
    ASSERT_NE(syncChunk, nullptr);

    // 异步获取应该立即返回缓存
    auto future = m_manager->getChunkAsync(10, 10, &ChunkStatuses::FULL);

    auto status = future.wait_for(std::chrono::milliseconds(100));
    EXPECT_NE(status, std::future_status::timeout);

    ChunkData* asyncChunk = future.get();
    EXPECT_EQ(syncChunk, asyncChunk);  // 应该是同一个实例

    m_manager->shutdown();
    m_workerPool->shutdown();
}

// ============================================================================
// 区块卸载测试
// ============================================================================

TEST_F(ServerChunkManagerTest, UnloadChunk) {
    m_manager->getChunkSync(0, 0);
    EXPECT_TRUE(m_manager->hasChunk(0, 0));

    m_manager->unloadChunk(0, 0);
    EXPECT_FALSE(m_manager->hasChunk(0, 0));
}

TEST_F(ServerChunkManagerTest, UnloadChunk_WithHolder) {
    m_manager->getOrCreateSingleChunkLifecycleManager(5, 5);
    m_manager->getChunkSync(5, 5);

    EXPECT_TRUE(m_manager->hasChunk(5, 5));
    EXPECT_EQ(m_manager->singleChunkLifecycleManagerCount(), 1);

    m_manager->unloadChunk(5, 5);
    EXPECT_FALSE(m_manager->hasChunk(5, 5));
    // 持有者可能仍然存在，但区块数据已卸载
}

// ============================================================================
// 区块持有者测试
// ============================================================================

TEST_F(ServerChunkManagerTest, GetOrCreateHolder) {
    SingleChunkLifecycleManager* holder = m_manager->getOrCreateSingleChunkLifecycleManager(10, 20);
    ASSERT_NE(holder, nullptr);
    EXPECT_EQ(holder->x(), 10);
    EXPECT_EQ(holder->z(), 20);
    EXPECT_EQ(m_manager->singleChunkLifecycleManagerCount(), 1);
}

TEST_F(ServerChunkManagerTest, GetOrCreateHolder_SameChunk) {
    SingleChunkLifecycleManager* holder1 = m_manager->getOrCreateSingleChunkLifecycleManager(5, 5);
    SingleChunkLifecycleManager* holder2 = m_manager->getOrCreateSingleChunkLifecycleManager(5, 5);

    EXPECT_EQ(holder1, holder2);
    EXPECT_EQ(m_manager->singleChunkLifecycleManagerCount(), 1);
}

TEST_F(ServerChunkManagerTest, GetHolder) {
    m_manager->getOrCreateSingleChunkLifecycleManager(3, 7);

    SingleChunkLifecycleManager* holder = m_manager->getSingleChunkLifecycleManager(3, 7);
    ASSERT_NE(holder, nullptr);
    EXPECT_EQ(holder->x(), 3);
    EXPECT_EQ(holder->z(), 7);

    SingleChunkLifecycleManager* nullHolder = m_manager->getSingleChunkLifecycleManager(100, 100);
    EXPECT_EQ(nullHolder, nullptr);
}

// ============================================================================
// 票据管理测试
// ============================================================================

TEST_F(ServerChunkManagerTest, UpdatePlayerPosition) {
    m_workerPool->start();
    m_manager->initialize();

    m_manager->updatePlayerPosition(1, 0.0, 0.0);

    // 应该创建区块持有者
    EXPECT_GE(m_manager->singleChunkLifecycleManagerCount(), 1);

    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerTest, RemovePlayer) {
    m_workerPool->start();
    m_manager->initialize();

    m_manager->updatePlayerPosition(1, 0.0, 0.0);
    EXPECT_GE(m_manager->singleChunkLifecycleManagerCount(), 1);

    m_manager->removePlayer(1);

    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerTest, SetViewDistance) {
    m_manager->setViewDistance(8);
    EXPECT_EQ(m_manager->viewDistance(), 8);

    m_manager->setViewDistance(16);
    EXPECT_EQ(m_manager->viewDistance(), 16);
}

// ============================================================================
// Tick 测试
// ============================================================================

TEST_F(ServerChunkManagerTest, Tick) {
    m_workerPool->start();
    m_manager->initialize();

    // 多次 tick 不应崩溃
    for (int i = 0; i < 100; ++i) {
        m_manager->tick();
    }

    m_manager->shutdown();
    m_workerPool->shutdown();
}

// ============================================================================
// 统计测试
// ============================================================================

TEST_F(ServerChunkManagerTest, LoadedChunkCount) {
    EXPECT_EQ(m_manager->loadedChunkCount(), 0);

    m_manager->getChunkSync(0, 0);
    EXPECT_EQ(m_manager->loadedChunkCount(), 1);

    m_manager->getChunkSync(1, 0);
    m_manager->getChunkSync(0, 1);
    EXPECT_EQ(m_manager->loadedChunkCount(), 3);
}

TEST_F(ServerChunkManagerTest, PendingTaskCount) {
    m_workerPool->start();
    m_manager->initialize();

    // 没有待处理任务
    EXPECT_EQ(m_manager->pendingTaskCount(), 0);

    m_manager->shutdown();
    m_workerPool->shutdown();
}

// ============================================================================
// 生成器测试
// ============================================================================

TEST_F(ServerChunkManagerTest, GeneratorNotNull) {
    EXPECT_NE(m_manager->generator(), nullptr);
}

TEST_F(ServerChunkManagerTest, GeneratedChunkHasBlocks) {
    m_workerPool->start();
    m_manager->initialize();

    ChunkData* chunk = m_manager->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 检查区块是否有一些非空气方块
    bool hasNonAirBlocks = false;
    for (int y = 0; y < world::CHUNK_HEIGHT && !hasNonAirBlocks; ++y) {
        for (int z = 0; z < 16 && !hasNonAirBlocks; ++z) {
            for (int x = 0; x < 16; ++x) {
                const BlockState* state = chunk->getBlockState(x, y, z);
                if (state && !state->isAir()) {
                    hasNonAirBlocks = true;
                    break;
                }
            }
        }
    }

    EXPECT_TRUE(hasNonAirBlocks) << "Generated chunk should have non-air blocks";

    m_manager->shutdown();
    m_workerPool->shutdown();
}

// ============================================================================
// 线程安全测试
// ============================================================================

TEST_F(ServerChunkManagerTest, ConcurrentChunkAccess) {
    m_workerPool->start();
    m_manager->initialize();

    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};

    // 多线程同时访问区块
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, &successCount, i]() {
            for (int j = 0; j < 10; ++j) {
                int x = (i * 10 + j) % 20 - 10;
                int z = (i * 10 + j + 50) % 20 - 10;
                ChunkData* chunk = m_manager->getChunkSync(x, z);
                if (chunk) {
                    successCount++;
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(successCount, 40);
    EXPECT_GT(m_manager->loadedChunkCount(), 0);

    m_manager->shutdown();
    m_workerPool->shutdown();
}
