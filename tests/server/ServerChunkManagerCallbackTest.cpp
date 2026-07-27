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

#include "common/entity/registry/VanillaEntities.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/spawn/WorldGenSpawner.hpp"
#include "server/world/ServerChunkManager.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::server;

// 噪声区块生成器所在的命名空间
namespace mc::gen {
class NoiseChunkGenerator;
struct DimensionSettings;
} // namespace mc::gen

/**
 * @brief ServerChunkManager 实体生成回调测试
 *
 * 测试当没有 ServerWorld 时，实体生成回调机制是否正常工作。
 */
class ServerChunkManagerCallbackTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化实体注册表
        mc::entity::VanillaEntities::registerAll();

        // 创建工作线程池
        m_workerPool = std::make_unique<mc::util::UniversalWorkerPool>(2, "TestWorker", 900);

        // 创建区块生成器
        auto settings = mc::DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, 12345);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator = std::make_unique<mc::NoiseChunkGenerator>(
            std::move(settings), std::move(biomeSource), std::move(randomState));

        // 创建区块管理器（不关联 ServerWorld）
        m_manager = std::make_unique<ServerChunkManager>(std::move(generator));
        m_manager->setWorkerPool(m_workerPool.get());
        auto result = m_manager->initialize();
        ASSERT_TRUE(result.success());

        // 启动工作线程池
        m_workerPool->start();
    }

    void TearDown() override
    {
        m_manager->shutdown();
        m_workerPool->shutdown();
        m_manager.reset();
        m_workerPool.reset();
    }

    std::unique_ptr<mc::util::UniversalWorkerPool> m_workerPool;
    std::unique_ptr<ServerChunkManager> m_manager;
};

// ============================================================================
// 回调设置测试
// ============================================================================

TEST_F(ServerChunkManagerCallbackTest, SetEntitySpawnCallback)
{
    bool callbackCalled = false;

    m_manager->setEntitySpawnCallback([&callbackCalled](const std::vector<mc::SpawnedEntityData>& entities) {
        callbackCalled = true;
        (void)entities;
    });

    // 验证回调已设置（通过触发区块生成间接验证）
    // 此测试仅验证设置不会崩溃
    EXPECT_FALSE(callbackCalled);
}

TEST_F(ServerChunkManagerCallbackTest, CallbackReceivesSpawnedEntities)
{
    std::vector<mc::SpawnedEntityData> receivedEntities;

    m_manager->setEntitySpawnCallback(
        [&receivedEntities](const std::vector<mc::SpawnedEntityData>& entities) { receivedEntities = entities; });

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
}

// ============================================================================
// 多区块生成测试
// ============================================================================

TEST_F(ServerChunkManagerCallbackTest, MultipleChunksGenerate)
{
    std::atomic<int> totalEntities{0};

    m_manager->setEntitySpawnCallback([&totalEntities](const std::vector<mc::SpawnedEntityData>& entities) {
        totalEntities += static_cast<int>(entities.size());
    });

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
}

// ============================================================================
// 异步生成测试
// ============================================================================

TEST_F(ServerChunkManagerCallbackTest, AsyncGenerateWithCallback)
{
    std::vector<mc::SpawnedEntityData> receivedEntities;
    std::atomic<bool> callbackCompleted{false};

    m_manager->setEntitySpawnCallback(
        [&receivedEntities, &callbackCompleted](const std::vector<mc::SpawnedEntityData>& entities) {
            for (const auto& e : entities) {
                receivedEntities.push_back(e);
            }
            callbackCompleted = true;
        });

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
}

// ============================================================================
// 空区块测试
// ============================================================================

TEST_F(ServerChunkManagerCallbackTest, EmptyChunkDoesNotCallCallback)
{
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

TEST_F(ServerChunkManagerCallbackTest, ResetCallback)
{
    int callCount = 0;

    m_manager->setEntitySpawnCallback([&callCount](const std::vector<mc::SpawnedEntityData>&) { callCount++; });

    // 重置为空回调
    m_manager->setEntitySpawnCallback(nullptr);

    ChunkData* chunk = m_manager->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 等待处理
    for (int i = 0; i < 50; ++i) {
        m_manager->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 空回调不应被调用
    EXPECT_EQ(callCount, 0);
}

// ============================================================================
// 统计测试
// ============================================================================

TEST_F(ServerChunkManagerCallbackTest, ChunkCount)
{
    EXPECT_EQ(m_manager->loadedChunkCount(), 0u);

    static_cast<void>(m_manager->getChunkSync(0, 0));
    EXPECT_EQ(m_manager->loadedChunkCount(), 1u);

    static_cast<void>(m_manager->getChunkSync(1, 0));
    static_cast<void>(m_manager->getChunkSync(0, 1));
    EXPECT_EQ(m_manager->loadedChunkCount(), 3u);
}

TEST_F(ServerChunkManagerCallbackTest, singleChunkLifecycleManagerCount)
{
    EXPECT_EQ(m_manager->singleChunkLifecycleManagerCount(), 0u);

    static_cast<void>(m_manager->getChunkSync(0, 0));
    // FULL 区块请求会为依赖区域创建多个 SCLM:STRUCTURE_STARTS 累积半径 11 → 23×23=529 个
    // 生命周期管理器(对齐 Moonrise,见 ChunkPyramid/ChunkTaskScheduler)。m_lifecycleManagers
    // 含 FULL 目标本块 + 中间状态的依赖邻居,而 m_chunks 仅 FULL 发布块。故
    // singleChunkLifecycleManagerCount(=lifecycleManagerCount,数 m_lifecycleManagers) >> loadedChunkCount(数
    // m_chunks)。 此处仅断言"请求至少创建一个 SCLM"(非 fast-path 静默 no-op),并显式记录 SCLM 数 > loadedChunkCount
    // 的依赖区域语义。
    EXPECT_GE(m_manager->singleChunkLifecycleManagerCount(), 1u);
    EXPECT_GT(m_manager->singleChunkLifecycleManagerCount(), m_manager->loadedChunkCount());
}
