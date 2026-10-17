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

#include "common/TempDirHelper.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include "server/world/storage/SingleLevelStorageManager.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
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

        m_workerPool = std::make_unique<mc::util::UniversalWorkerPool>(2, "TestWorker", 900);

        ServerWorldConfig config;
        config.seed = 12345;
        config.viewDistance = 8;
        m_world = std::make_unique<ServerWorld>(config);

        // 打开存档并挂载 manager 到 world，再调用 world->initialize()。
        // 后处理路径 _postProcessChunk 会调 m_world->tickManager().scheduleFluidTick，
        // m_tickManager 仅由 world->initialize() 创建；不初始化时为空 → 解引用空指针
        // ACCESS_VIOLATION at 0x10（scheduleFluidTick 内 m_fluidTicks）。与 ServerChunkManagerTest
        // 同一修复模式（镜像 ServerWorldTest/StructureReferencesRaceTest）。
        // TempDirHelper 的 token 含 PID，跨进程天然唯一，避免 CTest -j16 下多进程同秒同计数器碰撞。
        m_testDir = mc::test::makeUniqueTestDir("mc_scm_postprocess_test");

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        m_world->setSharedStorage(&m_storage);

        auto settings = DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*m_world, std::move(generator));
        chunkManager->setWorkerPool(m_workerPool.get());
        m_manager = chunkManager.get();
        m_world->setChunkManager(std::move(chunkManager));

        // 计数 onChunkLoaded/callback 触发次数（m_chunkLoadedCallback 在 _drainPendingPostProcess
        // 中于 onChunkLoaded 之后调用，去重后同一区块只触发一次）。
        m_manager->setChunkLoadedCallback([this](ChunkCoord, ChunkCoord) { ++m_chunkLoadedCallCount; });

        ASSERT_TRUE(m_world->initialize().success()) << "ServerWorld::initialize failed";
    }

    void TearDown() override
    {
        // 先 shutdown manager（停止 worker、清任务），再 reset world（销毁 manager），最后关存档。
        if (m_manager != nullptr) {
            m_manager->shutdown();
        }
        m_workerPool->shutdown();
        m_world.reset();
        m_workerPool.reset();
        m_storage.close();
        // TempDirHelper 内置 10 次重试，覆盖 Windows 上 RocksDB 后台线程延迟释放句柄的窗口。
        mc::test::removeTestDir(m_testDir);
    }

    std::unique_ptr<mc::util::UniversalWorkerPool> m_workerPool;
    std::unique_ptr<ServerWorld> m_world;
    // m_manager 由 m_world 持有（setChunkManager 转移所有权），这里仅持有裸指针供测试访问。
    ServerChunkManager* m_manager = nullptr;
    int m_chunkLoadedCallCount = 0;
    // 区块加载回调的当前/峰值嵌套深度。重入回归用例以此代替"是否栈溢出"作为观测量：
    // gtest 主线程栈远大于生产 std::thread 的 512KB，溢出与否不可靠，而嵌套深度是等价契约。
    int m_callbackDepth = 0;
    int m_maxCallbackDepth = 0;
    world::storage::SingleLevelStorageManager m_storage;
    std::filesystem::path m_testDir;

    // TEST_F 生成的测试类派生自本 fixture，不继承 friend 访问权限，
    // 故由 fixture 成员（friend）转发 _enqueuePostProcess，供测试体调用。
    void enqueuePostProcessForTest(
        ChunkCoord x, ChunkCoord z, std::vector<SpawnedEntityData>&& entities, bool needsPostProcess)
    {
        m_manager->_enqueuePostProcess(x, z, std::move(entities), needsPostProcess);
    }

    void setTicketLevelForTest(ChunkCoord x, ChunkCoord z, i32 level)
    {
        m_manager->_onTicketLevelChanged(x, z, 46, level);
    }

    void drainUnloadForTest() { m_manager->_drainPendingUnloadFinishes(); }
};

// ============================================================================
// 去重测试
// ============================================================================

TEST_F(ServerChunkManagerPostProcessTest, TicketUpdatesDoNotPublishPendingStorageResults)
{
    m_workerPool->start();
    ASSERT_TRUE(m_manager->initialize().success());
    ChunkData stored(0, 0);
    stored.createSection(world::toSectionIndex(64))->setBlockState(0, 0, 0, &VanillaBlocks::STONE->defaultState());
    ASSERT_TRUE(m_storage.saveChunk(stored, 0).success());

    // 存储未注入 IO 池，在调用线程读完并入队，使交接时机完全确定。
    auto future = m_manager->requestChunkAsync(0, 0, world::chunk::ChunkStatuses::FULL);
    m_manager->processTicketUpdates();
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(0)), std::future_status::timeout);
    EXPECT_EQ(m_manager->tryToGetChunkInMem(0, 0), nullptr);
    m_manager->processPendingChunkTasks();
    ASSERT_EQ(future.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    ASSERT_NE(future.get(), nullptr);
    EXPECT_EQ(m_chunkLoadedCallCount, 0);
}

TEST_F(ServerChunkManagerPostProcessTest, LoadCompletionOnlyConsumesEntrySnapshot)
{
    m_workerPool->start();
    ASSERT_TRUE(m_manager->initialize().success());
    for (ChunkCoord x = 0; x < 2; ++x) {
        ChunkData stored(x, 0);
        stored.createSection(world::toSectionIndex(64))->setBlockState(0, 0, 0, &VanillaBlocks::STONE->defaultState());
        ASSERT_TRUE(m_storage.saveChunk(stored, 0).success());
    }

    std::future<ChunkData*> second;
    bool firstCompleted = false;
    m_manager->requestChunkAsync(0, 0, world::chunk::ChunkStatuses::FULL, [&](bool success, ChunkData* chunk) {
        firstCompleted = success && chunk != nullptr;
        second = m_manager->requestChunkAsync(1, 0, world::chunk::ChunkStatuses::FULL);
    });
    m_manager->processPendingChunkTasks();
    ASSERT_TRUE(firstCompleted);
    ASSERT_TRUE(second.valid());
    EXPECT_EQ(second.wait_for(std::chrono::milliseconds(0)), std::future_status::timeout);
    EXPECT_EQ(m_manager->tryToGetChunkInMem(1, 0), nullptr);
    m_manager->processPendingChunkTasks();
    ASSERT_EQ(second.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
    ASSERT_NE(second.get(), nullptr);
}

TEST_F(ServerChunkManagerPostProcessTest, GenerationSchedulingDoesNotWaitForBusyArea)
{
    m_workerPool->start();
    ASSERT_TRUE(m_manager->initialize().success());
    auto future = m_manager->requestChunkAsync(0, 0, world::chunk::ChunkStatuses::EMPTY);
    std::promise<void> locked;
    auto acquired = locked.get_future();
    std::promise<void> release;
    auto released = release.get_future();
    std::thread blocker([&] {
        auto lock = m_manager->getTaskScheduler()->schedulingLockArea().lock(
            0, 0, 2 * ChunkTaskScheduler::getMaxAccessRadius());
        locked.set_value();
        // 超时可保证旧的阻塞实现也会结束测试，不让测试进程永久挂起。
        released.wait_for(std::chrono::milliseconds(250));
    });
    acquired.wait();
    const auto start = std::chrono::steady_clock::now();
    m_manager->processPendingChunkTasks();
    const auto elapsed = std::chrono::steady_clock::now() - start;
    release.set_value();
    blocker.join();
    EXPECT_LT(elapsed, std::chrono::milliseconds(100));
    ASSERT_EQ(future.wait_for(std::chrono::seconds(5)), std::future_status::ready);
    EXPECT_NE(future.get(), nullptr);
}

TEST_F(ServerChunkManagerPostProcessTest, BusyUnloadFinishesRetryWithOriginalHolder)
{
    m_workerPool->start();
    ASSERT_TRUE(m_manager->initialize().success());
    ChunkData stored(0, 0);
    stored.createSection(world::toSectionIndex(64))->setBlockState(0, 0, 0, &VanillaBlocks::STONE->defaultState());
    ASSERT_TRUE(m_storage.saveChunk(stored, 0).success());
    auto* loaded = m_manager->requestFullChunkSync(0, 0);
    ASSERT_NE(loaded, nullptr);
    loaded->setDirty(false);
    m_workerPool->waitForCompletion();
    m_manager->unloadChunkSync(0, 0);
    std::promise<void> locked;
    auto acquired = locked.get_future();
    std::promise<void> release;
    auto released = release.get_future();
    std::thread blocker([&] {
        auto lock = m_manager->getTaskScheduler()->schedulingLockArea().lock(
            0, 0, 2 * ChunkTaskScheduler::getMaxAccessRadius());
        locked.set_value();
        released.wait_for(std::chrono::milliseconds(250));
    });
    acquired.wait();
    const auto start = std::chrono::steady_clock::now();
    drainUnloadForTest();
    const auto elapsed = std::chrono::steady_clock::now() - start;
    EXPECT_TRUE(m_manager->hasChunkInMem(0, 0));
    release.set_value();
    blocker.join();
    EXPECT_LT(elapsed, std::chrono::milliseconds(100));
    drainUnloadForTest();
    EXPECT_FALSE(m_manager->hasChunkInMem(0, 0));
    EXPECT_EQ(m_manager->lifecycleManagerCount(), 0u);
}

TEST_F(ServerChunkManagerPostProcessTest, BusyTicketLevelsKeepOnlyLatestDemand)
{
    m_workerPool->start();
    ASSERT_TRUE(m_manager->initialize().success());
    auto& holder = m_manager->ensureTaskScheduler()->getOrCreateHolder(0, 0);
    std::promise<void> locked;
    auto acquired = locked.get_future();
    std::promise<void> release;
    auto released = release.get_future();
    std::thread blocker([&] {
        auto lock = m_manager->getTaskScheduler()->schedulingLockArea().lock(
            0, 0, 2 * ChunkTaskScheduler::getMaxAccessRadius());
        locked.set_value();
        released.wait_for(std::chrono::milliseconds(250));
    });
    acquired.wait();
    const auto start = std::chrono::steady_clock::now();
    setTicketLevelForTest(0, 0, 33);
    setTicketLevelForTest(0, 0, 45);
    m_manager->processTicketUpdates();
    const auto elapsed = std::chrono::steady_clock::now() - start;
    release.set_value();
    blocker.join();
    EXPECT_LT(elapsed, std::chrono::milliseconds(100));
    m_manager->processTicketUpdates();
    EXPECT_EQ(holder.level(), 45);
    EXPECT_EQ(holder.sourceState(), world::chunk::SingleChunkLifecycleManager::SourceState::Unknown);
}

TEST_F(ServerChunkManagerPostProcessTest, LoadBudgetPreservesOldResultsBeforeNewCompletions)
{
    ASSERT_TRUE(m_manager->initialize().success());
    for (ChunkCoord x = 0; x < 3; ++x) {
        ChunkData stored(x, 0);
        stored.createSection(world::toSectionIndex(64))->setBlockState(0, 0, 0, &VanillaBlocks::STONE->defaultState());
        ASSERT_TRUE(m_storage.saveChunk(stored, 0).success());
    }
    std::vector<ChunkCoord> completed;
    const auto callback = [&](bool success, ChunkData* chunk) {
        ASSERT_TRUE(success);
        ASSERT_NE(chunk, nullptr);
        completed.push_back(chunk->x());
        if (chunk->x() == 0) {
            m_manager->requestChunkAsync(2, 0, world::chunk::ChunkStatuses::FULL, [&](bool ok, ChunkData* next) {
                ASSERT_TRUE(ok);
                ASSERT_NE(next, nullptr);
                completed.push_back(next->x());
            });
            // 单项耗尽预算后，剩余旧项必须保留并先于此回调新产生的完成项交付。
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    };
    m_manager->requestChunkAsync(0, 0, world::chunk::ChunkStatuses::FULL, callback);
    m_manager->requestChunkAsync(1, 0, world::chunk::ChunkStatuses::FULL, callback);
    m_manager->processPendingChunkTasks();
    EXPECT_EQ(completed, (std::vector<ChunkCoord>{0}));
    for (int attempt = 0; attempt < 10 && completed.size() < 3; ++attempt) {
        m_manager->processPendingChunkTasks();
    }
    EXPECT_EQ(completed, (std::vector<ChunkCoord>{0, 1, 2}));
}

TEST_F(ServerChunkManagerPostProcessTest, PostProcessBudgetEventuallyDeliversEveryCallbackOnce)
{
    ASSERT_TRUE(m_manager->initialize().success());
    std::vector<ChunkCoord> completed;
    m_manager->setChunkLoadedCallback([&](ChunkCoord x, ChunkCoord) {
        completed.push_back(x);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    });
    for (ChunkCoord x = 0; x < 3; ++x) {
        enqueuePostProcessForTest(x, 0, {}, false);
    }
    m_manager->tick();
    EXPECT_EQ(completed, (std::vector<ChunkCoord>{0}));
    for (int attempt = 0; attempt < 10 && completed.size() < 3; ++attempt) {
        m_manager->tick();
    }
    m_manager->tick();
    EXPECT_EQ(completed, (std::vector<ChunkCoord>{0, 1, 2}));
}

TEST_F(ServerChunkManagerPostProcessTest, DoubleEnqueue_Dedup)
{
    m_workerPool->start();
    m_manager->initialize();

    // 同步生成区块（FULL 完成），worker 线程已入队一次 PendingPostProcess。
    ChunkData* chunk = m_manager->requestFullChunkSync(0, 0);
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

/**
 * @brief 存档命中路径的加载回调必须延后执行，且不得形成无界递归
 *
 * 区块加载回调会添加光照保活票据。结果发布和世界回调必须分阶段进行，
 * 主动推进存档完成通知时不能内联执行世界回调。
 *
 * 历史上该回调由 _onChunkLoadComplete 的**存档命中分支**内联调用，于是
 * "处理一个完成项 → 回调 → 再次排空队列 → 处理下一项"，每项递归一层，深度正比于待加载区块数。
 * 主循环跑在 std::thread 创建的线程上（macOS 默认栈 512KB），玩家加入触发视距内数百区块同时
 * 加载时直接撞穿栈保护页——在 macOS arm64 上被报成 SIGILL（"Could not determine thread index
 * for stack guard region"）而非 SIGSEGV，极具误导性。
 *
 * 用例必须走**存档命中**分支才有效：生成路径的回调本就由 onChunkGenComplete 经 _enqueuePostProcess
 * 延后，只加载新生成区块的用例即使把回调改回内联也照样通过（是恒真的无效回归）。
 * 故此处先生成落盘、再卸载、最后并发重载，并让完成项在排空期间堆积（递归的必要条件）。
 */
TEST_F(ServerChunkManagerPostProcessTest, StoredChunkLoadCompleteDefersCallback_NoUnboundedRecursion)
{
    m_workerPool->start();
    m_manager->initialize();

    constexpr i32 kRadius = 1;
    constexpr int kChunks = (2 * kRadius + 1) * (2 * kRadius + 1);

    // 阶段一：生成并落盘。
    for (i32 dx = -kRadius; dx <= kRadius; ++dx) {
        for (i32 dz = -kRadius; dz <= kRadius; ++dz) {
            ASSERT_NE(m_manager->requestFullChunkSync(dx, dz), nullptr) << "生成失败: " << dx << "," << dz;
        }
    }
    m_manager->tick();

    // 阶段二：全部卸载。stage1 发起异步保存，stage3 收尾（移出内存）由后续 tick 驱动——
    // 因此"内存中已无该区块"即意味着存档已写入，后续重载必定命中存档。
    for (i32 dx = -kRadius; dx <= kRadius; ++dx) {
        for (i32 dz = -kRadius; dz <= kRadius; ++dz) {
            m_manager->unloadChunkSync(dx, dz);
        }
    }
    for (int i = 0; i < 400 && m_manager->loadedChunkCount() > 0; ++i) {
        m_manager->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    ASSERT_EQ(m_manager->loadedChunkCount(), 0) << "卸载完成后不应有区块驻留内存";

    // 阶段三：并发重载。回调形态与生产环境一致（会重入票据处理链）。
    // 嵌套深度代替"是否栈溢出"作为观测量：gtest 主线程栈远大于生产 std::thread 的 512KB，
    // 溢出与否不可靠，而回调嵌套深度是等价契约。
    m_chunkLoadedCallCount = 0;
    m_maxCallbackDepth = 0;
    m_manager->setChunkLoadedCallback([this](ChunkCoord x, ChunkCoord z) {
        const int depth = ++m_callbackDepth;
        m_maxCallbackDepth = std::max(m_maxCallbackDepth, depth);
        ++m_chunkLoadedCallCount;
        m_manager->addLightTicket(x, z);
        --m_callbackDepth;
    });

    std::vector<std::future<ChunkData*>> futures;
    futures.reserve(static_cast<size_t>(kChunks));
    for (i32 dx = -kRadius; dx <= kRadius; ++dx) {
        for (i32 dz = -kRadius; dz <= kRadius; ++dz) {
            futures.push_back(m_manager->requestChunkAsync(dx, dz, mc::world::chunk::ChunkStatuses::FULL));
        }
    }

    // 等待期间主动 pump 完成队列——这正是原始崩溃的排空时机：队列中已堆积多个完成项，
    // 处理其中任一项时若回调内联执行，就会同步回到排空并接着处理其余项，逐项递归。
    for (auto& future : futures) {
        while (future.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready) {
            m_manager->processPendingChunkTasks();
        }
        ASSERT_NE(future.get(), nullptr) << "存档命中重载失败";
    }

    // 存档命中路径不得在排空期间内联触发加载回调。
    EXPECT_EQ(m_chunkLoadedCallCount, 0)
        << "存档命中路径的加载回调必须在 tick() 的后处理排水中延后执行，不得在排空期间内联触发";
    EXPECT_EQ(m_maxCallbackDepth, 0) << "尚未 tick，加载回调不应被触发过";

    // 回调延后到 tick() 的后处理排水执行，且该排水每轮只消费当轮快照，
    // 期间新入队的项留到下一轮，故多 tick 直至计数收敛。
    for (int i = 0; i < 50 && m_chunkLoadedCallCount < kChunks; ++i) {
        m_manager->tick();
    }

    EXPECT_LE(m_maxCallbackDepth, 1) << "区块加载回调不得相互嵌套——嵌套即意味着完成项处理链发生了自递归"
                                        "（峰值深度 "
                                     << m_maxCallbackDepth << "）";
    EXPECT_GE(m_chunkLoadedCallCount, kChunks)
        << "重载的每个区块都应至少触发一次加载回调（实际 " << m_chunkLoadedCallCount << " 次）";

    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerPostProcessTest, UnloadClearsDedup_AllowsReprocess)
{
    m_workerPool->start();
    m_manager->initialize();

    // 区块数据先发布，worker 随后才入队后处理；同步请求返回不保证下一次 tick 已取得该条目。
    // 有限等待加载回调，仍要求每个生命周期恰好执行一次。
    const auto waitForLoadCallbacks = [this](int expectedCount) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        do {
            m_manager->tick();
            if (m_chunkLoadedCallCount >= expectedCount) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        } while (std::chrono::steady_clock::now() < deadline);
    };
    ASSERT_NE(m_manager->requestFullChunkSync(0, 0), nullptr);
    waitForLoadCallbacks(1);
    ASSERT_EQ(m_chunkLoadedCallCount, 1);

    // 卸载：unloadChunkSync 触发异步存档保存（stage1/2），stage3 收尾（清 m_chunks 与
    // m_postProcessedChunks key）由 _drainPendingUnloadFinishes 在后续 tick() 中完成。
    // 这里循环 tick() 直至区块真正移出内存，匹配生产 tick 驱动的异步卸载语义。
    m_manager->unloadChunkSync(0, 0);
    for (int i = 0; i < 200 && m_manager->hasChunkInMem(0, 0); ++i) {
        m_manager->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    ASSERT_FALSE(m_manager->hasChunkInMem(0, 0)) << "异步卸载完成后区块应移出内存";

    // 真实重载应恢复加载回调；卸载若未清除去重 key，回调计数会一直停留在 1。
    ASSERT_NE(m_manager->requestFullChunkSync(0, 0), nullptr);
    waitForLoadCallbacks(2);
    ASSERT_EQ(m_chunkLoadedCallCount, 2) << "卸载清除 key 后，重载区块应再次执行加载回调";

    // 重载完成后再次入队同一生命周期的条目，验证去重仍有效。
    enqueuePostProcessForTest(0, 0, {}, /*needsPostProcess=*/false);
    m_manager->tick();
    m_manager->tick();

    EXPECT_EQ(m_chunkLoadedCallCount, 2) << "同一生命周期的重复后处理不应再次触发加载回调";

    m_manager->shutdown();
    m_workerPool->shutdown();
}

} // namespace mc::server
