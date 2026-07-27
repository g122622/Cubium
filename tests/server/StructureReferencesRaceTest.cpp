/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to furnished to any persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// 复现测试：跑图时 STRUCTURE_REFERENCES 阶段崩溃
//
// 崩溃栈：
//   ChunkPrimer::getIntersectingStructures (ChunkPrimer.hpp:326)
//   NoiseChunkGenerator::generateStructureReferences (NoiseChunkGenerator.cpp:266)
//
// 现象：start->isValid() 读取地址 0x50（shared_ptr<StructureStart> 内部指针损坏）。
// 视距调大后更容易触发，强烈提示并发 race。
//
// 本测试用更多 worker 线程 + 大视距 + 玩家移动触发卸载/重生成循环，
// 最大化 STRUCTURE_STARTS/STRUCTURE_REFERENCES/NOISE/FEATURES 并发读邻居 m_structureStarts 的概率。
// 若崩溃为并发 race，本测试应能复现 ACCESS_VIOLATION 或 ASan/TSan 报告。

#include "common/TempDirHelper.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeTags.hpp"
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
#include <filesystem>
#include <future>
#include <thread>
#include <vector>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

using namespace mc;
using namespace mc::server;

namespace {

// 测试固件：大视距 + 多 worker，最大化并发
class StructureReferencesRaceTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        // 预初始化生物群系标签：区块生成 worker 会并发调用 BiomeTags::getTag()/
        // HAS_STRUCTURE_*()（generateStructureStarts 遍历结构集时）。在工作线程启动前
        // 完成初始化，避免多 worker 首次懒初始化的 call_once 串行化（虽然 call_once 已
        // 保证正确性，但提前初始化消除首块生成时的屏障等待），并与其他测试夹具一致。
        mc::world::biome::BiomeTags::initialize();

        // 8 个 worker 线程，最大化 STRUCTURE_REFERENCES 等无区域互斥任务的并发度
        m_workerPool = std::make_unique<mc::util::UniversalWorkerPool>(8, "StructRaceWorker", 900);

        // 打开存档：ServerWorld::initialize 要求 m_storage 已设置且 isOpen()。
        // 复用 ServerWorldTest 的模式，在临时目录中打开一个 RocksDB 存档。
        // 由 TempDirHelper 生成唯一目录：token 含 PID，跨进程天然唯一，避免 CTest -j16 下
        // 多进程同秒同计数器碰撞导致 WorldSessionLock 残留句柄引发 ERROR_SHARING_VIOLATION。
        m_testDir = mc::test::makeUniqueTestDir("mc_struct_race_test");

        world::storage::SingleLevelStorageConfig storageConfig;
        auto openResult = m_storage.open(m_testDir, storageConfig);
        ASSERT_TRUE(openResult.success()) << openResult.error().message();

        ServerWorldConfig config;
        config.seed = 12345;
        // 大视距：更多区块同时生成，更多邻居 m_structureStarts 并发读
        config.viewDistance = 16;

        // 把区块管理器挂到世界（与 ServerWorldTest 一致），使 world->initialize() 完整初始化
        // m_tickManager 等。此前固件直接构造独立的 ServerChunkManager 且不调用 world->initialize()，
        // 导致 m_tickManager 为空：玩家 ticket 驱动区块到 FULL 后，_postProcessChunk 调用
        // m_world->tickManager().scheduleFluidTick 解引用空 m_tickManager → ACCESS_VIOLATION at 0x10。
        m_world = std::make_unique<ServerWorld>(config);
        m_world->setSharedStorage(&m_storage);

        auto settings = DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, config.seed);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*m_world, std::move(generator));
        chunkManager->setWorkerPool(m_workerPool.get());
        m_world->setChunkManager(std::move(chunkManager));
        m_manager = m_world->chunkManager();

        ASSERT_TRUE(m_world->initialize().success()) << "ServerWorld::initialize failed";
    }

    void TearDown() override
    {
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
    world::storage::SingleLevelStorageManager m_storage;
    std::filesystem::path m_testDir;
};

} // namespace

// 单阶段：生成一个区块网格，触发大量并发的 STRUCTURE_STARTS/STRUCTURE_REFERENCES。
// 不移动玩家，只验证纯并发生成是否崩溃。
TEST_F(StructureReferencesRaceTest, GenerateGrid_NoMove)
{
    m_workerPool->start();
    ASSERT_TRUE(m_manager->initialize().success());

    // 生成 12x12 区块网格（视距 16 范围内），全部到 FULL。
    // 网格中心区域会有大量结构（村庄/神殿等），触发 m_structureStarts 填充与跨区块引用。
    constexpr int HALF = 6;
    std::vector<std::future<ChunkData*>> futures;
    futures.reserve(static_cast<size_t>(HALF * 2 + 1) * static_cast<size_t>(HALF * 2 + 1));

    for (int x = -HALF; x <= HALF; ++x) {
        for (int z = -HALF; z <= HALF; ++z) {
            futures.push_back(m_manager->getChunkAsync(x, z, &ChunkStatuses::FULL));
        }
    }

    // 等待全部完成（带超时，崩溃会表现为进程终止/ASan 报告）。
    // 异步存档加载路径把完成回调入队 m_pendingLoadCompletes，需主线程 tick() 出队处理
    // （_drainPendingLoadCompletes→_onChunkLoadComplete→_completeReadyWaiters fulfill promise）。
    // 本测试不另起 tick 线程，故在主线程等待 future 期间主动 pump tick()，避免 promise 永不完成。
    for (auto& f : futures) {
        if (!f.valid()) {
            continue;
        }
        constexpr auto kTimeout = std::chrono::seconds(120);
        const auto deadline = std::chrono::steady_clock::now() + kTimeout;
        while (f.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready) {
            m_manager->tick();
            ASSERT_LT(std::chrono::steady_clock::now(), deadline) << "Grid generation timed out (possible deadlock)";
        }
        ChunkData* chunk = f.get();
        ASSERT_NE(chunk, nullptr);
    }

    m_manager->shutdown();
    m_workerPool->shutdown();
}

// 针对性复现 0x50 崩溃：use-after-free of neighbour ChunkPrimer during STRUCTURE_REFERENCES。
//
// 根因（已通过代码分析确认）：
//   ChunkProgressionTask::executeStatusStep 在 worker 线程执行 generateStructureReferences 时，
//   通过 StaticChunkCache2D<ChunkPrimer*>（裸指针，无所有权）遍历 17x17 邻居，调用
//   neighbor->getIntersectingStructures（ChunkPrimer.hpp:326 读 m_structureStarts）。
//   StaticChunkCache2D 在 scheduleStatusStep 时构建，仅靠邻居 holder 的 m_neighboursUsingThisChunk
//   引用计数防止邻居被卸载。但：
//     - worker 线程执行 executeStatusStep 期间不持调度锁；
//     - 中心区块被卸载时（_onTicketLevelChanged/isSafeToUnload 或 _checkChunkUnloading→unloadChunkSync），
//       unloadChunkSync→cancelGeneration(center) 清空 m_generationTask 并释放 288 个邻居的引用计数
//       （removeNeighbourUsingChunk），随后 isSafeToUnload(center) 可能为 true（m_neighboursUsingThisChunk==0），
//       holder 从 m_lifecycleManagers 移除（center 的 primer 由 task 的 m_holder shared_ptr 保活，但邻居无此保护）；
//     - 邻居 m_neighboursUsingThisChunk 归零后，后续 _checkChunkUnloading→unloadChunkSync(neighbour) 即可销毁
//       邻居 holder 及其 m_currentChunk（ChunkPrimer）；
//     - worker 线程仍持 StaticChunkCache2D 中的裸 ChunkPrimer* → 解引用已释放内存 → ACCESS_VIOLATION at 0x50。
//   abortSignal 仅在 executeStatusStep 起止检查（ChunkProgressionTask.cpp:175/229），generateStructureReferences
//   的 17x17 循环内不检查，无法在卸载窗口内退出。
//
// 本测试最大化该窗口：高并发 STRUCTURE_REFERENCES 进行中 + 激进玩家传送触发卸载/取消，
// 迫使 cancelGeneration 释放引用计数与邻居卸载发生在 worker 遍历邻居期间。
TEST_F(StructureReferencesRaceTest, TeleportUnloadDuringStructureReferences_0x50Repro)
{
    m_workerPool->start();
    ASSERT_TRUE(m_manager->initialize().success());

    // 先在原点周围生成一片区块到 STRUCTURE_REFERENCES，使半径 8 邻居完成 STRUCTURE_STARTS
    // （m_structureStarts 已填充），为后续 STRUCTURE_REFERENCES 读邻居提供结构起点。
    // 不生成到 FULL：避免 _finalizeGeneratedChunkSync 入队 _postProcessChunk（scheduleFluidTick 0x10 路径），
    // 隔离 0x50 崩溃窗口。
    {
        std::vector<std::future<ChunkData*>> seeds;
        for (int x = -4; x <= 4; ++x) {
            for (int z = -4; z <= 4; ++z) {
                seeds.push_back(m_manager->getChunkAsync(x, z, &ChunkStatuses::STRUCTURE_REFERENCES));
            }
        }
        for (auto& f : seeds) {
            if (!f.valid()) {
                continue;
            }
            // seed 生成在 teleporter tick 线程启动前，主线程等待 future 期间主动 pump tick()
            // 处理异步存档加载完成（_drainPendingLoadCompletes），否则 promise 永不 fulfill。
            constexpr auto kTimeout = std::chrono::seconds(120);
            const auto deadline = std::chrono::steady_clock::now() + kTimeout;
            while (f.wait_for(std::chrono::milliseconds(1)) != std::future_status::ready) {
                m_manager->tick();
                ASSERT_LT(std::chrono::steady_clock::now(), deadline) << "seed generation timed out";
            }
            (void)f.get();
        }
    }

    // 玩家初始位置在原点，建立 ticket。
    m_manager->updatePlayerPosition(1, 0.0, 0.0);
    for (int i = 0; i < 30; ++i) {
        m_manager->tick();
    }

    std::atomic<bool> stop{false};
    std::atomic<int> crashFlag{0};

    // 看门狗：检测死锁/挂起。
    constexpr int STALL_THRESHOLD_SECONDS = 600;
    std::thread watchdog([&]() {
        int lastPending = -1;
        int noProgressSamples = 0;
        for (int sec = 0; sec < STALL_THRESHOLD_SECONDS * 2 && !stop.load(std::memory_order::acquire); ++sec) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (sec % 10 == 0) {
                const size_t pending = m_workerPool->pendingTaskCount();
                const size_t running = m_workerPool->runningTaskCount();
                const int pendingInt = static_cast<int>(pending);
                if (lastPending < 0 || pendingInt < lastPending) {
                    noProgressSamples = 0;
                } else {
                    ++noProgressSamples;
                }
                lastPending = pendingInt;
                spdlog::info("[watchdog] {}s: pending={}, running={}, noProgress={}",
                    sec / 2,
                    pending,
                    running,
                    noProgressSamples);
                if (noProgressSamples >= 6 && pending > 0 && running == 0) {
                    spdlog::info("[watchdog] deadlock: workers idle but pending={}", pending);
                    break;
                }
                // worker 池完全空闲（pending=0, running=0）但持续无进展：
                // 说明 generator 线程的 future.get() 永不完成（promise 泄漏），dump 卡住 holder 诊断根因。
                if (noProgressSamples >= 6 && pending == 0 && running == 0) {
                    spdlog::info("[watchdog] stall: workers idle, no in-flight tasks — dumping stuck holders");
                    m_manager->_debugDumpStuckHolders();
                    break;
                }
            }
        }
        if (!stop.load(std::memory_order::acquire)) {
            spdlog::info("[watchdog] stall > {}s, abort", STALL_THRESHOLD_SECONDS);
            std::abort();
        }
    });

    // 线程 A：激进传送玩家。每次传送到远处，触发大批区块 ticket 级别下降（shouldLoad=false）→
    // _checkChunkUnloading→unloadChunkSync→cancelGeneration（释放邻居引用计数）→邻居卸载。
    // 再传回原点附近，触发重新生成。反复横跳最大化 cancel 与 STRUCTURE_REFERENCES 并发窗口。
    constexpr int TELEPORT_ITERATIONS = 200;
    constexpr double FAR_RADIUS_CHUNKS = 48.0; // 远离原点的传送距离（区块单位）
    std::thread teleporter([&]() {
        try {
            for (int i = 0; i < TELEPORT_ITERATIONS && !stop.load(std::memory_order::acquire); ++i) {
                // 交替远点与原点附近，每次都驱动 ticket 更新 + 卸载检查
                if (i % 2 == 0) {
                    const double angle = (i * 0.3) * 3.14159265358979;
                    const double wx = std::cos(angle) * FAR_RADIUS_CHUNKS * 16.0;
                    const double wz = std::sin(angle) * FAR_RADIUS_CHUNKS * 16.0;
                    m_manager->updatePlayerPosition(1, wx, wz);
                } else {
                    // 原点附近抖动，重新请求生成刚被卸载的区块
                    const double jx = ((i * 37) % 32 - 16) * 16.0;
                    const double jz = ((i * 53) % 32 - 16) * 16.0;
                    m_manager->updatePlayerPosition(1, jx, jz);
                }
                // 每个 tick 触发 _checkChunkUnloading（每 UNLOAD_CHECK_INTERVAL_TICKS=20 tick 一次卸载扫描）
                for (int t = 0; t < 25; ++t) {
                    m_manager->tick();
                }
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }
            // 传送循环结束后继续驱动 tick() 直到 generators 全部完成（stop 被置位）。
            // 异步存档加载把完成回调入队 m_pendingLoadCompletes，必须由主线程 tick() 出队
            // （_drainPendingLoadCompletes→_onChunkLoadComplete→_completeReadyWaiters fulfill promise）。
            // 若传送循环一结束就停止 tick()，仍持有 in-flight future 的 generator 线程的 f.get() 会永久挂起
            // （worker 池空闲 pending=0/running=0 但 promise 永不完成 → 测试死锁）。
            // 此处对齐真实游戏：主 tick 线程持续运行，玩家位置更新与区块请求由其他线程发起。
            // processTicketUpdatesSync（tick 内）与 updatePlayerPosition 共享 m_ticketManager 无锁状态，
            // 二者必须同线程串行——teleporter 同时负责二者，满足单线程不变量。
            while (!stop.load(std::memory_order::acquire)) {
                m_manager->tick();
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }
        }
        catch (...) {
            crashFlag.store(3, std::memory_order::release);
        }
    });

    // 线程 B/C/D：并发请求生成区块到 STRUCTURE_REFERENCES（而非 FULL），驱动 STRUCTURE_STARTS/
    // STRUCTURE_REFERENCES 在多 worker 上并发，同时避免触发 FULL 后处理路径（_postProcessChunk/
    // scheduleFluidTick），隔离 0x50 崩溃窗口。请求 STRUCTURE_REFERENCES 仍会经依赖图驱动半径 8 邻居
    // 完成 STRUCTURE_STARTS，StaticChunkCache2D 在 scheduleStatusStep 捕获邻居裸 ChunkPrimer*。
    // 高并发 + 大量 in-flight 请求，最大化"某区块 STRUCTURE_REFERENCES 进行中"与"另一线程卸载其
    // 邻居/中心"重叠，迫使 cancelGeneration 释放引用计数 → 邻居 primer 被卸载 → worker 解引用悬空指针。
    constexpr int GEN_THREADS = 3;
    constexpr int GEN_ITERATIONS = 120;
    constexpr int MAX_INFLIGHT = 16;
    std::vector<std::thread> generators;
    generators.reserve(GEN_THREADS);
    for (int tid = 0; tid < GEN_THREADS; ++tid) {
        generators.emplace_back([&, tid]() {
            try {
                std::vector<std::future<ChunkData*>> inflight;
                inflight.reserve(MAX_INFLIGHT + 1);
                for (int i = 0; i < GEN_ITERATIONS && !stop.load(std::memory_order::acquire); ++i) {
                    // 围绕原点的伪随机区块，混入远点，使生成与卸载区重叠
                    const int base = (tid * 17 + i * 7) % 40 - 20;
                    const int x = base + ((i * 5) % 8);
                    const int z = ((tid * 13 + i * 11) % 40) - 20 + ((i * 3) % 6);
                    auto future = m_manager->getChunkAsync(x, z, &ChunkStatuses::STRUCTURE_REFERENCES);
                    inflight.push_back(std::move(future));
                    if (static_cast<int>(inflight.size()) > MAX_INFLIGHT) {
                        inflight.front().get();
                        inflight.erase(inflight.begin());
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                }
                for (auto& f : inflight) {
                    if (f.valid()) {
                        (void)f.get();
                    }
                }
            }
            catch (...) {
                crashFlag.store(4, std::memory_order::release);
            }
        });
    }

    // 先 join generators：它们的 f.get() 排空依赖 teleporter 持续 tick() 驱动异步存档加载完成。
    // generators 全部完成后置位 stop，teleporter 的 tick 循环才退出，最后 join teleporter。
    // （若先 join teleporter，其 tick 循环在 generators 仍在排空 future 时就停止 → 死锁。）
    for (auto& g : generators) {
        g.join();
    }
    stop.store(true, std::memory_order::release);
    teleporter.join();
    watchdog.join();

    // crashFlag 非 0 表示 generator/teleporter 线程捕获到异常（worker 线程的 ACCESS_VIOLATION
    // 会直接进程终止，不进入 catch；此处仅捕获逻辑层异常）。
    EXPECT_EQ(crashFlag.load(std::memory_order::acquire), 0) << "teleporter/generator thread caught an exception";

    m_manager->removePlayer(1);
    m_manager->shutdown();
    m_workerPool->shutdown();
}

// 完整跑图场景：玩家移动 + 异步请求生成，触发卸载/重生成循环。
// 对齐用户报告的"跑图时崩溃"场景：视距调大后更容易触发。
TEST_F(StructureReferencesRaceTest, MovePlayerAndGenerate_RaceRepro)
{
    m_workerPool->start();
    ASSERT_TRUE(m_manager->initialize().success());

    // 玩家初始位置在原点
    m_manager->updatePlayerPosition(1, 0.0, 0.0);
    for (int i = 0; i < 30; ++i) {
        m_manager->tick();
    }

    std::atomic<bool> stop{false};
    std::atomic<int> crashFlag{0};

    // 看门狗：检测死锁/挂起
    constexpr int STALL_THRESHOLD_SECONDS = 300;
    std::thread watchdog([&]() {
        int lastPending = -1;
        int noProgressSamples = 0;
        for (int sec = 0; sec < STALL_THRESHOLD_SECONDS * 2 && !stop.load(std::memory_order::acquire); ++sec) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            if (sec % 10 == 0) {
                const size_t pending = m_workerPool->pendingTaskCount();
                const size_t running = m_workerPool->runningTaskCount();
                const int pendingInt = static_cast<int>(pending);
                if (lastPending < 0 || pendingInt < lastPending) {
                    noProgressSamples = 0;
                } else {
                    ++noProgressSamples;
                }
                lastPending = pendingInt;
                spdlog::info("[watchdog] {}s: pending={}, running={}, noProgress={}",
                    sec / 2,
                    pending,
                    running,
                    noProgressSamples);
                if (noProgressSamples >= 6 && pending > 0 && running == 0) {
                    spdlog::info("[watchdog] deadlock: workers idle but pending={}", pending);
                    break;
                }
                // worker 池完全空闲（pending=0, running=0）但持续无进展：
                // 说明 generator 线程的 future.get() 永不完成（promise 泄漏），dump 卡住 holder 诊断根因。
                if (noProgressSamples >= 6 && pending == 0 && running == 0) {
                    spdlog::info("[watchdog] stall: workers idle, no in-flight tasks — dumping stuck holders");
                    m_manager->_debugDumpStuckHolders();
                    break;
                }
            }
        }
        if (!stop.load(std::memory_order::acquire)) {
            spdlog::info("[watchdog] stall > {}s, abort", STALL_THRESHOLD_SECONDS);
            std::abort();
        }
    });

    // 线程 A：移动玩家，触发票据级别变化 → 卸载/重生成
    constexpr int MOVE_ITERATIONS = 40;
    constexpr double MOVE_RADIUS_CHUNKS = 10.0;
    std::thread mover([&]() {
        try {
            for (int i = 0; i < MOVE_ITERATIONS && !stop.load(std::memory_order::acquire); ++i) {
                const double angle = (i % 32) * (3.14159265358979 / 16.0);
                const double radius = MOVE_RADIUS_CHUNKS * 16.0;
                const double wx = std::cos(angle) * radius;
                const double wz = std::sin(angle) * radius;
                m_manager->updatePlayerPosition(1, wx, wz);
                m_manager->tick();
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            // 移动循环结束后继续驱动 tick() 直到 generator 完成（stop 置位），避免 generator 的
            // in-flight future 因异步存档加载完成回调无人出队而永久挂起（与 0x50Repro 同因）。
            while (!stop.load(std::memory_order::acquire)) {
                m_manager->tick();
                std::this_thread::sleep_for(std::chrono::microseconds(200));
            }
        }
        catch (...) {
            crashFlag.store(1, std::memory_order::release);
        }
    });

    // 线程 B：异步请求生成区块（驱动 worker 执行 onChunkGenComplete）
    constexpr int GEN_ITERATIONS = 60;
    constexpr int MAX_INFLIGHT = 12;
    std::thread generator([&]() {
        try {
            std::vector<std::future<ChunkData*>> inflight;
            inflight.reserve(MAX_INFLIGHT + 1);
            for (int i = 0; i < GEN_ITERATIONS && !stop.load(std::memory_order::acquire); ++i) {
                const int x = (i * 7) % 32 - 16;
                const int z = (i * 13) % 32 - 16;
                auto future = m_manager->getChunkAsync(x, z, &ChunkStatuses::FULL);
                inflight.push_back(std::move(future));
                if (static_cast<int>(inflight.size()) > MAX_INFLIGHT) {
                    inflight.front().get();
                    inflight.erase(inflight.begin());
                }
                if (i % 8 == 0) {
                    (void)m_manager->getChunkSync(x, z);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
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

    // 先 join generator：其 f.get() 排空依赖 mover 持续 tick() 驱动异步存档加载完成。
    // generator 完成后置位 stop，mover 的 tick 循环才退出，最后 join mover。
    generator.join();
    stop.store(true, std::memory_order::release);
    mover.join();
    watchdog.join();

    EXPECT_EQ(crashFlag.load(std::memory_order::acquire), 0)
        << "Concurrent generate/move test detected an exception in worker thread";

    m_manager->removePlayer(1);
    m_manager->shutdown();
    m_workerPool->shutdown();
}
