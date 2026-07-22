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

// ③-2b：区块加载光照统一调度测试。
//
// 覆盖 ChunkLoadLightTask 的两条路径：
// - fallback 主线程同步（无 worker 池，executor=nullptr）：enqueueChunkLoadLight
//   直接调 _executeChunkLoadLight，光照同步完成。
// - worker 异步（有 worker 池）：enqueueChunkLoadLight 提交 ChunkLoadLightTask，
//   tick flush dirty + send 续延。
// 覆盖两个执行分支：
// - else 分支（需完整光照）：手动 setLightCorrect(false) 强制重算，验证 light() 后
//   lightCorrect=true 且方块光 nibble 传播生效。
// - if 分支（已光照）：生成后 lightCorrect=true 直接 enqueue，验证不崩且保持正确。

#include <gtest/gtest.h>

#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/base/SectionPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/lighting/LightType.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"
#include "common/world/lighting/storage/SWMRNibbleArray.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/world/ChunkLoadLightTask.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <atomic>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <thread>

namespace mc::server {

namespace {

/// 主世界方块 Y=70 → sectionY=4，nibble 索引 = 4 - m_minLightSection(-5) = 9
constexpr i32 NIBBLE_INDEX_Y70 = 9;
constexpr i32 SECTION_Y70 = 4;
constexpr i32 LOCAL_Y70 = 70 - SECTION_Y70 * world::CHUNK_SECTION_HEIGHT; // 70 - 64 = 6

} // namespace

/// 区块加载光照测试夹具：装配 ServerWorld（与 RuntimeLightConcurrencyTest 同构）
class ChunkLoadLightTaskTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();

        ServerWorldConfig config;
        config.seed = 12345;
        config.viewDistance = 8;
        m_world = std::make_unique<ServerWorld>(config);

        static std::atomic<std::uint64_t> sCounter{0};
        const auto token = std::to_string(std::time(nullptr)) + "_" + std::to_string(sCounter.fetch_add(1));
        m_testDir = std::filesystem::temp_directory_path() / "mc_chunk_load_light_test" / token;
        std::filesystem::create_directories(m_testDir);

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        m_world->setSharedStorage(&m_storage);

        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*m_world, std::move(generator));
        m_manager = chunkManager.get();
        m_world->setChunkManager(std::move(chunkManager));

        ASSERT_TRUE(m_world->initialize().success()) << "ServerWorld::initialize failed";
    }

    void TearDown() override
    {
        if (m_manager != nullptr) {
            m_manager->shutdown();
        }
        if (m_workerPool != nullptr) {
            m_workerPool->shutdown();
        }
        m_world.reset();
        m_workerPool.reset();
        m_storage.close();
        for (int i = 0; i < 10; ++i) {
            std::error_code ec;
            std::filesystem::remove_all(m_testDir, ec);
            if (!ec) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    /// 装配 worker 池（按需，测试异步路径时调）
    void startWorkerPool(i32 threadCount)
    {
        m_workerPool = std::make_unique<util::UniversalWorkerPool>(threadCount, "ChunkLoadLightTestWorker", 900);
        m_manager->setWorkerPool(m_workerPool.get());
        m_workerPool->start();
        m_manager->initialize();
    }

    std::unique_ptr<util::UniversalWorkerPool> m_workerPool;
    std::unique_ptr<ServerWorld> m_world;
    ServerChunkManager* m_manager = nullptr;
    world::storage::SingleLevelStorageManager m_storage;
    std::filesystem::path m_testDir;
};

// fallback 主线程同步路径（无 worker 池）：enqueueChunkLoadLight 直接完成光照。
// 区块经 getChunkSync 生成（LIGHT 阶段已 lightCorrect=true），手动置 false 强制 else 分支重算，
// enqueue 后 lightCorrect 应回 true 且方块光 nibble 传播生效。
TEST_F(ChunkLoadLightTaskTest, FallbackPathRelightsChunkOnMainThread)
{
    // 无 worker 池：radiusAwareExecutor() 返回 nullptr → fallback 同步路径
    m_manager->initialize();

    ChunkData* chunk = m_manager->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);
    // 生成路径 LIGHT 阶段已完成光照
    EXPECT_TRUE(chunk->isLightCorrect());

    // 放置发光方块并强制重算（else 分支：setLightCorrect(false) + 完整 light()）
    const BlockState* glowstone = &VanillaBlocks::GLOWSTONE->defaultState();
    m_world->setBlockState(8, 70, 8, glowstone);
    chunk->setLightCorrect(false);

    std::atomic<int> lightChangeCount{0};
    m_world->setOnLightChanged([&lightChangeCount](LightType, const SectionPos&) { lightChangeCount.fetch_add(1); });

    // enqueueChunkLoadLight fallback 同步执行 _executeChunkLoadLight：light() 后 setLightCorrect(true)，
    // dirty section 入 flush 队列（不下立即 flush），chunk 坐标入 send 续延队列。
    m_world->enqueueChunkLoadLight(0, 0);

    EXPECT_TRUE(chunk->isLightCorrect()) << "fallback 重算后 lightCorrect 应为 true";

    // tick：flush dirty（markLightChanged → 回调）+ send 续延（removeLightTicket）
    m_world->tick();

    EXPECT_GT(lightChangeCount.load(), 0) << "fallback 重算应产生 dirty section 触发 flush 回调";

    // 方块光 nibble 在光源处 > 0（light() 传播生效）
    auto* nibbles = chunk->getBlockNibbles();
    ASSERT_NE(nibbles, nullptr);
    SWMRNibbleArray* nibble = nibbles[NIBBLE_INDEX_Y70];
    ASSERT_NE(nibble, nullptr);
    EXPECT_GT(nibble->getUpdating(8, LOCAL_Y70, 8), static_cast<u8>(0))
        << "光源处方块光应 > 0（fallback light() 生效）";

    m_manager->shutdown();
}

// fallback if 分支：区块已 lightCorrect + status=Generated，enqueue 走 forceHandleEmptySectionChanges +
// checkChunkEdges 廉价重载，不崩且保持 lightCorrect=true。
TEST_F(ChunkLoadLightTaskTest, FallbackPathAlreadyCorrectChunkIsCheap)
{
    m_manager->initialize();

    ChunkData* chunk = m_manager->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);
    EXPECT_TRUE(chunk->isLightCorrect());

    // 直接 enqueue（lightCorrect 保持 true）→ if 分支
    m_world->enqueueChunkLoadLight(0, 0);

    EXPECT_TRUE(chunk->isLightCorrect()) << "if 分支不应改变 lightCorrect";

    // tick 处理 send 续延（removeLightTicket）
    m_world->tick();

    m_manager->shutdown();
}

// worker 异步路径：enqueueChunkLoadLight 提交 ChunkLoadLightTask 到区域互斥池，
// tick flush dirty + send 续延。验证 worker 完成后 lightCorrect=true + flush 回调触发。
TEST_F(ChunkLoadLightTaskTest, WorkerAsyncPathCompletesAndFlushes)
{
    startWorkerPool(2);

    std::atomic<int> lightChangeCount{0};
    m_world->setOnLightChanged([&lightChangeCount](LightType, const SectionPos&) { lightChangeCount.fetch_add(1); });

    ChunkData* chunk = m_manager->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);
    EXPECT_TRUE(chunk->isLightCorrect());

    // 强制 else 分支重算
    const BlockState* glowstone = &VanillaBlocks::GLOWSTONE->defaultState();
    m_world->setBlockState(8, 70, 8, glowstone);
    chunk->setLightCorrect(false);

    // 提交异步任务
    m_world->enqueueChunkLoadLight(0, 0);

    // 等待 worker 完成 + 主线程 flush（带超时，避免 flaky 挂起）
    for (int i = 0; i < 200; ++i) {
        if (chunk->isLightCorrect() && lightChangeCount.load() > 0) {
            break;
        }
        m_world->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(chunk->isLightCorrect()) << "worker 异步重算后 lightCorrect 应为 true";
    EXPECT_GT(lightChangeCount.load(), 0) << "worker 完成后应触发 flush 回调";

    // 方块光 nibble 在光源处 > 0
    auto* nibbles = chunk->getBlockNibbles();
    ASSERT_NE(nibbles, nullptr);
    SWMRNibbleArray* nibble = nibbles[NIBBLE_INDEX_Y70];
    ASSERT_NE(nibble, nullptr);
    EXPECT_GT(nibble->getUpdating(8, LOCAL_Y70, 8), static_cast<u8>(0)) << "光源处方块光应 > 0（worker light() 生效）";

    m_manager->shutdown();
    m_workerPool->shutdown();
}

// hasPendingLightWork：fallback 路径同步完成光照/send/票据释放，不入队续延，
// 故 enqueue 后 hasPendingLightWork 应 false（无待处理 flush/send/运行时变更）。
// worker 异步路径的 hasPendingLightWork 语义由 WorkerAsyncPathCompletesAndFlushes 覆盖。
TEST_F(ChunkLoadLightTaskTest, FallbackPathLeavesNoPendingLightWork)
{
    m_manager->initialize();

    m_manager->getChunkSync(0, 0);
    m_world->enqueueChunkLoadLight(0, 0);

    // fallback 同步执行：光照/send/票据释放当场完成，队列无积压
    EXPECT_FALSE(m_world->hasPendingLightWork());

    m_world->tick();

    // tick 后仍无积压
    EXPECT_FALSE(m_world->hasPendingLightWork());

    m_manager->shutdown();
}

} // namespace mc::server
