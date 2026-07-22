/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights
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
 */

// ③-1 运行时光照 worker 传播 + 主线程 flush visible 的并发测试。
// 覆盖：RuntimeLightingProvider 保活/范围/dirty 收集、fallback 同步路径、
// flush 队列往返、worker 异步传播 + 主线程 flush。

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
#include "server/world/RuntimeLightingProvider.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerLightQueue.hpp"
#include "server/world/ServerWorld.hpp"
#include <atomic>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <thread>

namespace mc::server {

namespace {

/// 主世界方块 Y=70 → sectionY=4，nibble 索引 = 4 - m_minLightSection(-5) = 9
/// （与 ServerLightQueueTest 一致，验证传播结果时用）
constexpr i32 NIBBLE_INDEX_Y70 = 9;
constexpr i32 SECTION_Y70 = 4;
constexpr i32 LOCAL_Y70 = 70 - SECTION_Y70 * world::CHUNK_SECTION_HEIGHT; // 70 - 64 = 6

} // namespace

/// 运行时光照并发测试夹具：装配带 workerPool 的 ServerWorld（照搬 PostProcessTest 模式）
class RuntimeLightConcurrencyTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        m_workerPool = std::make_unique<util::UniversalWorkerPool>(2, "RuntimeLightTestWorker", 900);

        ServerWorldConfig config;
        config.seed = 12345;
        config.viewDistance = 8;
        m_world = std::make_unique<ServerWorld>(config);

        static std::atomic<std::uint64_t> sCounter{0};
        const auto token = std::to_string(std::time(nullptr)) + "_" + std::to_string(sCounter.fetch_add(1));
        m_testDir = std::filesystem::temp_directory_path() / "mc_runtime_light_test" / token;
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
        chunkManager->setWorkerPool(m_workerPool.get());
        m_manager = chunkManager.get();
        m_world->setChunkManager(std::move(chunkManager));

        ASSERT_TRUE(m_world->initialize().success()) << "ServerWorld::initialize failed";
    }

    void TearDown() override
    {
        if (m_manager != nullptr) {
            m_manager->shutdown();
        }
        m_workerPool->shutdown();
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

    std::unique_ptr<util::UniversalWorkerPool> m_workerPool;
    std::unique_ptr<ServerWorld> m_world;
    ServerChunkManager* m_manager = nullptr;
    world::storage::SingleLevelStorageManager m_storage;
    std::filesystem::path m_testDir;
};

// RuntimeLightingProvider：5×5 保活范围内 getChunkForLight 返回已加载区块指针，范围外 nullptr
TEST_F(RuntimeLightConcurrencyTest, ProviderKeepaliveAndRange)
{
    m_workerPool->start();
    m_manager->initialize();

    // 同步生成中心区块 (0,0)，其邻居部分会被加载（radiusAware 生成会触达半径2）
    ChunkData* center = m_manager->getChunkSync(0, 0);
    ASSERT_NE(center, nullptr);

    RuntimeLightingProvider provider(*m_world, 0, 0);

    // 5×5 范围内：已加载的区块返回非空指针，未加载的返回 nullptr（均合法）
    // 中心 (0,0) 必然已加载
    IChunk* centerViaProvider = provider.getChunkForLight(0, 0);
    EXPECT_EQ(centerViaProvider, static_cast<IChunk*>(center));

    // 5×5 外（半径3）：必须返回 nullptr
    EXPECT_EQ(provider.getChunkForLight(3, 0), nullptr);
    EXPECT_EQ(provider.getChunkForLight(0, 3), nullptr);
    EXPECT_EQ(provider.getChunkForLight(-3, -3), nullptr);

    m_manager->shutdown();
    m_workerPool->shutdown();
}

// RuntimeLightingProvider：markLightChanged 收集 dirty section，takeDirtySections 取出后清空
TEST_F(RuntimeLightConcurrencyTest, ProviderCollectsDirtySections)
{
    m_workerPool->start();
    m_manager->initialize();

    m_manager->getChunkSync(0, 0);

    RuntimeLightingProvider provider(*m_world, 0, 0);

    provider.markLightChanged(LightType::SKY, SectionPos(0, SECTION_Y70, 0));
    provider.markLightChanged(LightType::BLOCK, SectionPos(0, SECTION_Y70, 0));
    provider.markLightChanged(LightType::BLOCK, SectionPos(1, SECTION_Y70, 0));

    auto dirty = provider.takeDirtySections();
    EXPECT_EQ(dirty.size(), 3u);
    EXPECT_EQ(dirty[0].first, LightType::SKY);
    EXPECT_EQ(dirty[1].first, LightType::BLOCK);
    EXPECT_EQ(dirty[2].first, LightType::BLOCK);

    // 取出后清空
    auto dirty2 = provider.takeDirtySections();
    EXPECT_TRUE(dirty2.empty());

    m_manager->shutdown();
    m_workerPool->shutdown();
}

// flush 队列往返：_enqueueLightFlush 入队 dirty，tick 后 _drainPendingLightFlushes 触发 markLightChanged 回调
TEST_F(RuntimeLightConcurrencyTest, FlushQueueRoundTrip)
{
    m_workerPool->start();
    m_manager->initialize();

    // 生成中心区块，使 markLightChanged 内部 tryToGetChunkInMem 命中、_syncLightDataToChunk 有 nibble 可读
    m_manager->getChunkSync(0, 0);

    // 用 setOnLightChanged 计数 flush 实际触发
    std::atomic<int> lightChangeCount{0};
    m_world->setOnLightChanged([&lightChangeCount](LightType, const SectionPos&) { lightChangeCount.fetch_add(1); });

    // 手动入队 3 个 dirty section（模拟 worker 完成）
    std::vector<std::pair<LightType, SectionPos>> dirty;
    dirty.emplace_back(LightType::SKY, SectionPos(0, SECTION_Y70, 0));
    dirty.emplace_back(LightType::BLOCK, SectionPos(0, SECTION_Y70, 0));
    dirty.emplace_back(LightType::BLOCK, SectionPos(1, SECTION_Y70, 0));
    m_world->_enqueueLightFlush(std::move(dirty));

    // tick 触发 _drainPendingLightFlushes → 逐项 markLightChanged → 回调
    m_world->tick();

    EXPECT_EQ(lightChangeCount.load(), 3);

    m_manager->shutdown();
    m_workerPool->shutdown();
}

// worker 异步传播 + 主线程 flush：start workerPool，setBlockState 入队，tick 提交 worker，
// 等待 worker 完成 + 下一 tick flush，验证光照传播 + flush 回调触发
TEST_F(RuntimeLightConcurrencyTest, WorkerAsyncPropagationAndFlush)
{
    m_workerPool->start();
    m_manager->initialize();

    std::atomic<int> lightChangeCount{0};
    m_world->setOnLightChanged([&lightChangeCount](LightType, const SectionPos&) { lightChangeCount.fetch_add(1); });

    ChunkData* chunk = m_manager->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // 放置发光方块，入队光照变更
    const BlockState* glowstone = &VanillaBlocks::GLOWSTONE->defaultState();
    m_world->setBlockState(8, 70, 8, glowstone);

    // tick 1：drainAndProcess(*this) 提交 RuntimeLightTask 到 worker 池
    m_world->tick();

    // 等待 worker 完成传播 + 入队 dirty section（带超时，避免 flaky 挂起）
    for (int i = 0; i < 200; ++i) {
        if (lightChangeCount.load() > 0) {
            break;
        }
        // tick 推进 flush（worker 完成后下一 tick drain flush）
        m_world->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // flush 回调应被触发（worker 传播 → dirty 入队 → 主线程 flush → markLightChanged → 回调）
    EXPECT_GT(lightChangeCount.load(), 0) << "worker 异步传播后应触发 flush 回调";

    // 验证方块光 nibble 在光源处 > 0（worker 传播生效）
    auto* nibbles = chunk->getBlockNibbles();
    ASSERT_NE(nibbles, nullptr);
    SWMRNibbleArray* nibble = nibbles[NIBBLE_INDEX_Y70];
    ASSERT_NE(nibble, nullptr);
    EXPECT_GT(nibble->getUpdating(8, LOCAL_Y70, 8), static_cast<u8>(0)) << "光源处方块光应 > 0（worker 异步传播生效）";

    m_manager->shutdown();
    m_workerPool->shutdown();
}

} // namespace mc::server
