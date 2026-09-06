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

/**
 * @file PerlinNoisePerfTest.cpp
 * @brief PerlinNoise / BlendedNoise 采样性能微基准测试
 *
 * 用于评估 interleave_count 等 SIMD pragma 调参对噪声采样吞吐量的影响。
 * 直接调用 getValue / compute 大量次数，用 std::chrono::steady_clock 测量墙钟耗时，
 * 输出 ns/op 供前后对比。用 volatile sink 防止编译器死码消除。
 */

#include "common/core/Types.hpp"
#include "common/world/gen/density/BlendedNoise.hpp"
#include "common/world/gen/noise/PerlinNoise.hpp"

#include <chrono>
#include <cstdio>
#include <vector>
#include <gtest/gtest.h>

namespace mc {
namespace {

using world::gen::density::BlendedNoise;
using world::gen::noise::PerlinNoise;

/// 重复调用 noise.getValue，返回总耗时（纳秒）。
[[nodiscard]] i64 benchGetValue(const PerlinNoise& noise, int iterations)
{
    volatile f64 sink = 0.0;
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        const f64 x = static_cast<f64>(i) * 0.137;
        const f64 y = static_cast<f64>(i) * 0.231;
        const f64 z = static_cast<f64>(i) * 0.373;
        sink += noise.getValue(x, y, z);
    }
    const auto t1 = std::chrono::steady_clock::now();
    // 防止整个循环被优化掉：如果 sink 是 NaN 就强制写入 benchmark 框架。
    if (sink != sink) {
        EXPECT_FALSE(true) << "sink is NaN";
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
}

/// 重复调用 noise.getValueScalar（纯标量路径），返回总耗时（纳秒）。
[[nodiscard]] i64 benchGetValueScalar(const PerlinNoise& noise, int iterations)
{
    volatile f64 sink = 0.0;
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        const f64 x = static_cast<f64>(i) * 0.137;
        const f64 y = static_cast<f64>(i) * 0.231;
        const f64 z = static_cast<f64>(i) * 0.373;
        sink += noise.getValueScalar(x, y, z);
    }
    const auto t1 = std::chrono::steady_clock::now();
    if (sink != sink) {
        EXPECT_FALSE(true) << "sink is NaN";
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
}

/// 重复调用 noise.compute，返回总耗时（纳秒）。
[[nodiscard]] i64 benchCompute(const BlendedNoise& noise, int iterations)
{
    volatile f64 sink = 0.0;
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        const i32 bx = i * 7 - 3;
        const i32 by = i * 13 + 32;
        const i32 bz = i * 11 - 5;
        sink += noise.compute(bx, by, bz);
    }
    const auto t1 = std::chrono::steady_clock::now();
    if (sink != sink) {
        EXPECT_FALSE(true) << "sink is NaN";
    }
    return std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
}

// 4-octave: 覆盖 NormalNoise 常见配置
TEST(PerlinNoisePerfTest, GetValue4Octave)
{
    const PerlinNoise noise(0ULL, -2, {1.0, 1.0, 1.0, 1.0});
    constexpr int kIters = 2'000'000;
    const i64 ns = benchGetValue(noise, kIters);
    std::printf("[PERF] GetValue4Octave: %lld ns/op (%d iters)\n", static_cast<long long>(ns / kIters), kIters);
    RecordProperty("ns_per_op", std::to_string(ns / kIters));
    RecordProperty("total_ns", std::to_string(ns));
    RecordProperty("iterations", std::to_string(kIters));
}

// 8-octave: 覆盖 BlendedNoise mainNoise 配置
TEST(PerlinNoisePerfTest, GetValue8Octave)
{
    const PerlinNoise noise(12345ULL, -7, std::vector<f64>(8, 1.0));
    constexpr int kIters = 2'000'000;
    const i64 ns = benchGetValue(noise, kIters);
    std::printf("[PERF] GetValue8Octave: %lld ns/op (%d iters)\n", static_cast<long long>(ns / kIters), kIters);
    RecordProperty("ns_per_op", std::to_string(ns / kIters));
    RecordProperty("total_ns", std::to_string(ns));
    RecordProperty("iterations", std::to_string(kIters));
}

// 16-octave: 覆盖 BlendedNoise min/maxNoise 配置
TEST(PerlinNoisePerfTest, GetValue16Octave)
{
    const PerlinNoise noise(999ULL, -15, std::vector<f64>(16, 1.0));
    constexpr int kIters = 1'000'000;
    const i64 ns = benchGetValue(noise, kIters);
    std::printf("[PERF] GetValue16Octave: %lld ns/op (%d iters)\n", static_cast<long long>(ns / kIters), kIters);
    RecordProperty("ns_per_op", std::to_string(ns / kIters));
    RecordProperty("total_ns", std::to_string(ns));
    RecordProperty("iterations", std::to_string(kIters));
}

// BlendedNoise::compute: 完整密度计算路径
TEST(PerlinNoisePerfTest, BlendedNoiseCompute)
{
    const BlendedNoise noise(0ULL, 0.25, 0.125, 80.0, 160.0, 8.0);
    constexpr int kIters = 500'000;
    const i64 ns = benchCompute(noise, kIters);
    std::printf("[PERF] BlendedNoiseCompute: %lld ns/op (%d iters)\n", static_cast<long long>(ns / kIters), kIters);
    RecordProperty("ns_per_op", std::to_string(ns / kIters));
    RecordProperty("total_ns", std::to_string(ns));
    RecordProperty("iterations", std::to_string(kIters));
}

// === 标量对比基准 ===
TEST(PerlinNoisePerfTest, GetValueScalar4Octave)
{
    const PerlinNoise noise(0ULL, -2, {1.0, 1.0, 1.0, 1.0});
    constexpr int kIters = 2'000'000;
    const i64 ns = benchGetValueScalar(noise, kIters);
    std::printf("[PERF] GetValueScalar4Octave: %lld ns/op (%d iters)\n", static_cast<long long>(ns / kIters), kIters);
    RecordProperty("ns_per_op", std::to_string(ns / kIters));
    RecordProperty("total_ns", std::to_string(ns));
    RecordProperty("iterations", std::to_string(kIters));
}

TEST(PerlinNoisePerfTest, GetValueScalar8Octave)
{
    const PerlinNoise noise(12345ULL, -7, std::vector<f64>(8, 1.0));
    constexpr int kIters = 2'000'000;
    const i64 ns = benchGetValueScalar(noise, kIters);
    std::printf("[PERF] GetValueScalar8Octave: %lld ns/op (%d iters)\n", static_cast<long long>(ns / kIters), kIters);
    RecordProperty("ns_per_op", std::to_string(ns / kIters));
    RecordProperty("total_ns", std::to_string(ns));
    RecordProperty("iterations", std::to_string(kIters));
}

TEST(PerlinNoisePerfTest, GetValueScalar16Octave)
{
    const PerlinNoise noise(999ULL, -15, std::vector<f64>(16, 1.0));
    constexpr int kIters = 1'000'000;
    const i64 ns = benchGetValueScalar(noise, kIters);
    std::printf("[PERF] GetValueScalar16Octave: %lld ns/op (%d iters)\n", static_cast<long long>(ns / kIters), kIters);
    RecordProperty("ns_per_op", std::to_string(ns / kIters));
    RecordProperty("total_ns", std::to_string(ns));
    RecordProperty("iterations", std::to_string(kIters));
}

} // namespace
} // namespace mc
