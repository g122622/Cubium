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
#include "common/world/chunk/data/PalettedContainer.hpp"

#include "PerfettoProfilerAdapter.hpp"

#include <benchmark/benchmark.h>
#include <fmt/format.h>

#include <cstddef>
#include <vector>

using namespace mc;
using namespace mc::trace;
using namespace mc::world::chunk;

namespace {

// 每次迭代的随机访问次数：4×VOLUME，摊薄迭代级固定开销（trace 事件、计数器）。
constexpr i32 ACCESS_COUNT = PalettedContainer::VOLUME * 4;

// 固定随机种子：访问序列跨运行、跨 k 完全一致，保证各档位曲线可比。
constexpr u32 PATTERN_SEED = 0x9E3779B9u;

// 预热轮数：单轮 = ACCESS_COUNT 次随机访问。8 轮足以把工作集全部压进 L1/L2
// （k=4096 时位存储 6KB + 调色板 16KB + 哈希表 32KB + 访问序列 ≈ 120KB），
// 消除首轮冷缓存对测量口径的污染。
// 【重要】预热在测量循环之前执行：google/benchmark 只对 `for (auto _ : state)`
// 循环体内的迭代计时，循环之前的装配/预热一律不计入结果。
constexpr i32 WARMUP_ROUNDS = 8;

/**
 * @brief 预生成的随机访问序列（装配阶段完成，不计入测量口径）
 *
 * 索引在 0~VOLUME-1 上均匀随机；取值限定在 {1..k} 这 k 个值内，因此写入永远不会
 * 让调色板增长——测的是稳态随机读写，而不是调色板扩容/模式转换。
 */
struct AccessPattern {
    std::vector<i32> indices; ///< 随机线性索引
    std::vector<u32> values;  ///< 随机取值（写用例使用）
};

/// 生成随机访问序列（xorshift32，固定种子）。
[[nodiscard]] AccessPattern makeAccessPattern(i32 k)
{
    AccessPattern pattern;
    pattern.indices.reserve(static_cast<size_t>(ACCESS_COUNT));
    pattern.values.reserve(static_cast<size_t>(ACCESS_COUNT));

    u32 state = PATTERN_SEED;
    const auto nextRandom = [&state]() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    };

    for (i32 i = 0; i < ACCESS_COUNT; ++i) {
        pattern.indices.push_back(static_cast<i32>(nextRandom() % static_cast<u32>(PalettedContainer::VOLUME)));
        pattern.values.push_back(static_cast<u32>(nextRandom() % static_cast<u32>(k)) + 1);
    }
    return pattern;
}

/// 装配容器：把 VOLUME 个位置均匀填成 k 种取值（各约 VOLUME/k 个位置）。
///
/// 先 fill(1) 建立均匀态基线，再逐格 set()：k=1 时全部 set 命中唯一值、容器保持均匀态
/// （bits=0、无 storage）；k≥2 时首次 set 升到 1 位并建立反向哈希表，随后调色板按新值
/// 出现顺序增长到恰好 k 项、位宽升到 ceil(log2(k))。与生产路径（ChunkSection 逐格写入）
/// 一致，含真实的 _onResize 位宽提升。
void buildContainer(PalettedContainer& container, i32 k)
{
    container.fill(1);
    for (i32 index = 0; index < PalettedContainer::VOLUME; ++index) {
        container.set(index, static_cast<u32>(index % k) + 1);
    }
}

/// 追加公共计数器：元素种类数、位数、调色板大小、估算内存占用、预热规模。
void _addCommonCounters(::benchmark::State& state, i32 k, const PalettedContainer& container)
{
    state.counters["distinct_values"] = ::benchmark::Counter(static_cast<double>(k));
    state.counters["bits_per_entry"] = ::benchmark::Counter(static_cast<double>(container.bitsPerEntry()));
    state.counters["palette_size"] = ::benchmark::Counter(static_cast<double>(container.paletteSize()));
    state.counters["memory_bytes"] = ::benchmark::Counter(static_cast<double>(container.estimatedMemoryUsage()));
    // 预热规模（不计入结果，仅作口径记录）。
    state.counters["warmup_accesses"] =
        ::benchmark::Counter(static_cast<double>(ACCESS_COUNT) * static_cast<double>(WARMUP_ROUNDS));
}

/**
 * @brief 读预热：按同一随机序列读 WARMUP_ROUNDS 轮，把 storage / palette / 访问序列
 * 全部压进 L1/L2。调用点在测量循环之前，不计入结果。
 */
void warmUpReads(PalettedContainer& container, const AccessPattern& pattern)
{
    for (i32 round = 0; round < WARMUP_ROUNDS; ++round) {
        for (i32 i = 0; i < ACCESS_COUNT; ++i) {
            ::benchmark::DoNotOptimize(container.get(pattern.indices[static_cast<size_t>(i)]));
        }
    }
}

/**
 * @brief 写预热：按同一随机序列写 WARMUP_ROUNDS 轮（写入值取自当前调色板，不改变模式）。
 * 调用点在测量循环之前，不计入结果。
 */
void warmUpWrites(PalettedContainer& container, const AccessPattern& pattern)
{
    for (i32 round = 0; round < WARMUP_ROUNDS; ++round) {
        for (i32 i = 0; i < ACCESS_COUNT; ++i) {
            container.set(pattern.indices[static_cast<size_t>(i)], pattern.values[static_cast<size_t>(i)]);
        }
    }
    ::benchmark::ClobberMemory();
}

/**
 * @brief 随机读基准（按元素种类数量扫描位宽）
 *
 * 一次迭代 = 对固定随机索引序列做 4×VOLUME 次 get()。容器只有一种工作模式
 * （调色板 + 位压缩 + 反向哈希表），k 只决定位宽：k=1 为均匀态（bits=0、无 storage，
 * 直取 palette[0]），k≥2 时 bits = max(1, ceil(log2(k)))（k=2..4096 → 1..12 位）。
 *
 * 报告 reads_per_second（User Counter，迭代不变速率），并附带 bits_per_entry /
 * palette_size / memory_bytes 便于与内存一起权衡。
 */
void PalettedContainerRandomRead(::benchmark::State& state)
{
    const i32 k = static_cast<i32>(state.range(0));
    PalettedContainer container;
    buildContainer(container, k);
    const AccessPattern pattern = makeAccessPattern(k);

    // 预热（不计入测量）：让缓存就绪后再开始计时。
    warmUpReads(container, pattern);

    mc::benchmark::PerfettoProfilerAdapter::setCaseName(fmt::format("paletted_container_random_read/k={}", k));
    state.SetLabel(fmt::format("k={} bits={} palette={}", k, container.bitsPerEntry(), container.paletteSize()));

    for (auto _ : state) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Benchmark.Run, "PalettedContainer::randomRead");
        for (i32 i = 0; i < ACCESS_COUNT; ++i) {
            ::benchmark::DoNotOptimize(container.get(pattern.indices[static_cast<size_t>(i)]));
        }
    }

    state.counters["reads_per_second"] =
        ::benchmark::Counter(static_cast<double>(ACCESS_COUNT), ::benchmark::Counter::kIsIterationInvariantRate);
    _addCommonCounters(state, k, container);
}

/**
 * @brief 随机写基准（按元素种类数量扫描位宽）
 *
 * 一次迭代 = 对固定随机索引序列做 4×VOLUME 次 set()，写入值取自当前 k 种取值集合，
 * 因此调色板不会增长（测稳态写入：反向哈希表查找 + 位存储读改写，以及不同位宽/
 * 调色板驻留大小的成本）。
 */
void PalettedContainerRandomWrite(::benchmark::State& state)
{
    const i32 k = static_cast<i32>(state.range(0));
    PalettedContainer container;
    buildContainer(container, k);
    const AccessPattern pattern = makeAccessPattern(k);

    // 预热（不计入测量）：让缓存与写路径（含 HashMap 槽位）就绪后再开始计时。
    warmUpWrites(container, pattern);

    mc::benchmark::PerfettoProfilerAdapter::setCaseName(fmt::format("paletted_container_random_write/k={}", k));
    state.SetLabel(fmt::format("k={} bits={} palette={}", k, container.bitsPerEntry(), container.paletteSize()));

    for (auto _ : state) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Benchmark.Run, "PalettedContainer::randomWrite");
        for (i32 i = 0; i < ACCESS_COUNT; ++i) {
            container.set(pattern.indices[static_cast<size_t>(i)], pattern.values[static_cast<size_t>(i)]);
        }
        // 写入必须对后续读取可见，禁止编译器把整轮写入优化掉。
        ::benchmark::ClobberMemory();
    }

    state.counters["writes_per_second"] =
        ::benchmark::Counter(static_cast<double>(ACCESS_COUNT), ::benchmark::Counter::kIsIterationInvariantRate);
    _addCommonCounters(state, k, container);
}

/**
 * @brief 元素种类数量扫描点（横轴），共 32 档
 *
 * 覆盖均匀态（k=1，bits=0）与每个位宽档（bits 1..12，各取低/中/高）：
 * 既能看到"1 位/2 位/3 位"这些新支持的窄位宽（真实地形段的常见档位），也能对比非 2 的
 * 幂位宽（跨 u64 字读写）与 2 的幂位宽（bits=1/2/4/8，无跨字）的差异。
 *
 * 这里用展开宏而不是函数：`BENCHMARK()` 宏展开成一个**声明**（内部生成 static 变量，
 * 返回值不能作为表达式实参传递），因此只能把同一份清单展开进两个 BENCHMARK 声明，
 * 避免读/写用例各手写 32 项 Arg 链产生漂移。
 */
#define MC_PALETTED_KIND_ARGS(BM) \
    BM->Arg(1)                    \
        ->Arg(2)                  \
        ->Arg(3)                  \
        ->Arg(4)                  \
        ->Arg(6)                  \
        ->Arg(8)                  \
        ->Arg(12)                 \
        ->Arg(16)                 \
        ->Arg(17)                 \
        ->Arg(24)                 \
        ->Arg(32)                 \
        ->Arg(33)                 \
        ->Arg(48)                 \
        ->Arg(64)                 \
        ->Arg(65)                 \
        ->Arg(96)                 \
        ->Arg(128)                \
        ->Arg(129)                \
        ->Arg(192)                \
        ->Arg(256)                \
        ->Arg(257)                \
        ->Arg(384)                \
        ->Arg(512)                \
        ->Arg(513)                \
        ->Arg(768)                \
        ->Arg(1024)               \
        ->Arg(1025)               \
        ->Arg(1536)               \
        ->Arg(2048)               \
        ->Arg(2049)               \
        ->Arg(3072)               \
        ->Arg(4096)

} // namespace

MC_PALETTED_KIND_ARGS(BENCHMARK(PalettedContainerRandomRead)->Unit(::benchmark::kMicrosecond));
MC_PALETTED_KIND_ARGS(BENCHMARK(PalettedContainerRandomWrite)->Unit(::benchmark::kMicrosecond));

#undef MC_PALETTED_KIND_ARGS
