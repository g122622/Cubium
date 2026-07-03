/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING NO EVENT FOR ANY KIND, either EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "common/util/thread/ServerWorkerPool.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::server;

namespace mc::server {

/**
 * @brief ServerChunkManager 后处理去重测试（直接入队）
 *
 * 通过 friend 直接调用 _enqueuePostProcess 验证 m_postProcessedChunks 去重：
 * 同一区块连续入队两次 PendingPostProcess，tick 一次后 onChunkLoaded/callback/
 * spawn/postProcess 应只执行一次。
 */
class ServerChunkManagerPostProcessTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        m_workerPool = std::make_unique<mc::util::ServerWorkerPool>(2, "TestWorker");

        ServerWorldConfig config;
        config.seed = 12345;
        config.viewDistance = 8;
        m_world = std::make_unique<ServerWorld>(config);

        auto settings = DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        m_manager = std::make_unique<ServerChunkManager>(*m_world, std::move(generator));
        m_manager->setWorkerPool(m_workerPool.get());

        // 计数 onChunkLoaded/callback 触发次数（m_chunkLoadedCallback 在 _drainPendingPostProcess
        // 中于 onChunkLoaded 之后调用，去重后同一区块只触发一次）。
        m_manager->setChunkLoadedCallback([this](ChunkCoord, ChunkCoord) { ++m_chunkLoadedCallCount; });
    }

    void TearDown() override
    {
        m_manager.reset();
        m_workerPool.reset();
        m_world.reset();
    }

    std::unique_ptr<mc::util::ServerWorkerPool> m_workerPool;
    std::unique_ptr<ServerWorld> m_world;
    std::unique_ptr<ServerChunkManager> m_manager;
    int m_chunkLoadedCallCount = 0;

    // TEST_F 生成的测试类派生自本 fixture，不继承 friend 访问权限，
    // 故由 fixture 成员（friend）转发 _enqueuePostProcess，供测试体调用。
    void enqueuePostProcessForTest(
        ChunkCoord x, ChunkCoord z, std::vector<SpawnedEntityData>&& entities, bool needsPostProcess)
    {
        m_manager->_enqueuePostProcess(x, z, std::move(entities), needsPostProcess);
    }
};

// ============================================================================
// 去重测试
// ============================================================================

TEST_F(ServerChunkManagerPostProcessTest, DoubleEnqueue_Dedup)
{
    m_workerPool->start();
    m_manager->initialize();

    // 同步生成区块（FULL 完成），worker 线程已入队一次 PendingPostProcess。
    ChunkData* chunk = m_manager->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // tick 之前再直接入队一次（模拟重复入队/竞态）：同一区块、带实体、needsPostProcess=true。
    // tick 之后 _drainPendingPostProcess 应只处理第一次（生成路径入队的条目），
    // 第二次（直接入队的条目）被 m_postProcessedChunks 去重丢弃。
    // 注意：直接入队的条目排在生成路径入队条目之后，但去重发生在 drain 遍历时——
    // 无论顺序，同一区块 key 只处理一次。
    std::vector<SpawnedEntityData> entities;
    entities.emplace_back("minecraft:pig", 1.0f, 100.0f, 1.0f);
    enqueuePostProcessForTest(0, 0, std::move(entities), /*needsPostProcess=*/true);

    m_manager->tick();

    // 去重：区块加载回调只触发一次（生成路径的入队 + 直接入队，仅第一次执行）。
    EXPECT_EQ(m_chunkLoadedCallCount, 1)
        << "重复入队的 PendingPostProcess 应被 m_postProcessedChunks 去重，callback 只触发一次";

    // postProcess 只执行一次（isPostProcessingDone 双重保护）。
    EXPECT_TRUE(chunk->isPostProcessingDone()) << "postProcess 应在 tick() drain 之后完成";

    // 多次 tick 不应重复触发。
    m_manager->tick();
    m_manager->tick();
    EXPECT_EQ(m_chunkLoadedCallCount, 1) << "多次 tick 不应重复触发后处理";

    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerPostProcessTest, UnloadClearsDedup_AllowsReprocess)
{
    m_workerPool->start();
    m_manager->initialize();

    m_manager->getChunkSync(0, 0);
    m_manager->tick();
    ASSERT_EQ(m_chunkLoadedCallCount, 1);

    // 卸载：unloadChunkSync 清除 m_postProcessedChunks 中的 key。
    m_manager->unloadChunkSync(0, 0);
    EXPECT_FALSE(m_manager->hasChunkInMem(0, 0));

    // 直接入队一个存档加载风格的 PendingPostProcess（needsPostProcess=false），
    // 模拟卸载后重载的 onChunkLoaded/callback。tick 后应执行（key 已清除）。
    enqueuePostProcessForTest(0, 0, {}, /*needsPostProcess=*/false);
    m_manager->tick();

    EXPECT_EQ(m_chunkLoadedCallCount, 2) << "卸载清除 key 后，重新入队的后处理应执行（callback 计数 +1）";

    m_manager->shutdown();
    m_workerPool->shutdown();
}

} // namespace mc::server
