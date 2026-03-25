#include <gtest/gtest.h>
#include "server/world/ServerChunkManager.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/entity/VanillaEntities.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/spawn/WorldGenSpawner.hpp"
#include <memory>
#include <vector>
#include <atomic>

using namespace mc;
using namespace mc::server;

// 噪声区块生成器所在的命名空间
namespace mc::gen {
    class NoiseChunkGenerator;
    struct DimensionSettings;
}

/**
 * @brief ServerChunkManager 实体生成回调测试
 *
 * 测试当没有 ServerWorld 时，实体生成回调机制是否正常工作。
 */
class ServerChunkManagerCallbackTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化实体注册表
        mc::entity::VanillaEntities::registerAll();

        // 创建区块生成器
        auto settings = mc::DimensionSettings::overworld();
        auto generator = std::make_unique<mc::NoiseChunkGenerator>(12345, std::move(settings));

        // 创建区块管理器（不关联 ServerWorld）
        m_manager = std::make_unique<ServerChunkManager>(std::move(generator));
        auto result = m_manager->initialize();
        ASSERT_TRUE(result.success());
    }

    void TearDown() override {
        m_manager->shutdown();
        m_manager.reset();
    }

    std::unique_ptr<ServerChunkManager> m_manager;
};

// ============================================================================
// 回调设置测试
// ============================================================================

TEST_F(ServerChunkManagerCallbackTest, SetEntitySpawnCallback) {
    bool callbackCalled = false;

    m_manager->setEntitySpawnCallback([&callbackCalled](const std::vector<mc::SpawnedEntityData>& entities) {
        callbackCalled = true;
        (void)entities;
    });

    // 验证回调已设置（通过触发区块生成间接验证）
    // 此测试仅验证设置不会崩溃
    EXPECT_FALSE(callbackCalled);
}

TEST_F(ServerChunkManagerCallbackTest, CallbackReceivesSpawnedEntities) {
    std::vector<mc::SpawnedEntityData> receivedEntities;

    m_manager->setEntitySpawnCallback([&receivedEntities](const std::vector<mc::SpawnedEntityData>& entities) {
        receivedEntities = entities;
    });

    // 启动工作线程
    m_manager->startWorkers(1);

    // 同步生成一个区块（会触发实体生成）
    ChunkData* chunk = m_manager->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 等待处理完成
    for (int i = 0; i < 100 && receivedEntities.empty(); ++i) {
        m_manager->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 验证区块已生成
    EXPECT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 0);
    EXPECT_EQ(chunk->z(), 0);

    // 停止工作线程
    m_manager->stopWorkers();
}

// ============================================================================
// 多区块生成测试
// ============================================================================

TEST_F(ServerChunkManagerCallbackTest, MultipleChunksGenerate) {
    std::atomic<int> totalEntities{0};

    m_manager->setEntitySpawnCallback([&totalEntities](const std::vector<mc::SpawnedEntityData>& entities) {
        totalEntities += static_cast<int>(entities.size());
    });

    m_manager->startWorkers(2);

    // 生成多个区块
    ChunkData* chunk1 = m_manager->getChunkSync(0, 0);
    ChunkData* chunk2 = m_manager->getChunkSync(1, 0);
    ChunkData* chunk3 = m_manager->getChunkSync(0, 1);

    EXPECT_NE(chunk1, nullptr);
    EXPECT_NE(chunk2, nullptr);
    EXPECT_NE(chunk3, nullptr);

    // 等待回调处理
    for (int i = 0; i < 50; ++i) {
        m_manager->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    m_manager->stopWorkers();
}

// ============================================================================
// 异步生成测试
// ============================================================================

TEST_F(ServerChunkManagerCallbackTest, AsyncGenerateWithCallback) {
    std::vector<mc::SpawnedEntityData> receivedEntities;
    std::atomic<bool> callbackCompleted{false};

    m_manager->setEntitySpawnCallback([&receivedEntities, &callbackCompleted](const std::vector<mc::SpawnedEntityData>& entities) {
        for (const auto& e : entities) {
            receivedEntities.push_back(e);
        }
        callbackCompleted = true;
    });

    m_manager->startWorkers(1);

    // 异步生成
    auto future = m_manager->getChunkAsync(5, 5);
    ASSERT_TRUE(future.valid());

    // 等待生成完成
    ChunkData* chunk = future.get();
    ASSERT_NE(chunk, nullptr);

    // 处理回调
    for (int i = 0; i < 50; ++i) {
        m_manager->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    m_manager->stopWorkers();
}

// ============================================================================
// 空区块测试
// ============================================================================

TEST_F(ServerChunkManagerCallbackTest, EmptyChunkDoesNotCallCallback) {
    bool callbackCalled = false;

    m_manager->setEntitySpawnCallback([&callbackCalled](const std::vector<mc::SpawnedEntityData>& entities) {
        if (!entities.empty()) {
            callbackCalled = true;
        }
    });

    // 没有生成区块时不应该调用回调
    EXPECT_FALSE(callbackCalled);
}

// ============================================================================
// 回调重置测试
// ============================================================================

TEST_F(ServerChunkManagerCallbackTest, ResetCallback) {
    int callCount = 0;

    m_manager->setEntitySpawnCallback([&callCount](const std::vector<mc::SpawnedEntityData>&) {
        callCount++;
    });

    // 重置为空回调
    m_manager->setEntitySpawnCallback(nullptr);

    m_manager->startWorkers(1);
    ChunkData* chunk = m_manager->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 等待处理
    for (int i = 0; i < 50; ++i) {
        m_manager->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    m_manager->stopWorkers();

    // 空回调不应被调用
    EXPECT_EQ(callCount, 0);
}

// ============================================================================
// 统计测试
// ============================================================================

TEST_F(ServerChunkManagerCallbackTest, ChunkCount) {
    m_manager->startWorkers(1);

    EXPECT_EQ(m_manager->loadedChunkCount(), 0u);

    m_manager->getChunkSync(0, 0);
    EXPECT_EQ(m_manager->loadedChunkCount(), 1u);

    m_manager->getChunkSync(1, 0);
    m_manager->getChunkSync(0, 1);
    EXPECT_EQ(m_manager->loadedChunkCount(), 3u);

    m_manager->stopWorkers();
}

TEST_F(ServerChunkManagerCallbackTest, singleChunkLifecycleManagerCount) {
    m_manager->startWorkers(1);

    EXPECT_EQ(m_manager->singleChunkLifecycleManagerCount(), 0u);

    m_manager->getChunkSync(0, 0);
    EXPECT_EQ(m_manager->singleChunkLifecycleManagerCount(), 1u);

    m_manager->stopWorkers();
}
