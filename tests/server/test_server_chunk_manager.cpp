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

#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/storage/SingleLevelStorageManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <future>
#include <thread>
#include <vector>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

using namespace mc;
using namespace mc::server;

// ============================================================================
// ServerChunkManager 测试固件
// ============================================================================

class ServerChunkManagerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 初始化方块注册表
        VanillaBlocks::initialize();

        // 创建工作线程池
        m_workerPool = std::make_unique<mc::util::UniversalWorkerPool>(2, "TestWorker", 900);

        // 创建测试用的 ServerWorld
        ServerWorldConfig config;
        config.seed = 12345;
        config.viewDistance = 8;
        m_world = std::make_unique<ServerWorld>(config);

        // 打开存档：ServerWorld::initialize 要求 m_storage 已设置且 isOpen()。
        // 复用 ServerWorldTest/StructureReferencesRaceTest 的模式，在临时目录打开 RocksDB 存档。
        // 必须调用 world->initialize()：后处理测试的 _postProcessChunk 会调
        // m_world->tickManager().scheduleFluidTick，而 m_tickManager 仅由 world->initialize() 创建，
        // 不初始化时为空 → 解引用空指针 ACCESS_VIOLATION at 0x10（scheduleFluidTick 内 m_fluidTicks）。
        static std::atomic<std::uint64_t> s_counter{0};
        const auto token = std::to_string(std::time(nullptr)) + "_" + std::to_string(s_counter.fetch_add(1));
        m_testDir = std::filesystem::temp_directory_path() / "mc_scm_test" / token;
        std::filesystem::create_directories(m_testDir);

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        m_world->setSharedStorage(&m_storage);

        // 创建区块管理器并挂到世界（与 ServerWorldTest 一致），使 world->initialize() 完整初始化
        // m_tickManager / m_lightManager 等。m_manager 持有裸指针（所有权归 m_world）。
        auto settings = DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*m_world, std::move(generator));
        chunkManager->setWorkerPool(m_workerPool.get());
        m_manager = chunkManager.get();
        m_world->setChunkManager(std::move(chunkManager));

        // 设置区块加载回调计数器：每个区块完成主线程后处理（onChunkLoaded + callback）时 +1。
        // 用于验证去重：同一区块的 callback 至多触发一次（m_postProcessedChunks 保证）。
        m_manager->setChunkLoadedCallback(
            [this](ChunkCoord, ChunkCoord) { m_chunkLoadedCallCount.fetch_add(1, std::memory_order::relaxed); });

        ASSERT_TRUE(m_world->initialize().success()) << "ServerWorld::initialize failed";
    }

    void TearDown() override
    {
        // 先 shutdown manager（停止 worker、清任务），再 reset world（销毁 manager 与 world 子系统），
        // 最后关存档。顺序与 StructureReferencesRaceTest 一致，避免 world 析构时 manager 仍在跑。
        if (m_manager != nullptr) {
            m_manager->shutdown();
        }
        m_workerPool->shutdown();
        m_world.reset();
        m_workerPool.reset();
        m_storage.close();
        // 重试删除：Windows 上 RocksDB 后台线程可能延迟释放文件句柄。
        for (int i = 0; i < 10; ++i) {
            std::error_code ec;
            std::filesystem::remove_all(m_testDir, ec);
            if (!ec) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::unique_ptr<mc::util::UniversalWorkerPool> m_workerPool;
    std::unique_ptr<ServerWorld> m_world;
    // m_manager 由 m_world 持有（setChunkManager 转移所有权），这里仅持有裸指针供测试访问。
    ServerChunkManager* m_manager = nullptr;
    // 原子计数器：ConcurrentGenerateAndUnloadRace 在非主线程调用 tick()，_drainPendingPostProcess
    // 可能从该线程触发 m_chunkLoadedCallback，故需原子访问避免数据竞争。
    std::atomic<int> m_chunkLoadedCallCount{
        0}; ///< 区块加载回调触发次数（主线程后处理 _drainPendingPostProcess 期间累加）
    world::storage::SingleLevelStorageManager m_storage;
    std::filesystem::path m_testDir;
};

// ============================================================================
// 构造和生命周期测试
// ============================================================================

TEST_F(ServerChunkManagerTest, Constructor)
{
    EXPECT_EQ(m_manager->loadedChunkCount(), 0);
    EXPECT_EQ(m_manager->singleChunkLifecycleManagerCount(), 0);
    EXPECT_FALSE(m_workerPool->isRunning());
}

TEST_F(ServerChunkManagerTest, Initialize)
{
    m_workerPool->start();
    auto result = m_manager->initialize();
    EXPECT_TRUE(result.success());
    EXPECT_TRUE(m_workerPool->isRunning());
    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerTest, Shutdown)
{
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

TEST_F(ServerChunkManagerTest, GetChunk_NotExists)
{
    ChunkData* chunk = m_manager->tryToGetChunkInMem(0, 0);
    EXPECT_EQ(chunk, nullptr);
}

TEST_F(ServerChunkManagerTest, HasChunk_NotExists)
{
    EXPECT_FALSE(m_manager->hasChunkInMem(0, 0));
    EXPECT_FALSE(m_manager->hasChunkInMem(100, 100));
}

TEST_F(ServerChunkManagerTest, GetChunkSync_CreatesChunk)
{
    ChunkData* chunk = m_manager->getChunkSync(0, 0);

    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 0);
    EXPECT_EQ(chunk->z(), 0);
    EXPECT_TRUE(m_manager->hasChunkInMem(0, 0));
}

TEST_F(ServerChunkManagerTest, RequestFullChunkSync_GeneratesStructureReferenceDependencies)
{
    ChunkData* chunk = m_manager->requestChunkSync(0, 0, ChunkStatuses::FULL);

    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 0);
    EXPECT_EQ(chunk->z(), 0);
    EXPECT_TRUE(chunk->isFullyGenerated());
}

TEST_F(ServerChunkManagerTest, RequestStructureReferencesSync_GeneratesDirectDependencies)
{
    // 非 FULL 目标状态：区块生成到 STRUCTURE_REFERENCES 后返回 primer 的 ChunkData（共享所有权）。
    // 中间状态区块不发布到 m_chunks（m_chunks 仅保留 FULL 区块，保证 tryToGetChunkInMem 快速路径
    // 不返回中间状态区块），故 hasChunkInMem 为 false。验证请求返回有效区块且 holder 存在。
    ChunkData* chunk = m_manager->requestChunkSync(0, 0, ChunkStatuses::STRUCTURE_REFERENCES);

    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 0);
    EXPECT_EQ(chunk->z(), 0);
    // STRUCTURE_REFERENCES 未达 FULL，区块不发布到 m_chunks
    EXPECT_FALSE(m_manager->hasChunkInMem(0, 0));
    // holder 存在（生成链路已创建生命周期管理器）
    EXPECT_GE(m_manager->singleChunkLifecycleManagerCount(), 1);
}

TEST_F(ServerChunkManagerTest, GetChunkSync_ReturnsSameChunk)
{
    ChunkData* chunk1 = m_manager->getChunkSync(5, 10);
    ChunkData* chunk2 = m_manager->getChunkSync(5, 10);

    EXPECT_EQ(chunk1, chunk2);
}

TEST_F(ServerChunkManagerTest, GetChunkSync_AfterGeneration)
{
    m_manager->getChunkSync(3, 7);

    ChunkData* chunk = m_manager->tryToGetChunkInMem(3, 7);
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(chunk->x(), 3);
    EXPECT_EQ(chunk->z(), 7);
}

TEST_F(ServerChunkManagerTest, GetChunkSync_MultipleChunks)
{
    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            ChunkData* chunk = m_manager->getChunkSync(x, z);
            ASSERT_NE(chunk, nullptr) << "Failed to generate chunk (" << x << ", " << z << ")";
        }
    }

    EXPECT_EQ(m_manager->loadedChunkCount(), 25);

    for (int x = -2; x <= 2; ++x) {
        for (int z = -2; z <= 2; ++z) {
            EXPECT_TRUE(m_manager->hasChunkInMem(x, z));
        }
    }
}

// ============================================================================
// 异步区块访问测试
// ============================================================================

TEST_F(ServerChunkManagerTest, GetChunkAsync_AfterInit)
{
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
    EXPECT_TRUE(m_manager->hasChunkInMem(0, 0));

    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerTest, GetChunkAsync_Callback)
{
    m_workerPool->start();
    m_manager->initialize();

    std::atomic<bool> completed{false};
    ChunkData* resultChunk = nullptr;

    m_manager->getChunkAsync(
        5,
        5,
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

TEST_F(ServerChunkManagerTest, GetChunkAsync_AlreadyCached)
{
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
    EXPECT_EQ(syncChunk, asyncChunk); // 应该是同一个实例

    m_manager->shutdown();
    m_workerPool->shutdown();
}

// ============================================================================
// 区块卸载测试
// ============================================================================

TEST_F(ServerChunkManagerTest, UnloadChunk)
{
    m_manager->getChunkSync(0, 0);
    EXPECT_TRUE(m_manager->hasChunkInMem(0, 0));

    m_manager->unloadChunkSync(0, 0);
    EXPECT_FALSE(m_manager->hasChunkInMem(0, 0));
}

// ============================================================================
// 票据管理测试
// ============================================================================

TEST_F(ServerChunkManagerTest, UpdatePlayerPosition)
{
    m_workerPool->start();
    m_manager->initialize();

    m_manager->updatePlayerPosition(1, 0.0, 0.0);

    // 应该创建区块持有者
    EXPECT_GE(m_manager->singleChunkLifecycleManagerCount(), 1);

    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerTest, RemovePlayer)
{
    m_workerPool->start();
    m_manager->initialize();

    m_manager->updatePlayerPosition(1, 0.0, 0.0);
    EXPECT_GE(m_manager->singleChunkLifecycleManagerCount(), 1);

    m_manager->removePlayer(1);

    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerTest, SetViewDistance)
{
    m_manager->setViewDistance(8);
    EXPECT_EQ(m_manager->viewDistance(), 8);

    m_manager->setViewDistance(16);
    EXPECT_EQ(m_manager->viewDistance(), 16);
}

// ============================================================================
// Tick 测试
// ============================================================================

TEST_F(ServerChunkManagerTest, Tick)
{
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

TEST_F(ServerChunkManagerTest, LoadedChunkCount)
{
    EXPECT_EQ(m_manager->loadedChunkCount(), 0);

    m_manager->getChunkSync(0, 0);
    EXPECT_EQ(m_manager->loadedChunkCount(), 1);

    m_manager->getChunkSync(1, 0);
    m_manager->getChunkSync(0, 1);
    EXPECT_EQ(m_manager->loadedChunkCount(), 3);
}

TEST_F(ServerChunkManagerTest, PendingTaskCount)
{
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

TEST_F(ServerChunkManagerTest, GeneratorNotNull)
{
    EXPECT_NE(m_manager->generator(), nullptr);
}

TEST_F(ServerChunkManagerTest, GeneratedChunkHasBlocks)
{
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

TEST_F(ServerChunkManagerTest, ConcurrentChunkAccess)
{
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

// ============================================================================
// 并发生成与卸载竞态测试
//
// 复现场景：worker 线程执行 ChunkProgressionTask::execute → onChunkGenComplete 期间，
// 主线程 tick→_checkChunkUnloading→unloadChunkSync 销毁 holder，导致 use-after-free
// （notifyWaitingNeighbours 读取已释放 holder 的 m_waitingNeighbours，size=385290616 垃圾值）。
//
// 测试策略：
// 1. 启动 worker 池
// 2. 一个线程不断请求生成新区块（触发 ChunkTaskScheduler 调度，worker 执行 onChunkGenComplete）
// 3. 同时不断移除玩家/更新位置（触发票据级别变化→_checkChunkUnloading→unloadChunkSync）
// 4. 持续运行若干秒，观察是否崩溃
//
// 若 holder 在 onChunkGenComplete 期间被 unloadChunkSync 销毁，测试会触发 access violation
// （notifyWaitingNeighbours 读取已释放内存）。
// ============================================================================
TEST_F(ServerChunkManagerTest, ConcurrentGenerateAndUnloadRace)
{
    m_workerPool->start();
    m_manager->initialize();

    // 玩家初始位置在原点，生成初始区块
    m_manager->updatePlayerPosition(1, 0.0, 0.0);
    for (int i = 0; i < 50; ++i) {
        m_manager->tick();
    }

    std::atomic<bool> stop{false};
    std::atomic<int> crashFlag{0}; // 非零表示检测到异常（用于诊断）

    // 看门狗线程：监控 pending/running 趋势，无进展（running==0 且 pending>0）判定死锁。
    // 正常负载下 mover+generator 各 20 次、每次 sleep 1~2ms，应在数十秒内完成；
    // 若 holder 因依赖图清理缺陷永久阻塞（m_waitingNeighbours 非空 → isSafeToUnload 永假），
    // unloadChunkSync 重试会级联阻塞邻居，导致 stall。
    // STALL_THRESHOLD_SECONDS 为硬超时（真正死锁时 running==0 触发提前 abort；纯吞吐量不足时
    // running>0 持续下降，硬超时给 worker 足够时间消化级联生成任务）。
    constexpr int STALL_THRESHOLD_SECONDS = 300;
    std::thread watchdog([&]() {
        int lastPending = -1;
        int noProgressSamples = 0; // 连续 pending 未下降的 5 秒采样数（6 次=30 秒=死锁）
        bool dumpedStuck = false;
        for (int sec = 0; sec < STALL_THRESHOLD_SECONDS * 2 && !stop.load(std::memory_order::acquire); ++sec) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            // 每 5 秒输出一次状态快照，观察 pending/running/holder 计数趋势
            if (sec % 10 == 0) {
                const size_t pending = m_workerPool->pendingTaskCount();
                const size_t running = m_workerPool->runningTaskCount();
                const size_t holders = m_manager->lifecycleManagerCount();
                const int pendingInt = static_cast<int>(pending);
                if (lastPending < 0 || pendingInt < lastPending) {
                    // pending 下降：有进展，重置计数
                    noProgressSamples = 0;
                } else {
                    // pending 未下降（持平或上升）：累加无进展计数
                    ++noProgressSamples;
                }
                lastPending = pendingInt;
                spdlog::info("[watchdog] {}s: pending={}, running={}, holders={}, noProgressSamples={}",
                    sec / 2,
                    pending,
                    running,
                    holders,
                    noProgressSamples);
                // 30 秒（6 个 5 秒采样）连续无进展判定为死锁。
                // 真正的生成负载下 pending 会持续下降（即使缓慢），死锁时 pending 冻结。
                // running=0 且 pending>0：worker 全空闲但队列有任务（队列饥饿/区域互斥活锁）。
                // running>0 且 pending 冻结：worker 卡在某个永不完成的任务（executeTask 死循环/死锁）。
                // 注意：级联生成期间 pending 会先增长（onChunkGenComplete 重调度入队 > worker 出队）后下降，
                // 单纯 pending 增长不是死锁。仅当 running==0（worker 全空闲）且 pending>0 时才判定死锁——
                // worker 空闲说明队列中的任务因区域互斥/依赖图缺陷无法执行。
                if (noProgressSamples >= 6 && pending > 0 && running == 0) {
                    spdlog::info("[watchdog] 检测到真正死锁：worker 全空闲但 pending={} 不下降", pending);
                    break;
                }
                // 诊断：200s 时 dump 一次卡住的 holder（不 abort），定位依赖图泄漏/isSafeToUnload 永假。
                // generator 的 inflight.front().get() 阻塞导致测试无法完成，但 running>0（worker 活跃），
                // 真正死锁条件（running==0）不触发。用固定时点 dump 捕获卡住状态。
                if (sec == 400 && !dumpedStuck) {
                    dumpedStuck = true;
                    spdlog::info(
                        "[watchdog] 90s 诊断 dump：pending={}, running={}, holders={}", pending, running, holders);
                    m_workerPool->debugDumpRunningTasks();
                    m_manager->_debugDumpStuckHolders();
                }
            }
        }
        if (!stop.load(std::memory_order::acquire)) {
            spdlog::info("[watchdog] 测试 stall 超过 {}s，强制 abort 以定位死锁。pendingTaskCount={}, "
                         "runningTaskCount={}, holders={}",
                STALL_THRESHOLD_SECONDS,
                m_workerPool->pendingTaskCount(),
                m_workerPool->runningTaskCount(),
                m_manager->lifecycleManagerCount());
            m_workerPool->debugDumpState();
            m_workerPool->debugDumpRunningTasks();
            m_manager->_debugDumpStuckHolders();
            std::abort();
        }
    });

    // 线程 A：移动玩家位置，触发票据级别变化 → _checkChunkUnloading → unloadChunkSync。
    // 6 区块半径（viewDistance=8 范围内），减少并发 inflight 任务数，
    // 避免任务饥饿（大规模 mover+generator 会堆积大量 inflight 任务级联阻塞）。
    constexpr int MOVE_ITERATIONS = 20;
    constexpr double MOVE_RADIUS_CHUNKS = 6.0;
    std::thread mover([&]() {
        try {
            for (int i = 0; i < MOVE_ITERATIONS && !stop.load(std::memory_order::acquire); ++i) {
                const double angle = (i % 32) * (3.14159265358979 / 16.0);
                const double radius = MOVE_RADIUS_CHUNKS * 16.0;
                const double wx = std::cos(angle) * radius;
                const double wz = std::sin(angle) * radius;
                m_manager->updatePlayerPosition(1, wx, wz);
                m_manager->tick(); // tick 处理票据更新 + _checkChunkUnloading
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        }
        catch (...) {
            crashFlag.store(1, std::memory_order::release);
        }
    });

    // 线程 B：请求生成区块，驱动 worker 执行 onChunkGenComplete。
    // 用 MAX_INFLIGHT 限制未完成 future 数量，避免 inflight 任务堆积导致的级联阻塞
    // （旧版本无限制地创建 future，大量 inflight 请求在卸载竞态下级联阻塞）。
    // 坐标范围缩小到 ±12（24x24），与 mover 移动范围重叠，复用已生成区块减少总工作量。
    constexpr int GEN_ITERATIONS = 20;
    constexpr int MAX_INFLIGHT = 6;
    std::thread generator([&]() {
        try {
            std::vector<std::future<ChunkData*>> inflight;
            inflight.reserve(MAX_INFLIGHT + 1);
            for (int i = 0; i < GEN_ITERATIONS && !stop.load(std::memory_order::acquire); ++i) {
                const int x = (i * 7) % 24 - 12;
                const int z = (i * 13) % 24 - 12;
                auto future = m_manager->getChunkAsync(x, z, &ChunkStatuses::FULL);
                inflight.push_back(std::move(future));
                // 超过 MAX_INFLIGHT 时回收最旧的 future（等待其完成或取消）
                if (static_cast<int>(inflight.size()) > MAX_INFLIGHT) {
                    inflight.front().get(); // 等待最旧的 future 完成（释放 holder 引用）
                    inflight.erase(inflight.begin());
                }
                if (i % 10 == 0) {
                    // 偶尔同步请求，增加 worker 与主线程的交错
                    (void)m_manager->getChunkSync(x, z);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            // 回收剩余 inflight
            for (auto& f : inflight) {
                if (f.valid()) {
                    (void)f.get();
                }
            }
        }
        catch (...) {
            crashFlag.store(2, std::memory_order::release);
        }
    });

    mover.join();
    generator.join();
    stop.store(true, std::memory_order::release);
    watchdog.join();

    // 若任何线程捕获到异常，标记失败
    EXPECT_EQ(crashFlag.load(std::memory_order::acquire), 0)
        << "Concurrent generate/unload test detected an exception in worker thread";

    // 清理：移除玩家并 shutdown
    m_manager->removePlayer(1);
    m_manager->shutdown();
    m_workerPool->shutdown();
}

// ============================================================================
// 后处理去重测试
//
// 验证 ServerChunkManager 的主线程后处理去重（m_postProcessedChunks +
// ChunkData::isPostProcessingDone）保证同一区块的 onChunkLoaded / m_chunkLoadedCallback /
// spawnEntitiesFromChunkGeneration / _postProcessChunk 至多执行一次。
// ============================================================================

TEST_F(ServerChunkManagerTest, PostProcessDoneFlag_AfterGeneration)
{
    m_workerPool->start();
    m_manager->initialize();

    // 同步生成区块：worker 线程 _finalizeGeneratedChunkSync 入队 PendingPostProcess，
    // 但 _drainPendingPostProcess 仅在 tick() 中执行，故生成完成后 isPostProcessingDone 仍为 false。
    ChunkData* chunk = m_manager->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);
    EXPECT_FALSE(chunk->isPostProcessingDone()) << "postProcess 应在 tick() drain 之前未执行";

    // tick 触发 _drainPendingPostProcess：执行 _postProcessChunk 并置 isPostProcessingDone=true。
    m_manager->tick();

    EXPECT_TRUE(chunk->isPostProcessingDone()) << "postProcess 应在 tick() drain 之后完成";
    EXPECT_EQ(m_chunkLoadedCallCount.load(std::memory_order::acquire), 1) << "区块加载回调应恰好触发一次";

    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerTest, OnChunkLoadedOnce_GenerationPath)
{
    m_workerPool->start();
    m_manager->initialize();

    m_manager->getChunkSync(0, 0);
    m_manager->tick();

    // 多次 tick 不应重复触发后处理（m_postProcessedChunks 去重）。
    m_manager->tick();
    m_manager->tick();

    EXPECT_EQ(m_chunkLoadedCallCount.load(std::memory_order::acquire), 1) << "多次 tick 不应重复触发区块加载回调";

    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerTest, UnloadClearsPostProcessedFlag)
{
    m_workerPool->start();
    m_manager->initialize();

    m_manager->getChunkSync(0, 0);
    m_manager->tick();
    ASSERT_EQ(m_chunkLoadedCallCount.load(std::memory_order::acquire), 1);

    // 卸载：unloadChunkSync 触发异步存档保存（stage1/2），stage3 收尾（清 m_chunks 与
    // m_postProcessedChunks key）由 _drainPendingUnloadFinishes 在后续 tick() 中完成。
    // 循环 tick() 直至区块真正移出内存，匹配生产 tick 驱动的异步卸载语义。
    m_manager->unloadChunkSync(0, 0);
    for (int i = 0; i < 200 && m_manager->hasChunkInMem(0, 0); ++i) {
        m_manager->tick();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    EXPECT_FALSE(m_manager->hasChunkInMem(0, 0)) << "异步卸载完成后区块应移出内存";

    // 重新生成：存档中 isPostProcessingDone 不持久化（重载为新 ChunkData，标志为 false），
    // 重新入队 PendingPostProcess，tick 后应再次触发回调（计数 +1）。
    m_manager->getChunkSync(0, 0);
    m_manager->tick();

    EXPECT_EQ(m_chunkLoadedCallCount.load(std::memory_order::acquire), 2)
        << "卸载后重新加载应重新执行后处理（m_postProcessedChunks 已清除 key）";

    m_manager->shutdown();
    m_workerPool->shutdown();
}

TEST_F(ServerChunkManagerTest, PostProcessDoneFlag_NotSetBeforeTick)
{
    m_workerPool->start();
    m_manager->initialize();

    ChunkData* chunk = m_manager->getChunkSync(0, 0);
    ASSERT_NE(chunk, nullptr);

    // tick 之前：区块已生成（在 m_chunks 中），但后处理尚未 drain。
    EXPECT_TRUE(m_manager->hasChunkInMem(0, 0));
    EXPECT_FALSE(chunk->isPostProcessingDone()) << "tick 之前 postProcess 不应执行";

    m_manager->tick();
    EXPECT_TRUE(chunk->isPostProcessingDone()) << "tick 之后 postProcess 应完成";

    m_manager->shutdown();
    m_workerPool->shutdown();
}
