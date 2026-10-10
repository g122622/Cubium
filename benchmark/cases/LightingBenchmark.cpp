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
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "server/world/lighting/engine/BlockLightEngine.hpp"
#include "server/world/lighting/manager/WorldLightManager.hpp"

#include "MemoryProfiler.hpp"
#include "PerfettoProfilerAdapter.hpp"

#include <benchmark/benchmark.h>

using namespace mc;
using namespace mc::trace;

namespace {

using mc::BlockPos;
using mc::ChunkData;
using mc::ChunkLoadStatus;
using mc::SectionPos;
using mc::StarLightLightingProvider;
using mc::WorldLightManager;
using mc::world::CHUNK_SHIFT;
using mc::world::CHUNK_WIDTH;

/// 光照基准 provider：只服务单个 ChunkData（与生产 RuntimeLightingProvider 的
/// 坐标路由语义一致，但无 5×5 保活与脏区收集——基准只测引擎本体）。
class BenchmarkLightProvider final : public StarLightLightingProvider {
public:
    explicit BenchmarkLightProvider(ChunkData* chunk)
        : m_chunk(chunk)
    {}

    mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) override
    {
        if (m_chunk != nullptr && m_chunk->x() == x && m_chunk->z() == z) {
            return m_chunk;
        }
        return nullptr;
    }

    const mc::IChunk* getChunkForLight(mc::ChunkCoord x, mc::ChunkCoord z) const override
    {
        if (m_chunk != nullptr && m_chunk->x() == x && m_chunk->z() == z) {
            return m_chunk;
        }
        return nullptr;
    }

    const mc::BlockState* getBlockStateForLight(const BlockPos& pos) const override
    {
        if (m_chunk == nullptr) {
            return nullptr;
        }
        return m_chunk->getBlockState(pos.x & 0xF, pos.y, pos.z & 0xF);
    }

    mc::IWorld* getWorld() override { return nullptr; }
    const mc::IWorld* getWorld() const override { return nullptr; }
    void markLightChanged(mc::LightType, const SectionPos&) override {}
    bool hasSkyLight() const override { return false; }
    i32 getMinBuildHeight() const override { return mc::world::MIN_BUILD_HEIGHT; }
    i32 getMaxBuildHeight() const override { return mc::world::MAX_BUILD_HEIGHT; }
    i32 getSectionCount() const override
    {
        return (mc::world::MAX_BUILD_HEIGHT - mc::world::MIN_BUILD_HEIGHT) >> 4;
    }

private:
    ChunkData* m_chunk;
};

// 光照基准常量：单区块 (0,0)，光源层的 Y 坐标取世界中部（典型地表附近）。
constexpr mc::ChunkCoord BENCH_CHUNK_X = 0;
constexpr mc::ChunkCoord BENCH_CHUNK_Z = 0;
constexpr i32 BENCH_LIGHT_Y = 70;

/**
 * @brief 光照引擎本体基准（批量方块光更新）
 *
 * 直接驱动 TLS 方块光引擎（acquireBlockLightEngine → blocksChangedInChunk →
 * release），与生产运行时路径的引擎调用形态一致，但不经过 ServerLightQueue/
 * RuntimeLightTask 等 tick 级调度（那属于系统吞吐，由 chunk_generation 间接覆盖）。
 *
 * 测量口径：一次迭代 = 对 (0,0) 区块的一个 16×16 截面层交替放置/移除光源
 * （GLOWSTONE / AIR），共 CHUNK_WIDTH² 次方块光变更。报告 blocks_per_second。
 *
 * 放置与移除交替：保证每轮迭代的光照传播工作量稳定（纯移除或纯放置会让
 * 后续迭代的状态单调趋同，传播范围逐渐缩小，吞吐虚高）。
 */
void Lighting(::benchmark::State& state)
{
    VanillaBlocks::initialize();

    auto chunk = std::make_unique<ChunkData>(BENCH_CHUNK_X, BENCH_CHUNK_Z);
    chunk->setStatus(ChunkLoadStatus::Generated);
    BenchmarkLightProvider provider(chunk.get());

    // 预填：在基准层摆满光源并传播一次，使迭代开始前光照场处于"满光源"稳态。
    chunk->setBlockState(8, BENCH_LIGHT_Y, 8, &mc::VanillaBlocks::GLOWSTONE->defaultState());
    auto* engine = WorldLightManager::acquireBlockLightEngine();
    engine->updateSectionStatus(SectionPos(static_cast<i32>(BENCH_CHUNK_X), BENCH_LIGHT_Y >> 4,
                                    static_cast<i32>(BENCH_CHUNK_Z)),
        false);
    engine->light(&provider, chunk.get(), false);
    WorldLightManager::releaseBlockLightEngine(engine);

    // 每次迭代的方块位置列表：整个 16×16 截面层。
    std::vector<BlockPos> positions;
    positions.reserve(static_cast<size_t>(CHUNK_WIDTH * CHUNK_WIDTH));
    for (i32 x = 0; x < CHUNK_WIDTH; ++x) {
        for (i32 z = 0; z < CHUNK_WIDTH; ++z) {
            positions.emplace_back(BENCH_CHUNK_X * CHUNK_WIDTH + x, BENCH_LIGHT_Y, BENCH_CHUNK_Z * CHUNK_WIDTH + z);
        }
    }

    // 截面层对应的 section 索引位图（blocksChangedInChunk 的 changedSections 参数）。
    std::vector<bool> changedSections(
        static_cast<size_t>((mc::world::MAX_BUILD_HEIGHT - mc::world::MIN_BUILD_HEIGHT) >> 4), false);
    changedSections[static_cast<size_t>((BENCH_LIGHT_Y - mc::world::MIN_BUILD_HEIGHT) >> 4)] = true;

    // 光源放置/移除交替：奇数迭代放 GLOWSTONE，偶数迭代放 AIR（移除）。
    bool placeLights = true;
    constexpr i32 CHUNK_WIDTH_I64 = CHUNK_WIDTH;

    // 设置本用例的 Perfetto trace 文件名主干。
    mc::benchmark::PerfettoProfilerAdapter::setCaseName("lighting");

    for (auto _ : state) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Benchmark.Run, "Lighting::layerBatch");

        auto* tlsEngine = WorldLightManager::acquireBlockLightEngine();
        if (placeLights) {
            tlsEngine->blocksChangedInChunk(&provider,
                static_cast<i32>(BENCH_CHUNK_X),
                static_cast<i32>(BENCH_CHUNK_Z),
                positions,
                changedSections);
        } else {
            // 移除：同位置集合作废光源（坐标相同，引擎读当前方块状态发现变空即衰减）。
            // 逐位置把方块换成 AIR 后统一提交。
            tlsEngine->blocksChangedInChunk(&provider,
                static_cast<i32>(BENCH_CHUNK_X),
                static_cast<i32>(BENCH_CHUNK_Z),
                positions,
                changedSections);
        }
        WorldLightManager::releaseBlockLightEngine(tlsEngine);

        // 翻转光照场：本轮结束把整层方块在 GLOWSTONE/AIR 之间切换，保证下一轮
        // 传播工作量与上一轮互补且总量稳定。
        const mc::BlockState* nextState =
            placeLights ? &mc::VanillaBlocks::AIR->defaultState() : &mc::VanillaBlocks::GLOWSTONE->defaultState();
        for (const auto& pos : positions) {
            chunk->setBlockState(pos.x & 0xF, pos.y, pos.z & 0xF, nextState);
        }
        placeLights = !placeLights;
    }

    const f64 blocksPerIteration = static_cast<f64>(CHUNK_WIDTH_I64 * CHUNK_WIDTH);
    state.counters["blocks_per_second"] =
        ::benchmark::Counter(blocksPerIteration, ::benchmark::Counter::kIsIterationInvariantRate);
    state.counters["blocks_per_iteration"] = ::benchmark::Counter(blocksPerIteration);
}

} // namespace

BENCHMARK(Lighting)->Unit(::benchmark::kMicrosecond);
