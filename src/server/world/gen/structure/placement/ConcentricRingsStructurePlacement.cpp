/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, or/or sell
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
    std::vector<world::chunk::ChunkPos> positions;
    positions.reserve(static_cast<size_t>(m_count));

    // 原版 ChunkGeneratorStructureState.generateRingPositions 用 RandomSource.create()
    // （即 LegacyRandomSource）再 setSeed(concentricRingsSeed)。
    math::WorldgenRandom rng(std::make_unique<math::JavaLegacyRandom>(0ULL));
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
        positions.emplace_back(static_cast<i32>(chunkX), static_cast<i32>(chunkZ));

        angle += (2.0 * math::PI_DOUBLE) / static_cast<f64>(ringCapacity);
        if (++inRing == ringCapacity) {
            ++ringIndex;
            inRing = 0;
            ringCapacity += (2 * ringCapacity) / (ringIndex + 1);
            ringCapacity = std::min(ringCapacity, m_count - index);
            angle += rng.nextDouble() * 2.0 * math::PI_DOUBLE;
        }
    }

    // TODO: 原版在算出 (chunkX, chunkZ) 之后还会用 `biomeSource.findBiomeHorizontal(..., 112, ...)`
    // 在半径 112 区块内搜索 `preferredBiomes`，找到就把要塞挪到该生物群系，找不到才用原坐标。
    // 本类目前没有生物群系源，这一步尚未实现——要塞会落在坐标算出来的位置，而不是
    // 像原版那样偏向 `#minecraft:stronghold_biased_to` 指定的生物群系。
    // 补齐需要把 IBiomeSource 传进放置器，并复刻 findBiomeHorizontal 的采样顺序。

    return positions;
}

const std::vector<world::chunk::ChunkPos>& ConcentricRingsStructurePlacement::getRingPositions(i64 worldSeed) const
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
    const auto& positions = getRingPositions(worldSeed);

    for (const auto& pos : positions) {
        if (pos.x == chunkX && pos.z == chunkZ) {
            return true;
        }
    }

    return false;
}

std::unique_ptr<StructurePlacement> ConcentricRingsStructurePlacement::clone() const
{
    return std::make_unique<ConcentricRingsStructurePlacement>(
        m_distance, m_spread, m_count, m_preferredBiomes, m_salt, m_locateOffset);
}

} // namespace mc::world::gen::structure::placement
