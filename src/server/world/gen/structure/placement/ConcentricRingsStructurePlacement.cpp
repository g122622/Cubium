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

#include "ConcentricRingsStructurePlacement.hpp"

#include "common/core/Types.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/JavaLegacyRandom.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/math/random/WorldgenRandom.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include "server/world/gen/structure/placement/StructurePlacement.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace mc::world::gen::structure::placement {

// ============================================================================
// ConcentricRingsStructurePlacement
// ============================================================================

ConcentricRingsStructurePlacement::ConcentricRingsStructurePlacement(
    i32 distance, i32 spread, i32 count, std::vector<BiomeId> preferredBiomes, i64 salt, math::Vector3i locateOffset)
    : m_distance(distance)
    , m_spread(spread)
    , m_count(count)
    , m_preferredBiomes(std::move(preferredBiomes))
{
    m_locateOffset = locateOffset;
    m_frequencyReduction = FrequencyReductionMethod::Default;
    m_frequency = 1.0f;
    m_salt = salt;
    // 同心环放置不需要排斥区
    m_exclusionZone = std::nullopt;
}

std::vector<world::chunk::ChunkPos> ConcentricRingsStructurePlacement::generateRingPositions(i64 worldSeed) const
{
    const auto full = generateRingLayout(worldSeed);
    std::vector<world::chunk::ChunkPos> positions;
    positions.reserve(full.size());
    for (const auto& entry : full) {
        positions.push_back(entry.chunk);
    }
    return positions;
}

std::vector<ConcentricRingsStructurePlacement::RingEntry> ConcentricRingsStructurePlacement::generateRingLayout(
    i64 worldSeed) const
{
    std::vector<RingEntry> result;
    result.reserve(static_cast<size_t>(m_count));

    // 原版 ChunkGeneratorStructureState.generateRingPositions 用 RandomSource.create()
    // （即 LegacyRandomSource）再 setSeed(concentricRingsSeed)。
    //
    // 这里必须用 `JavaLegacyRandom` 本体而不是 `WorldgenRandom` 包装：后者只复刻了
    // `BitRandomSource` 的位宽抽取，`nextLong()` 一次消耗两次内层 nextLong，与
    // LegacyRandomSource 的语义不同——而 `fork()` 恰好依赖 `nextLong()` 的消耗次数。
    math::JavaLegacyRandom rng(0ULL);
    rng.setSeed(static_cast<u64>(worldSeed));

    // 同心环参数
    // 原版 ChunkGeneratorStructureState.generateRingPositions：
    //
    //   int j = count(); int i = distance(); int k = spread();
    //   double d0 = rng.nextDouble() * PI * 2.0;   // 起始角度
    //   int l = 0, i1 = 0;                          // i1 = 当前环索引
    //   for (int j1 = 0; j1 < j; j1++) {
    //       double d1 = 4*i + i*i1*6 + (rng.nextDouble() - 0.5) * (i*2.5);
    //       ...
    //       d0 += (PI * 2) / k;
    //       if (++l == k) {                       // 本环放满了
    //           i1++; l = 0;
    //           k += 2 * k / (i1 + 1);            // 环容量动态扩张
    //           k = Math.min(k, j - j1);
    //           d0 += rng.nextDouble() * PI * 2.0; // 环间角度随机扰动
    //       }
    //   }
    //
    // 此前这里是硬编码的环容量表（3,6,10,15,21,28,36,9），且半径公式少了开头的
    // `4*i` 系数、缺少环间角度扰动——三者叠加使整个要塞分布与原版完全不同
    // （由差分能力 structure_starts 查出：9 个要塞单元在原版有、Cubium 完全没有）。
    const f64 baseDistance = static_cast<f64>(m_distance);
    i32 ringCapacity = m_spread;
    f64 angle = rng.nextDouble() * 2.0 * math::PI_DOUBLE;
    i32 inRing = 0;
    i32 ringIndex = 0;

    for (i32 index = 0; index < m_count; ++index) {
        const f64 radius = (4.0 * baseDistance) + (baseDistance * static_cast<f64>(ringIndex) * 6.0) +
            (rng.nextDouble() - 0.5) * (baseDistance * 2.5);
        const f64 chunkX = std::round(std::cos(angle) * radius);
        const f64 chunkZ = std::round(std::sin(angle) * radius);

        // 原版紧接在算出 (k1, l1) 之后、角度步进之前调用 `randomsource.fork()`：
        //
        //   public RandomSource fork() { return new LegacyRandomSource(this.nextLong()); }
        //
        // 这里的 `this` 是 LegacyRandomSource 自己，它的 `nextLong()` 是
        // `((long)next(32) << 32) + next(32)`——**两次** 48 位 LCG 抽取。而
        // `WorldgenRandom::nextLong()` 会先拆成两次 `next(32)` 再拼，看起来一样，
        // 但它每一位都走「内层 nextLong 取高位」的路径，一次调用消耗**两次**
        // 内层 nextLong（四次 LCG 抽取），与 fork 的实际语义不同，序列会整体错位。
        // 因此 fork 必须直接调用底层 LCG 的 nextLong，不能经由包装类。
        RingEntry entry;
        entry.chunk = world::chunk::ChunkPos(static_cast<i32>(chunkX), static_cast<i32>(chunkZ));
        entry.forkSeed = static_cast<u64>(rng.nextLong());
        result.push_back(entry);

        angle += (2.0 * math::PI_DOUBLE) / static_cast<f64>(ringCapacity);
        if (++inRing == ringCapacity) {
            ++ringIndex;
            inRing = 0;
            ringCapacity += (2 * ringCapacity) / (ringIndex + 1);
            ringCapacity = std::min(ringCapacity, m_count - index);
            angle += rng.nextDouble() * 2.0 * math::PI_DOUBLE;
        }
    }

    return result;
}

const std::vector<world::chunk::ChunkPos>& ConcentricRingsStructurePlacement::ringLayoutChunks(i64 worldSeed) const
{
    // 检查缓存是否有效
    if (m_cachedPositions.has_value() && m_cachedSeed == worldSeed) {
        return m_cachedPositions.value();
    }

    // 加锁生成
    std::lock_guard<std::mutex> lock(m_cacheMutex);

    // 双重检查（可能另一个线程已经生成了）
    if (m_cachedPositions.has_value() && m_cachedSeed == worldSeed) {
        return m_cachedPositions.value();
    }

    m_cachedPositions = generateRingPositions(worldSeed);
    m_cachedSeed = worldSeed;

    return m_cachedPositions.value();
}

bool ConcentricRingsStructurePlacement::isStructureChunk(i64 worldSeed, i32 chunkX, i32 chunkZ) const
{
    // 同心环放置不需要频率缩减和排斥区检查
    // 直接在环形位置列表中查找
    const auto& positions = ringLayoutChunks(worldSeed);

    for (const auto& pos : positions) {
        if (pos.x == chunkX && pos.z == chunkZ) {
            return true;
        }
    }

    return false;
}

const std::vector<world::chunk::ChunkPos>& ConcentricRingsStructurePlacement::resolveRingPositions(
    i64 worldSeed, const std::function<std::vector<world::chunk::ChunkPos>()>& compute) const
{
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    if (m_resolvedPositions.has_value() && m_resolvedSeed == worldSeed) {
        return m_resolvedPositions.value();
    }

    // 持锁计算：并发到达的判定线程在这里排队等待，而不是各跑一遍 128 次群系搜索。
    m_resolvedPositions = compute();
    m_resolvedSeed = worldSeed;
    return m_resolvedPositions.value();
}

std::unique_ptr<StructurePlacement> ConcentricRingsStructurePlacement::clone() const
{
    return std::make_unique<ConcentricRingsStructurePlacement>(
        m_distance, m_spread, m_count, m_preferredBiomes, m_salt, m_locateOffset);
}

} // namespace mc::world::gen::structure::placement
