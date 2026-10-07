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

#include "common/profiler/TraceEvents.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkSection.hpp"
#include "common/world/chunk/data/light/SWMRNibbleArray.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/gen/RandomState.hpp"
#include "server/world/gen/biome/source/MultiNoiseBiomeSource.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "server/world/gen/density/DensityFunctionLoader.hpp"
#include "server/world/gen/noise/NoiseLoader.hpp"
#include "server/world/gen/settings/DimensionSettings.hpp"
#include "server/world/gen/settings/NoiseSettingsLoader.hpp"
#include "server/world/gen/settings/WorldPresetLoader.hpp"
#include "server/world/storage/SingleLevelStorageManager.hpp"

#include "common/core/GameDirectory.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

#include "MemoryProfiler.hpp"
#include "PerfettoProfilerAdapter.hpp"

#include <benchmark/benchmark.h>
#include <spdlog/spdlog.h>

#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <set>
#include <string>
#include <system_error>
#include <vector>

using namespace mc;
using namespace mc::server;
using namespace mc::trace;

namespace {

using mc::util::UniversalWorkerPool;
using mc::world::storage::SingleLevelStorageConfig;
using mc::world::storage::SingleLevelStorageManager;
namespace ChunkStatuses = mc::world::chunk::ChunkStatuses;

// 固定基准常量：seed 与中心区块固定，保证结果可复现、跨运行可比。
constexpr u64 BENCH_SEED = 12345;
constexpr i32 CENTER_CHUNK_X = 0;
constexpr i32 CENTER_CHUNK_Z = 0;

// 线程档位（->Arg 编码，arg(0) = 生成系统内部线程池大小）。
constexpr i32 THREAD_COUNTS[] = {1, 2, 4, 8};
// 批量大小档位（->Arg 编码，arg(1) = 边长 n，每迭代生成 n×n 区块）。
constexpr i32 BATCH_SIZES[] = {8, 16, 32};

// 单批生成泵送循环的墙钟超时上限。正常单批（最大 32×32）远低于此值；
// 超时即判定生成状态机卡死（promise 永不 fulfill），SkipWithError 中止，避免无限空转。
constexpr std::chrono::seconds PUMP_TIMEOUT{300};

/// 数据驱动注册表一次性加载守卫：RandomState::create 查 NoiseSettingsRegistry，
/// 必须先从原版数据包加载 Noises/DensityFunctions/NoiseSettings/WorldPresets。
/// 与 tests/unit/main.cpp 的 WorldGenRegistryEnvironment 同一加载顺序（依赖拓扑）。
void ensureWorldGenRegistriesLoaded()
{
    static const bool s_loaded = []() {
        mc::VanillaBlocks::initialize();

        const auto dataPackDir = mc::GameDirectory::defaultDirectory().dataPacksDir();
        if (!std::filesystem::exists(dataPackDir)) {
            return false;
        }
        mc::resource::DataPackRepository repo;
        auto scanResult = repo.scanDirectory(dataPackDir);
        if (!scanResult.success() || scanResult.value() == 0) {
            return false;
        }

        // 加载顺序：noise → density_function → noise_settings → world_preset（依赖拓扑，
        // 与 tests/unit/main.cpp 一致）。flat preset 对 chunk_generation 基准非必需，跳过。
        (void)mc::world::gen::noise::NoiseLoader::loadFromDataPackRepository(repo);
        (void)mc::world::gen::density::DensityFunctionLoader::loadFromDataPackRepository(repo);
        (void)mc::world::gen::settings::NoiseSettingsLoader::loadFromDataPackRepository(repo);
        (void)mc::world::gen::settings::WorldPresetLoader::loadFromDataPackRepository(repo);
        return true;
    }();
    if (!s_loaded) {
        spdlog::error("chunk_generation: worldgen datapack registries failed to load; "
                      "RandomState::create will assert (datapacks missing at ~/minecraft_reborn?)");
    }
}

/// 每线程档位的装配状态（ChunkGeneration benchmark 的 fixture，在 Setup 回调中重建）。
struct ChunkGenFixture {
    std::unique_ptr<UniversalWorkerPool> workerPool;
    std::unique_ptr<SingleLevelStorageManager> storage;
    std::filesystem::path storageDir;
    std::unique_ptr<ServerWorld> world;
    ServerChunkManager* manager = nullptr; // 所有权在 world（setChunkManager 转移）

    void build(i32 threadCount)
    {
        VanillaBlocks::initialize();

        // 生成线程池：线程数即本档位的被测变量。rankBase 与生产 ServerCompute(100) 分组一致。
        workerPool = std::make_unique<UniversalWorkerPool>(threadCount, "BenchCompute", 100);
        workerPool->start();

        ServerWorldConfig config;
        config.seed = BENCH_SEED;
        config.viewDistance = 8;
        world = std::make_unique<ServerWorld>(config);

        // 内存态临时存档（ RocksDB 临时目录，不参与耗时测量口径的核心路径——
        // 全新目录下所有区块都是存档缺失→纯生成路径，且基准结束后整体删除）。
        std::error_code ec;
        storageDir = std::filesystem::temp_directory_path(ec) / "mc_bench_chunkgen";
        std::filesystem::remove_all(storageDir, ec);
        std::filesystem::create_directories(storageDir, ec);
        storage = std::make_unique<SingleLevelStorageManager>();
        auto openResult = storage->open(storageDir, SingleLevelStorageConfig{});
        if (openResult.failed()) {
            spdlog::error("chunk_generation: failed to open temp storage: {}", openResult.error().message());
            return;
        }

        world->setSharedStorage(storage.get());

        auto settings = mc::DimensionSettings::overworld();
        auto randomState = mc::world::gen::RandomState::create(settings, BENCH_SEED);
        auto biomeSource = mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        auto generator = std::make_unique<mc::NoiseChunkGenerator>(
            std::move(settings), std::move(biomeSource), std::move(randomState));
        auto chunkManager = std::make_unique<ServerChunkManager>(*world, std::move(generator));
        chunkManager->setWorkerPool(workerPool.get());
        manager = chunkManager.get();
        world->setChunkManager(std::move(chunkManager));

        (void)world->initialize();
        (void)manager->initialize();

        // 基准不注册持久票据，距离图无合法源；若允许自动卸载，"卸载→UNLOAD_COOLDOWN 票据→
        // 距离图源→扩散创建→再卸载"会形成正反馈环，淹没生成吞吐测量。抑制自动卸载候选判定，
        // 仅在 PauseTiming 区间手动 unloadChunkSync。
        manager->setSuppressAutoUnloadForBenchmark(true);
    }

    void destroy()
    {
        if (manager != nullptr) {
            manager->shutdown();
            manager = nullptr;
        }
        if (workerPool != nullptr) {
            workerPool->shutdown();
        }
        world.reset();
        if (storage != nullptr) {
            storage->close();
            storage.reset();
        }
        std::error_code ec;
        std::filesystem::remove_all(storageDir, ec);
        workerPool.reset();
    }
};

/// 当前 fixture（Setup/Teardown 回调与 benchmark 函数之间经此共享；
/// google/benchmark 单线程提交模型下无并发访问）。
ChunkGenFixture g_fixture;

void chunkGenSetup(const ::benchmark::State& state)
{
    ensureWorldGenRegistriesLoaded();
    g_fixture.build(static_cast<i32>(state.range(0)));
}

void chunkGenTeardown(const ::benchmark::State&)
{
    g_fixture.destroy();
}

// ============================================================================
// TODO(临时诊断): 生成结束后把整批区块的全部 section 调色板位数导出为 CSV，
// TODO(临时诊断): 用于评估各 section 的位宽/模式分布；定位完成后删除本段与调用点。
// ============================================================================

/// 导出当前已加载区块的 section 调色板信息到
/// `benchmark_results/palette_bits/chunk_palette_bits_threads=<N>_batch=<side>.csv`。
///
/// 列：chunk_x,chunk_z,section_index,section_min_y,present,bits_per_entry,palette_size
/// （未创建的段 present=0、bits=-1；palette_size 为唯一值个数）
///
/// 调用前提：本批生成已全部完成（所有 future 就绪）、且处于 PauseTiming 区间，
/// worker 不再触碰这些区块，故直接读 section 快照。
void dumpSectionPaletteBits(const mc::server::ServerChunkManager& manager, i32 threadCount, i32 side)
{
    const std::filesystem::path outputPath = std::filesystem::current_path() / "benchmark_results" / "palette_bits" /
        fmt::format("chunk_palette_bits_threads={}_batch={}.csv", threadCount, side);

    std::error_code ec;
    std::filesystem::create_directories(outputPath.parent_path(), ec);
    std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        spdlog::warn("chunk_palette_bits: failed to open {}", outputPath.string());
        return;
    }

    output << "chunk_x,chunk_z,section_index,section_min_y,present,bits_per_entry,palette_size,container_bytes\n";

    size_t chunkCount = 0;
    size_t presentSectionCount = 0;
    size_t containerBytesTotal = 0;
    manager.forEachLoadedChunk([&](const mc::ChunkData& chunk) {
        ++chunkCount;
        const std::array<const mc::world::chunk::ChunkSection*, mc::world::CHUNK_SECTIONS> sections =
            chunk.getSections();
        for (i32 sectionIndex = 0; sectionIndex < mc::world::CHUNK_SECTIONS; ++sectionIndex) {
            const i32 sectionMinY = mc::world::MIN_BUILD_HEIGHT + sectionIndex * mc::world::CHUNK_SECTION_HEIGHT;
            const mc::world::chunk::ChunkSection* section = sections[static_cast<size_t>(sectionIndex)];
            if (section == nullptr) {
                output << chunk.x() << ',' << chunk.z() << ',' << sectionIndex << ',' << sectionMinY << ",0,-1,0,0\n";
                continue;
            }
            const mc::world::chunk::PalettedContainer& container = section->blockStates();
            output << chunk.x() << ',' << chunk.z() << ',' << sectionIndex << ',' << sectionMinY << ",1,"
                   << container.bitsPerEntry() << ',' << container.paletteSize() << ','
                   << container.estimatedMemoryUsage() << '\n';
            ++presentSectionCount;
            containerBytesTotal += container.estimatedMemoryUsage();
        }
        return true;
    });

    output.close();
    spdlog::info("chunk_palette_bits: {} chunks / {} sections, container_bytes={:.2f}MB -> {}",
        chunkCount,
        presentSectionCount,
        static_cast<double>(containerBytesTotal) / 1048576.0,
        outputPath.string());
}

// ============================================================================
// TODO(临时诊断): 统计 nibble（天空光/方块光）的缓冲分配量，用于评估"常量态"
// TODO(临时诊断): 优化的上限；定位完成后删除本段与调用点。
// ============================================================================

/// 统计当前已加载区块全部 nibble 缓冲的**实际物化量**，输出到
/// `benchmark_results/nibble_stats/nibble_stats_threads=<N>_batch=<side>.csv`。
///
/// 列：chunk_x,chunk_z,layer(0=sky,1=block),light_section_index,state_visible,
///     has_storage,materialized_bytes,all_full,all_zero
///
/// 【为何需要本统计】`SWMRNibbleArray` 每段 2048 B，天空光 + 方块光共 2×26=52 段/区块，
/// 理论满配 106 KB/区块。但 nibble 是延迟分配的——`setFull()`/`setZero()` 会**立即物化**
/// 2048 B 实体缓冲（不像原版 `DataLayer.fill(v)` 只设默认值并把 data 置 null）。
/// 本统计给出"实际物化了多少 B"与"其中全 15 / 全 0 的占比"，即常量态优化的收益上限。
///
/// 调用前提同 dumpSectionPaletteBits：本批生成已全部完成、处于 PauseTiming 区间。
void dumpNibbleStats(const mc::server::ServerChunkManager& manager, i32 threadCount, i32 side)
{
    const std::filesystem::path outputPath = std::filesystem::current_path() / "benchmark_results" / "nibble_stats" /
        fmt::format("nibble_stats_threads={}_batch={}.csv", threadCount, side);

    std::error_code ec;
    std::filesystem::create_directories(outputPath.parent_path(), ec);
    std::ofstream output(outputPath, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        spdlog::warn("nibble_stats: failed to open {}", outputPath.string());
        return;
    }

    output << "chunk_x,chunk_z,layer,light_section_index,state_visible,has_storage,materialized_bytes,all_full,"
              "all_zero\n";

    size_t totalBytes = 0;
    size_t totalFull = 0;
    size_t totalZero = 0;
    size_t materializedCount = 0;
    size_t slotCount = 0;

    manager.forEachLoadedChunk([&](const mc::ChunkData& chunk) {
        for (i32 layer = 0; layer < 2; ++layer) {
            const auto& nibbles = (layer == 0) ? chunk.skyNibbles() : chunk.blockNibbles();
            for (i32 lightSection = 0; lightSection < mc::ChunkData::LIGHT_SECTIONS; ++lightSection) {
                ++slotCount;
                const mc::SWMRNibbleArray& nibble = nibbles[static_cast<size_t>(lightSection)];

                // 用 toByteArray() 判断实际物化：Null/Uninit 返回空 vector（零字节）；
                // 否则返回 2048 字节副本（注意这是可见侧的快照）。
                const std::vector<u8> bytes = nibble.toByteArray();
                const bool materialized = !bytes.empty();
                const size_t byteCount = bytes.size();
                bool allFull = materialized;
                bool allZero = materialized;
                if (materialized) {
                    for (const u8 b : bytes) {
                        if (b != 0xFF) {
                            allFull = false;
                        }
                        if (b != 0x00) {
                            allZero = false;
                        }
                        if (!allFull && !allZero) {
                            break;
                        }
                    }
                }

                if (materialized) {
                    totalBytes += byteCount;
                    ++materializedCount;
                    if (allFull) {
                        ++totalFull;
                    }
                    if (allZero) {
                        ++totalZero;
                    }
                }

                output << chunk.x() << ',' << chunk.z() << ',' << layer << ',' << lightSection << ','
                       << (nibble.isNullVisible() ? "Null" : (nibble.isUninitializedVisible() ? "Uninit" : "Init"))
                       << ',' << (materialized ? 1 : 0) << ',' << byteCount << ',' << (allFull ? 1 : 0) << ','
                       << (allZero ? 1 : 0) << '\n';
            }
        }
        return true;
    });

    output.close();
    spdlog::info("nibble_stats: slots={} materialized={} bytes={:.2f}MB allFull={} allZero={} -> {}",
        slotCount,
        materializedCount,
        static_cast<double>(totalBytes) / 1048576.0,
        totalFull,
        totalZero,
        outputPath.string());
}

/**
 * @brief 区块生成吞吐基准（生产级并行生成系统）
 *
 * 走完整的生产链路：ServerChunkManager 请求聚合 → ChunkTaskScheduler 邻居依赖
 * 调度（ReentrantAreaLock 区域锁）→ UniversalWorkerPool 区域互斥执行 →
 * ChunkProgressionTask 逐状态推进 → FULL 完成回收。
 *
 * 测量口径：一次迭代 = 向生成系统提交一批（边长 n 的方形区域，n×n 个区块）
 * FULL 生成请求，阻塞等待全部完成。报告 chunks/sec 吞吐。
 *
 * 参数：arg(0) = 生成系统内部线程池大小（1/2/4/8）；arg(1) = 批量边长（8/16/32）。
 * 基准进程本身始终单线程提交（google/benchmark 未用 ->Threads），线程变量完全
 * 通过生成系统内部线程池大小施加。
 */
void ChunkGeneration(::benchmark::State& state)
{
    const i32 side = static_cast<i32>(state.range(1));
    const i32 totalChunks = side * side;
    const i32 half = side / 2;

    // 设置本用例的 Perfetto trace 文件名主干（setup/teardown 边界经适配器读取）。
    mc::benchmark::PerfettoProfilerAdapter::setCaseName(
        fmt::format("chunk_generation/threads={}/batch={}", state.range(0), side));

    // TODO(临时诊断): 每个 (threads, batch) 档位每个进程只导出一次 CSV。
    // 注册了 MemoryManager 后 google/benchmark 会把基准函数跑两遍（内存指标一遍 + 计时一遍），
    // 用进程级静态集合去重，避免同一文件被重复写、日志出现两行。
    static std::set<std::string> s_dumpedPaletteBits;
    static std::set<std::string> s_dumpedNibbleStats;
    const std::string paletteBitsKey = fmt::format("{}x{}", state.range(0), side);
    bool paletteBitsDumped = s_dumpedPaletteBits.contains(paletteBitsKey);
    bool nibbleStatsDumped = s_dumpedNibbleStats.contains(paletteBitsKey);

    for (auto _ : state) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Benchmark.Run, "ChunkGeneration::batch");

        // 提交一批 FULL 请求（以中心区块为几何中心的 n×n 区域），收集 future。
        std::vector<std::future<mc::ChunkData*>> futures;
        futures.reserve(static_cast<size_t>(totalChunks));
        for (i32 dx = -half; dx < half; ++dx) {
            for (i32 dz = -half; dz < half; ++dz) {
                futures.push_back(g_fixture.manager->requestChunkAsync(
                    CENTER_CHUNK_X + dx, CENTER_CHUNK_Z + dz, ChunkStatuses::FULL));
            }
        }

        // 生产语义的驱动模型：主线程 tick 泵送 + worker 池并行生成。
        // ServerChunkManager 的线程模型要求主线程 tick 驱动调度推进
        // （_drainPendingLoadCompletes/_drainPendingPostProcess 均只在 tick 中出队，
        // requestChunkSync 的同步等待也是靠主动 pump 这一队列实现的）。异步批量提交
        // 后只等 future 不 tick 会死锁（存档解析完成回调永不交接）。故这里复现生产
        // 主循环节拍：未全部完成前持续 tick()，这是"一帧内提交的生成负载"的真实口径。
        //
        // 墙钟超时守卫：退出条件是全部 future 就绪，若某个请求的 promise 永不 fulfill
        // （生成状态机卡在中间态 / 完成信号丢失），本循环会永久空转（主线程满速 tick、
        // results.json 永不追加）。正常单批生成耗时远低于此上限，超时即判定卡死并
        // SkipWithError 中止，把"卡死"变成可诊断的失败而非无限空转。
        const auto pumpStart = std::chrono::steady_clock::now();
        auto lastProgressLog = pumpStart;
        size_t lastLoggedRemaining = futures.size();
        size_t remaining = futures.size();
        std::vector<bool> done(futures.size(), false);
        while (remaining > 0) {
            // 先收割已完成的请求（非阻塞），全部完成则退出泵送循环。
            for (size_t i = 0; i < futures.size(); ++i) {
                if (!done[i] && futures[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    done[i] = true;
                    --remaining;
                }
            }
            if (remaining == 0) {
                break;
            }
            const auto now = std::chrono::steady_clock::now();
            // 每 5 秒打印一次进度：用于区分"硬卡死"（remaining 恒定）与"渐进收敛"（remaining 缓慢下降）。
            if (now - lastProgressLog > std::chrono::seconds(5)) {
                const auto elapsedS = std::chrono::duration_cast<std::chrono::seconds>(now - pumpStart).count();
                spdlog::info("chunk_generation: progress t={}s remaining={}/{} (delta {} since last log)",
                    elapsedS,
                    remaining,
                    futures.size(),
                    lastLoggedRemaining - remaining);
                lastLoggedRemaining = remaining;
                lastProgressLog = now;
            }
            if (now - pumpStart > PUMP_TIMEOUT) {
                state.PauseTiming();
                spdlog::error("chunk_generation: pump loop timed out after {}s with {} / {} futures still pending",
                    std::chrono::duration_cast<std::chrono::seconds>(PUMP_TIMEOUT).count(),
                    remaining,
                    futures.size());
                state.ResumeTiming();
                state.SkipWithError("chunk generation stalled: pump loop timeout");
                return;
            }
            g_fixture.manager->tick();
        }

        // 校验全部请求成功；任何失败（nullptr）都视为基准数据无效。
        for (auto& future : futures) {
            if (ChunkData* chunk = future.get(); chunk == nullptr) {
                state.SkipWithError("chunk generation returned null chunk");
                return;
            }
        }

        // TODO(临时诊断): 首次迭代生成完成后导出整批区块的全部 section 调色板位数。
        // 放在 PauseTiming 区间内：CSV 的 I/O 不进入测量口径。
        if (!paletteBitsDumped) {
            state.PauseTiming();
            dumpSectionPaletteBits(*g_fixture.manager, static_cast<i32>(state.range(0)), side);
            state.ResumeTiming();
            s_dumpedPaletteBits.insert(paletteBitsKey);
            paletteBitsDumped = true;
        }

        // TODO(临时诊断): 同一次快照上导出 nibble 缓冲统计（常量态优化的收益上限）。
        if (!nibbleStatsDumped) {
            state.PauseTiming();
            dumpNibbleStats(*g_fixture.manager, static_cast<i32>(state.range(0)), side);
            state.ResumeTiming();
            s_dumpedNibbleStats.insert(paletteBitsKey);
            nibbleStatsDumped = true;
        }

        // TODO(临时诊断): MC_BENCH_FREEZE_STEADY=1 时在稳态快照后立刻 _Exit(0)（跳过全部析构与
        // TODO(临时诊断): atexit），使 heaptrack 的"未释放"报告即稳态存活分配的真实构成。
        // TODO(临时诊断): 用于给稳态 104.8 MB 做逐项归因（massif 的 detailed 子树在本版本不可用）。
        if (std::getenv("MC_BENCH_FREEZE_STEADY") != nullptr) {
            std::_Exit(0);
        }

        // 卸载本批区块（计时暂停区间内，不计入测量）：否则第二轮迭代起全部命中
        // 内存缓存，测的是缓存查询而非生成。逐区块 unloadChunkSync（存档命中已清空
        // 且未修改，生成区块标记脏会触发一次落盘——位于暂停区间，不影响口径），
        // 随后 tick 排空异步卸载收尾队列，等待内存缓存清空（有界轮询防死循环）。
        state.PauseTiming();
        for (i32 dx = -half; dx < half; ++dx) {
            for (i32 dz = -half; dz < half; ++dz) {
                g_fixture.manager->unloadChunkSync(CENTER_CHUNK_X + dx, CENTER_CHUNK_Z + dz);
            }
        }
        for (int drainTick = 0; drainTick < 1000 && g_fixture.manager->loadedChunkCount() > 0; ++drainTick) {
            g_fixture.manager->tick();
        }
        state.ResumeTiming();
    }

    state.counters["chunks_per_second"] =
        ::benchmark::Counter(static_cast<double>(totalChunks), ::benchmark::Counter::kIsIterationInvariantRate);
    state.counters["chunks"] = ::benchmark::Counter(static_cast<double>(totalChunks));
    state.counters["threads"] = ::benchmark::Counter(static_cast<double>(state.range(0)));
}

} // namespace

BENCHMARK(ChunkGeneration)
    ->Args({1, 8})
    ->Args({2, 8})
    ->Args({4, 8})
    ->Args({8, 8})
    ->Args({1, 16})
    ->Args({2, 16})
    ->Args({4, 16})
    ->Args({8, 16})
    ->Args({1, 32})
    ->Args({2, 32})
    ->Args({4, 32})
    ->Args({8, 32})
    ->Setup(chunkGenSetup)
    ->Teardown(chunkGenTeardown)
    ->Unit(::benchmark::kMillisecond);
